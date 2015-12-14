#include "company.h"

#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "worker.h"
#include "util.h"

#include <stdlib.h>
#include <unistd.h>

int idx;                        // company index
int id;                         // company identifier
long ip;                        // company ip
long wrk_ip;                    // worker base ip
int do_exit;                    // when we can exit server
double_queue_t bqueue;          // bank message queue
double_queue_t mqueue;          // museum message queue
double_queue_t cqueue;          // company message queue
double_queue_t *bQ = &bqueue;
double_queue_t *mQ = &mqueue;
double_queue_t *cQ = &cqueue;
int number_of_workers;          // number of workers
pthread_t *workers;             // worker threads

void cleanup() {
  bold("Running cleanup.");
  // TODO
  return;
}

int server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int * is_notification,
      int source) {
  // TODO
	return -1;
}

// main function for company
int main(int argc, char *argv[]) {
  // for first time we can use PID as IP
  bold("Starting company...");

  log("Connecting to bank and museum...");
  {
  	if(double_queue_init(mQ, museum_key1, museum_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
    if(double_queue_init(bQ, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
  	if(double_queue_init(cQ, company_key1, company_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
  }

  log("Getting initialization data from bank");
	{
		// TODO
	}
  
  log("Starting workers...");
	/*
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
	*/
  // We assume we are honest so we do not have to check for saldo every time
  log("Asking for current saldo.");
	/*
  long long int saldo;
  {
    void *query;
    size_t qlen;
    combine(&query, &qlen, "%c %i %i", ACT_BANK_GET_SALDO, global->idx, global->id);
    char *response;
    size_t resplen;
    if(double_queue_query(global->bQ, query, qlen, (void**)&response, &resplen, &skip_notifications, NULL, global->ip, bank_ip) == -1) {
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
	*/
	// TODO
  sleep(10);
  cleanup();
  return 0;
}
