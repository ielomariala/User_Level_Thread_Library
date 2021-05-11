#include <stdio.h>
#include <assert.h>
#include "thread.h"
#include "signals.h"


void *thread_sender_func() {
    //thread_signal_t *th_sig = (thread_signal_t *)arg;
    signal_t * to_send = get_signal(SIG_1);
    fprintf(stderr, "i'm sender, i send signal SIG_1\n");
    signal_send(to_send);
    //thread_signal_free(th_sig); 
    thread_exit(NULL);
    return (void*) 0xdeadbeef;
}

void *thread_receiver_func() {
    fprintf(stderr, "i'm receiver\n");
    //thread_signal_t *th_sig = (thread_signal_t *)arg;
    fprintf(stderr, "Before sigwait\n");
    signal_wait(get_signal(SIG_1));
    fprintf(stderr, "After Sigwait\n");
    //thread_signal_free(th_sig);
    thread_exit(NULL);
    return (void*) 0xdeadbeef;
}

int main() {
    thread_t th1, th2;
    int err;

    thread_signal_create();

    err = thread_create(&th1, thread_receiver_func, NULL);
    assert(!err);
    
    err = thread_create(&th2, thread_sender_func , NULL);
    assert(!err);

    
    err = thread_join(th1, NULL);
    assert(!err);
    
    err = thread_join(th2, NULL);
    assert(!err);
    
    return 0;
}