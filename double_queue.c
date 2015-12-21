#include "double_queue.h"
#include "debug.h"
#include "util.h"

#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

const int DOUBLE_QUEUE_INTERRUPTED = 7;

int double_queue_init_err(double_queue_t *q, int owner, key_t key1, key_t key2, int flags1, int flags2, int(*error_f)(void*), void *error_a) {
  q->owner = owner;
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

int double_queue_init(double_queue_t *q, int owner, key_t key1, key_t key2, int flags1, int flags2) {
  return double_queue_init_err(q, owner, key1, key2, flags1, flags2, simple_error, NULL);
}

void double_queue_close(double_queue_t *q) {
  if(q->owner) {
    if(q->id1 >= 0) {
      if(msgctl(q->id1, IPC_RMID, NULL) == -1) warning("Closing of message queue (direction C->S) failed.");
    }
    if(q->id2 >= 0) {
      if(msgctl(q->id2, IPC_RMID, NULL) == -1) warning("Closing of message queue (direction S->C) failed.");
    }
  }
}

// Query - sends query and waits for result (client side)
int double_queue_query_err(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *exit_after_signal,
      long source, long destination,
      int (*error_f)(void*), void *error_a) {
  // Parameter checks
  if(query_length > 1024 /* maksymalna długość komunikatu */) {
    error("Too long message.");
    return error_f(error_a);
  }
  // Prepare query
  void * real_query = NULL;
  size_t real_query_length;
  char * res = NULL;
  char * answer_buffer = malloc(1024+sizeof(long));
  if(answer_buffer == NULL)
    goto allocation_failed;
  if(combine(&real_query, &real_query_length, "%l %b %l", destination, query, query_length, source) != 0)
    goto allocation_failed;
  // Send query
resend:
  if(msgsnd(q->id1, real_query, real_query_length - sizeof(long), 0) == -1) {
    if(errno == EINTR) {
      log("Interrupted during waiting...");
      if(*exit_after_signal) {
        free(real_query);
        free(answer_buffer);
        return DOUBLE_QUEUE_INTERRUPTED;
      } else {
        goto resend;
      }
    }
    errorp();
    free(real_query);
    free(answer_buffer);
    return error_f(error_a);
  }
  free(real_query);
  // Wait for result
  int ret;
rerecv:
  ret = msgrcv(q->id2, answer_buffer, 1024+sizeof(long) /* nie potrzebujemy adresu zwrotnego */, source, 0);
  if(ret == -1 && errno == EINTR) {
    log("Message receiving interrupted.");
    if(*exit_after_signal) {
      free(answer_buffer);
      return DOUBLE_QUEUE_INTERRUPTED;
    } else {
      goto rerecv;
    }
  }
  if(ret == -1 || ret == 0) {
    errorp();
    free(answer_buffer);
    return error_f(error_a);
  }
  res = malloc(ret);
  if(res == NULL)
    goto allocation_failed;
  memcpy(res, answer_buffer+sizeof(long), ret);
  (*result) = res;
  (*result_length) = ret;
    
  free(answer_buffer);
  return 0;
  
allocation_failed:
  if(real_query) free(real_query);
  if(answer_buffer) free(answer_buffer);
  if(res) free(res);
  error("Allocation failed.");
  return error_f(error_a);
}

int double_queue_query(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int *exit_after_signal,
      long source, long destination) {
  return double_queue_query_err(q, query, query_length, result, result_length, exit_after_signal, source, destination, simple_error, NULL);
}

// Listen - receives query and sends result (server side)
int double_queue_listen_err(
      double_queue_t *q,
      int (*server)(void *, size_t, void **, size_t *, int, void*),
      void *arg, int *exit_after_signal,
      int destination,
      int (*error_f)(void*), void *error_a) {
  // Prepare buffers
  void *response = NULL;
  size_t response_length;
  unsigned char * response_buffer = NULL;
  void * query = NULL;
  query = malloc(1024+2*sizeof(long)  /* message + source */);
  if(query == NULL) goto allocation_failed;
  // Receive
  int ret;
rerecv:
  ret = msgrcv(q->id1, query, 1024+2*sizeof(long), destination, 0);
  if(ret == -1 && errno == EINTR) {
    log("Message receiving interrupted.");
    if(*exit_after_signal) {
      free(query);
      return DOUBLE_QUEUE_INTERRUPTED;
    } else {
      goto rerecv;
    }
  }
  if(ret == -1 || ret < sizeof(long)) {
    error("Message received failed.");
    free(query);
		return error_f(error_a);
  }
  long source = 0;
  ret -= sizeof(long);
  memcpy(&source, query + ret + sizeof(long), sizeof(long));
    
  if(server(query+sizeof(long), ret, &response, &response_length, source, arg) == -1) {
    error("Server procedure failed.");
    free(query);
    if(response != NULL) free(response);
    return error_f(error_a);
  }
  if(response_length > 1024) {
    error("Response is too long.");
    free(query);
    free(response);
    return error_f(error_a);
  }
  if(response == NULL) {
    error("No response");
    free(query);
    free(response);
    return error_f(error_a);
  }
  response_buffer = malloc(1024 + sizeof(long) /* address + body */);
  if(response_buffer == NULL)
    goto allocation_failed;
  memcpy(response_buffer, &source, sizeof(long));
  memcpy(response_buffer + sizeof(long), response, response_length);
resend:
  if(msgsnd(q->id2, response_buffer, response_length, 0) == -1) {
    if(errno == EINTR) {
      log("Message sending interrupted.");
      if(*exit_after_signal) {
        free(query);
        free(response);
        free(response_buffer);
        return DOUBLE_QUEUE_INTERRUPTED;
      } else {
        goto resend;
      }
    } else {
      error("Sending of data failed.");
      free(query);
      free(response);
      free(response_buffer);
      return error_f(error_a);
    }
  }
  free(response);
  free(response_buffer);
	free(query);
  return 0;
  
allocation_failed:
  error("Allocation failed.");
  if(query) free(query);
  if(response) free(response);
  if(response_buffer) free(response_buffer);
  return error_f(error_a);
}

int double_queue_listen(
			double_queue_t *q,
			int (*server)(void*, size_t, void **, size_t*, int, void*),
      void *arg, int *exit_after_signal,
			int destination) {
	return double_queue_listen_err(q, server, arg, exit_after_signal, destination, simple_error, NULL);
}
