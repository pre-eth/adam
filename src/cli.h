#ifndef CLI_H
#define CLI_H
  #include <stdio.h>

  #include "util.h"

  #define STRINGIZE(a)    #a
  #define STRINGIFY(a)    STRINGIZE(a)

  #define MAJOR           1
  #define MINOR           4
  #define PATCH           0

  #define VERSION         "v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH)
  #define VERSION_HELP    "Version of this software (" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")"

  #define OPTSTR          ":hvlebxow:a:r::u::s::n::"
  #define ARG_COUNT       12

  #define BITBUF_SIZE     1024
  #define ASSESS_BITS     1000000ULL
  #define ASSESS_LIMIT    8000

  #define PRINT_4(i, j)   print_binary(_bptr + i,       _ptr[(j) + 0]), \
                          print_binary(_bptr + 64 + i,  _ptr[(j) + 1]), \
                          print_binary(_bptr + 128 + i, _ptr[(j) + 2]), \
                          print_binary(_bptr + 192 + i, _ptr[(j) + 3])  

  #define GET_1(i)        _ptr[i]
  #define GET_2(i)        _ptr[i], _ptr[i + 1]
  #define GET_3(i)        _ptr[i], _ptr[i + 1], _ptr[i + 2]

  // Forward declaration - see adam.h for details
  typedef struct rng_data rng_data;

  u8  bits(rng_data *data);
  u8  assess(const u16 limit, rng_data *data);
  u64 a_to_u(const char *s, const u64 min, const u64 max);
  u8  open_file(char *file_name, u8 ask_type);
  u8  help(void);
  u8  uuid(u8 limit, rng_data *data);
  u8  examine(u64 *restrict _ptr, const u16 limit, u64 *seed, u64 *nonce);
  u8  infinite(rng_data *data);
#endif
