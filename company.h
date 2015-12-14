#ifndef _ARCHEO_COMPANY_H_
#define _ARCHEO_COMPANY_H_

#include "double_queue.h"

typedef struct {
  int idx;
  int id;         // identyfikator firmy
  int k;          // liczba pracowników
  long ip;         // ip firmy
  long worker_waitroom;
  long worker_base_ip;        // bazowe ip workerów - będą oni mieli adresy: wip+0, wip+1, ..., wip+k-1
  double_queue_t *cQ;
} company_init_t;

typedef struct {
  int idx;
  int id;
  int *artifacts;
} company_report_t;

void * company(void *arg);

#endif /* end of include guard: _ARCHEO_COMPANY_H_ */
