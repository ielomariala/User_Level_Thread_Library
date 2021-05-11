#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

int tab[100];
int tmp[100];
unsigned long length;

struct args{
    int i;
    int j;
};

/* 
 * Fusion sort 
 */
static void *fusionSort(void *arg) {

    thread_t th, th2;
    int err;
    struct args *a = (struct args *) arg;

    /* on passe un peu la main aux autres pour eviter de faire uniquement la partie gauche de l'arbre */
    thread_yield();
    if(a->j <= a->i){
        return (void*)NULL;
    }
    int m = (a->i + a->j)/2;
 
    struct args a1;
    a1.i = a->i;
    a1.j = m;
    err = thread_create(&th, fusionSort, (void*)(&a1));
    assert(!err);
    struct args a2;
    a2.i = m+1;
    a2.j = a->j;
    err = thread_create(&th2, fusionSort, (void*)(&a2));

    err = thread_join(th, NULL);
    assert(!err);
    err = thread_join(th2, NULL);
    assert(!err); 
    

    int pg = a->i;     //indice de début du sous-tableau de gauche
    int pd = m + 1; //indice de début du sous-tableau de droite
    int k;
    for(k = a->i; k <= a->j; k++) {
        if(pg == m + 1) { //sous-tableau gauche fini
            tmp[k] = tab[pd];
            pd++;
        }
        else if (pd == a->j + 1) { //sous-tableau droite fini
            tmp[k] = tab[pg];
            pg++;
        }
        /* Paragraphe de triage */
        else if (tab[pg] < tab[pd]) {
            tmp[k] = tab[pg];
            pg++;
        }
        else { 
            tmp[k] = tab[pd];
            pd++;
        }
    }

    for(k = a->i; k <= a->j; k++) {
        tab[k] = tmp[k];
    }
    return (void*) NULL;
}

int main(int argc, char *argv[])
{
    //unsigned long value, res;
    struct timeval tv1, tv2;
    struct args arg;
    double s;


    if (argc < 2) {
        printf("argument manquant: plusieurs nombres à trier (en moins un) ");
        return -1;
    }

    int i = 0;
    for(i = 0; i < argc - 1; i++){
        tab[i] = atoi(argv[i+1]);
    }
    length = argc - 1;
    arg.i = 0;
    arg.j = length - 1; 
    gettimeofday(&tv1, NULL);
    fusionSort((void *)&arg);
    gettimeofday(&tv2, NULL);
    s = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;

    for(i = 0; (unsigned) i < length; i++) {
        printf(" %d\t", tab[i]);
    }
    printf("tableau trié en %e s\n", s );
    return 0;
}