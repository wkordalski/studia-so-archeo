typedef struct {
  int id1, id2;
} double_queue_t;

int double_queue_init_err(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2, int(*error_f)(void*), void *error_a);
int double_queue_init(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2);

void close(double_queue_t *q);
