#ifndef _ARCHEO_COMPANY_H_
#define _ARCHEO_COMPANY_H_

typedef struct {
  int idx;
  int id;         // identyfikator firmy
  int k;          // liczba pracowników
} company_init_t;

void * company(void *arg);

#endif /* end of include guard: _ARCHEO_COMPANY_H_ */
