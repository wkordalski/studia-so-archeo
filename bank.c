static int LiczbaFirm;
static int OgraniczenieArtefaktow;
static int OplataStala;

typedef struct {
  int id;         // identyfikator firmy
  int saldo;      // saldo firmy
  int k;          // liczba pracowników
} firma_t;

int main(int argc, const char *argv[]) {
  //
  // Czytanie danych
  //
  if(argc != 4) {
    fprintf(stderr, "Wrong number of command-line parameters.");
    return 1;
  }
  if(sscanf(argv[1], "%d", &LiczbaFirm) == -1) {
    fprintf(stderr, "Wrong first parameter.\n");
    return 1;
  }
  if(sscanf(argv[2], "%d", &OplataStala) == -1) {
    fprintf(stderr, "Wrong second parameter.\n");
    return 1;
  }
  if(sscanf(argv[3], "%d", &OgraniczenieArtefaktow) == -1) {
    fprintf(stderr, "Wrong third parameter.\n");
    return 1;
  }
  if((Firmy = malloc(LiczbaFirm * sizeof(firma_t))) == NULL) {
    fprintf(stderr, "Allocation failed\n");
    return 1;
  }
  for(int i = 0; i < LiczbaFirm; i++) {
    scanf("%d %d %d", &(Firmy[i].id), &(Firmy[i].saldo), &(Firmy[i].k));
  }
}
