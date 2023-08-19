#ifndef CLI_H
#define CLI_H

  #include <stdio.h>

  #include "util.h"

  #define STRINGIZE(a)  #a
  #define STRINGIFY(a)  STRINGIZE(a)

  #define MAJOR         1
  #define MINOR         0
  #define PATCH         0

  #define VERSION       "v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH)
  #define VERSION_HELP  "Version of this software (" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")"

  #define OPTSTR        ":hvldbn:r:p:a:u::"
  #define ARG_MAX       5
  #define ARG_COUNT     9

  #define BITBUF_SIZE   1024
  #define ASSESS_BITS   1000000
  #define ASSESS_LIMIT  1000

  #define POPCNT_4(i)   POPCNT(_ptr[(i) + 0]) + POPCNT(_ptr[(i) + 1]) + POPCNT(_ptr[(i) + 2]) + POPCNT(_ptr[(i) + 3])
  #define PRINT_4(i, j) \
                        print_binary(_bptr + i,       _ptr[(j) + 0]), \
                        print_binary(_bptr + 64 + i,  _ptr[(j) + 1]), \
                        print_binary(_bptr + 128 + i, _ptr[(j) + 2]), \
                        print_binary(_bptr + 192 + i, _ptr[(j) + 3])  \

  #define GET_1(i)      ptr[i]
  #define GET_2(i)      ptr[i], ptr[i + 1]
  #define GET_3(i)      ptr[i], ptr[i + 1], ptr[i + 2]


  u8  stream_live(u64 *restrict ptr);
  u64 stream_bits(FILE *fptr, u64 *restrict _ptr, const u64 limit);
  u16 a_to_u(const char *s, const u16 min, const u16 max);
  u8  help();

#endif