#include "common.h"
#include "debug.h"
#include "double_queue.h"
#include "util.h"

#include <stdlib.h>

long long int get_money_of_company(double_queue_t *q, int idx, int id, long source) {
  void *query, *response;
  size_t qlen, response_size;
  int exit_on_signal = 0;
  combine(&query, &qlen, "%c %i %i", ACT_BANK_GET_SALDO, idx, id);
  if(double_queue_query(q, query, qlen, &response, &response_size, &exit_on_signal, source, bank_ip) != 0) {
    error("Error connecting to bank.");
    free(query);
    return -1;
  }
  long long int saldo = -1;
  free(query);
  if(match(response, response_size, "@c %ll", ACT_OK, &saldo) != 0) {
    error("Expected OK message - something went wrong.");
    free(response);
    return -1;
  }
  free(response);
  return saldo;
}

int change_money_of_company(double_queue_t *q, int idx, int id, long long int delta, long source) {
  void *query, *response;
  size_t qlen, response_size;
  int exit_on_signal = 0;
  combine(&query, &qlen, "%c %i %i %ll", ACT_BANK_CHANGE_SALDO, idx, id, delta);
  if(double_queue_query(q, query, qlen, &response, &response_size, &exit_on_signal, source, bank_ip) != 0) {
    error("Error connecting to bank.");
    free(query);
    return -1;
  }
  free(query);
  if(match(response, response_size, "@c", ACT_OK) == 0) {
    free(response);
    return 0;
  } else if(match(response, response_size, "@c", ACT_ERROR) == 0) {
    free(response);
    return 1;
  } else {
    error("Expected OK message - something went wrong.");
    free(response);
    return -1;
  }
}