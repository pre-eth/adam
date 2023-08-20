#include <getopt.h>

#include "adam.h"
#include "cli.h"

int main(int argc, char **argv) {
  // The algorithm requires at least the construction of 3 maps of size BUF_SIZE
  // Offsets logically represent each individual map, but it's all one buffer
  u64 buffer[BUF_SIZE * 3] ALIGN(64);
  u64 *restrict buf_ptr = &buffer[0];

  u8  precision = 64;
  u32 results = 0;
  u64 limit = ASSESS_BITS;

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
        limit *= a_to_u(optarg, 1, ASSESS_LIMIT);

        char file_name[65];
        get_file_name:
          printf("Enter file name: ");
          if (fgets(file_name, sizeof(file_name), stdin) == NULL) {
            fputs("\e[1;31mPlease enter a valid file name\e[m\n", stderr);
            goto get_file_name;
          }

        FILE *fptr;
        char c;
        get_file_type:
          printf("Select file type - ASCII [0] or BINARY [1]: ");
          scanf("%c", &c);
          if (c == '0') {
            fptr = fopen(file_name, "w+");
            c = stream_ascii(fptr, buf_ptr, limit);
          } else if (c == '1') {
            fptr = fopen(file_name, "wb+");
            c = stream_bytes(fptr, buf_ptr, limit);
          } else {
            fputs("\e[1;31mValue must be 0 or 1\e[m\n", stderr);
            goto get_file_type;
          }

        if (UNLIKELY(!c))
          return fputs("\e[1;31mError while creating file. Exiting.\e[m\n", stderr);
        
        return fclose(fptr);
      case 'b':
        return stream_ascii(stdout, buf_ptr, __UINT64_MAX__);
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
        return fputs("\e[1;31mPrecision must be either 8, 16, 32, or 64 bits\e[m\n", stderr);
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
        return fputs("\e[1;31mOption is invalid or missing required argument\e[m\n", stderr);             
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
      printf(",\n");
      goto print_buffer;
    }

  putchar('\n');

  return 0;
}