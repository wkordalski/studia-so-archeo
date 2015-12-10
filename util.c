#include "util.h"

#include <stdio.h>

int simple_error(void *unused) {
  return -1;
}

void print_hexadecimal(char *s, int l) {
  for(int i = 0; i < l; i++)
    printf("%02x ", (unsigned char)(s[i]));
  printf("\n");
}
