#include "double_queue.h"
#include "debug.h"
#include "util.h"

#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

const char ACT_WAIT_REQUEST = 0; /* gets queue number */

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
  q->conds_count = 0;
  q->conds_capa = 1;
  q->conds = malloc(sizeof(condition_queue_info_t));
  if(q->conds == NULL) {
    error("Allocation failed.");
    q->conds_capa = 0;
    return error_f(error_a);
  }
  return 0;
}

int double_queue_init(double_queue_t *q, key_t key1, key_t key2, int flags1, int flags2) {
  return double_queue_init_err(q, key1, key2, flags1, flags2, simple_error, NULL);
}

void double_queue_close(double_queue_t *q) {
  if(q->id1 >= 0) {
    if(msgctl(q->id1, IPC_RMID, NULL) == -1) warning("Closing of message queue (direction C->S) failed.");
  }
  if(q->id2 >= 0) {
    if(msgctl(q->id2, IPC_RMID, NULL) == -1) warning("Closing of message queue (direction S->C) failed.");
  }
  if(q->conds != NULL) {
    free(q->conds);
  }
}

// Query - sends query and waits for result (client side)
int double_queue_query_err(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int (*on_notification)(void *, size_t, void *),
      void *arg, int *exit_after_signal,
      long source, long destination,
      int (*error_f)(void*), void *error_a) {
  // Parameter checks
  if(query_length > 1024 /* maksymalna długość komunikatu */) {
    error("Too long message.");
    return error_f(error_a);
  }
  // Prepare query
  void * real_query = malloc(query_length + 2 * sizeof(long)+1);
  if(real_query == NULL) {
    error("Allocation failed.");
    return error_f(error_a);
  }
  char * answer_buffer = malloc(1024+1+sizeof(long) /* bajt notyfikacji */);
  if(answer_buffer == NULL) {
    error("Allocation failed.");
    free(real_query);
    return error_f(error_a);
  }
  char zero = 0;
  memcpy(real_query, &destination, sizeof(long));
  memcpy(real_query + sizeof(long), query, query_length);
  memcpy(real_query + sizeof(long) + query_length, &source, sizeof(long));
  memcpy(real_query + sizeof(long) * 2 + query_length, &zero, 1);
  // Send query
  if(msgsnd(q->id1, real_query, query_length + sizeof(long)+1 /* bez pola "rodzaj" */, 0 /* wait if full */) == -1) {
    error("Message sending failed.");
    free(real_query);
    free(answer_buffer);
    return error_f(error_a);
  }
  free(real_query);
  // Wait for result
  char is_notification = 0;
  do {
    int ret = msgrcv(q->id2, answer_buffer, 1024+1+sizeof(long) /* nie potrzebujemy adresu zwrotnego */, source, 0);
    if(ret == -1 || ret == 0) {
      error("Message receiving failed.");
      free(answer_buffer);
      return error_f(error_a);
    }
    char * res = malloc(ret - 1);
    if(res == NULL) {
      error("Allocation failed.");
      free(answer_buffer);
      return error_f(error_a);
    }
    memcpy(res, answer_buffer+sizeof(long), ret - 1);
    is_notification = (answer_buffer[sizeof(long)+ret-1] == 0)?0:1;
    if(is_notification) {
      on_notification(res, ret-1, arg);
    } else {
      (*result) = res;
      (*result_length) = ret-1;
      break;
    }
  } while(is_notification);
  free(answer_buffer);
  return 0;
}

int double_queue_query(
      double_queue_t *q,
      void * query, size_t query_length,
      void ** result, size_t *result_length,
      int (*on_notification)(void*, size_t, void*),
      void *arg, int *exit_after_signal,
      long source, long destination) {
  return double_queue_query_err(q, query, query_length, result, result_length, on_notification, arg, exit_after_signal, source, destination, simple_error, NULL);
}

// Listen - receives query and sends result (server side)
int double_queue_listen_err(
      double_queue_t *q,
      int (*server)(void *, size_t, void **, size_t *, int *, int, void*),
      void *arg, int *exit_after_signal,
      int destination,
      int (*error_f)(void*), void *error_a) {
  // Prepare buffers
  void * query = malloc(1024+2*sizeof(long)+1  /* message + source */);
  if(query == NULL) {
    error("Allocation failed.");
    return error_f(error_a);
  }
  // Receive
  int ret = msgrcv(q->id1, query, 1024+2*sizeof(long)+1, destination, 0);
  if(ret == -1 || ret < sizeof(long)+1) {
    error("Message received failed.");
    free(query);
		return error_f(error_a);
  }
  long source = 0;
  char do_wait = 0;
  ret -= (sizeof(long)+1);
  memcpy(&source, query + ret + sizeof(long), sizeof(long));
  memcpy(&do_wait, query + ret + 2* sizeof(long), 1);
  if(do_wait) {
    long queue_no;
    if(match(query, ret, "%l", &queue_no) == -1) {
      error("Bad wait query.");
      free(query);
      return error_f(error_a);
    }
    int idx = 0;
    for(; idx < q->conds_count; idx++) {
      if(q->conds[idx].id == queue_no) break;
    }
    if(idx >= q->conds_count) {
      if(q->conds_count >= q->conds_capa) {
        q->conds_capa *= 2;
        q->conds = realloc(q->conds, q->conds_capa);
        if(q->conds == NULL) {
          error("Allocation failed.");
          free(query);
          return error_f(error_a);
        }
      }
      q->conds[idx].id = queue_no;
      q->conds[idx].count = 1;
      q->conds_count++;
    } else {
      q->conds[idx].count++;
    }
  } else {
  	int is_notification = 0;
  	do {
  		void *response = NULL;
  		size_t response_length;
      is_notification = 0;
  		if(server(query+sizeof(long), ret, &response, &response_length, &is_notification, source, arg) == -1) {
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
  		unsigned char * response_buffer = malloc(1024 + sizeof(long) + 1 /* address + body + notification */);
  		if(response_buffer == NULL) {
  			error("Allocation failed.");
  			free(query);
  			free(response);
  			return error_f(error_a);
  		}
  		memcpy(response_buffer, &source, sizeof(long));
  		memcpy(response_buffer + sizeof(long), response, response_length);
  		response_buffer[response_length + sizeof(long)] = (is_notification == 0?0:1);
  		if(msgsnd(q->id2, response_buffer, response_length + 1, 0) == -1) {
  			error("Sending of data failed.");
  			free(query);
  			free(response);
  			free(response_buffer);
  			return error_f(error_a);
  		}
  		free(response);
  		free(response_buffer);
  	} while(is_notification);
  }
	free(query);
  return 0;
}

int double_queue_listen(
			double_queue_t *q,
			int (*server)(void*, size_t, void **, size_t*, int *, int, void*),
      void *arg, int *exit_after_signal,
			int destination) {
	return double_queue_listen_err(q, server, arg, exit_after_signal, destination, simple_error, NULL);
}



int double_queue_wait_err(
      double_queue_t *q,
      long source, long destination, long queue,
			int *exit_after_signal,
      int (*error_f)(void*), void *error_a) {
  // Prepare query
  void * real_query;
  size_t real_query_length;
  if(combine(&real_query, &real_query_length, "%l %l %l %c", destination, queue, source, 1) != 0) {
    error("Combine failed.");
    return error_f(error_a);
  }
  char * answer_buffer = malloc(1024+1+sizeof(long) /* bajt notyfikacji */);
  if(answer_buffer == NULL) {
    error("Allocation failed.");
    free(real_query);
    return error_f(error_a);
  }
  // Send query
  if(msgsnd(q->id1, real_query, real_query_length - sizeof(long) /* bez pola "rodzaj" */, 0 /* wait if full */) == -1) {
    error("Message sending failed.");
    free(real_query);
    free(answer_buffer);
    return error_f(error_a);
  }
  free(real_query);
  // Wait for result
  int ret = msgrcv(q->id2, answer_buffer, 1024+1+sizeof(long) /* nie potrzebujemy adresu zwrotnego */, 0 /* signals do not have specified receiver */, 0);
  if(ret == -1) {
    error("Message receiving failed.");
    free(answer_buffer);
    return error_f(error_a);
  } else if(ret == 0) {
    // OK - we have been signalled
    free(answer_buffer);
    return 0;
  } else {
    warning("We got some data? Maby some programmer mistake...");
    free(answer_buffer);
    return error_f(error_a);
  }
}

int double_queue_wait(
      double_queue_t *q,
      long source, long destination, long queue,
			int *exit_after_signal) {
  return double_queue_wait_err(q, source, destination, queue, exit_after_signal, simple_error, NULL);
}


int double_queue_signal_one_err(
      double_queue_t *q,
      long queue, int *exit_after_signal,
      int (*error_f)(void*), void *error_a) {
  // get the queue
  if(q->conds_count == 0) {
    error("No such queue.");
    return error_f(error_a);
  }
  int idx = 0;
  for(; idx < q->conds_count; idx++) {
    if(q->conds[idx].id == queue) break;
  }
  if(idx >= q->conds_count) {
    error("No such queue.");
    return error_f(error_a);
  }
  // check if somebody is waiting
  if(q->conds[idx].count == 0) {
    error("Nobody waiting on this queue.");
    return error_f(error_a);
  }
  // then send signal message
  void *msg;
  size_t msg_len;
  combine(&msg, &msg_len, "%l", 1);
  if(msgsnd(q->id2, msg, msg_len - sizeof(long) /* bez pola "rodzaj" */, 0 /* wait if full */) == -1) {
    error("Sending of message failed.");
    free(msg);
    return error_f(error_a);
  }
  q->conds[idx].count--;
  free(msg);
  return 0;
}

int double_queue_signal_one(
      double_queue_t *q,
      long queue, int *exit_after_signal) {
  return double_queue_signal_one_err(q, queue, exit_after_signal, simple_error, NULL);
}


int double_queue_signal_all_err(
      double_queue_t *q,
      long queue, int *exit_after_signal,
      int (*error_f)(void*), void *error_a) {
  // get the queue
  if(q->conds_count == 0) {
    error("No such queue.");
    return error_f(error_a);
  }
  int idx = 0;
  for(; idx < q->conds_count; idx++) {
    if(q->conds[idx].id == queue) break;
  }
  if(idx >= q->conds_count) {
    error("No such queue.");
    return error_f(error_a);
  }
  // signal all clients
  void *msg;
  size_t msg_len;
  combine(&msg, &msg_len, "%l", 0xffffffff /* fajna stała */);

  while(q->conds[idx].count > 0) {
    if(msgsnd(q->id2, msg, msg_len - sizeof(long) /* bez pola "rodzaj" */, 0 /* wait if full */) == -1) {
      error("Sending of message failed.");
      free(msg);
      return error_f(error_a);
    }
    q->conds[idx].count--;
  }
  free(msg);
  return 0;
}

int double_queue_signal_all(
      double_queue_t *q,
      long queue, int *exit_after_signal) {
  return double_queue_signal_all_err(q, queue, exit_after_signal, simple_error, NULL);
}
