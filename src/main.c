#include <getopt.h>

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
  return fprintf(stderr, "\e[1;31m%s\e[m\n", s);
}

static u8 fill_seeds(double seed, double *seeds, regd *seeds_reg) {
  const regd dbl_and = SIMD_SETPD(2.22507385850720088902458687609E-308),
             dbl_or  = SIMD_SETPD(0.5);

  seeds[0] = seed;
  seeds[1] = CHAOTIC_FN(1.0 - seed);
  seeds[2] = 1.0 - CHAOTIC_FN(seeds[1]);
  seeds[3] = CHAOTIC_FN(seeds[2]); 
  regd d1 = SIMD_LOADPD(seeds);

  d1 = SIMD_ANDPD(dbl_and, d1);
  d1 = SIMD_ORPD(d1, dbl_or);
  d1 = SIMD_SUBPD(d1, dbl_or);

  *seeds_reg = d1;
}

/* 
  The algorithm requires at least the construction of 3 maps of size BUF_SIZE
  Offsets logically represent each individual map, but it's all one buffer

  static is necessary because otherwise buffer is initiated with junk that 
  removes the deterministic nature of the algorithm
*/
static u64 buffer[BUF_SIZE * 3] ALIGN(SIMD_LEN);

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

  u64 seed;
  while (!(idx = SEED64(&seed))); 
  idx = 0;

  double seeds[SIMD_LEN >> 3] ALIGN(SIMD_LEN);
  double chseed = ((double) seed / (double) __UINT64_MAX__) * 0.5;
  regd seeds_reg;
  fill_seeds(chseed, seeds, &seeds_reg);

  register u64 nonce = ((u64) time(NULL)) ^ GOLDEN_RATIO ^ seed;

  register short opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      case 'l':
        return infinite(buf_ptr, &seeds_reg, nonce);
      case 'a':
        limit = a_to_u(optarg, 1, ASSESS_LIMIT);
        assess(buf_ptr, limit, &seeds_reg, nonce);
        goto show_params;
      case 'b':
        return bits(buf_ptr, &seeds_reg, nonce);
      case 'x':
        fmt = "0x%lX";
      break;
      case 'w':
        const u8 p = a_to_u(optarg, 8, 64);
        if (LIKELY(!(p & (p - 1)))) {
          precision = p;
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
      case 'd':
        results = SEQ_SIZE >> CTZ(precision);
      break;
      case 'r':
        results = a_to_u(optarg, 1, SEQ_SIZE >> CTZ(precision));
      break;
      case 's':
        show_seed = (optarg == NULL);
        if (!show_seed) {
          int res = sscanf(optarg, "%lf", &chseed);
          if (!res || res == EOF || chseed <= 0.0 || chseed >= 0.5) 
            return err("Seed must be a valid decimal within (0.0, 0.5)");
          fill_seeds(chseed, seeds, &seeds_reg);
        }
      break;
      case 'n':
        show_nonce = (optarg == NULL);
        if (!show_nonce)
          nonce = a_to_u(optarg, 1, __UINT64_MAX__);
      break;
      case 'u':
        limit = optarg ? a_to_u(optarg, 1, 128) : 1;
        uuid(buf_ptr, limit, &seeds_reg, nonce);
        goto show_params;
      default:
        return err("Option is invalid or missing required argument");             
    }
  }

  adam(buf_ptr, &seeds_reg, nonce);

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
      printf("\e[1;36mSEED:\e[m %.15f\n", chseed);

    if (UNLIKELY(show_nonce))
      printf("\e[1;36mNONCE:\e[m %lu\n", nonce);

  return 0;
}