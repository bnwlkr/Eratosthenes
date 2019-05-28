<h1> Eratosthenes </h1>

This is a report for a UBC directed studies in concurrency and parallelism. I will look at various related libraries in C, and implement the [Sieve of Eratosthenes](https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes) as a common thread (heh) between them for the sake of comparison.

<h2> Small Portable Coroutines </h2>

To start off, I looked at a [tiny coroutine library](https://fanf.livejournal.com/105413.html). Here is all of the library's code:

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

<h4>Array-based Subroutine Sieve</h4>

I started by implementing a subroutine (normal function calls) version of the sieve to familiarize myself with how it works and to make comparisons with the coroutine version. code: [sieve_sub.c](sieve_sub.c)

<h4>Array-based Coroutine Sieve</h4>

After looking at the example at the example provided by Tony Finch in the link above, I started implementing the coroutined version. The coroutined version simply spawns a new worker every time it encounters a prime, in order to filter out its multiples. code: [sieve_co_array.c](sieve_co_array.c)

When I started looking into writing a coroutined version of the sieve, I was confused about how the coroutines (instead of subroutines) would yield any improvement in performance. If we start with an array of numbers that need to be filtered for primes, every number needs to be looked at at least once in both cases. If the coroutines are running one after the other, we shouldn't see any improvement in performance (and perhaps a decline in performance because of the slight overhead of dealing with the coroutines). The timing results concur:
<h5> ./sieve_sub 10000000 </h5>

```
real	0m0.154s
user	0m0.142s
sys	0m0.007s
```

<h5> ./sieve_co_array 10000000 </h5>

```
real	0m0.160s
user	0m0.141s
sys	0m0.010s
```

To paraphrase my professor, the coroutined implementation doesn't provide any advantage, but it allows the computation to be structured in a way that it could be parallelized. If all of the filtering workers were to run in parallel, the performance would improve approximately by a factor of the number of filtering workers, `sqrt(n)`.

<h5>TODO: test this assumption with threads</h3>

Another potential reason to use coroutines for the sieve is in a streaming situation. If we aren't sure how many prime numbers we need when the program starts, using subroutines would become problematic. We would start filtering all of the factors of 2 until... forever. If we stopped at some pre-defined bound we would have to keep track of where each filtering subroutine was in the process, which would basically make them centrally-coordinated coroutines. Filtering coroutines could instead periodically yield their results to their filtering coroutine friends in a filtering pipeline (while automatically remembering where they were in their computations). This would allow the program to output a continuous stream of prime numbers indefinitely (a [generator](https://matthias.benkard.de/journal/116)).

<h4> Classification </h4>

I will attempt to classify the small portable coroutine library according to [this](http://www.inf.puc-rio.br/~roberto/docs/MCC15-04.pdf) paper.

<h5> Control Transfer Mechanism </h5>

This coroutine library creates symmetric coroutines, i.e. coroutines can pass control to eachother using `coto`.

<h5> Class </h5>

The coroutines created by this library are constrained. They cannot be treated as first-class objects.

<h5> Stackfulness </h5>

Coroutines all run on the same stack. The coroutines can be suspended and resumed from within nested functions.

<h2> libaco </h2>

<h4> A Streaming Sieve </h4>

Following up on the discussion about streaming scenarios above, I decided to implement my own prime number streamer using the [libaco](https://github.com/hnes/libaco) coroutine library. code: [sieve_co_stream.c](sieve_co_stream.c)

This prints out prime numbers indefinitely - or until there is no more space for new filtering coroutines (workers) on the heap. I can modify the code so that it terminates at a certain number of primes by including a `FINISHED` case in the `Task` enum that informs the main_co that it should stop sending numbers into the pipeline, deallocate all of the workers, and then exit.


<h2> libuv </h2>

The next thing to do was to try out the [libuv](github.com/libuv/libuv) library. This sieve works by having the worker routines be async callbacks registered on a `uv_async_t` handle in the event loop. The workers call eachother to send data down the pipeline. code: [sieve_luv.c](sieve_luv.c)


<h2> MPICH </h2>

Moving on to interprocess communication, I implemented the sieve using `MPICH`, an instance of the Message Passing Interface standard. In this case, the workers are heavy-weight processes that pass eachother numbers in MPI messages. As expected, the overhead to setup the processes and the OS switching between processes during execution makes this implementation pretty slow. Here are the timing results for 10 primes for `sieve_mpi`:


```
real	0m0.272s
user	0m0.281s
sys	0m0.254s
```

And 10 primes using `libuv` callbacks:

```
real	0m0.007s
user	0m0.002s
sys	0m0.003s
```

Obviously, the point of this part of the investigation is not to compare performance, but its cool to see some evidence of how things are working under the hood. There's a big difference between one thread calling a new function (as in `libuv`) and a context switch between threads (as in `MPICH`)!

And here's the code: [sieve_mpi.c](sieve_mpi.c)








