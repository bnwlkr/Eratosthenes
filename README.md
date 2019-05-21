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

<h2>Array-based Subroutine Sieve</h2>

I started by implementing a subroutine (normal function calls) version of the sieve to familiarize myself with how it works and to make comparisons with the coroutine version. 

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

<h2>Array-based Coroutine Sieve</h2>

After looking at the example at the example provided by Tony Finch in the link above, I started implementing the coroutined version. The coroutined version simply spawns a new worker every time it encounters a prime, in order to filter out its multiples.


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

<h2> A Streaming Sieve </h2>

Following up on the discussion about streaming scenarios above, I decided to implement my own prime number streamer using the [libaco](https://github.com/hnes/libaco) coroutine library. Here is the code:

```
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "aco.h"


#define WORKERS_INCREMENT 10

enum Task {RUN_NEXT, RESTART} task;

struct Worker {
  aco_t* routine;
  int pipe;
};

struct Worker * workers;
static int num_workers = 1;
static int worker_capacity = WORKERS_INCREMENT;

void worker() {
  struct Worker * self = (struct Worker *) aco_get_arg();
  int idx = (self - workers) / sizeof(struct Worker);
  int prime = self->pipe;
  printf("%d\n", prime);
  task = RESTART;
  aco_yield();
  while (1) {
    int num = self->pipe;
    assert(num);
    if (num % prime != 0) {
      if (idx == num_workers-1) { // this is the current last in the pipeline
        if (idx == worker_capacity-1) { // worker capacity reached
          workers = realloc(workers, (worker_capacity + WORKERS_INCREMENT) * sizeof(struct Worker));
          if (!workers) {
            perror("couldn't reallocate workers\n");
            exit(EXIT_FAILURE);
          }
        }
        workers[idx+1] = (struct Worker) {.routine = aco_create(self->routine->main_co, self->routine->share_stack, 0, worker, &workers[idx+1]), .pipe = num};
        task = RUN_NEXT;
        aco_yield();
      } else {
        workers[idx+1].pipe = num; // send the value to the next worker
        task = RUN_NEXT;
        aco_yield();
      }
    } else {
      task = RESTART;
      aco_yield();
    }
  }
}


int main() {  
  aco_thread_init(NULL);
  aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
  aco_share_stack_t* sstk = aco_share_stack_new(0);
  workers = malloc(WORKERS_INCREMENT * sizeof(struct Worker));
  workers[0] = (struct Worker) {.routine = aco_create(main_co, sstk, 0, worker, &workers[0])};
  int next = 2;
  while (1) {
    int worker_idx = 0;
    workers[0].pipe = next++;
    aco_resume(workers[0].routine); // start the first worker
    switch(task) {
      case RUN_NEXT:
        aco_resume(workers[++worker_idx].routine);
      case RESTART:
        continue;
    }
  }
}
```

This prints out prime numbers indefinitely - or until there is no more space for new filtering coroutines (workers) on the heap.

<h2> A Sieve Server </h2>

The next thing to do was try out [libuv](https://github.com/libuv/libuv). To do this I decided to implement a simple UDP server that responds to requests asking whether or not a given number is prime. Here is the code:

```
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_DATA_SIZE 512

int is_prime(int num) {
     if (num <= 1) return 0;
     if (num % 2 == 0 && num > 2) return 0;
     for(int i = 3; i < num / 2; i+= 2) {
         if (num % i == 0)
             return 0;
     }
     return 1;
}

void alloc_recv_buf(uv_handle_t *receive_handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char*) malloc(sizeof(char)*MAX_DATA_SIZE); 
  buf->len = MAX_DATA_SIZE;
}

void udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
  char str[MAX_DATA_SIZE];
  strncpy(str, buf->base, nread);
  str[nread] = '\0';
  char * end;
  int req = (int) strtol(str, &end, 10);
  uv_buf_t sndbuf;
  if (is_prime(req)) {
    sndbuf = uv_buf_init("1", 1);
  } else {
    sndbuf = uv_buf_init("0", 1);
  }
  uv_udp_send(NULL, handle, &sndbuf, 1, addr, NULL);
  free(sndbuf.base);
  free(buf->base);
}


int main () {
  struct sockaddr_in server_addr;
  uv_udp_t udp_handle;
  uv_loop_t* loop;
  loop = uv_default_loop();
  uv_ip4_addr("0.0.0.0", 8000, &server_addr);
  uv_udp_init(loop, &udp_handle);
  uv_udp_bind(&udp_handle, (struct sockaddr *)&server_addr, 0);
  uv_udp_recv_start(&udp_handle, alloc_recv_buf, udp_recv);
  uv_run(loop, UV_RUN_DEFAULT);
}

```

The server creates an event loop and registers UDP receipt as an event that the event loop should be interested in. When it receives a request, it simply checks if the numbers is prime and responds accordingly. The next thing to do is to attempt to integrate `libaco` with `libuv`. I don't believe this is possible in a single thread. The `libaco` coroutine stack will likely run in a separate thread to the I/O loop created by `libuv`, and then they will exchange information. I will look into this today.
















