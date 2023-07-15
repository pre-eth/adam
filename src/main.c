#include "adam.h"
#include "cli.h"

int main(int argc, char** argv) {
  if (argc - 1 > ARG_MAX) 
    return fputs("ERROR: Invalid number of arguments", stderr);

  double seed = DEFAULT_SEED;
  u8 rounds = ROUNDS, precision = 8;
  u16 results = 0;

  int opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return !help();
      case 'v':
        return puts(VERSION);
      case 'l':
        return stream_live();
      case 'b':
        return stream_bits(__UINT64_MAX__);
      case 'p':
        u8 p = a_to_u(optarg, 8, 64);
        // Adapted from http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
        if (p && !((p + 1) & p)) {
          precision = p >> 3;
          break;  
        } 
        return fputs("Precision must be either 8, 16, 32, or 64 bits", stderr);
      case 'd':
        results = BUF_SIZE >> ((precision & 7) >> 1);
        break;
      case 'n':
        results = a_to_u(optarg, 1, BUF_SIZE);
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
  u32 buffer[BUF_SIZE] ALIGN(SIMD_INC);
  
  u32* restrict buf_ptr = &buffer;

  generate(buf_ptr, seed, rounds);
  
  u16 idx = 0;

  u64 num;
  print_num:
    num = 0;
    switch (precision) {
      case 1:
        num  = buffer[idx] & 0xFF000000;
        break;
      case 2:
        num  = ((buffer[idx] & 0xFF000000) << 8)
             | (buffer[idx + 1] & 0xFF000000);
        break;
      case 4:
        num  = ((buffer[idx] & 0xFF000000) << 24)
             | ((buffer[idx + 1] & 0xFF000000) << 16)
             | ((buffer[idx + 2] & 0xFF000000) << 8)
             | (buffer[idx + 3] & 0xFF000000);
        break;
      case 8:
        num  = ((buffer[idx] & 0xFF000000) << 56)
             | ((buffer[idx + 1] & 0xFF000000) << 48)
             | ((buffer[idx + 2] & 0xFF000000) << 40)
             | ((buffer[idx + 3] & 0xFF000000) << 32)
             | ((buffer[idx + 4] & 0xFF000000) << 24)
             | ((buffer[idx + 5] & 0xFF000000) << 16)
             | ((buffer[idx + 6] & 0xFF000000) << 8)
             | (buffer[idx + 7] & 0xFF000000);
        break;      
    }
    
    idx += precision;

    printf("%llu", num);

    if (results-- > 0) {
      printf(", ");
      goto print_num;
    }

  putchar('\n');

  return 0;
}