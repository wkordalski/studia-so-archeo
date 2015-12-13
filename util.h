#ifndef _ARCHEO_UTIL_H_
#define _ARCHEO_UTIL_H_

#include <stddef.h>

int simple_error(void *);
int skip_notifications(void *, size_t, void*);
void print_hexadecimal(char *s, int l);

int match(void *buff, size_t length, char *format, ...);
int combine(void **buff, size_t *len, char *format, ...);

#endif /* end of include guard: _ARCHEO_UTIL_H_ */
