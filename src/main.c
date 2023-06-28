#include <stdio.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include "adam.h"
#include "cli.h"

int main(int argc, char** argv) {
  if (argc - 1 > ARG_MAX) 
    return fputs("ERROR: Invalid number of arguments", stderr);

  float seed = DEFAULT_SEED;
  u8 rounds = ROUNDS, results = 0, precision = 8;

  int opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return 
        putchar('v'),
        putchar('0' + MAJOR), putchar('.'),
        putchar('0' + MINOR), putchar('.'),
        putchar('0' + PATCH);  
      case 'l':
        return stream_live();
      case 'b':
        return stream_bits(__UINT64_MAX__);
      case 'd':
        results = 255;
        break;
      case 'n':
        results = a_to_u(optarg, 0, 255);
        break;
      case 'r':
        rounds = a_to_u(optarg, 8, 26) + 1;
        break;
      case 'p':
        u8 p = a_to_u(optarg, 7, 63);
        // Adapted from http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
        if (p && !((p + 1) & p)) {
          precision = p >> 3;
          break;  
        } else
          return fputs("Precision must be either 8, 16, 32, or 64 bits.", stderr);    
      default:
        return fputs("Option is invalid or missing required argument.", stderr);             
    }
  }

  // You can imagine this as a multidimensional 4D array of u8 with size 1024.
  u32 buffer[BUF_SIZE] ALIGN(32);
  
  u32* restrict buf_ptr = &buffer;

  u64 seed = generate(buf_ptr, seed, rounds);
  
  return 0;
}