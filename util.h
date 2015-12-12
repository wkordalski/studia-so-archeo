#ifndef _ARCHEO_UTIL_H_
#define _ARCHEO_UTIL_H_

#include <stddef.h>

int simple_error(void *);
void print_hexadecimal(char *s, int l);

int combine(void **buff, size_t *len, char *format, ...);

#endif /* end of include guard: _ARCHEO_UTIL_H_ */
