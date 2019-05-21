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
