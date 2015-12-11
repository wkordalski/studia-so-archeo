#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "company.h"

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

static int LiczbaFirm;
static int OgraniczenieArtefaktow;
static int OplataStala;

static double_queue_t bqueue;
static double_queue_t *bQ = &bqueue;

static double_queue_t cqueue;   // Queue for companies
static double_queue_t *cQ = &cqueue;



typedef struct {
  int id;
  long long int saldo;
} konto_t;

static konto_t *Konta;
static pthread_t *Firmy;

static int do_exit = 0;

void cleanup() {
	bold("Running cleanup...");

  log("Waiting for company threads.");
  if(Firmy != NULL) {
    for(int i = 0; i < LiczbaFirm; i++) {
      void *status;
      if(Firmy[i] != 0) pthread_join(Firmy[i], &status);
    }
  }

  log("Closing message queue,");
  double_queue_close(bQ);

  log("Freeing memory");
  if(Firmy != NULL) free(Firmy);
  if(Konta != NULL) free(Konta);
}

int wait_for_museum_connection(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int * is_notification,
      int source) {
  if(source != museum_ip) {
    error("Wrong sender address (waiting from connection from museum).");
    return -1;
  }
  if(query_length != 1 || ((char*)query)[0] != ACT_BANK_WITH_MUSEUM_CONNECT_REQUEST) {
    error("Expected BANK WITH MUSEUM CONNECT REQUEST message.");
    return -1;
  }

  char *resp = malloc(5);
  resp[0] = ACT_OK;
  memcpy(resp + 1, &LiczbaFirm, sizeof(int));
  (*response) = resp;
  (*response_length) = 5;
  return 0;
}

int server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int * is_notification,
      int source) {
  if(query_length <= 0) {
    error("Empty message - something went wrong.");
    return -1;
  }
  char cmd = ((char*)query)[0];
  if(cmd == ACT_BANK_GET_SALDO) {
    if(query_length != 1 + sizeof(int) * 2) {
      error("Wrong message length.");
      return -1;
    }
    int idx;
    int id;
    memcpy(&idx, query + 1, sizeof(int));
    memcpy(&id, query + 1 + sizeof(int), sizeof(int));
    if(idx >= LiczbaFirm || Konta[idx].id != id) {
      warning("Wrong data for get saldo command - access denied - %d %d.", idx, id);
      //print_hexadecimal()
      char *resp = malloc(2);
      resp[0] = ACT_ERROR;
      resp[1] = ERR_ACCESS_DENIED;
      (*response) = resp;
      (*response_length) = 2;
      return 0;
    }
    else {
      char *resp = malloc(1 + sizeof(long long int));
      resp[0] = ACT_OK;
      memcpy(resp + 1, &(Konta[idx].saldo), sizeof(long long int));
      (*response) = resp;
      (*response_length) = 1+sizeof(long long int);
      return 0;
    }
  }
  else {
    error("Unsupported command.");
    return -1;
  }
}

int main(int argc, const char *argv[]) {
  bold("Starting bank process.");

  if(argc != 4) {
    error("Wrong number of command-line parameters.");
    return 1;
  }
  if(sscanf(argv[1], "%d", &LiczbaFirm) == -1) {
    error("Wrong first parameter.\n");
    return 1;
  }
  if(sscanf(argv[2], "%d", &OplataStala) == -1) {
    error("Wrong second parameter.\n");
    return 1;
  }
  if(sscanf(argv[3], "%d", &OgraniczenieArtefaktow) == -1) {
    error("Wrong third parameter.\n");
    return 1;
  }

  //
  // Create message queues
  //
  log("Creating message queues for bank and companies.");
  {
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

  bold("Waiting for museum connection request...");
  {
    if(double_queue_listen(bQ, wait_for_museum_connection, bank_ip) == -1) {
      error("Listening error during waiting for museum connection request.");
      cleanup();
      return 1;
    }
  }

  log("Reading data from stdin.");
  {
    if((Firmy = malloc(LiczbaFirm * sizeof(pthread_t))) == NULL) {
      error("Allocation failed\n");
      cleanup();
      return 1;
    }
    for(int i = 0; i < LiczbaFirm; i++) Firmy[i] = 0;
    if((Konta = malloc(LiczbaFirm * sizeof(konto_t))) == NULL) {
      error("Allocation failed\n");
      cleanup();
      return 1;
    }
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    int worker_base = 3 + LiczbaFirm;
    for(int i = 0; i < LiczbaFirm; i++) {
      company_init_t *cidata = malloc(sizeof(company_init_t));
      if(cidata == NULL) {
        error("Allocation failed.");
        pthread_attr_destroy(&attrs);
        cleanup();
        return 1;
      }
      scanf("%d %d %d", &(cidata->id), &(Konta[i].saldo), &(cidata->k));
      Konta[i].id = cidata->id;
      cidata->idx = i;
      cidata->ip = 3 + i;
      cidata->wip = worker_base;
      worker_base += cidata->k;
      // we can now start new thread
      int ret = pthread_create(Firmy+i, &attrs, company, cidata);
      if(ret != 0) {
        error("Failed with creating thread.");
        pthread_attr_destroy(&attrs);
        cleanup();
        return 1;
      }
    }
    pthread_attr_destroy(&attrs);
  }

  while(!do_exit) {
    if(double_queue_listen(bQ, server, bank_ip) == -1) {
      error("Error during listening for queries.");
      cleanup();
      return 1;
    }
  }
  cleanup();
  return 0;
}
