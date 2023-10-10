#ifndef CLI_H
#define CLI_H
  #include <stdio.h>

  #include "util.h"

  #define STRINGIZE(a)    #a
  #define STRINGIFY(a)    STRINGIZE(a)

  #define MAJOR           1
  #define MINOR           3
  #define PATCH           5

  #define VERSION         "v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH)
  #define VERSION_HELP    "Version of this software (" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")"

  #define OPTSTR          ":hvlebxow:a:r::u::s::n::"
  #define ARG_COUNT       12

  #define BITBUF_SIZE     1024
  #define ASSESS_BITS     1000000
  #define ASSESS_LIMIT    5000

  #define PRINT_4(i, j)   print_binary(_bptr + i,       _ptr[(j) + 0]), \
                          print_binary(_bptr + 64 + i,  _ptr[(j) + 1]), \
                          print_binary(_bptr + 128 + i, _ptr[(j) + 2]), \
                          print_binary(_bptr + 192 + i, _ptr[(j) + 3])  

  #define GET_1(i)        _ptr[i]
  #define GET_2(i)        _ptr[i], _ptr[i + 1]
  #define GET_3(i)        _ptr[i], _ptr[i + 1], _ptr[i + 2]

  u8  bits(u64 *restrict _ptr, u64 *seed, u64 *nonce);
  u8  assess(u64 *restrict _ptr, const u16 limit, u64 *seed, u64 *nonce);
  u64 a_to_u(const char *s, const u64 min, const u64 max);
  u8  help();
  u8  uuid(u64 *restrict _ptr, const u8 limit, u64 *seed, u64 *nonce);
  u8  examine(u64 *restrict _ptr, const u16 limit, u64 *seed, u64 *nonce);
  u8  infinite(u64 *restrict _ptr, u64 *seed, u64 *nonce);
#endif