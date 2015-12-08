#ifndef _ARCHEO_DEBUG_H_
#define _ARCHEO_DEBUG_H_

#ifndef NDEBUG
  #include <errno.h>
  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
  #include <sys/types.h>
  #define print_debug_message(lvl, col, crt, s, args...) fprintf(stderr, "[%s] %d\t(%s:%d):     \t" col "" s "" crt "\n", lvl, getpid(), __FILE__, __LINE__, ## args)
  #define errorp() print_debug_message("\x1b[31m EE \x1b[0m", "", "", "%s", strerror(errno))
#else
  #define print_debug_message(lvl, col, crt, s, args...) do {} while(0)
  #define errorp() print_debug_message("", "", "", "")
#endif

#define log(s, args...) print_debug_message("    ", "", "", s, ## args)
#define warning(s, args...) print_debug_message("\x1b[33m WW \x1b[0m", "", "", s, ## args)
#define error(s, args...) print_debug_message("\x1b[1;31m EE \x1b[0m", "", "", s, ## args)
#define bold(s, args...) print_debug_message("    ", "\x1b[1;37m", "\x1b[0m", s, ## args)

#endif /* end of include guard: _ARCHEO_DEBUG_H_ */
