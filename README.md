# Eratosthenes

This is a report on an implementation of the Sieve of Eratosthenes (for UBC directed studies) using a [tiny coroutine library](https://fanf.livejournal.com/105413.html) in standard C. Here is the code:

```
#define STACKDIR - // set to + for upwards and - for downwards
#define STACKSIZE (1<<12)

static jmp_buf thread[1000];
static void *coarg;
static char *tos; // top of stack
static char * primes;
static int prime;
static int n;

void *coto(jmp_buf here, jmp_buf there, void *arg) {
  coarg = arg;
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
```

`cogo` initializes coroutines, and `coto` passes control between them. 

I started by implementing a subroutine (normal function calls) version of the sieve to familiarize myself with how it works and to make comparisons with the coroutine version. 

<h2>sieve_sub</h2>

```
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
  primes[0] = 1; primes[1] = 1;
  for (int p = 2; p <= sqrt(n); p++) {
    if (!primes[p]) {
      filter(&primes, p, n);
    }
  }
  return 0;
}
```

After looking at the example at the example provided by Tony Finch in the link above, I started implementing the coroutined version. The coroutined version simply spawns a new worker every time it encounters a prime, in order to filter out its multiples.


<h2>sieve_co_array</h2>

```
static void filter (void * arg) {
  int p = *(int*)arg;
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
      prime = p;
      cogo(thread[0], filter, &prime); // spawn a coroutine to filter this prime's multiples
    }
  }
  return 0;
}

```

When I started looking into writing a coroutined version of the sieve, I was confused about how the coroutines (instead of subroutines) would yield any improvement in performance. If we start with an array of numbers that need to be filtered for primes, every number needs to be looked at at least once in both cases. If the coroutines are running one after the other, we shouldn't see any improvement in performance (and perhaps a decline in performance because of the slight overhead of dealing with the coroutines). The timing results concur:
<h4> ./sieve_sub 10000000 </h4>

```
real	0m0.154s
user	0m0.142s
sys	0m0.007s
```

<h4> ./sieve_co_array 10000000 </h4>

```
real	0m0.160s
user	0m0.141s
sys	0m0.010s
```

To paraphrase my professor, the coroutined implementation doesn't provide any advantage, but it allows the computation to be structured in a way that it could be parallelized. If all of the filtering workers were to run in parallel, the performance would improve approximately by a factor of the number of filtering workers, `sqrt(n)`.

<h5>TODO: test this assumption with threads</h3>

Another potential reason to use coroutines for the sieve is in a streaming situation. If we aren't sure how many prime numbers we need when the program starts, using subroutines would become problematic. We would start filtering all of the factors of 2 until... forever. If we stopped at some pre-defined bound we would have to keep track of where each filtering subroutine was in the process, which would basically make them centrally-coordinated coroutines. Filtering coroutines could instead periodically yield their results to their filtering coroutine friends in a filtering pipeline (while automatically remembering where they were in their computations). This would allow the program to output a continuous stream of prime numbers indefinitely (a [generator](https://matthias.benkard.de/journal/116)).

<h2>Classification of Small Portable Coroutines</h2>

I will attempt to classify the miniature coroutine library according to [this](http://www.inf.puc-rio.br/~roberto/docs/MCC15-04.pdf) paper.

<h4> Control Transfer Mechanism </h4>

This coroutine library creates symmetric coroutines, i.e. coroutines can pass control to eachother using `coto`.

<h4> Class </h4>

The coroutines created by this library are constrained. They cannot be treated as first-class objects.

<h4> Stackfulness </h4>

Coroutines all run on the same stack. The coroutines can be suspended and resumed from within nested functions.

<h2> A streaming sieve </h2>


















