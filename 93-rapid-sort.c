#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

int tab[100];

struct args{
    int first;
    int last;
};

void swap(int *a, int *b) {
    int tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

/* 
 * Rapid sort 
 */
static void *rapidSort(void *arg) {

    thread_t th, th2;
    int err;
    struct args *a = (struct args *) arg;

    /* on passe un peu la main aux autres pour eviter de faire uniquement la partie gauche de l'arbre */
    thread_yield();

    int pivot, i, j;
    if( ((struct args *) arg)->first <((struct args *) arg)->last) {
        pivot = a->first;
        i = a->first;
        j = a->last;
        while (i < j) {
            while(tab[i] <= tab[pivot] && i < a->last)
                i++;
            while(tab[j] > tab[pivot])
                j--;
            if(i < j) {
                swap(&(tab[i]), &(tab[j]));
            }
        }
        swap(&(tab[pivot]), &(tab[j]));
        struct args a1;
        a1.first = a->first;
        a1.last = j-1;
        err = thread_create(&th, rapidSort, (void*)(&a1));
        assert(!err);
        struct args a2;
        a2.first = j+1;
        a2.last = a->last;
        err = thread_create(&th2, rapidSort, (void*)(&a2));
        assert(!err);
        err = thread_join(th, NULL);
        assert(!err);
        err = thread_join(th2, NULL);
        assert(!err); 
    }
    return (void*)NULL;
}

int main(int argc, char *argv[])
{
    //unsigned long value, res;
    struct timeval tv1, tv2;
    struct args arg;
    double s;
    unsigned long length;

    if (argc < 2) {
        printf("argument manquant: plusieurs nombres à trier (en moins un) ");
        return -1;
    }

    int i = 0;
    for(i = 0; i < argc - 1; i++){
        tab[i] = atoi(argv[i+1]);
        //printf("tab %d\n", tab[i]);
    }
    length = argc - 1;
    arg.first = 0;
    arg.last = length - 1; 
    gettimeofday(&tv1, NULL);
    rapidSort((void *)&arg);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;

    for(i = 0; (unsigned) i < length; i++)  {
        printf(" %d\t", tab[i]);
    }
    printf("tableau trié en %e s\n", s );
    return 0;
}
