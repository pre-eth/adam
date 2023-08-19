#include <getopt.h>

#include "adam.h"
#include "cli.h"

int main(int argc, char **argv) {
  // The algorithm requires at least the construction of 3 maps of size BUF_SIZE
  // Offsets logically represent each individual map, but it's all one buffer
  u64 buffer[BUF_SIZE * 3] ALIGN(64);
  u64 *restrict buf_ptr = &buffer[0];

  u8 precision = 64;
  u16 results = 0;
  u32 limit = ASSESS_BITS;

  int opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      case 'l':
        return stream_live(buf_ptr);
      case 'a':
        if (optarg != NULL) 
          limit *= a_to_u(optarg, 1, ASSESS_LIMIT);

        char file_name[65];
        file_name[65] = '\0';
        printf("Enter file name: ");
        scanf("%64s", &file_name);
        char c = '\0';

        FILE *fptr;
        do {
          puts("Select file type: [0] ASCII [1] BINARY");
          scanf("%c", &c);
          switch (c) {
            case '0':
              fptr = fopen(file_name, "w+");
            break;
            case '1':
              fptr = fopen(file_name, "wb+");
            break;
            default:
              fputs("\e[1;31mERROR: Valid options are 0 or 1\e[m\n", stderr);
              continue;
          }
        } while(1);
        results = stream_bits(fptr, buf_ptr, limit);
        fclose(fptr);
        return printf("\n\e[1;36mPRINTED %u BITS (%u ONES, %u ZEROES)\e[m\n", limit, results, limit - results);
      case 'b':
        results = stream_bits(stdout, buf_ptr, __UINT32_MAX__);
        return printf("\n\e[1;36mPRINTED %u BITS (%u ONES, %u ZEROES)\e[m\n", limit, results, limit - results);
      case 'p':
        const u8 p = a_to_u(optarg, 8, 64);
        if (LIKELY(!(p & (p - 1)))) {
          /*
            This line will basically "floor" results to the max value of results
            possible for this new precision in case it exceeds the possible limit
            This can be avoided by ordering your arguments so that -p comes first
          */
          const u8 max = BUF_SIZE << CTZ(precision);
          results -= (results > max) * (results - max);
          break;  
        } 
        return fputs("\e[1;31mERROR: Precision must be either 8, 16, 32, or 64 bits\e[m\n", stderr);
      case 'd':
        results = SEQ_SIZE >> CTZ(precision);
        break;
      case 'n':
        results = a_to_u(optarg, 1, BUF_SIZE << CTZ(precision)) - 1;
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

  register u8 idx = 0;
  register u64 mask = (1UL << precision) - 1;

  print_buffer:
    printf("%llu", buf_ptr[idx] & mask);
    mask <<= precision;
    idx += !mask;
    mask |= (!mask << precision) - !mask;

    if (results-- > 0) {
      printf(", ");
      goto print_buffer;
    }

  putchar('\n');

  return 0;
}