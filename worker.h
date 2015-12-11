#ifndef _ARCHEO_WORKER_H_
#define _ARCHEO_WORKER_H_

typedef struct {
  int idx;
  int cidx;
  long boss_ip;
  long ip;
} worker_init_t;

void * worker(void *arg);

#endif /* end of include guard: _ARCHEO_WORKER_H_ */
