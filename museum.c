#include "debug.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

static int *Teren;
static int *Szacunek;
static int OplataStala;
static int OgraniczenieArtefaktow;
static int Dlugosc;
static int Glebokosc;
static int *Kolekcje;

static key_t key_input = 77;
static int queue_input = -1;

static key_t key_output = 654165468;
static int queue_output = -1;

static int * Rezerwacje;

static int *ChceGadac;

void cleanup() {
  bold("Running cleanup...");
  if(queue_input >= 0) {
    msgctl(queue_input, IPC_RMID, NULL);
    queue_input = -1;
  }
  if(queue_output >= 0) {
    msgctl(queue_output, IPC_RMID, NULL);
    queue_output = -1;
  }
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
  log("Creating message queues for museum.");

  if((queue_input = msgget(key_input, 0644 | IPC_CREAT)) == -1) {
    errorp();
    cleanup();
    return 1;
  }
  if((queue_output = msgget(key_output, 0644 | IPC_CREAT)) == -1) {
    errorp();
    cleanup();
    return 1;
  }

  //
  // Read data
  //
  log("Reading data from stdin");
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

  //
  // Słuchaj, kto chce gadać i rozmawiaj...
  //
  bold("Listening for queries...");

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
