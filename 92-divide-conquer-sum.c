#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

unsigned long tab[100];

struct args{
    int inf;
    int sup;
    int len;
};

/* 
 * Divide and conquer sum
 */
static void * sum(void *arg)
{
  thread_t th, th2;
  int err;
  void *res = NULL, *res2 = NULL;
  struct args * a = (struct args *) arg;
  /* on passe un peu la main aux autres pour eviter de faire uniquement la partie gauche de l'arbre */
  thread_yield();

    int m = (a->inf+a->sup)/2-1;
    if(a->len <= 2){
        if (a->len == 1)
          return (void*) tab[a->inf];
        else if (a->len == 2)
          return (void*) (tab[a->inf]+tab[a->sup]);
        else {
          printf("ERROR: length = %d\n", a->len);
          return (void*) 0;
        }
    }
    else {
        struct args a1;
        a1.inf = a->inf;
        a1.sup = m;
        a1.len = a1.sup - a1.inf + 1;
        err = thread_create(&th, sum, (void*)(&a1));
        assert(!err);

        struct args a2;
        a2.inf = m + 1;
        a2.sup = a->sup;
        a2.len = a2.sup - a2.inf + 1;
        err = thread_create(&th2, sum, (void*)(&a2));
        assert(!err);

        err = thread_join(th, &res);
        assert(!err);
        err = thread_join(th2, &res2);
        assert(!err);
    }

  return (void*)((unsigned long) res + (unsigned long) res2);
}

int main(int argc, char *argv[])
{
  unsigned long res;
  struct timeval tv1, tv2;
  double s;
  unsigned long length = 0;

  if (argc < 2) {
    printf("arguments manquant: plusieurs nombres constituants le tableau à sommer\n");
    return -1;
  }

  int i;
  for(i = 0; i < argc - 1; i++){
    tab[i] = atoi(argv[i+1]);
  }
  length = argc - 1;
  struct args arg;
  arg.inf = 0;
  arg.sup = length - 1;
  arg.len = arg.sup - arg.inf + 1 ;
  gettimeofday(&tv1, NULL);
  res = (unsigned long) sum((void *)&arg);
  gettimeofday(&tv2, NULL);
  s = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;
  printf("Somme de tableau est %ld obtenu en %e s \n", res, s);
  return 0;
}