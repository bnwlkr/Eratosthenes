/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <math.h>
#include <string.h>
#include "utils.h"


#define STACKDIR - // set to + for upwards and - for downwards
#define STACKSIZE (1<<12)

static jmp_buf thread[1000];
static void *coarg;
static char *tos; // top of stack
static char * primes;

void *coto(jmp_buf here, jmp_buf there, void *arg) {
  coarg = arg;
  printf("in coto\n");
  if (setjmp(here)) { return(coarg); }
  longjmp(there, 1);
}

void *cogo(jmp_buf here, void (*fun)(void*), void *arg) {
  if (tos == NULL) { tos = (char*)&arg; }
  tos += STACKDIR STACKSIZE;
  char n[STACKDIR (tos - (char*)&arg)];
  coarg = n; // ensure optimizer keeps n
  if (setjmp(here)) { return(coarg); }
  fun(arg);
  abort();
}

static int prime;
static int n;


// spawn a coroutine to filter this prime's multiples
static void filter (void * arg) {
  int p = *(int*)arg;
  // printf("in filterer for: %d\n", p);
  for (int i = pow(p,2); i<=n; i+=p) {
    primes[i] = 1;
  }
  coto(thread[1], thread[0], (char*) arg + 2);
}


int main(int argc, char* argv[]) {
  if (argc < 2) { printf("too few arguments\n"); exit(EXIT_FAILURE); }
  n = atoi(argv[1]);
  primes = malloc((n+1)*sizeof(char));
  memset(primes, 0, n+1);
  primes[0] = 1;
  primes[1] = 1;
  for (int p = 2; p <= sqrt(n); p++) {
    if (!primes[p]) {
      // printf("spawning filterer for: %d\n", p);
      prime = p;
      cogo(thread[0], filter, &prime);
      // printf("back in main\n");
    }
  }
  print(primes, n+1);
  return 0;
}
