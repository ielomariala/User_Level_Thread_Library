#include "thread.h"
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <valgrind/valgrind.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#ifndef USE_PTHREAD
#define STACK_SIZE 64*1024
#define PREEMPTION_TIME 8000 //us
#define DEFAULT_PRIORITY 10
#define MAX_PRIO 20
#define MIN_PRIO 0


typedef struct thread {
    size_t valgrind_id; // Id Vlgrind cf. Ressources Valgrind
    ucontext_t my_uc; // Context 
    thread_t joiner; // joiner of thread used in thread_join
    void* retval; // Return value of thread
    int exited; // Thread have been exited or no
    STAILQ_ENTRY(thread) link; // linking in thread_queue
    unsigned int priority; // Thread's Priority
    int in_queue; // Is thread in queue?
} thread;

/**********************   
    Global Variables 
***********************/
thread* current_thread = NULL; // Thread currently running
STAILQ_HEAD(headname, thread) thread_queue; // Thread queue
thread* main_thread = NULL; // Thread of main function
thread* last_thread_inserted = NULL; // For caution, I had a problem when inserting main thread in tail when it is already there, the queue would go infinitely with numerous main threads in tail
thread_t join_main_thread = NULL; // Thread we need to free when main is first thread to exit
int mprotected = 1; // The stack of all threads is protected, it is 0 when a thread has a stack overflow
int mask = 0; // Mask of preemption alarm signals, 1 if mask is activated, 0 otherwise

/***************************  
    Functions Definitions 
****************************/
/*
 * Constructor to initialize thread queue and malloc main thread
 */
void initialize_libthread(void) __attribute__((constructor));

/*
 * Destructor to free what needs to be freed (main thread if not already freed) and all of thread queue if it's not empty
 */
void finalize_libthread(void) __attribute__((destructor));

/*
 * get the return value when making context and exit the thread
 */
void* get_retval(void* (*func)(void*), void *funcarg);

/*
 * debugging
 */
void debug_stailq();

/*
 * Free thread t
 */
void thread_free(thread_t t, int free_main);

/*
 * Returns 1 if t is main_thread, 0 otherwise
 */
int is_main_thread(thread_t t);

/*
 * Returns 1 if t is in thread queue, 0 otherwise
 */
int thread_is_in_queue(thread_t t);

/*
 * Insert the thread t into thread_queue, it compares also the last_thread_inserted and t, 
 * if they are equal, it don't insert t in the queue.
 */
void insert_into_queue(thread* t);

/*
 * It remplaces the current_thread by the first thread on the threadqueue and it also removes 
 * it from thread_queue. 
 */
void swap_thread();

/*
 * Defines the time of the preemption of a thread
 */
int preemptiontime(thread* t);

/*
 * The handler used for the sigalrm signal
 */
void handler(int signum);

/*
 * Helps for masking a signal given
 */
void sig_mask(int signum);

/*
 * Helps for unmasking a signal given
 */
void sig_unmask(int signum);

/*
 * Clean the waiting signum received while the 
 * was blocked  
 */
void clear_waiting_sigalrm(int signum);

/*
 * Install the timer for the reception of te sigalrm 
 * depending on the preemption time of each thread 
 */
void install_timer();

/*
 * Unlock the signum by:
    -> initializing the timer.
    -> clearing the waiting signals.
    -> unmasking the signal.
 */
void unlock_sig(int signum);

/*******************************  
    Functions Implementation 
********************************/
/* Recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    #ifdef DEBUG
    printf("CURRENT THREAD = %p\n", current_thread);
    #endif
    //sig_mask(SIGALRM);
    //thread* t = current_thread;
    //sig_unmask(SIGALRM);
    return current_thread;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */


int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    #ifdef DEBUG
    printf("Debug thread_create\n");
    #endif

    thread* new_thread = malloc(sizeof(thread)); // create new thread
    if(new_thread == NULL){
        #ifdef DEBUG
        printf("Debug: malloc unsuccessful\n");
        #endif
        return -1;
    }
    new_thread->joiner = NULL; // Set its father to current thread
    new_thread->retval = NULL; // Set return value to NULL   // PQ pas retval = func(funcarg)? c'est comme si on est en séquentiel
    new_thread->exited = 0; // Thread is not exited yet
    new_thread->priority = DEFAULT_PRIORITY;
    new_thread->in_queue = 0;
    /* D'après: Ressources ucontext_t */
    getcontext(&new_thread->my_uc);
    (new_thread->my_uc.uc_stack).ss_size = STACK_SIZE;
    #ifdef NO_MPROTECT
    (new_thread->my_uc.uc_stack).ss_sp = malloc(STACK_SIZE);
    #else
    long page_size = sysconf(_SC_PAGE_SIZE);
    void *addr = mmap(NULL, new_thread->my_uc.uc_stack.ss_size + 2 * page_size,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(mprotect(addr, page_size, PROT_NONE == -1)){
        perror("Mprotect (1) error\n");
        return -1;
    }
    if(mprotect(addr + page_size + new_thread->my_uc.uc_stack.ss_size, page_size, PROT_NONE)){
        perror("Mprotect (2) error\n");
        return -1;
    }
    new_thread->my_uc.uc_stack.ss_sp = addr + page_size;
    #endif
   
    if(new_thread == NULL){
        #ifdef DEBUG
        printf("Debug: malloc unsuccessful\n");
        #endif
        return -1;
    }
    new_thread->my_uc.uc_link = NULL;
    makecontext(&new_thread->my_uc, (void(*)(void))*get_retval, 2,(void (*)(void)) func ,funcarg);

    /* D'après: Ressoures Valgrind */
    new_thread->valgrind_id = VALGRIND_STACK_REGISTER(new_thread->my_uc.uc_stack.ss_sp,
						 new_thread->my_uc.uc_stack.ss_sp + new_thread->my_uc.uc_stack.ss_size);
    #ifndef NO_PREEMPTION
    sig_mask(SIGALRM);
    #endif
    insert_into_queue(new_thread);
    #ifndef NO_PREEMPTION
    sig_unmask(SIGALRM);
    #endif 
    *newthread = new_thread; // Set variable *newthread to the created thread
    return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void) {
    #ifdef DEBUG
    printf("Debug thread_yield\n");
    #endif
    #ifndef NO_PREEMPTION
    sig_mask(SIGALRM);
    #endif

    if(STAILQ_EMPTY(&thread_queue)){ // if there is no active thread is queue return 0 == yield to yourself
        #ifdef DEBUG
        printf("Debug thread_yield: queue is empty\n");
        #endif
        //fprintf(stderr, "yielded if queue empty\n");
        sig_unmask(SIGALRM);
        return 0;  
    }

    assert(current_thread != NULL);
    if ( !current_thread->exited){ // if current thread is not exited insert it in active thread queue, so we can return to it afterwards
        insert_into_queue(current_thread);   
        //fprintf(stderr, "yielded else\n"); 
    }
    //fprintf(stderr, "going to swap thread\n");
    swap_thread();
    return 0;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t t, void **retval) {
    #ifdef DEBUG
    printf("Debug thread_join joining %p with current %p\n", t, current_thread);
    #endif 
    assert(t != NULL);
    #ifndef NO_PREEMPTION
    sig_mask(SIGALRM);
    #endif
    if(t->exited == 0 ){ // If thr == t is not already finished, normally it should be finished from thread_create
        t->joiner = thread_self(); // Set its joiner to current thread

        #ifndef NO_DEADLOCK
        /* Deadlock Paragraph */
        thread* tmp = current_thread;
        assert(tmp != NULL);
        //printf("%p \n", tmp->joiner);
        //printf("%p\n", tmp);
        while (tmp->joiner != NULL){
            if (tmp->joiner == t){
                printf("DEADLOCK DETECTED\n");
                return -1;
            }
            tmp = tmp->joiner;
        }
        /* End Deadlock Paragraph */
        #endif

        insert_into_queue(t);
        swap_thread();
    }
    if(retval != NULL){ // Memorize returned value in correspondant parameter 
        *retval = t->retval; 
    }
    /*  Freeing paragraph */
    thread_free(t, 0); // Freeing thread as it is finished and we no longer need it
    t = NULL;

    if (!mprotected){
        return -1;
    }
    
    return 0;  
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
 void thread_exit(void *retval) {
    #ifdef DEBUG
    printf("Debug thread_exit exiting %p\n", current_thread);
    #endif
    #ifndef NO_PREEMPTION
    sig_mask(SIGALRM);
    #endif
    current_thread->retval = retval; // seting retval of current thread
    current_thread->exited = 1; // Set exited to true 
    
    /* Inserting joiner in thread queue and thread_yielding */
    assert(current_thread != NULL);
    if ( !is_main_thread(current_thread) &&
        current_thread->joiner != NULL &&
        !current_thread->joiner->exited ){ // Inserting joiner thread into thread queue if it's not null nad it's not already exited

        insert_into_queue(current_thread->joiner);
    }


    if (!STAILQ_EMPTY(&thread_queue)){
        #ifndef NO_PREEMPTION
        unlock_sig(SIGALRM);
        #endif
        thread_yield(); // running another thread
    }

    else if (!is_main_thread(current_thread)){
        setcontext(&main_thread->my_uc); // if queue is empty, return to main thread
    }
    if(current_thread != main_thread){
        join_main_thread = thread_self();
    }
    exit(0);
} 


// handler for SIGSEGV
void sigsegv_handler(int signal __attribute__((unused))) {
    mprotected = 0;
   thread_exit(NULL);
}

void initialize_libthread(){
    /*
    #ifdef NO_MPROTECT
    printf("Executing with No stack overflow detection!\n");
    #endif
    #ifdef NO_PREEMPTION
    printf("Executing with No preemption!\n");
    #endif
    #ifdef NO_DEADLOCK
    printf("Executing with No deadlock detection!\n");
    #endif
    */
    STAILQ_INIT(&thread_queue);  
    current_thread = malloc(sizeof(thread));
    if(current_thread == NULL){
        #ifdef DEBUG
        printf("Debug: malloc unsuccessful\n");
        #endif
        exit(0);
    }
    current_thread->joiner = NULL;  
    current_thread->retval = NULL; 
    current_thread->exited = 0; 
    current_thread->priority = DEFAULT_PRIORITY;
    current_thread->in_queue = 0; 
    // Use ucontext functions 
    getcontext(&current_thread->my_uc);
    (current_thread->my_uc.uc_stack).ss_size = STACK_SIZE;
    #ifdef NO_MPROTECT
    (current_thread->my_uc.uc_stack).ss_sp = malloc(STACK_SIZE);
    #else
    long page_size = sysconf(_SC_PAGE_SIZE);
    void *addr = mmap(NULL, current_thread->my_uc.uc_stack.ss_size + 2 * page_size,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(mprotect(addr, page_size, PROT_NONE == -1)){
        perror("Mprotect (1) error\n");
        
    }
    if(mprotect(addr + page_size + current_thread->my_uc.uc_stack.ss_size, page_size, PROT_NONE)){
        perror("Mprotect (2) error\n");
        
    }
    current_thread->my_uc.uc_stack.ss_sp = addr + page_size;
    //(current_thread->my_uc.uc_stack).ss_sp = mmap(NULL, STACK_SIZE + 2*page_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    #endif

    if(current_thread->my_uc.uc_stack.ss_sp == NULL){
        #ifdef DEBUG
        printf("Debug: malloc unsuccessful\n");
        #endif
        exit(0);
    }
    current_thread->my_uc.uc_link = NULL;
    main_thread = current_thread;

    #ifndef NO_MPROTECT
    static char sigsegv_stack[SIGSTKSZ];
    stack_t ss = {
            .ss_size = SIGSTKSZ,
            .ss_sp = sigsegv_stack,
    };
    struct sigaction sa = {
            .sa_handler = sigsegv_handler,
            .sa_flags = SA_ONSTACK,
    };
    sigaltstack(&ss, NULL);
    sigfillset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    #endif

    #ifndef NO_PREEMPTION
    struct sigaction sigact = {
            .sa_handler = handler,
    };
    sigfillset(&sigact.sa_mask);
    if (sigaction(SIGALRM, &sigact, NULL) == -1) {
        perror("init_thread: installing sigaction");
    }

    install_timer(SIGALRM);
    #endif

    #ifdef DEBUG
    printf("MAIN _ THREAD = %p\n", main_thread);
    #endif
}

void finalize_libthread(){
    #ifdef DEBUG
    printf("Finalizing: main_thread %p current_thread %p join_main_thread %p\n", main_thread, current_thread, join_main_thread);
    #endif
    if (join_main_thread != NULL){
        thread_free(join_main_thread, 0);
    }
    if ( main_thread != NULL){
        thread_free(main_thread, 1);
    }
    if (!STAILQ_EMPTY(&thread_queue)){
        thread* n1 = STAILQ_FIRST(&thread_queue);
        while (n1 != NULL) {
            thread * n2 = STAILQ_NEXT(n1, link);
            thread_free(n1, 0);
            n1 = n2;
        }
    }
}

void* get_retval(void* (*func)(void*), void *funcarg){
    #ifdef DEBUG
    printf("Debug (get_retval) current %p main %p\n", current_thread, main_thread);
    #endif
    void *retval;
    retval = func(funcarg); // Memorizing return value as we need to return it
    #ifdef DEBUG
    printf("Debug (get_retval): end func(funcarg) current %p main %p\n",current_thread, main_thread);
    #endif
    #ifdef DEBUG
    printf("retval is %p value \n",retval );
    #endif
    assert(current_thread != NULL);
    if ( !current_thread->exited){
        #ifdef DEBUG
        printf("Debug (get_retval): just before exiting thread\n");
        #endif
        thread_exit(retval);
    }
    return retval; // Dead code because of thread_exit
}

void thread_free(thread_t t, int free_main){
    #ifdef DEBUG
    printf("Debug (thread_free) freeing %p\n", t);
    #endif
    if (free_main || !is_main_thread(t)){
        if (t != NULL){ 
            if(t->my_uc.uc_stack.ss_sp != NULL){
                /* D'après: Ressoures Valgrind */
                VALGRIND_STACK_DEREGISTER(t->valgrind_id);
                #ifdef NO_MPROTECT
                free(t->my_uc.uc_stack.ss_sp);
                #else
                long page_size = sysconf(_SC_PAGE_SIZE);
                munmap(t->my_uc.uc_stack.ss_sp - page_size, t->my_uc.uc_stack.ss_size + 2 * page_size);
                #endif 
                t->my_uc.uc_stack.ss_sp = NULL;
            }
            if (t->in_queue == 1){
                STAILQ_REMOVE(&thread_queue, t, thread,link);
                t->in_queue = 0;
            }
            free(t);
            t = NULL;
        }
    }
}
  

#ifdef DEBUG
void debug_stailq(){
    thread* n1 = STAILQ_FIRST(&thread_queue);
    while (n1 != NULL) {
        //printf("-----------------\n");
        thread * n2 = STAILQ_NEXT(n1, link);
        printf("Adress = %p\n", n1);
        n1 = n2;
    }
}
#endif

int is_main_thread(thread_t t){
    return (t == main_thread);
}

int thread_is_in_queue(thread_t t){
    thread* n1 = STAILQ_FIRST(&thread_queue);
    while (n1 != NULL) {
        thread * n2 = STAILQ_NEXT(n1, link);
        if (t == n1)
            return 1;
        n1 = n2;
    }
    return 0;
}

void insert_into_queue(thread* t){
    #ifdef PREEMPTION
    sig_mask(SIGALRM);
    #endif
    if (t->in_queue == 0){
        #ifdef DEBUG
        printf("Debug : just before inserting thread \n");
        debug_stailq();
        #endif
        STAILQ_INSERT_TAIL(&thread_queue, t, link); // Insert thread into thread queue
        last_thread_inserted = t;
        t->in_queue = 1;
        #ifdef DEBUG
        printf("Debug : just after inserting thread \n");
        debug_stailq();
        #endif
    }

}

void swap_thread(){

    thread* previous_thread = thread_self(); // preparing for a context swap
    current_thread = STAILQ_FIRST(&thread_queue); // Getting pointer to first element in thread queue
    #ifdef DEBUG
    printf("Debug : just before removing first \n");
    debug_stailq();
    #endif
    STAILQ_REMOVE_HEAD(&thread_queue, link); // removing first element in thread queue
    current_thread->in_queue = 0;
    #ifdef DEBUG
    printf("Debug : just after removing first \n");
    debug_stailq();
    #endif


    #ifdef DEBUG
    printf("-------------------\n");
    printf("Debugging: prev = %p\n", previous_thread);
    printf("Debugging: curren = %p\n", current_thread);
    printf("Debugging: main = %p\n", main_thread);
    #endif
    #ifndef NO_PREEMPTION
    unlock_sig(SIGALRM);
    #endif
    swapcontext(&previous_thread->my_uc, &current_thread->my_uc);
}

//Preemption & Priority 

int thread_modif_prio(thread* t, int priority){
    if(t == NULL){
        return -1;
    }

    if(priority > MAX_PRIO || priority < MIN_PRIO ){
        return -1;
    }

    t->priority = priority;
    return 0;
}

int preemptiontime(thread* t){
    if (t == NULL)
        return -1;
    return PREEMPTION_TIME + ((PREEMPTION_TIME * (10 - t->priority))/10)/2; //To adapt after adding priority 
}

void handler(int signum __attribute__((unused))){
    if (mask == 0){
        //printf("received handler\n");
        thread_yield();
    }
}

void sig_mask(int signum __attribute__((unused))){
    mask = 1;
    //printf("blocked sigalrm\n");
  /*  sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signum);
    int rc = sigprocmask(SIG_BLOCK, &set, NULL);
    if (rc != 0)
        perror("sigprocmask block unsuccessful\n");*/
}

void sig_unmask(int signum __attribute__((unused))){
    //printf("unblock sig\n");
    mask = 0;
 /*   sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signum);
    int rc = sigprocmask(SIG_UNBLOCK, &set, NULL);
    if(rc != 0)
        perror("sigprocmask unblock unsuccessful\n");*/
}

void unlock_sig(int signum){
    install_timer(signum);
    clear_waiting_sigalrm(signum);
    sig_unmask(signum);
}

void clear_waiting_sigalrm(int signum){
    struct sigaction old_sigact;
    //Signum handler is unchanged with sa_timer
    if(sigaction(signum, NULL, &old_sigact) == -1){
        perror("waiting sigalrm\n");
    }

    //Ignore the signum waiting
    signal(signum, SIG_IGN);

    //Associate the handler used before to signum
    if(sigaction(signum, &old_sigact, NULL) == -1){
        perror("waiting sigalrm\n");
    }
}

void install_timer(){
    struct itimerval timer = {
        .it_interval = {.tv_sec = 0, .tv_usec = preemptiontime(current_thread)},
        .it_value = {.tv_sec = 0, .tv_usec = preemptiontime(current_thread)}
        };
    //With ITIMER_REAL: Decrements in real time, and delivers "SIGALRM" upon expiration. 
    if(setitimer(ITIMER_REAL, &timer, NULL ) == -1){
        perror("install timer\n");
    }    
}






// Mutex

#ifdef USE_PREEMPTION
struct thread__mutex {
     thread_t owner;
     int destroyed;
     pthread_spinlock_t lock;
 };

/*
 * To initialize mutex. 
 * Returns 0 if mutex was initialized and -1 otherwise.
 */
int thread_mutex_init(struct thread_mutex_t* fake){
    struct thread__mutex *mutex = (void *) fake;
  if(mutex != NULL){
    printf("mutex initialization\n");
    mutex->destroyed = 0;
    mutex->owner = NULL;
    errno = pthread_spin_init(&(mutex->lock), PTHREAD_PROCESS_SHARED);
    printf("Initialized spin lock\n");
    if (errno != 0) {
        perror("(thread mutex init)");
    }
    //STAILQ_INIT(&(mutex->mutexqueue));
    return 0;
  }
  return -1;
}

/*
 * To destroy the mutex.
 * Returns 0 if mutex was destroyed and -1 otherwise.
 */
int thread_mutex_destroy(struct thread_mutex_t *fake){
  struct thread__mutex *mutex = (void *) fake;
  if(mutex != NULL){
    mutex->destroyed = 1;
    mutex->owner = NULL;
    errno = pthread_spin_destroy(&(mutex->lock));
    if (errno != 0) {
        perror("(thread mutex destroy)");
    }
    return 0;
  }

  return -1;
}

/*
 * To lock the mutex. 
 */
int thread_mutex_lock(struct thread_mutex_t *fake){
  struct thread__mutex *mutex = (void *) fake;
  if (mutex == NULL || mutex->destroyed){
      return -1;
  }
  printf("locking the thread...\n");
  pthread_spin_lock(&(mutex->lock));
  printf("locked !\n");
  mutex->owner = current_thread;
  return 0;
}

/*
 * To unlock the mutex
 */
int thread_mutex_unlock(struct thread_mutex_t *fake){
  struct thread__mutex *mutex = (void *) fake;
  if(mutex == NULL || mutex->destroyed)
    return -1;
  printf("Unlocking the thread...\n");
  errno = pthread_spin_unlock(&(mutex->lock));
  if (errno != 0)
      perror("(thread mutex unlock)");
  mutex->owner = NULL;
  return 0;
}

/*
#else

#ifdef PASSIVE

int thread_mutex_init(thread_mutex_t* mutex){
    if(mutex != NULL){
        mutex->destroyed = 0;
        mutex->owner = NULL;
        STAILQ_INIT(&(mutex->waitqueue));
        return 0;
    }
    return -1;
}

int thread_mutex_destroy(thread_mutex_t *mutex){
    if(mutex != NULL){
        mutex->destroyed = 1;
        mutex->owner = NULL;
        return 0;
    }

    return -1;
}

int thread_mutex_lock(thread_mutex_t *mutex){
    if (mutex == NULL || mutex->destroyed || mutex->owner == current_thread)
        return -1;
    while(mutex->owner != NULL){
        if(!STAILQ_EMPTY(&thread_queue)){
            printf("Not empty\n");
        struct thread* tmp = STAILQ_FIRST(&thread_queue);
        STAILQ_REMOVE_HEAD(&thread_queue, link);
        struct thread* prev = current_thread;
        current_thread = tmp;

        STAILQ_INSERT_TAIL(&(mutex->waitqueue), prev, link);
        swapcontext(&(prev->my_uc), &(current_thread->my_uc));
        }
    }
    mutex->owner = current_thread;
    return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex){
    if(mutex->destroyed == 1 || mutex->owner != current_thread)
        return -1;
    if(!STAILQ_EMPTY(&(mutex->waitqueue))){
        struct thread* tmp = STAILQ_FIRST(&(mutex->waitqueue));
        STAILQ_REMOVE_HEAD(&(mutex->waitqueue), link);
        STAILQ_INSERT_TAIL(&thread_queue, tmp, link);
    }

    mutex->owner = NULL;
    return 0;
}*/
#else

#include <stdatomic.h>
struct thread__mutex {
    thread_t owner;
    int *destroyed;
    atomic_flag *lock;
};

struct thread__sem {
    atomic_int *max_count;
    atomic_int *count;
    thread_mutex_t *lock;
    atomic_int *destroyed;
};


/*
 * To initialize mutex.
 * Returns 0 if mutex was initialized and -1 otherwise.
 */
int thread_mutex_init(thread_mutex_t* fake){
    assert(sizeof(*fake) >= sizeof(struct thread__mutex));
    if (fake != NULL) {
        struct thread__mutex *mutex = (void *) fake;
        mutex->destroyed = malloc(sizeof(int));
        mutex->lock = malloc(sizeof(atomic_flag));
        if(mutex != NULL){
            *(mutex->destroyed) = 0;
            mutex->owner = NULL;
            atomic_flag_clear(mutex->lock);
            //STAILQ_INIT(&(mutex->mutexqueue));
            return 0;
        }
    }
    return -1;
}

/*
 * To destroy the mutex.
 * Returns 0 if mutex was destroyed and -1 otherwise.
 */
int thread_mutex_destroy(thread_mutex_t *fake){
    struct thread__mutex *mutex = (void *) fake;
    if(mutex != NULL){
        free(mutex->destroyed);
        free(mutex->lock);
        mutex->owner = NULL;
        return 0;
    }

    return -1;
}

/*
 * To lock the mutex.
 */
int thread_mutex_lock(thread_mutex_t *fake){
    struct thread__mutex *mutex = (void *) fake;

    if (mutex == NULL || mutex->destroyed == NULL || mutex->owner == current_thread){
        return -1;
    }
    while(atomic_flag_test_and_set(mutex->lock)){
        thread_yield();
    }
    mutex->owner = thread_self();
    return 0;
}

/*
 * To unlock the mutex
 */
int thread_mutex_unlock(thread_mutex_t *fake){
    struct thread__mutex *mutex = (void *) fake;

    if(mutex->destroyed == NULL || mutex->owner != current_thread)
        return -1;

    atomic_flag_clear(mutex->lock);
    mutex->owner = NULL;
    return 0;
}

int thread_sem_init(thread_sem_t *fake, int counter) {
    assert(sizeof(*fake) >= sizeof(struct thread__sem));
    if (fake != NULL) {
        struct thread__sem *sem = (void *) fake;
        sem->destroyed = malloc(sizeof *(sem->destroyed));
        sem->count = malloc(sizeof *(sem->count));
        sem->max_count = malloc(sizeof *(sem->max_count));
        sem->lock = malloc(sizeof(*(sem->lock)));
        atomic_store(sem->destroyed, 0);
        atomic_store(sem->count, counter);
        atomic_store(sem->max_count, atomic_load(sem->count));
        if (thread_mutex_init(sem->lock) != 0) {
            perror("thread_sem_init: Lock for semaphore initialization failed\n");
            return -1;
        }
        return 0;
    }
    perror("thread_sem_init: semaphore pointer is NULL\n");
    return -1;
}

int thread_sem_wait(thread_sem_t *fake) {
    struct thread__sem *sem = (void *) fake;
    while (1) {
        thread_mutex_lock(sem->lock);
        if (atomic_load(sem->count) > 0) {
            atomic_fetch_sub(sem->count, 1);
            thread_mutex_unlock(sem->lock);
            return 0;
        }
        thread_mutex_unlock(sem->lock);
        thread_yield();
    }
}

int thread_sem_post(thread_sem_t *fake) {
    struct thread__sem *sem = (void *) fake;
    thread_mutex_lock(sem->lock);
    if (atomic_load(sem->count) >= atomic_load(sem->max_count)) {
        perror("thread_sem_post: Trying to unlock but already over max count\n");
        return -1;
    }
    atomic_fetch_add(sem->count, 1);
    thread_mutex_unlock(sem->lock);
    return 0;
}

int thread_sem_destroy(thread_sem_t *fake) {
    struct thread__sem *sem = (void *) fake;
    thread_mutex_destroy(sem->lock);
    free(sem->max_count);
    free(sem->count);
    free(sem->destroyed);
    free(sem->lock);
    return 0;
}

#endif // USE_PREEMPTION
#endif
