#include <stdlib.h>
#include <stdio.h>

#include "uv.h"

static int * primes;
static int num_primes;

void worker (uv_async_t* handle) {
  int * data = (int*)handle->data;
  int idx = data[0];
  if (idx == num_primes) { uv_close((uv_handle_t*)handle, NULL); return; }
  int value = data[1];
  if (idx == -1) { // generator
    data[0] = 0;
    data[1] = value+1;
    uv_async_send(handle);
  } else {
    int filter = primes[idx];
    if (!filter) {
      primes[idx] = value;
      data[0] = -1;
      uv_async_send(handle);
    } else {
      if (value % filter) {
        data[0] = idx+1;
        uv_async_send(handle);
      } else {
        data[0] = -1;
        uv_async_send(handle);
      }
    }
  }
}

int main (int argc, char* argv[]) {
  if (argc < 2) {
    printf("how many primes?\n");
    exit(EXIT_FAILURE);
  }
  num_primes = atoi(argv[1]);
  primes = calloc(num_primes, sizeof(int));
  uv_loop_t * loop = uv_default_loop();
  uv_async_t handle;
  uv_async_init(loop, &handle, worker);
  handle.data = malloc(2*sizeof(int));
  handle.data = (int[]){-1,1};
  primes[0] = 2;
  uv_async_send(&handle);
  uv_run(loop, UV_RUN_DEFAULT);
  printf("[");
  for (int i = 0; i < num_primes; i++) {
    printf(" %d ", primes[i]);
  }
  printf("]\n");
  free(primes);
}

