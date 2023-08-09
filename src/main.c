#include <getopt.h>

#include "adam.h"
#include "cli.h"

// The algorithm requires at least the construction of 3 maps of size BUF_SIZE
// Offsets are used to logically represent each individual map, but it's just
// all one buffer
static u64 buffer[BUF_SIZE * 3] ALIGN(64);

int main(int argc, char **argv) {
  if (argc - 1 > ARG_MAX) 
    return fputs("\e[1;31mERROR: Invalid number of arguments\e[m\n", stderr);

  u8 precision = 8;
  u16 results = 0;
  
  u64 *restrict buf_ptr = &buffer[0];

  int opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      // case 'l':
      //   return stream_live();
      case 'b':
        u32 limit = 999999;
        if (optarg != NULL) 
          limit = a_to_u(optarg, 512, limit);
        const u32 ones = stream_bits(buf_ptr, limit);
        return printf("\e[1mPRINTED %u BITS (%u ONES, %u ZEROES)\e[m\n", ones, limit - ones);
      case 'p':
        const u8 p = a_to_u(optarg, 8, 64);
        if (LIKELY(!(p & (p - 1)))) {
          precision = p >> 3;
          /*
            This line will basically "floor" results to the max value of results
            possible for this new precision in case it exceeds the possible limit
            This can be avoided by ordering your arguments so that -p comes first
          */ 
          results -= (results > BUF_SIZE * precision) * (results - BUF_SIZE * precision);
          break;  
        } 
        return fputs("\e[1;31mERROR: Precision must be either 8, 16, 32, or 64 bits\e[m\n", stderr);
      case 'd':
        results = BUF_SIZE * precision;
        break;
      case 'n':
        results = a_to_u(optarg, 1, BUF_SIZE * precision) - 1;
        break;
      // case 's':
      //   puts("Enter a seed between 0.0 and 0.5");
      //   if (optarg != NULL) {
      //     scanf("%.15lf", &seed);
      //     while (seed < 0.0 || seed > 0.5)
      //       fputs("Seed value must satisfy range 0.0 < x < 0.5. Try again.", stderr);
      //       scanf("%.15lf", &seed);
      //     break;
      //   }
      default:
        return fputs("\e[1;31mERROR: Option is invalid or missing required argument\e[m\n", stderr);             
    }
  }

  adam(buf_ptr);

  // try condensing via cascade
  print_buffer:
    printf("%llu", print_num(buf_ptr, precision));
    buf_ptr += precision;

    if (results-- > 0) {
      printf(", ");
      goto print_buffer;
    }

  putchar('\n');

  return 0;
}