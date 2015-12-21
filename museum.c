// Wojciech Kordalski (wk359805)

// nie wchodzić na stderr, bo KDevelop się wiesza wtedy :D

#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "util.h"

#include "bank_utils.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

static int Dlugosc;
static int Glebokosc;
static int OplataStala;
static int OgraniczenieArtefaktow;
static int LiczbaFirm;
static int UruchomioneFirmy;

static pid_t bank_pid;

static int do_exit = 0;

static int *Teren;
static int *Szacunek;
static int *Rezerwacje;
static int *Wydobyte;

static int *Kolekcje;

static char **Raporty;
static int  *DlugosciRaportow;
static int  *OczekiwnaDlugoscRaportu;

static double_queue_t mqueue;
static double_queue_t bqueue;
static double_queue_t *mQ = &mqueue;
static double_queue_t *bQ = &bqueue;

void * delegate(void *arg);

typedef struct {
  // data pf delegate
  int zajety;
  long ip;
  int g;
  int l;
  pthread_t thread;
  int do_exit;
} delegate_t;

static delegate_t *Delegaci;

void cleanup() {
  bold("Running cleanup...");

  log("Closing message queues.");
  double_queue_close(mQ);
  double_queue_close(bQ);

  log("Freeing memory.");
  if(Teren)      free(Teren);
  if(Szacunek)   free(Szacunek);
  if(Rezerwacje) free(Rezerwacje);
  if(Wydobyte)   free(Wydobyte);
  if(Kolekcje)   free(Kolekcje);
  if(Raporty) {
    for(int i = 0; i < LiczbaFirm; i++) {
      if(Raporty[i] != NULL) free(Raporty[i]);
    }
    free(Raporty);
  }
  if(DlugosciRaportow) free(DlugosciRaportow);
  if(OczekiwnaDlugoscRaportu) free(OczekiwnaDlugoscRaportu);
  if(Delegaci)
    // TODO: some for
    free(Delegaci);
}

static void panic() {
  error("Panic inside museum!");
  log("Sending panic to bank and shutting down - no reports will be printed.");
  if(bank_pid > 0) {
    log("Waiting for bank's reaction for panic.");
    kill(bank_pid, SIGINT);
    while(1) pause();   // waits for termination by external SIGTERM
  } else {
    log("Bank is unknown - simply exitting.");
    cleanup();
    exit(1);            // we do not know bank
  }
}

int report_server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int source, void *arg) {
  char cmd = ((char*)query)[0];
  log("Report server!");
  if(cmd == ACT_MUSEUM_REPORT_INIT) {
    int id, len;
    if(match(query, query_length, "@c %i %i", ACT_MUSEUM_REPORT_INIT, &id, &len) == 0) {
      id--;
      if(Raporty[id] == NULL) {
        Raporty[id] = malloc(len+1);
        if(Raporty[id] == NULL) return -1;
        DlugosciRaportow[id] = 0;
        OczekiwnaDlugoscRaportu[id] = len;
        combine(response, response_length, "%c", ACT_OK);
        return 0;
      } else goto wrong_message;
    } else goto wrong_message;
  } else if(cmd == ACT_MUSEUM_SEND_REPORT_PART) {
    int id;
    size_t len = 1000;
    void *data = malloc(1010);
    if(data == NULL) return -1;
    if(match(query, query_length, "@c %i %bv", ACT_MUSEUM_SEND_REPORT_PART, &id, data, &len) == 0) {
      id--;
      if(len <= OczekiwnaDlugoscRaportu[id] - DlugosciRaportow[id]) {
        memcpy(Raporty[id] + DlugosciRaportow[id], data, len);
        DlugosciRaportow[id] += len;
        free(data);
        combine(response, response_length, "%c", ACT_OK);
        return 0;
      } else {
        free(data);
        goto wrong_message;
      }
    } else {
      free(data);
      goto wrong_message;
    }
  } else if(cmd == ACT_MUSEUM_COMPANY_EXIT) {
    if(match(query, query_length, "@c", ACT_MUSEUM_COMPANY_EXIT) == 0) {
      UruchomioneFirmy--;
      if(UruchomioneFirmy <= 0) {
        do_exit = 1;
      }
      combine(response, response_length, "%c", ACT_OK);
      return 0;
    } else {
      goto wrong_message;
    }
  } else return -1;
wrong_message:
  warning("Got wrong message.");
  combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
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
  if(cmd == ACT_MUSEUM_GET_ESTIMATIONS) {
    int pL, pP, pT, pB;
    if(match(query, query_length, "@c %i %i %i %i", ACT_MUSEUM_GET_ESTIMATIONS, &pL, &pP, &pT, &pB) == 0) {
      if(!(1 <= pL && pL <= pP && pP <= Dlugosc && 1 <= pT && pT <= pB && pB <= Glebokosc)) {
        combine(response, response_length, "%c %c", ACT_ERROR, ERR_INVALID_ARGUMENT);
        return 0;
      } else if((pP - pL + 1) * (pB - pT + 1) * sizeof(int) > 999) {
        combine(response, response_length, "%c %c", ACT_ERROR, ERR_INVALID_ARGUMENT);
        return 0;
      } else {
        pP--; pL--; pB--; pT--;
        int *res = malloc((pP - pL + 1) * (pB - pT + 1) * sizeof(int));
        int w = pP - pL + 1;
        for(int i = pL, ii = 0; i <= pP; i++, ii++) {
          for(int j = pT, jj = 0; j <= pB; j++, jj++) {
            res[ii * w + jj] = ((Wydobyte[i] < j)?Szacunek[i*Dlugosc + j]:0);
          }
        }
        combine(response, response_length, "%c %b", ACT_OK, res, (pP - pL + 1) * (pB - pT + 1) * sizeof(int));
        free(res);
        return 0;
      }
    } else {
      combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
      return 0;
    }
  } else if(cmd == ACT_MUSEUM_BUY_TERRAIN) {
    /*
     * idx (int), id (int), k (int), z (int) -> l (int), d (int), dip(long)
     */
    int k, z, idx, id;
    if(match(query, query_length, "@c %i %i %i %i", ACT_MUSEUM_BUY_TERRAIN, &idx, &id, &k, &z) == 0) {
      // Try to allocate and return result
      int ret = change_money_of_company(bQ, idx, id, -OplataStala, museum_ip);
      if(ret == 1 || z < OplataStala || Delegaci[idx].zajety != 0) {
        combine(response, response_length, "%c", ACT_ERROR);
        return 0;
      } else if(ret == -1) {
        return -1;
      }
      z -= OplataStala;
      // find first such block of terrain
      int l = -1;
      for(int i = 0; i < Dlugosc - k; i++) {
        l = i;
        for(int j = 0; j < k; j++) {
          if(Rezerwacje[i+j] > Wydobyte[i+j]) {
            l = -1;
            break;
          }
        }
        if(l != -1) break;
      }
      int g = 0;
      int sumcost = 0;
      for(int i = 0; i < Glebokosc; i++) {
        int line_cost = 0;
        for(int j = 0; j < k; j++) {
          if(Wydobyte[j+l] > i) continue;
          line_cost += Szacunek[(j+l)*Dlugosc+i];
          if(line_cost > z) {
            line_cost = z + 1;
            break;
          }
        }
        if(line_cost == z + 1) break;
        else if(line_cost + sumcost > z) break;
        sumcost += line_cost;
        g = i;
      }
      if(g == 0 || sumcost > z) {
        combine(response, response_length, "%c", ACT_ERROR);
        return 0;
      }
      int ret2 = change_money_of_company(bQ, idx, id, -z, museum_ip);
      if(ret2 == 1) {
        combine(response, response_length, "%c", ACT_ERROR);
        return 0;
      } else if(ret2 == -1) {
        return -1;
      }
      // OK - create delegate
      Delegaci[idx].zajety = 1;
      Delegaci[idx].ip = delegate_base_ip + idx;
      Delegaci[idx].l = l;
      Delegaci[idx].g = g;
      Delegaci[idx].do_exit = 0;
      
      pthread_attr_t attrs;
      pthread_attr_init(&attrs);
      
      int ret3 = pthread_create(&(Delegaci[idx].thread), &attrs, delegate, Delegaci+idx);
      if(ret3 != 0) {
        error("Failed with creating thread.");
        pthread_attr_destroy(&attrs);
        return -1;
      }
      pthread_attr_destroy(&attrs);
      combine(response, response_length, "%c %i %i %l", ACT_OK, l, g, delegate_base_ip + idx);
      return 0;
    } else {
      combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
      return 0;
    }
  } else if(cmd == ACT_MUSEUM_REPORT_INIT || cmd == ACT_MUSEUM_SEND_REPORT_PART
         || cmd == ACT_MUSEUM_COMPANY_EXIT) {
    if(report_server(query, query_length, response, response_length, source, arg) == 0)
      return 0;
    else
      return -1;
  } else {
    warning("Unsupported command 0x%x.", cmd);
    combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
    return 0;
  }
  assert(0);
  return -1;
}

int reduced_server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int source, void *arg) {
  if(query_length <= 0) {
    error("Empty message - something went wrong.");
    return -1;
  }
  char cmd = ((char*)query)[0];
  if(cmd == ACT_MUSEUM_GET_ESTIMATIONS) {
    goto reject;
  } else if(cmd == ACT_MUSEUM_BUY_TERRAIN) {
    goto reject;
  } else if(cmd == ACT_MUSEUM_REPORT_INIT || cmd == ACT_MUSEUM_SEND_REPORT_PART 
         || cmd == ACT_MUSEUM_COMPANY_EXIT) {
    if(report_server(query, query_length, response, response_length, source, arg) == 0)
      return 0;
    else
      return -1;
  } else {
    warning("Unsupported command 0x%x.", cmd);
    combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
    return 0;
  }
  assert(0);
  return -1;
reject:
  combine(response, response_length, "%c", ACT_REJECT);
  return 0;
}


void on_sigint(int type) {
  if(bank_pid <= 0) {
    cleanup();
    exit(1);
  }
  else
  {
    do_exit = 1;
  }
}

void on_sigterm(int type) {
  // simply exit after cleanup
  log("Got SIGTERM - exitting.");
  cleanup();
  exit(1);
}

int main(int argc, const char *argv[]) {
  bold("Starting museum process.");
  
  signal(SIGTERM, on_sigterm);
  signal(SIGINT, on_sigint);
  
  if(argc != 5) {
    error("Wrong command-line input - four parameters needed.");
    panic();
  }
  if(sscanf(argv[1], "%d", &Dlugosc) != 1) {
    error("First parameter should be a number.");
    panic();
  }
  if(sscanf(argv[2], "%d", &Glebokosc) != 1) {
    error("Second parameter should be a number.");
    panic();
  }
  if(sscanf(argv[3], "%d", &OplataStala) != 1) {
    error("Third parameter should be a number.");
    panic();
  }
  if(sscanf(argv[4], "%d", &OgraniczenieArtefaktow) != 1) {
    error("Fourth parameter should be a number.");
    panic();
  } else {
    OgraniczenieArtefaktow++; // bo chcemy większe (>=), a nie ściśle większe (>)
  }

  //
  // Create message queues
  //
  log("Creating museum message queue.");

  if(double_queue_init(mQ, 1, museum_key1, museum_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
    error("Failed creating message queue.");
    panic();
  }

  log("Creating bank message queue.");

  if(double_queue_init(bQ, 0, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
    error("Failed creating message queue.");
    panic();
  }

  //
  // Read data
  //
  log("Reading data from stdin");
  {
    if((Teren = malloc(Dlugosc*Glebokosc*sizeof(int))) == NULL) {
      error("Allocation failed.");
      panic();
    }
    if((Szacunek = malloc(Dlugosc*Glebokosc*sizeof(int))) == NULL) {
      error("Allocation failed.");
      panic();
    }
    if((Kolekcje = malloc(OgraniczenieArtefaktow*sizeof(int))) == NULL) {
      error("Allocation failed.");
      panic();
    } else {
      for(int i = 0; i < OgraniczenieArtefaktow; i++) {
        Kolekcje[i] = 0;
      }
    }
    if((Rezerwacje = malloc(Dlugosc*sizeof(int))) == NULL) {
      error("Allocation failed.");
      panic();
    } else {
      for(int i = 0; i < Dlugosc; i++) {
        Rezerwacje[i] = 0;
      }
    }
    if((Wydobyte = malloc(Dlugosc*sizeof(int))) == NULL) {
      error("Allocation failed.");
      panic();
    } else {
      for(int i = 0; i < Dlugosc; i++) {
        Wydobyte[i] = 0;
      }
    }
    for(int i = 0; i < Dlugosc; i++) {
      for(int j = 0; j < Glebokosc; j++) {
        scanf("%d", Teren+i*Glebokosc+j);
      }
    }

    for(int i = 0; i < Dlugosc; i++) {
      for(int j = 0; j < Glebokosc; j++) {
        scanf("%d", Szacunek+i*Glebokosc+j);
      }
    }
  }
  
  bold("Connecting to bank...");
  {
    void *query, *response;
    size_t qlen, response_size;
    pid_t pid = getpid();
    int exit_on_signal = 0;
    combine(&query, &qlen, "%c %b", ACT_MUSEUM_WITH_BANK_CONNECT_REQUEST, &pid, sizeof(pid_t));
    if(double_queue_query(bQ, query, qlen, (void**)(&response), &response_size, &exit_on_signal, museum_ip, bank_ip) != 0) {
      error("Error connecting to bank.");
      free(query);
      panic();
    }
    free(query);
    if(match(response, response_size, "@c %i %b", ACT_OK, &LiczbaFirm, &bank_pid, sizeof(pid_t)) != 0) {
      error("Expected OK message - something went wrong.");
      free(response);
      panic();
    }
    UruchomioneFirmy = LiczbaFirm;
    free(response);
  }
  
  bold("Creating company information buffers...");
  {
    Raporty = malloc(LiczbaFirm * sizeof(char*));
    DlugosciRaportow = malloc(LiczbaFirm * sizeof(int));
    OczekiwnaDlugoscRaportu = malloc(LiczbaFirm * sizeof(int));
    
    if(Raporty == NULL || DlugosciRaportow == NULL || OczekiwnaDlugoscRaportu == NULL) {
      error("Allocation failed");
      panic();
    }
    
    for(int i = 0; i < LiczbaFirm; i++) {
      Raporty[i] = NULL;
    }
  }
  
  bold("Started listening...");
  while(!do_exit) {
    int exit_on_signal = 1;
    int ret = 0;
    if((ret = double_queue_listen(mQ, server, NULL, &exit_on_signal, museum_ip)) == -1) {
      error("Error during listening for queries.");
      panic();
    }
    if(ret == DOUBLE_QUEUE_INTERRUPTED) break;
  }
  
  bold("Calling bank to exit all companies.");
  {
    void *query, *response;
    size_t qlen, resplen;
    combine(&query, &qlen, "%c", ACT_BANK_EXIT_ALL_COMPANIES);
    int exit_after_signal = 0;
    if(double_queue_query(bQ, query, qlen, &response, &resplen, &exit_after_signal, museum_ip, bank_ip) == -1) {
      error("Failed sending company termination query to bank.");
    }
    free(query);
    if(match(response, resplen, "@c", ACT_OK) != 0) {
      error("Wrong response - maby some error.");
    }
    free(response);
  }
  
  // handle companies reports
  while(UruchomioneFirmy > 0) {
    int exit_on_signal = 0;
    int ret = 0;
    if((ret = double_queue_listen(mQ, reduced_server, NULL, &exit_on_signal, museum_ip)) == -1) {
      error("Error during listening for queries.");
      panic();
    }
  }
  
  // print reports
  for(int i = 0; i < Dlugosc; i++) {
    for(int j = 0; j < Glebokosc; j++) {
      if(Wydobyte[i] > j) printf("0");
      else if(Rezerwacje[i] > j) printf("1");
      else printf("2");
    }
    printf("\n");
  }
  for(int i = 0; i < LiczbaFirm; i++) {
    if(Raporty[i] != NULL) {
      Raporty[i][DlugosciRaportow[i]] = 0;
      printf("\n%s", Raporty[i]);
    } else {
      printf("\nFirma %d nie przesłała raportu.", i+1);
    }
  }
  
  log("Terminating bank");
  {
    void *query, *response;
    size_t qlen, resplen;
    combine(&query, &qlen, "%c", ACT_BANK_EXIT);
    int exit_after_signal = 0;
    if(double_queue_query(bQ, query, qlen, &response, &resplen, &exit_after_signal, museum_ip, bank_ip) == -1) {
      error("Failed sending bank termination query.");
    }
    free(query);
    if(match(response, resplen, "@c", ACT_OK) != 0) {
      error("Wrong response - maby some error.");
    }
    free(response);
  }
  
  cleanup();
  return 0;
}



int delegate_server(
      void *query, size_t query_length,
      void **response, size_t *response_length,
      int source, void *arg) {
  if(query_length <= 0) {
    error("Empty message - something went wrong.");
    return -1;
  }
  delegate_t *data = (delegate_t*)arg;
  char cmd = ((char*)query)[0];
  if(cmd == ACT_DELEGATE_GIVE_WORK) {
    // match
    // check if such part is not used and available
    // send symbol
  } else if(cmd == ACT_DELEGATE_CHECK_WORK) {
    // match
    // check if result | symbol
    // increment Wydobyte
    // if everything is Wydobyte, then do_exit = true
    // send OK
  } else {
    warning("Unsupported command 0x%x.", cmd);
    combine(response, response_length, "%c %c", ACT_ERROR, ERR_WRONG_MESSAGE);
    return 0;
  }
  return -1;
}


void * delegate(void *arg) {
  bold("Started delegate");
  delegate_t *data = (delegate_t*)arg;
  while(!data->do_exit) {
    int exit_on_signal = 0;
    int ret = 0;
    if((ret = double_queue_listen(mQ, delegate_server, data, &exit_on_signal, museum_ip)) == -1) {
      error("Error during listening for queries in delegate.");
      panic();
    }
  }
  return NULL;
}