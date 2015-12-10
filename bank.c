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



typedef struct {
  int id;
  int saldo;
} konto_t;

static konto_t *Konta;
static pthread_t *Firmy;

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
  (*is_notification) = 0;
  return 0;
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
  log("Creating message queues for bank.");
  {
  	if(double_queue_init(bQ, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
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

  // TODO
  cleanup();
  return 0;
}
