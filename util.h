#ifndef _ARCHEO_UTIL_H_
#define _ARCHEO_UTIL_H_

int simple_error(void *);

void * zmalloc(size_t size, void(*error_f)(void *), void *arg) {
  void *ret = malloc(size);
  if(ret == NULL) {
    error_f(arg);
  }
  return NULL;
}

#endif /* end of include guard: _ARCHEO_UTIL_H_ */
