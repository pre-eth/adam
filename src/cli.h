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

  #define OPTSTR        ":hvldn:r:p:a::b::u::"
  #define ARG_MAX       5
  #define ARG_COUNT     10

  #define BITBUF_SIZE   1024
  #define ASSESS_MULT   1000000

  u64 stream_bits(const u64 *restrict _ptr, const u64 limit);
  u32 a_to_u(const char *s, const u32 min, const u32 max);
  u8 help();

#endif