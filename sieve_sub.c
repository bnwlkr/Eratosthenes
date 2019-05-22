#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

// generate prime numbers up to n be filtering with subroutines

void filter(char ** primes, int p, int n) {
  for (int i = pow(p,2); i<=n; i+=p) {
    (*primes)[i] = 1;
  }
}

int main (int argc, char * argv[]) {
  if (argc < 2) { printf("too few arguments\n"); exit(EXIT_FAILURE); }
  int n = atoi(argv[1]);
  char * primes = malloc((n+1)*sizeof(char));
  memset(primes, 0, n+1);
  primes[0] = 1;
  primes[1] = 1;
  for (int p = 2; p <= sqrt(n); p++) {
    if (!primes[p]) {
      filter(&primes, p, n);
    }
  }
  printf("[");
  for (int i = 0; i <= n; i++) {
    if (primes[i] == 0) {
      printf(" %d ", i);
    }
  }
  printf("]\n");
  return 0;
}
