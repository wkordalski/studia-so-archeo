#include "company.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>

typedef struct {
  // TODO: globals for company thread
} company_t;

int cleanup_company(company_t *data) {
  // TODO
}

void* company(void *arg) {
  // main function for company
  // gets firma_t
  company_init_t *com_init = (company_init_t*)arg;
  log("Running company %d.", com_init->idx);
  sleep(10);
  log("Stopping company %d.", com_init->idx);
  free(com_init);
  return 0;
}
