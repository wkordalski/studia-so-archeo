#include "worker.h"

#include "debug.h"

#include <stdlib.h>

typedef struct {
  // TODO
} worker_t;

void cleanup_worker(worker_t *global) {
  // TODO
}

void * worker(void *arg) {
  // TODO
  worker_t *global;
  worker_init_t *wrk_init = (worker_init_t*)arg;

  bold("Starting worker %d of company %d.", wrk_init->idx, wrk_init->cidx);
  /*
  if(double_queue_init(global->bQ, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
    error("Failed creating message queue.");
    cleanup_company(global);
    return NULL;
  }
  */
  return NULL;
}
