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

int skip_notifications(void *content, size_t size, void* arg) {
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

static int is_format_start_command_character(char c) {
  if(c == '%') return 1;
  if(c == '@') return 1;
  if(c == '$') return 1;
  if(c == '"') return 1;
  if(c == '\'') return 1;
  return 0;
}

static int parse_format_command(char **format, char *type, char **argument, int *argument_length, char **options, int *options_length) {
  if(isspace(**format)) (*format)++;
  if(**format == 0) return 1;  // END OF FORMAT
  *type = **format;
  switch(*type) {
    case '%':
    case '@':
      (*format)++;
      *argument = *format;
      *argument_length = 0;
      *options = NULL;
      *options_length = 0;
      while(!isspace(**format) && **format != ':' && **format != 0 && !is_format_start_command_character(**format)) {
        (*argument_length)++;
        (*format)++;
      }
      if(**format == 0 || isspace(**format) || is_format_start_command_character(**format)) {
        return 0;
      } else if(**format == ':') {
        (*format)++;
        *options = *format;
        while(!isspace(**format) && **format != 0 && is_format_start_command_character(**format)) {
          (*options_length)++;
          (*format)++;
        }
        return 0;
      }
      break;
    case '$':
    case '"':
    case '\'':
      // get argument
      (*format)++;
      *argument = *format;
      *argument_length = 0;
      *options = NULL;
      *options_length = 0;
      while(**format != *type && **format != 0) {
        (*argument_length)++;
        (*format)++;
      }
      if(**format == 0) {
        error("Bad format string.");
        return -1;
      } else {
        (*type) = '$';
        (*format)++;
        return 0;
      }
      break;
  }
  return -1;
}

int match(void *buffer, size_t length, char *format, ...) {
  va_list ap;
  char *s = format;
  char *w = buffer;
  char type;
  char *arg, *opt;
  int  argl, optl;
  int ret;

  va_start(ap, format);

  while((ret = parse_format_command(&s, &type, &arg, &argl, &opt, &optl)) == 0) {
    if(type == '%') {
      if(strncmp("c", arg, argl) == 0) {
        // read charater
        if(optl > 0) goto fail;
        if(length < sizeof(char)) goto unmatched;
        char *ar = va_arg(ap, void*);
        *ar = *w;
        w++;
        length--;
      } else if(strncmp("h", arg, argl) == 0) {
        // read short
        if(optl > 0) goto fail;
        if(length < sizeof(short)) goto unmatched;
        short *ar = va_arg(ap, void*);
        memcpy(ar, w, sizeof(short));
        w += sizeof(short);
        length -= sizeof(short);
      } else if(strncmp("i", arg, argl) == 0) {
        // read int
        if(optl > 0) goto fail;
        if(length < sizeof(int)) goto unmatched;
        int *ar = va_arg(ap, void*);
        memcpy(ar, w, sizeof(int));
        w += sizeof(int);
        length -= sizeof(int);
      } else if(strncmp("l", arg, argl) == 0) {
        // read long
        if(optl > 0) goto fail;
        if(length < sizeof(long)) goto unmatched;
        long *ar = va_arg(ap, void*);
        memcpy(ar, w, sizeof(long));
        w += sizeof(long);
        length -= sizeof(long);
      } else if(strncmp("ll", arg, argl) == 0) {
        // read long long
        if(optl > 0) goto fail;
        if(length < sizeof(long long)) goto unmatched;
        long long *ar = va_arg(ap, void*);
        memcpy(ar, w, sizeof(long long));
        w += sizeof(long long);
        length -= sizeof(long long);
      } else if(strncmp("b", arg, argl) == 0) {
        // read binary
        if(optl > 0) goto fail;
        void* ar = va_arg(ap, void*);
        int br = va_arg(ap, int);
        if(length < br) goto unmatched;
        memcpy(ar, w, br);
        w += br;
        length -= br;
      } else if(strncmp("bv", arg, argl) == 0) {
        // read variable-length binary
        if(optl > 0) goto fail;
        if(length < sizeof(int)) goto unmatched;
        void* ar = va_arg(ap, void*); // data
        int* br = va_arg(ap, void*); // length
        memcpy(br, w, sizeof(int));
        w += sizeof(int);
        length -= sizeof(int);
        if(length < *br) goto unmatched;
        memcpy(ar, w, *br);
        w += *br;
        length -= *br;
      } else {
        goto fail;
      }
    } else if(type == '@') {
      // Make the checker
      if(strncmp("c", arg, argl) == 0) {
        if(optl > 0) goto fail;
        if(length < sizeof(char)) goto unmatched;
        char ar = va_arg(ap, int);
        if(memcmp(&ar, w, sizeof(char)) != 0) goto unmatched;
        w += sizeof(char);
        length -= sizeof(char);
      } else {
        error("Wrong or unsupported format!");
        goto fail;
      }
    } else if(type == '$') {
      // check constant
      if(length < argl) goto unmatched;
      if(memcmp(w, arg, argl) != 0) goto unmatched;
      w += argl;
      length -= argl;
    } else {
      goto fail;
    }
  }
  if(ret == -1) goto fail;
  if(length > 0) goto unmatched;

  return 0;

unmatched:
  va_end(ap);
  return 1;

fail:
  va_end(ap);
  return -1;
}

static int reserve_in_buffer(void **buffer, char **top, size_t *capacity, size_t needed) {
  void **w = (void**)top;
  if((*w - *buffer) + needed >= *capacity) {
    while((*w - *buffer) + needed >= *capacity) *capacity *= 2;
    void *new_buff = realloc(*buffer, *capacity);
    if(new_buff == NULL) {
      error("Allocation failed.");
      return -1;
    }
    *w = new_buff + (*w - *buffer);
    *buffer = new_buff;
  }
  return 0;
}

int combine(void **buffer, size_t *len, char *format, ...) {
  va_list ap;
  char *s = format;
  char *w = *buffer = malloc(64);
  size_t buffer_capacity = 64;
  char type;
  char *arg, *opt;
  int  argl, optl;
  int ret;

  va_start(ap, format);

  while((ret = parse_format_command(&s, &type, &arg, &argl, &opt, &optl)) == 0) {
    if(type == '%') {
      if(strncmp("c", arg, argl) == 0) {
        // write charater
        if(optl > 0) goto fail;
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, 1) == -1) {
          return -1;
        }
        char ar = va_arg(ap, int);
        *w = ar;
        w++;
      } else if(strncmp("h", arg, argl) == 0) {
        // write short
        if(optl > 0) goto fail;
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, sizeof(short)) == -1) {
          return -1;
        }
        short ar = va_arg(ap, int);
        memcpy(w, &ar, sizeof(short));
        w += sizeof(short);
      } else if(strncmp("i", arg, argl) == 0) {
        // write int
        if(optl > 0) goto fail;
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, sizeof(int)) == -1) {
          return -1;
        }
        int ar = va_arg(ap, int);
        memcpy(w, &ar, sizeof(int));
        w += sizeof(int);
      } else if(strncmp("l", arg, argl) == 0) {
        // write long
        if(optl > 0) goto fail;
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, sizeof(long)) == -1) {
          return -1;
        }
        long ar = va_arg(ap, long);
        memcpy(w, &ar, sizeof(long));
        w += sizeof(long);
      } else if(strncmp("ll", arg, argl) == 0) {
        // write long long
        if(optl > 0) goto fail;
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, sizeof(long long)) == -1) {
          return -1;
        }
        long long ar = va_arg(ap, long long);
        memcpy(w, &ar, sizeof(long long));
        w += sizeof(long long);
      } else if(strncmp("b", arg, argl) == 0) {
        // write binary
        if(optl > 0) goto fail;
        void* ar = va_arg(ap, void*);
        int br = va_arg(ap, int);
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, br) == -1) {
          return -1;
        }
        memcpy(w, ar, br);
        w += br;
      } else if(strncmp("bv", arg, argl) == 0) {
        // write variable-length binary
        if(optl > 0) goto fail;
        void* ar = va_arg(ap, void*);
        int br = va_arg(ap, int);
        if(reserve_in_buffer(buffer, &w, &buffer_capacity, sizeof(int) + br) == -1) {
          return -1;
        }
        memcpy(w, &br, sizeof(int));
        w += sizeof(int);
        memcpy(w, ar, br);
        w += br;
      } else {
        goto fail;
      }
    } else if(type == '$') {
      // write constant
      if(reserve_in_buffer(buffer, &w, &buffer_capacity, argl) == -1) {
        return -1;
      }
      memcpy(w, arg, argl);
      w += argl;
    } else {
      goto fail;
    }
  }
  if(ret == -1) goto fail;

  *len = (((void*)w) - *buffer);
  return 0;

fail:
  va_end(ap);
  return -1;
}
