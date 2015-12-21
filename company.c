#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "worker.h"
#include "util.h"

#include "bank_utils.h"

#include <stdlib.h>
#include <unistd.h>

int idx;                        // company index
int id;                         // company identifier
int number_of_workers;          // number of workers
long ip;                        // company ip
long workers_base_ip;           // worker base ip
long long int saldo = -1;
int max_artifact;

int do_exit;                    // when we can exit server

double_queue_t bqueue;          // bank message queue
double_queue_t mqueue;          // museum message queue
double_queue_t cqueue;          // company message queue
double_queue_t *bQ = &bqueue;
double_queue_t *mQ = &mqueue;
double_queue_t *cQ = &cqueue;

pthread_t *workers;             // worker threads

int workers_working = 0;        // to know if we need to buy new permission

int artifacts[1000*1000+1];


void send_report() {
  if(saldo == -1) return;
  char *data = malloc(16*1000*1000);
  int pos = 0;
  if(data) {
    pos = sprintf(data, "%d %lld\n", id, saldo);
    for(int i = 2; i < max_artifact; i++) {
      if(artifacts[i] > 0) {
        pos += sprintf(data + pos, "%d %d\n", i, artifacts[i]);
      }
    }
    // send data to museum
    {
      void *query, *response;
      size_t qlen, resplen;
      combine(&query, &qlen, "%c %i %i", ACT_MUSEUM_REPORT_INIT, id, pos);
      int exit_after_signal = 0;
      if(double_queue_query(mQ, query, qlen, &response, &resplen, &exit_after_signal, ip, museum_ip) == -1) {
        error("Failed sending report init query.");
      }
      free(query);
      if(match(response, resplen, "@c", ACT_OK) != 0) {
        error("Wrong response - maby some error.");
      }
      free(response);
    }
    int cpos = 0;
    while(cpos < pos) {
      int len = (pos - cpos > 1000)?1000:(pos - cpos);
      void *query, *response;
      size_t qlen, resplen;
      combine(&query, &qlen, "%c %i %bv", ACT_MUSEUM_SEND_REPORT_PART, id, data + cpos, len);
      int exit_after_signal = 0;
      if(double_queue_query(mQ, query, qlen, &response, &resplen, &exit_after_signal, ip, museum_ip) == -1) {
        error("Failed sending report init query.");
      }
      free(query);
      if(match(response, resplen, "@c", ACT_OK) != 0) {
        error("Wrong response - maby some error.");
      }
      free(response);
      cpos += len;
    }
    free(data);
  } else {
    // No report needed cause we can not allocate buffer
    error("Report sending failed.");
  }
}

void cleanup() {
  bold("Running cleanup.");
  // Sending exit commands...
  {
    void *query;
    size_t qlen;
    combine(&query, &qlen, "%c", ACT_MUSEUM_COMPANY_EXIT);
    void *response;
    size_t resplen;
    int exit_after_signal = 0;
    if(double_queue_query(mQ, query, qlen, &response, &resplen, &exit_after_signal, ip, museum_ip) == -1) {
      error("Failed sending query to bank.");
    }
    free(query);
    if(match(response, resplen, "@c", ACT_OK) != 0) {
      error("Wrong response - maby some error.");
    }
    free(response);
  }
  {
    void *query;
    size_t qlen;
    combine(&query, &qlen, "%c", ACT_BANK_COMPANY_EXIT);
    void *response;
    size_t resplen;
    int exit_after_signal = 0;
    if(double_queue_query(bQ, query, qlen, &response, &resplen, &exit_after_signal, ip, bank_ip) == -1) {
      error("Failed sending query to bank.");
    }
    free(query);
    if(match(response, resplen, "@c", ACT_OK) != 0) {
      error("Wrong response - maby some error.");
    }
    free(response);
  }

  log("Exitting cleanup.");
  return;
}

// main function for company
int main(int argc, char *argv[]) {
  bold("Starting company...");
  
  // Getting initialization data from argv
  if(argc != 6) {
    error("Wrong number of parameters...");
    cleanup();
    return 1;
  }
  
  if(sscanf(argv[1], "%d", &idx) == -1) {
    error("First argument is not a number.");
    cleanup();
    return 1;
  }
  if(sscanf(argv[2], "%d", &id) == -1) {
    error("Second argument is not a number.");
    cleanup();
    return 1;
  }
  if(sscanf(argv[3], "%d", &number_of_workers) == -1) {
    error("Third argument is not a number.");
    cleanup();
    return 1;
  }
  if(sscanf(argv[4], "%ld", &ip) == -1) {
    error("Fourth argument is not a number.");
    cleanup();
    return 1;
  }
  if(sscanf(argv[5], "%ld", &workers_base_ip) == -1) {
    error("Sixth argument is not a number.");
    cleanup();
    return 1;
  }
  

  log("Connecting to bank and museum...");
  {
  	if(double_queue_init(mQ, 0, museum_key1, museum_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
    if(double_queue_init(bQ, 0, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
  	if(double_queue_init(cQ, 0, company_key1, company_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		cleanup();
  		return 1;
  	}
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
  saldo = get_money_of_company(bQ, idx, id, ip);
  if(saldo == -1) {
    cleanup();
    return 1;
  }
  log("We have %lld units of gold.", saldo);
  /*
  while(!do_exit) {
    // if workers_working == 0 then get new permission or exit company
    // if some collection is full, sell it
    // else wait for being woken
  }
  */
  sleep(5);
  send_report();
  cleanup();
  return 0;
}
