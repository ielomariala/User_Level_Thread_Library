#ifndef __SIGNALS_H__
#define __SIGNALS_H__
#include "thread.h"


#define MAX_SIGNALS 6
#define SIG_NUM 4


typedef enum signals {  
        SIG_1=0,
        SIG_2, 
        SIG_3,
        SIG_KILL, 
        Error = -1 
} signal_type;

typedef void(*sig_handler)(signal_type);

typedef struct signal_t {
    signal_type type;
    sig_handler handler;
    sig_handler old_handler;
} signal_t;

typedef struct thread_signal_t {
    signal_t *tab_sig[MAX_SIGNALS];
    int curr_sig;
    thread_t* owner;
} thread_signal_t;


extern signal_t signals[SIG_NUM];

void thread_signal_create    ();

int                 signal_send                ( 
                                             signal_t *sig);

void                def_signal_handler         (signal_type sig);

void                signal_ignore              (signal_type sig);    

int                 signal_wait                ( 
                                             signal_t *sig);

signal_t    *       get_signal              (signal_type type);

int                 thread_signal_free      (thread_signal_t *th_sig);

void                sigaction_t             (signal_t *sig, sig_handler new_handler);

#endif /* __SIGNALS_H__ */