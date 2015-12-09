#include "common.h"
#include "debug.h"
#include "double_queue.h"

#include <stdio.h>
#include <stdlib.h>

static int LiczbaFirm;
static int OgraniczenieArtefaktow;
static int OplataStala;

static double_queue_t mqueue;		// queue managed by museum


typedef struct {
  int id;         // identyfikator firmy
  int saldo;      // saldo firmy
  int k;          // liczba pracowników
} firma_t;

static firma_t *Firmy;

void cleanup() {
	bold("Running cleanup...");
}

int main(int argc, const char *argv[]) {
  //
  // Czytanie danych
  //
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
  log("Creating message queues for museum.");

	if(double_queue_init(&mqueue, key_input, key_output, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
		error("Failed creating message queue.");
		cleanup();
		return 1;
	}
	
	{
		log("Connecting to museum and reporting number of queues.");
		char connect_msg[1+sizeof(int)];
		connect_msg[0] = ACT_BANK_CONNECT;
		memcpy(connect_msg+1, &LiczbaFirm, sizeof(int));
		char *response;
		size_t response_size;
		int is_notification;
		if(double_queue_query(&mqueue, connect_msg, 1+sizeof(int), &response, &response_size, &is_notification, bank_ip, museum_ip) == -1) {
			error("Error connecting to museum.");
			cleanup();
			return 1;
		}
		if(is_notification) {
			error("Expected OK message, not a notification.");
			cleanup();
			return 1;
		}
		if(response_size != 1 && response[0] != ACT_OK) {
			error("Expected OK message - something went wrong.");
			cleanup();
			return 1;
		}
	}
  
  log("Reading data from stdin.");
  if((Firmy = malloc(LiczbaFirm * sizeof(firma_t))) == NULL) {
    error("Allocation failed\n");
    return 1;
  }  
  for(int i = 0; i < LiczbaFirm; i++) {
    scanf("%d %d %d", &(Firmy[i].id), &(Firmy[i].saldo), &(Firmy[i].k));
  }
}
