#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"

/* test de l'équité de la cooperation via des thread_yield().
 *
 * plus la valeur est haute et proche de 3000, plus l'ordonnancement cooperatif est equitable.
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - thread_exit()
 * - thread_join()
 */

static unsigned v[3];
static int fini = 0;
static double score = 0;

static void * thfunc(void *arg)
{
  unsigned long myid = (unsigned long) arg;
  int err, i;
  for(i=0; i<1000 && !fini; i++) {
    err = thread_yield();
    assert(!err);
    v[myid]++;
    if (v[myid] == 1000 && !fini) {
      fini = 1;
      printf("le thread %lu a terminé\n", myid);
      printf("nombres de yield: %u %u %u\n", v[0], v[1], v[2]);
      score = (v[0]+v[1]+v[2]) / 3000.;
      printf("score: %lf\n", score );
    }
  }

  return NULL;
}

int main()
{
  thread_t th1, th2;
  int err;

  v[0] = v[1] = v[2] = 0;
  err = thread_create(&th1, thfunc, (void*)0UL);
  assert(!err);
  err = thread_create(&th2, thfunc, (void*)1UL);
  assert(!err);

  thfunc((void*)2UL);

  err = thread_join(th2, NULL);
  assert(!err);
  err = thread_join(th1, NULL);
  assert(!err);

  if ( score < .5 ) {
      return EXIT_FAILURE;
  }
  else {
      return EXIT_SUCCESS;
  }
}