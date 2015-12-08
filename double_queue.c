#include "double_queue.h"
#include "debug.h"
#include "util.h"

int double_queue_init_err(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2, int(*error_f)(void*), void *error_a) {
  q->id1 = q->id2 = -1;
  q->id1 = msgget(key1, flags1);
  if(q->id1 == -1) {
    error("Creation of message queue failed.");
    return error_f(error_a);
  }
  q->id2 = msgget(key2, flags2);
  if(q->id2 == -1) {
    error("Creation of message queue failed.");
    return error_f(error_a);
  }
  return 0;
}

int double_queue_init(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2) {
  return double_queue_init_err(q, key1, key2, flags1, flags2, simple_error, NULL);
}

void close(double_queue_t *q) {
  if(q->id1 >= 0) {
    if(msgctl(q->id1, IPC_RMID, NULL) == -1) warning("Closing of message queue failed.");
  }
  if(q->id2 >= 0) {
    if(msgctl(q->id2, IPC_RMID, NULL) == -1) warning("Closing of message queue failed.");
  }
}

// Query - sends query and waits for result (client side)
int double_queue_query_err(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination,
      void (*error_f)(void*), void *error_a) {
  // Parameter checks
  if(query_length > 1024 /* maksymalna długość komunikatu */) {
    error("Too long message.");
    return error_f(error_a);
  }
  // Prepare query
  void * real_query = malloc(query_length + 2 * sizeof(long));
  if(real_query == NULL) {
    error("Allocation failed.");
    return error_f(error_a);
  }
  void * answer_buffer = malloc(1024+1);
  if(answer_buffer == NULL) {
    error("Allocation failed.");
    free(real_query);
    return error_f(error_a);
  }
  memcpy(real_query, &destination, sizeof(long));
  memcpy(real_query + sizeof(long), &source, sizeof(long));
  memcpy(real_query + 2 * sizeof(long), query, query_length);
  // Send query
  if(msgsnd(q->id1, real_query, query_length + sizeof(long) /* bez pola "rodzaj" */, 0 /* wait if full */) == -1) {
    error("Message sending failed.");
    free(real_query);
    free(answer_buffer);
    return error_f(error_a);
  }
  free(real_query);
  // Wait for result
  int ret = msgrcv(q->id2, answer_buffer, 1024 /* nie potrzebujemy adresu zwrotnego */, source, 0);
  if(ret == -1 || ret == 0) {
    error("Message receiving failed.");
    free(answer_buffer);
    return error_f(error_a);
  }
  (*it_was_notification) = (answer_buffer[ret-1] == 0)?0:1;
  (*result) = answer_buffer;
  (*result_length) = ret-1;
  return 0;
}

int double_queue_query(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination) {
  return double_queue_query_err(q, query, query_length, result, result_length, it_was_notification, source, destination, simple_error, NULL);
}

// Response - receives result of previous query - because it got notification
// TODO

// Listen - receives query and sends result (server side)
int double_queue_listen_err(
      double_queue_t *q,
      int (*processor)(void * query, size_t query_length,
                       void ** result, size_t * result_length,
                       int *is_it_notification, int client),
      int source,
      int (*error_f)(void*), void *error_a) {
  // TODO
  // Can return notification - eg. when it should cause quiting...
  // If notification is returned then it is called once again
  // until it returns normall message.
}
