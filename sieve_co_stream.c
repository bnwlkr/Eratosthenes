#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "aco.h"


#define WORKERS_INCREMENT 10 // the number of new workers to allocate when the current number is exchausted

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
          workers = realloc(workers, (worker_capacity + WORKERS_INCREMENT) * sizeof(struct Worker)); // allocate more space for workers
          if (!workers) {
            perror("couldn't reallocate workers\n");
            exit(EXIT_FAILURE);
          }
        }
        workers[idx+1] = (struct Worker) {.routine = aco_create(self->routine->main_co, self->routine->share_stack, 0, worker, &workers[idx+1]), .pipe = num}; // create new worker
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
