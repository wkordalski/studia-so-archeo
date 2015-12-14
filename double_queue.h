#ifndef _ARCHEO_DOUBLE_QUEUE_H_
#define _ARCHEO_DOUBLE_QUEUE_H_

#include <sys/types.h>
#include <sys/ipc.h>

typedef struct {
  long id;
  int count;
} condition_queue_info_t;

typedef struct {
  int id1, id2;
  size_t conds_count;
  size_t conds_capa;
  condition_queue_info_t *conds;
} double_queue_t;

int double_queue_init_err(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2, int(*error_f)(void*), void *error_a);
int double_queue_init(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2);

void double_queue_close(double_queue_t *q);


// Query - sends query and waits for result (client side)
int double_queue_query_err(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int (*on_notification)(void*, size_t, void*),
      void *arg, int *exit_after_signal,
      long source, long destination,
      int (*error_f)(void*), void *error_a);

int double_queue_query(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int (*on_notification)(void*, size_t, void*),
      void *arg, int *exit_after_signal,
      long source, long destination);


// Listen - receives query and sends result (server side)
int double_queue_listen_err(
      double_queue_t *q,
      int (*server)(void *, size_t, void **, size_t *, int *, int, void*),
      void *arg, int *exit_after_signal,
      int destination,
      int (*error_f)(void*), void *error_a);

int double_queue_listen(
			double_queue_t *q,
			int (*server)(void*, size_t, void **, size_t*, int *, int, void*),
      void *arg, int *exit_after_signal,
			int destination);

// Simple wait and signal procedures.
int double_queue_wait_err(
      double_queue_t *q,
      long source, long destination, long queue,
			int *exit_after_signal,
      int (*error_f)(void*), void *error_a);

int double_queue_wait(
      double_queue_t *q,
      long source, long destination, long queue,
			int *exit_after_signal);


int double_queue_signal_one_err(
      double_queue_t *q,
      long queue, int *exit_after_signal,
      int (*error_f)(void*), void *error_a);

int double_queue_signal_one(
      double_queue_t *q,
      long queue, int *exit_after_signal);


int double_queue_signal_all_err(
      double_queue_t *q,
      long queue, int *exit_after_signal,
      int (*error_f)(void*), void *error_a);

int double_queue_signal_all(
      double_queue_t *q,
      long queue, int *exit_after_signal);

#endif /* end of include guard: _ARCHEO_DOUBLE_QUEUE_H_ */
