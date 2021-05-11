#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

int x;
int tab[100];

struct args{
    int inf;
    int sup;
};

/* 
 * Divide and conquer search (Dichotomic but without necesserally sorted table)
 */
static void * dichotomic(void *arg)
{
  thread_t th, th2;
  int err;
  void *res = NULL, *res2 = NULL;
  struct args * a = (struct args *) arg;
  /* on passe un peu la main aux autres pour eviter de faire uniquement la partie gauche de l'arbre */
  thread_yield();

  int m = (a->inf+a->sup)/2;
    if(tab[m] == x)
        return (void*) 1;
    else if (a->inf >= a->sup){
        return (void *) 0;
    }
    else {
        struct args a1;
        a1.inf = a->inf;
        a1.sup = m-1;
        err = thread_create(&th, dichotomic, (void*)(&a1));
        assert(!err);

        struct args a2;
        a2.inf = m+1;
        a2.sup = a->sup;
        err = thread_create(&th2, dichotomic, (void*)(&a2));
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

  if (argc < 3) {
    printf("arguments manquant: entier x recherché, plusieurs nombres constituants le tableau \n");
    return -1;
  }

  x = atoi(argv[1]);
  int i;
  for(i = 0; i < argc - 2; i++){
    tab[i] = atoi(argv[i+2]);
  }
  length = argc - 2;
  struct args arg;
  arg.inf = 0;
  arg.sup = length;
  gettimeofday(&tv1, NULL);
  res = (unsigned long) dichotomic((void *)&arg);
  gettimeofday(&tv2, NULL);
  s = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;
  if (res){ 
    printf("%d found in given table in %e s\n", x, s );
  }
  else{
    printf("%d not found in given table in %e s\n", x, s );
  }
  return 0;
}