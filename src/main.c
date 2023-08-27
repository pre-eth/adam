#include <getopt.h>

#include "adam.h"
#include "cli.h"

/*
  TODO
  
  UUID
  matrix
  ent
  gjrand
  bob's tests (chi.c and est.c)

*/

// The algorithm requires at least the construction of 3 maps of size BUF_SIZE
// Offsets logically represent each individual map, but it's all one buffer
static u64 buffer[BUF_SIZE * 3] ALIGN(SIMD_LEN);

int main(int argc, char **argv) {
  u64 *restrict buf_ptr = &buffer[0];

  register u8  precision = 64;
  register u16 results = 0;
  register u16 limit = 1;

  register u8 idx, show_seed, show_nonce;
  register u64 mask = (1UL << precision) - 1;
  
  u64 seed;
  while (!(idx = SEED64(&seed))); 

  double chseed = ((double) seed / (double) __UINT64_MAX__) * 0.5;
  u64 nonce = ((u64) time(NULL)) ^ GOLDEN_RATIO ^ seed;

  idx = show_seed = show_nonce = 0;
  
  int opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      case 'l':
        return infinite(buf_ptr, chseed, nonce);
      case 'a':
        limit = a_to_u(optarg, 1, ASSESS_LIMIT);
        return assess(buf_ptr, limit, chseed, nonce);
      case 'b':
        return bits(buf_ptr, chseed, nonce);
      case 'p':
        const u8 p = a_to_u(optarg, 8, 64);
        if (LIKELY(!(p & (p - 1)))) {
          precision = p;
          mask = (1UL << precision) - 1;
          /*
            This line will basically "floor" results to the max value of results
            possible for this new precision in case it exceeds the possible limit
            This can be avoided by ordering your arguments so that -p comes first
          */
          const register u8 max = BUF_SIZE << CTZ(precision);
          results -= (results > max) * (results - max);
          break;  
        } 
        return err("Precision must be either 8, 16, 32, or 64 bits");
      case 'd':
        results = SEQ_SIZE >> CTZ(precision);
      break;
      case 'r':
        results = a_to_u(optarg, 1, BUF_SIZE << CTZ(precision)) - 1;
      break;
      case 's':
        show_seed = (optarg == NULL);
        if (!show_seed) {
          int res = sscanf(optarg, "%lf", &chseed);
          if (!res || res == -1) 
            return err("Seed must be a valid decimal within (0.0, 0.5)");
        }
      break;
      case 'n':
        show_nonce = (optarg == NULL);
        if (!show_nonce)
          nonce = a_to_u(optarg, 1, __UINT64_MAX__);
      break;
      case 'u':
        limit = optarg ? a_to_u(optarg, 1, 128) : 1;
        return uuid(buf_ptr, limit, chseed, nonce);
      default:
        return err("Option is invalid or missing required argument");             
    }
  }

  clock_t start = clock();
  adam(buf_ptr, chseed, nonce);
  clock_t end = clock();
  double duration = (double)(end - start) / CLOCKS_PER_SEC;

  print_buffer:
    printf("%lu", buf_ptr[idx] & mask);
    mask <<= precision;
    idx += !mask;
    mask |= (!mask << precision) - !mask;

    if (results-- > 0) {
      printf(",\n");
      goto print_buffer;
    }

  putchar('\n');

  if (UNLIKELY(show_seed == 1))
    printf("\e[1;36mSEED:\e[m %.15f\n", chseed);

  if (UNLIKELY(show_nonce == 1))
    printf("\e[1;36mNONCE:\e[m %lu\n", nonce);


  printf("DURATION: %lfs\n", duration);

  return 0;
}