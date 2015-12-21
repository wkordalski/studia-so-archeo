#ifndef _ARCHEO_BANK_UTILS_H_
#define _ARCHEO_BANK_UTILS_H_

#include "double_queue.h"

long long int get_money_of_company(double_queue_t *q, int id, int idx, long source);
int change_money_of_company(double_queue_t *q, int id, int idx, long long int delta, long source);

#endif /* _ARCHEO_BANK_UTILS_H_ */