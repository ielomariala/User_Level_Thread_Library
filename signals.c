#include <stdio.h>
#include <stdlib.h>
#include "signals.h"

signal_t signals[SIG_NUM] = {
    {.type = SIG_1, .handler = def_signal_handler, .old_handler = def_signal_handler},
    {.type = SIG_2, .handler = def_signal_handler, .old_handler = def_signal_handler},
    {.type = SIG_3, .handler = def_signal_handler, .old_handler = def_signal_handler},
    {.type = SIG_KILL, .handler = def_signal_handler, .old_handler = def_signal_handler}
};
void thread_signal_create() ;

thread_signal_t *th;

signal_t no_signal = {.type = Error, .handler = NULL};

signal_t    *       get_signal              (signal_type type) {
    switch (type)
    {
    case SIG_1 :
        return &signals[0];
        break;
    case SIG_2 :
        return &signals[1];
        break;
    case SIG_3 :
        return &signals[2];
        break;
    case SIG_KILL :
        return &signals[3];
        break;
    default:
        return &no_signal;
        break;
    }
}

void thread_signal_create() {
    /*if (th == NULL) {
        return NULL;
    }*/
    thread_signal_t *th_sig = malloc(sizeof(thread_signal_t));
    if(th_sig != NULL){
        int i = 0;
        for (i=0; i<MAX_SIGNALS; i++){
            th_sig->tab_sig[i] = &no_signal;
        }
        //th_sig->owner = th;
        th_sig->curr_sig = 0;
    }
    th = th_sig; 
    //return th_sig;
}

int thread_signal_free(thread_signal_t *th_sig){
  if(th_sig != NULL){
    free(th_sig);
    return 0;
  }
  return -1;
}
/*
 * Sends signals
 */
void print_tab(thread_signal_t* th){
    for (int i = 0; i < th->curr_sig; i++){
        printf("type: %d\t", th->tab_sig[i]->type);
    }

    printf("\n");
}

int signal_send(signal_t *sig){
    printf("SIG_SEND th_sig: %p\n", th);
  if(((int)sig->type < SIG_NUM && (int)sig->type>-1) && (th->curr_sig < MAX_SIGNALS - 1)) {
    int index = th->curr_sig;
    (th->tab_sig)[index] = sig;
    th->curr_sig = (index + 1);
    printf("SIG_SEND signal: %p\n", th->tab_sig[index]);
    printf("SIG_SEND get signal sig1: %p\n", get_signal(SIG_1));
    print_tab(th);
    fprintf(stderr, "Sender entered to sig send \n");
    return 1;
  }
  return 0;
}

/*
 * default handler 
 */
void def_signal_handler(signal_type sig) {
    switch (sig)
    {
    case SIG_KILL:
        printf("Received signal SIG_KILL \n");
        thread_exit(NULL);
        break;
    case SIG_1 :
        printf("Received signal SIG_1 \n");
        break;
    case SIG_2 :
        printf("Received signal SIG_2 \n");
        break;
    case SIG_3 :
        printf("Received signal SIG_3 \n");
        break;
    default:
        break;
    }
  
}

/*
 * ignoring signal handler
 */

void signal_ignore(signal_type sig) {
    printf("Ignoring the signal %d received\n", (int)sig);
}

/*
 * Keeps thread waiting until signal reception
 */



int signal_wait(signal_t *sig) {
    printf("SIG_WAIT: th_sig: %p\n", th);
    fprintf(stderr, "At the beginning\n");
    if (th == NULL || (int)sig->type >= MAX_SIGNALS || (int)sig->type < 0) {
        fprintf(stderr, "I will return 0 and i don t know why \n");
        return 0;
    }
    int i = 0;
    fprintf(stderr, "Before while\n");
    while ((th->tab_sig)[th->curr_sig - 1] != sig){
        printf("Current sig : %d\n", th->curr_sig);
        printf("SIG_WAIT signal : %p\n", th->tab_sig[th->curr_sig - 1]);
        printf("SIG_WAIT get signal sig1: %p\n", get_signal(SIG_1));
        print_tab(th);
        thread_yield();
        i++;
        fprintf(stderr, "Still waiting for signal\n");
        if ( i > 20 ) {
            fprintf(stderr, "\033[1;31m");
            fprintf(stderr, "\n_________ Timeout did not receive signal ____________\n\n");
            fprintf(stderr, "\033[0m");
            break;
        }
    }
    if (sig->handler == NULL) {
        def_signal_handler(sig->type);
    }
    else {
        sig->handler(sig->type);
    }
    return 1;
}

void sigaction_t(signal_t *sig, sig_handler new_handler) {
    if (sig->type == SIG_KILL && new_handler == signal_ignore) {
        fprintf(stderr, "Signal SIG_KILL can not be ignored\n");
    }
    else {
        sig->old_handler = sig->handler;
        sig->handler = new_handler;
    }
}