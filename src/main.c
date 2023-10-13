#include <getopt.h>        // arg parsing
#include <sys/random.h>   // getentropy()

#include "adam.h"
#include "cli.h"

/*
  TODO

  Multi-threading
  -e (ent with bob's tests (chi.c and est.c))
  gjrand
  matrix

*/

FORCE_INLINE static u8 err(const char *s) {
  return fprintf(stderr, "\033[1;31m%s\033[m\n", s);
}

/* 
  The algorithm requires at least the construction of 3 maps of size BUF_SIZE
  Offsets logically represent each individual map, but it's all one buffer

  static is necessary because otherwise buffer is initiated with junk that 
  removes the deterministic nature of the algorithm
*/
#ifdef __AARCH64_SIMD__
  static u64 buffer[BUF_SIZE * 3] ALIGN(64);
#else
static u64 buffer[BUF_SIZE * 3] ALIGN(SIMD_LEN);
#endif

int main(int argc, char **argv) {
  u64 *restrict buf_ptr = &buffer[0];

  const char *fmt = "%lu";

  register u8 precision  = 64,
              idx        = 0,
              show_seed  = 0,
              show_nonce = 0;

  register u16 results = 1, 
               limit   = 0;

  register u64 mask = __UINT64_MAX__ - 1;

  u64 *restrict buf_ptr = &buffer[0];

  rng_data data;
  data.buffer = buf_ptr;
  getentropy(&data.seed[0], sizeof(u64) << 2); 
  data.nonce = ((u64) time(NULL)) ^ GOLDEN_RATIO;
  data.aa = data.bb = 0UL;
  data.reseed = 0;
  data.durations[0] = data.durations[1] = data.durations[2] = data.durations[3] = 0.0;

  register short opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      case 'l':
        data.reseed = 1;
        return infinite(&data);
      case 'a':
        limit = a_to_u(optarg, 1, ASSESS_LIMIT);
        if (!limit)
          return err("Multiplier must be within range [1, 8000]");
        data.reseed = 1;
        assess(limit, &data);
        goto show_params;
      case 'b':
        data.reseed = 1;
        return bits(&data);
      case 'x':
        fmt = "0x%lX";
      break;
      case 'o':
        fmt = "0o%lo";
      break;      
      case 'w':
        precision = a_to_u(optarg, 8, 64);
        if (LIKELY(!(precision & (precision - 1)))) {
          mask = (1UL << precision) - 1;
          /*
            This line will basically "floor" results to the max value of results
            possible for this new precision in case it exceeds the possible limit
            This can be avoided by ordering your arguments so that -p comes first
          */
          const u8 max = SEQ_SIZE >> CTZ(precision);
          results -= (results > max) * (results - max);
          break;  
        } 
        return err("Width must be either 8, 16, 32, or 64 bits");
      break;
      case 'r':
        if (optarg)
          results = a_to_u(optarg, 1, SEQ_SIZE >> CTZ(precision));   
        else
          results = SEQ_SIZE >> CTZ(precision);
      break;
      case 's':
        show_seed = (optarg == NULL);
        // if (!show_seed)
        //   seed = a_to_u(optarg, 1, __UINT64_MAX__);
      break;
      case 'n':
        show_nonce = (optarg == NULL);
        if (!show_nonce)
          data.nonce = a_to_u(optarg, 1, __UINT64_MAX__);
      break;
      case 'u':
        limit = 1;
        if (optarg) {
          limit = a_to_u(optarg, 1, 128);
          if (!limit)
            return err("Invalid amount specified. Value must be within range [1, 128]");
        }
        uuid(limit, &data);
        goto show_params;
      default:
        return err("Option is invalid or missing required argument");             
    }
  }

  adam(&data);

  // Need to do this for default precision because 
  // we can't rely on overflow arithmetic :(
  u8 inc = (precision == 64);
  print_buffer:
    printf(fmt, buf_ptr[idx] & mask);

    if (--results > 0) {
      printf(",\n");
      buf_ptr[idx] >>= precision;
      idx += (inc || !buf_ptr[idx]);
      goto print_buffer;
    }

  putchar('\n');

  show_params:
    if (UNLIKELY(show_seed))
      printf("\033[1;36mSEED:\033[m %llu\n", seed);

    if (UNLIKELY(show_nonce))
      printf("\033[1;36mNONCE:\033[m %llu\n", data.nonce);

  return 0;
}