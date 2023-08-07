#include "adam.h"
#include "cli.h"

int main(int argc, char** argv) {
  if (argc - 1 > ARG_MAX) 
    return fputs("\e[1;31mERROR: Invalid number of arguments\e[m", stderr);

  u8 precision = 8;
  u16 results = 0;

  // You can imagine this as a multidimensional 4D array of u8 with size 1024.
  u32 buffer[BUF_SIZE] ALIGN(SIMD_INC);
  u32* restrict buf_ptr = &buffer[0];

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
        int limit = 1000000;
        if (optarg != NULL) 
          limit = a_to_u(optarg, 8, limit);
        return 0;
      case 'p':
        u8 p = a_to_u(optarg, 8, 64);
        // Adapted from http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
        if (p && !((p + 1) & p)) {
          precision = p >> 3;
          break;  
        } 
        return fputs("\e[1;31mERROR: Precision must be either 8, 16, 32, or 64 bits\e[m", stderr);
      case 'd':
        results = BUF_SIZE >> (precision >> 1);
        break;
      case 'n':
        results = a_to_u(optarg, 1, BUF_SIZE >> (precision >> 1));
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
        return fputs("\e[1;31mERROR: Option is invalid or missing required argument\e[m", stderr);             
    }
  }

  puts("Generating numbers...");
  generate(buf_ptr);
  puts("Done.");

  u64 num;
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