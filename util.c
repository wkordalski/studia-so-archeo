#include "util.h"

#include "debug.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>

int simple_error(void *unused) {
  return -1;
}

void print_hexadecimal(char *s, int l) {
  for(int i = 0; i < l; i++)
    printf("%02x ", (unsigned char)(s[i]));
  printf("\n");
}

/*
 * Format string rules:
 *  % - placeholder (type : optional params)
 *  @ - matcher (only match) (type : optional params)
 *  $ - raw data (eg. $something$)
 *  ' - raw data (eg. 'something')
 *  " - raw data (eg. "something")
 *
 * Types:
 *  c - char
 *  s - short
 *  i - int
 *  l - long
 *  ll - long long
 *  b - binary (gets length and pointer)
 *  bv - variable length binary (writes length)
 */

static char buffer[1<<20];

static int count_args(char *s) {
  int count = 0;
  char delim = 0;

  while(*s) {
    // TODO
    if(delim && *s == delim) {
      delim = 0;
    } else if(delim) {
      // skip
    } else {
      if(*s == '$' || *s == '\'' || *s == '"') {
        delim = *s;
      } else if(*s == '%' || *s == '@') {
        count++;
      } else {
        // skip
      }
    }
    s++;
  }

  return count;
}
/*
int match(void *buffer, size_t length, char *format, ...) {
  va_list ap;
  // TODO: declarations
  int count = count_args(format);
  va_start(ap, format);

  // for each you can use va_arg(ap, type)

  va_end(ap);

  return result;
}
*/
int combine(void **buff, size_t *len, char *format, ...) {
  va_list ap;
  char *w = buffer;

  va_start(ap, format);

  // for each you can use va_arg(ap, type)
  char *s = format;
  while(*s) {
    if(*s == '%') {
      s++;
      if(*s == 'c') {                         // char
        s++;
        if(isspace(*s) || *s == 0) {
          char ar = va_arg(ap, int);
          *w = ar;
          w++;
        } else {
          va_end(ap);
          return -1;
        }
      } else if(*s == 's') {                  // short
        s++;
        if(isspace(*s) || *s == 0) {
          short ar = va_arg(ap, int);
          memcpy(w, &ar, sizeof(short));
          w += sizeof(short);
        } else {
          va_end(ap);
          return -1;
        }
      } else if(*s == 'i') {                  // int
        s++;
        if(isspace(*s) || *s == 0) {
          int ar = va_arg(ap, int);
          memcpy(w, &ar, sizeof(int));
          w += sizeof(int);
        } else {
          va_end(ap);
          return -1;
        }
      } else if(*s == 'l') {                  // long
        s++;
        if(isspace(*s) || *s == 0) {
          long ar = va_arg(ap, long);
          memcpy(w, &ar, sizeof(long));
          w += sizeof(long);
        } else if(*s == 'l') {                // long long
          s++;
          if(isspace(*s) || *s == 0) {
            long long ar = va_arg(ap, long long);
            memcpy(w, &ar, sizeof(long long));
            w += sizeof(long long);
          } else {
            va_end(ap);
            return -1;
          }
        } else {
          va_end(ap);
          return -1;
        }
      } else if(*s == 'b') {
        s++;
        if(isspace(*s) || *s == 0) {
          int blen = va_arg(ap, int);
          void *data = va_arg(ap, void *);
          memcpy(w, data, blen);
          w += blen;
        } else if(*s == 'v') {
          s++;
          if(isspace(*s) || *s == 0) {
            int blen = va_arg(ap, int);
            void *data = va_arg(ap, void *);
            memcpy(w, &blen, sizeof(int));
            w += sizeof(int);
            memcpy(w, data, blen);
            w += blen;
          } else {
            va_end(ap);
            return -1;
          }
        } else {
          va_end(ap);
          return -1;
        }
      }
    } else if(*s == '$' || *s == '\'' || *s == '"') {
      char delim = *s;
      s++;
      while(*s != delim && *s != 0) {
        *w = *s;
        w++;
        s++;
      }
      if(*s == 0) {
        va_end(ap);
        return -1;
      } else {
        s++;
      }
    } else if(isspace(*s)) {
      s++;
    } else {
      va_end(ap);
      return -1;
    }
  }

  va_end(ap);

  void * bfer = malloc(w - buffer);
  if(bfer == NULL) {
    error("Allocation failed.");
    return -1;
  }
  memcpy(bfer, buffer, w - buffer);
  *buff = bfer;
  *len = (w - buffer);
  log("Combined len = %d.", w - buffer);
  print_hexadecimal(bfer, w-buffer);
  return 0;
}
