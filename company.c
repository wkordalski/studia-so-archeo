#include "company.h"

#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "worker.h"
#include "util.h"

#include <stdlib.h>
#include <unistd.h>

typedef struct {
  int idx;
  int id;
  long ip;
  long wrk_ip;
  int do_exit;
  double_queue_t bqueue;
  double_queue_t mqueue;
  double_queue_t *bQ;
  double_queue_t *mQ;
  double_queue_t *cQ;
  int wrk_no;
  pthread_t *workers;
} company_t;

void cleanup_company(company_t *global) {
  bold("Running cleanup of company %d.", global->idx);

  free(global);
  return;
  // TODO
}

int server_company(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int * is_notification,
      int source) {
  // TODO
}

// main function for company
void* company(void *arg) {
  company_t *global;
  company_init_t *com_init = (company_init_t*)arg;

  bold("Running company %d with id = %d.", com_init->idx, com_init->id);

  if((global = malloc(sizeof(company_t))) == NULL) {
    error("Allocation failed.");
    return NULL;
  }

  global->do_exit = 0;
  global->bQ = &(global->bqueue);
  global->mQ = &(global->mqueue);
  global->cQ = com_init->cQ;
  global->idx = com_init->idx;
  global->id = com_init->id;
  global->ip = com_init->ip;
  global->wrk_ip = com_init->wip;
  global->wrk_no = com_init->k;

  log("Connecting to bank and museum...");
  {
  	if(double_queue_init(global->mQ, museum_key1, museum_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup_company(global);
  		return NULL;
  	}

    if(double_queue_init(global->bQ, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup_company(global);
  		return NULL;
  	}
  }

  log("Starting workers...");
  {
    if((global->workers = malloc(global->wrk_no * sizeof(pthread_t))) == NULL) {
      error("Allocation failed.");
      cleanup_company(global);
      return NULL;
    }
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    for(int i = 0; i < global->wrk_no; i++) {
      worker_init_t *widata = malloc(sizeof(worker_init_t));
      if(widata == NULL) {
        error("Allocation failed.");
        pthread_attr_destroy(&attrs);
        cleanup_company(global);
        return NULL;
      }
      widata->idx = i;
      widata->cidx = global->idx;
      widata->boss_ip = global->ip;
      widata->ip = global->wrk_ip + i;

      int ret = pthread_create(global->workers+i, &attrs, worker, widata);
      if(ret != 0) {
        error("Failed with creating thread.");
        pthread_attr_destroy(&attrs);
        cleanup_company(global);
        return NULL;
      }
    }
    pthread_attr_destroy(&attrs);
  }

  // We assume we are honest so we do not have to check for saldo every time
  log("Asking for current saldo.");
  long long int saldo;
  {
    void *query;
    size_t qlen;
    combine(&query, &qlen, "%c %i %i", ACT_BANK_GET_SALDO, global->idx, global->id);
    char *response;
    size_t resplen;
    int is_notification;
    if(double_queue_query(global->bQ, query, qlen, (void**)&response, &resplen, &is_notification, global->ip, bank_ip) == -1) {
      error("Failed sending query to bank.");
      free(query);
      cleanup_company(global);
      return NULL;
    }
    free(query);
    if(match(response, resplen, "@c %ll", ACT_OK, &saldo) != 0) {
      error("Wrong response - maby some error.");
      free(response);
      cleanup_company(global);
      return NULL;
    }
    free(response);
  }
  log("We have %lld units of gold.", saldo);

  sleep(10);
  cleanup_company(global);
  return 0;
}
