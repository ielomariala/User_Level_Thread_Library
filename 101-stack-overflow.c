#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include "thread.h"

static void* stack_overflow(void* v __attribute__((unused))){
    return stack_overflow(v);
}

int main()
{
    int err;
    thread_t th;
    err = thread_create(&th, stack_overflow, (void*)(NULL));
    assert(!err);    
    err = thread_join(th, NULL);
    assert(err == -1);
    return 0;
}