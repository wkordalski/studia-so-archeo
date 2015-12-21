#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "util.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h>

#include <pthread.h>

static int LiczbaFirm;
static int UruchomioneFirmy;
static int UruchomioneProcesy;
static int OgraniczenieArtefaktow;
static int OplataStala;

static double_queue_t bqueue;
static double_queue_t *bQ = &bqueue;

static double_queue_t cqueue;   // Queue for companies
static double_queue_t *cQ = &cqueue;

static pid_t museum_pid = 0;

// for exec...
char ex_pnm[16] = "./company";
char ex_idx[7];
char ex_id [7];
char ex_wrk[7];
char ex_ip [16];
char ex_wip[16];
char ex_qip[16]; 

typedef struct {
  int id;
  long long int saldo;
} konto_t;

static konto_t *Konta;
static pid_t *Firmy;

static int do_exit = 0;

static void cleanup_common() {  
  log("Waiting for company processes.");
  for(int i = 0; i < UruchomioneProcesy; i++) {
    int status;
    wait(&status);
  }

  log("Freeing memory");
  if(Firmy != NULL) free(Firmy);
  if(Konta != NULL) free(Konta);
  
  log("Closing message queue,");
  double_queue_close(bQ);
  double_queue_close(cQ);
}

static void panic() {
  error("Panic inside bank!");
  log("Quitting everything if possible and shutting down - no reports printed by museum.");
  if(museum_pid > 0) {
    kill(museum_pid, SIGTERM);
  }
  if(Firmy != NULL) {
    for(int i = 0; i < LiczbaFirm; i++) {
      if(Firmy[i] != 0) {
        kill(Firmy[i], SIGTERM);
      }
    }
  }
  cleanup_common();
  exit(1);
}

void cleanup() {
	bold("Running cleanup...");

  cleanup_common();
}

// should not close message queues!
void cleanup_after_exec_failed() {
  bold("Running cleanup of forked process.");
  bQ->owner = 0;
  cQ->owner = 0;
  cleanup_common();
}

int wait_for_museum_connection(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int source, void *arg) {
  if(source != museum_ip) {
    error("Wrong sender address (waiting from connection from museum).");
    return -1;
  }
  if(match(query, query_length, "@c %b", ACT_MUSEUM_WITH_BANK_CONNECT_REQUEST, &museum_pid, sizeof(pid_t)) != 0) {
    error("Expected BANK WITH MUSEUM CONNECT REQUEST message.");
    return -1;
  }
  pid_t mypid = getpid();
  combine(response, response_length, "%c %i %b", ACT_OK, LiczbaFirm, &mypid, sizeof(pid_t));
  return 0;
}

int server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int source, void *arg) {
  if(query_length <= 0) {
    error("Empty message - something went wrong.");
    return -1;
  }
  char cmd = ((char*)query)[0];
  if(cmd == ACT_BANK_GET_SALDO) {
    int idx;
    int id;
    if(match(query, query_length, "@c %i %i", ACT_BANK_GET_SALDO, &idx, &id) == 0) {
      if(idx >= LiczbaFirm || Konta[idx].id != id) {
        warning("Wrong data for get saldo command - access denied - %d %d.", idx, id);
        combine(response, response_length, "%c", ACT_ERROR);
        return 0;
      }
      else {
        combine(response, response_length, "%c %ll", ACT_OK, Konta[idx].saldo);
        return 0;
      }
    } else goto wrong_message;
  } else if(cmd == ACT_BANK_CHANGE_SALDO) {
    int idx;
    int id;
    long long int delta;
    if(match(query, query_length, "@c %i %i %ll", ACT_BANK_GET_SALDO, &idx, &id, &delta) == 0) {
      if(idx >= LiczbaFirm || Konta[idx].id != id) {
        warning("Wrong data for get saldo command - access denied - %d %d.", idx, id);
        combine(response, response_length, "%c", ACT_ERROR);
        return 0;
      }
      else {
        if(Konta[idx].saldo + delta < 0) {
          combine(response, response_length, "%c", ACT_ERROR);
          return 0;
        } else {
          Konta[idx].saldo += delta;
          combine(response, response_length, "%c", ACT_OK);
          return 0;
        }
      }
    } else goto wrong_message;
  } else if(cmd == ACT_BANK_COMPANY_EXIT) {
    if(match(query, query_length, "@c", ACT_BANK_COMPANY_EXIT) == 0) {
      UruchomioneFirmy--;
      combine(response, response_length, "%c", ACT_OK);
      return 0;
    } else goto wrong_message;
  } else if(cmd == ACT_BANK_EXIT_ALL_COMPANIES) {
    // museum quits simulation so it call bank with this
    for(int i = 0; i < LiczbaFirm; i++) {
      if(Firmy[i] != 0) {
        kill(Firmy[i], SIGINT);
      }
    }
    combine(response, response_length, "%c", ACT_OK);
    return 0;
  } else if(cmd == ACT_BANK_EXIT) {
    if(match(query, query_length, "@c", ACT_BANK_EXIT) == 0) {
      do_exit = 1;
      combine(response, response_length, "%c", ACT_OK);
      return 0;
    } else goto wrong_message;
  } else {
    warning("Unsupported command 0x%x.", cmd);
    goto wrong_message;
  }
  
  return -1;
wrong_message:
  combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
  return 0;
}

void on_sigint(int type) {
  // sigint => panic
  panic();
}

int main(int argc, const char *argv[]) {
  bold("Starting bank process.");

  signal(SIGINT, on_sigint);
  
  if(argc != 4) {
    error("Wrong number of command-line parameters.");
    panic();
  }
  if(sscanf(argv[1], "%d", &LiczbaFirm) == -1) {
    error("Wrong first parameter.\n");
    panic();
  }
  if(sscanf(argv[2], "%d", &OplataStala) == -1) {
    error("Wrong second parameter.\n");
    panic();
  }
  if(sscanf(argv[3], "%d", &OgraniczenieArtefaktow) == -1) {
    error("Wrong third parameter.\n");
    panic();
  }

  UruchomioneFirmy = LiczbaFirm;
  
  //
  // Create message queues
  //
  log("Creating message queues for bank and companies.");
  {
  	if(double_queue_init(bQ, 1, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
  		error("Failed creating message queue.");
  		panic();
  	}
    if(double_queue_init(cQ, 1, company_key1, company_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
      error("Failed creating message queue.");
  		panic();
  	}
  }

  bold("Waiting for museum connection request...");
  {
		int exit_on_signal = 0;
    if(double_queue_listen(bQ, wait_for_museum_connection, NULL, &exit_on_signal, bank_ip) == -1) {
      error("Listening error during waiting for museum connection request.");
      panic();
    }
  }

  log("Reading data from stdin.");
  {
    if((Firmy = malloc(LiczbaFirm * sizeof(pthread_t))) == NULL) {
      error("Allocation failed\n");
      panic();
    }
    for(int i = 0; i < LiczbaFirm; i++) Firmy[i] = 0;
    if((Konta = malloc(LiczbaFirm * sizeof(konto_t))) == NULL) {
      error("Allocation failed\n");
      panic();
    }
    long worker_current_base_ip = company_base_ip + LiczbaFirm * 2;
    for(int i = 0; i < LiczbaFirm; i++) {
      int fid, fsaldo, fwrkrs;
      scanf("%d %d %d", &fid, &fsaldo, &fwrkrs);
      Konta[i].id = fid;
      Konta[i].saldo = fsaldo;
      
      pid_t pid = fork();
      if(pid == 0) {
        sprintf(ex_idx, "%d", i);                               // indeks firmy
        sprintf(ex_id , "%d", fid);                             // id firmy
        sprintf(ex_wrk, "%d", fwrkrs);                          // liczba pracowników firmy
        sprintf(ex_ip , "%ld", company_base_ip + i * 2);         // adres firmy
        sprintf(ex_wip, "%ld", worker_current_base_ip);          // podstawa adresów pracowników firmy
        execlp("./company", ex_pnm, ex_idx, ex_id, ex_wrk, ex_ip, ex_wip, 0);
        errorp();
        cleanup_after_exec_failed();
        return 1;
      } else if(pid == -1) {
        errorp();
        panic();
      } else {
        Firmy[i] = pid;
        worker_current_base_ip += fwrkrs;
        UruchomioneProcesy++;
      }
    }
  }

  bold("Started listening...");
  while(!do_exit) {
		int exit_on_signal = 0;
    if(double_queue_listen(bQ, server, NULL, &exit_on_signal, bank_ip) == -1) {
      error("Error during listening for queries.");
      panic();
    }
  }
  cleanup();
  return 0;
}
