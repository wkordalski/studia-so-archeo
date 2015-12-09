#include <sys/types.h>
#include <sys/ipc.h>

typedef struct {
  int id1, id2;
} double_queue_t;

int double_queue_init_err(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2, int(*error_f)(void*), void *error_a);
int double_queue_init(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2);

void double_queue_close(double_queue_t *q);


// Query - sends query and waits for result (client side)
int double_queue_query_err(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination,
      int (*error_f)(void*), void *error_a);

int double_queue_query(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination);

// Response - receives result of previous query - because it got notification
// Query - sends query and waits for result (client side)
int double_queue_response_err(
      double_queue_t *q,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination /* useless */,
      int (*error_f)(void*), void *error_a);

int double_queue_response(
      double_queue_t *q,
      void ** result, size_t *result_length,
      int *it_was_notification,
      long source, long destination);

// Listen - receives query and sends result (server side)
int double_queue_listen_err(
      double_queue_t *q,
      int (*server)(void *, size_t, void **, size_t *, int *, int),
      int destination,
      int (*error_f)(void*), void *error_a);

int double_queue_listen(
			double_queue_t *q,
			int (*server)(void*, size_t, void **, size_t*, int *, int),
			int destination);