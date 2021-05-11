#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "thread.h"

/* test de faire une somme avec plein de thread sur un compteur partagé
 *
 * valgrind doit etre content.
 * Les résultats doivent etre égals au nombre de threads * 1000.
 * La durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 *
 * support nécessaire:
 * - thread_create()
 * - thread_exit()
 * - thread_join() sans récupération de la valeur de retour
 * - thread_mutex_init()
 * - thread_mutex_destroy()
 * - thread_mutex_lock()
 * - thread_mutex_unloc()
 */

#define MAX_ACCESS 3

thread_sem_t lock;
int counter = MAX_ACCESS;

static void * thfunc(void *_nb)
{
//    unsigned long nb = (unsigned long) _nb;
    printf("thread %i at your service !\n", *((int *) _nb));
    unsigned long i = 0;
//    int tmp;

    for(i=0; i<1000;i++) {
        /* Verrouille la section critique accédant a counter */
        thread_sem_wait(&lock);
        --counter;
        if (counter < 0) {
            printf("THREAD %i", *((int *) _nb));
            perror("TEST post sub: counter < 0\n");
        } else if (counter > MAX_ACCESS) {
            printf("THREAD %i", *((int *) _nb));
            perror("TEST post sub: counter > MAX_ACCESS");
        }
        thread_yield();
        printf("thread %i increasing counter...\n", *((int *) _nb));
        counter++;
        thread_yield();
        if (counter < 0) {
            printf("THREAD %i", *((int *) _nb));
            perror("TEST post inc: counter < 0\n");
        } else if (counter > MAX_ACCESS) {
            printf("THREAD %i", *((int *) _nb));
            perror("TEST post inc: counter > MAX_ACCESS");
        }
        ++counter;
        thread_sem_post(&lock);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    thread_t *th;
    int i, nbthrd;
    int err, nb;

    if (argc < 2) {
        printf("argument manquant: nombre de threads\n");
        return -1;
    }

    nb = atoi(argv[1]);
    nbthrd = nb;

    if (thread_sem_init(&lock, MAX_ACCESS)) {
        fprintf(stderr, "thread_sem_init(lock) failed\n");
        return -1;
    }

    th = malloc(nbthrd*sizeof(*th));
    if (!th) {
        perror("malloc");
        return -1;
    }

    /* on cree tous les threads */
    for(i=0; i<nbthrd; i++) {
        err = thread_create(&th[i], thfunc, (void*)((intptr_t)i));
        assert(!err);
    }
    printf("Created threads !\n");

    /* on leur passe la main, ils vont tous terminer */
    for(i=0; i<nb; i++) {
        thread_yield();
    }

    /* on les joine tous, maintenant qu'ils sont tous morts */
    for(i=0; i<nbthrd; i++) {
        err = thread_join(th[i], NULL);
        assert(!err);
    }

    free(th);
    thread_sem_destroy(&lock);

    if (counter != MAX_ACCESS) {
        perror("Wrong result ! counter is not MAX_ACCESS");
        return 1;
    } else {
        printf("63-semaphore: SUCCESS !\n");
    }
    return 0;
}