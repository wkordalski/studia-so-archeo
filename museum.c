#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

static int *Teren;
static int *Szacunek;
static int OplataStala;
static int OgraniczenieArtefaktow;
static int Dlugosc;
static int Glebokosc;
static int *Kolekcje;
static int LiczbaFirm;


static double_queue_t mqueue;
static double_queue_t *mQ = &mqueue;

static double_queue_t bqueue;
static double_queue_t *bQ = &bqueue;

static int * Rezerwacje;

static int *ChceGadac;

void cleanup() {
  bold("Running cleanup...");

  log("Closing message queues.");
  double_queue_close(mQ);

  log("Freeing memory.");
  free(Teren);
  free(Szacunek);
  free(Kolekcje);
}

int main(int argc, const char *argv[]) {
  bold("Starting museum process.");
  if(argc != 5) {
    error("Wrong command-line input - four parameters needed.");
    return 1;
  }
  if(sscanf(argv[1], "%d", &Dlugosc) != 1) {
    error("First parameter should be a number.");
    return 1;
  }
  if(sscanf(argv[2], "%d", &Glebokosc) != 1) {
    error("Second parameter should be a number.");
    return 1;
  }
  if(sscanf(argv[3], "%d", &OplataStala) != 1) {
    error("Third parameter should be a number.");
    return 1;
  }
  if(sscanf(argv[4], "%d", &OgraniczenieArtefaktow) != 1) {
    error("Fourth parameter should be a number.");
    return 1;
  } else {
    OgraniczenieArtefaktow++; // bo chcemy większe (>=), a nie ściśle większe (>)
  }

  //
  // Create message queues
  //
  log("Creating museum message queue.");

	if(double_queue_init(mQ, museum_key1, museum_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
		error("Failed creating message queue.");
		cleanup();
		return 1;
	}

  log("Creating bank message queue.");

  if(double_queue_init(bQ, bank_key1, bank_key2, 0700|IPC_CREAT, 0700|IPC_CREAT) == -1) {
		error("Failed creating message queue.");
		cleanup();
		return 1;
	}

  //
  // Read data
  //
  log("Reading data from stdin");
  {
    if((Teren = malloc(Dlugosc*Glebokosc*sizeof(int))) == NULL) {
      fprintf(stderr, "Allocation failed.");
      cleanup();
      return 1;
    }
    if((Szacunek = malloc(Dlugosc*Glebokosc*sizeof(int))) == NULL) {
      fprintf(stderr, "Allocation failed.");
      cleanup();
      return 1;
    }
    if((Kolekcje = malloc(OgraniczenieArtefaktow*sizeof(int))) == NULL) {
      fprintf(stderr, "Allocation failed.");
      cleanup();
      return 1;
    } else {
      for(int i = 0; i < OgraniczenieArtefaktow; i++) {
        Kolekcje[i] = 0;
      }
    }
    if((Rezerwacje = malloc(Dlugosc*sizeof(int))) == NULL) {
      fprintf(stderr, "Allocation failed.");
      cleanup();
      return 1;
    } else {
      for(int i = 0; i < Dlugosc; i++) {
        Rezerwacje[i] = 0;
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
  //
  // Słuchaj, kto chce gadać i rozmawiaj...
  //
  bold("Connecting to bank...");
  {
		void *query;
    size_t qlen;
    combine(&query, &qlen, "%c", ACT_BANK_WITH_MUSEUM_CONNECT_REQUEST);
		char *response;
		size_t response_size;
		if(double_queue_query(bQ, query, qlen, (void**)(&response), &response_size, &skip_notifications, NULL, museum_ip, bank_ip) == -1) {
			error("Error connecting to bank.");
      free(query);
			cleanup();
			return 1;
		}
    free(query);
		if(match(response, response_size, "@c %i", ACT_OK, &LiczbaFirm) != 0) {
			error("Expected OK message - something went wrong.");
			cleanup();
			return 1;
		}
    free(response);
	}
  bold("Listening for queries...");
  // TODO
  cleanup();
}

// L - lewa pozycja
// P - prawa pozycja
// G - głębokość
void WycenTeren(int L, int P, int G) {
  // TODO
}

// k - liczba pracowników
// Z - cena
void WydajPozwolenieKopania(int k, int Z) {
  // TODO
}

void KupKolekcje() {
  // TODO
}

void RaportMuzeum() {
  for(int i = 0; i < Dlugosc; i++) {
    int depth = 0;
    for(int j = 0; j < Glebokosc; j++) {
      int r = 0;
      if(Teren[i * Glebokosc + j] == 0) r = 0;
      else {
        if(Rezerwacje[i] > depth) r = 1;
        else r = 2;
        depth++;
      }
      printf("%d", r);
    }
    printf("\n");
  }

  // TODO: Raporty firm tutaj?
}

void Koncz() {
  // TODO
}

void OnSigINT() {
  // TODO
}
