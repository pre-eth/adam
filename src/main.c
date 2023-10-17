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

FORCE_INLINE static void rng_init(rng_data *data) {
  getentropy(&data->seed[0], sizeof(u64) << 2); 
  data->nonce = ((u64) time(NULL)) ^ GOLDEN_RATIO;
  data->aa = data->bb = 0UL;
  data->reseed = FALSE;
  data->durations[0] = data->durations[1] = data->durations[2] = data->durations[3] = 0.0;
}

int main(int argc, char **argv) {
  const char *fmt = "%lu";

  register u8 precision  = 64,
              idx        = 0,
              show_seed  = FALSE,
              show_nonce = FALSE;

  register u16 results = 1, 
               limit   = 0;

  register u64 mask = __UINT64_MAX__ - 1;

  rng_data data;
  rng_init(&data);

  register short opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
        return puts(VERSION);
      case 'l':
        data.reseed = TRUE;
        return infinite(&data);
      case 'a':
        limit = a_to_u(optarg, 1, ASSESS_LIMIT);
        if (!limit)
          return err("Multiplier must be within range [1, 8000]");
        data.reseed = TRUE;
        assess(limit, &data);
        goto show_params;
      case 'b':
        data.reseed = TRUE;
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
        if (!show_seed) {
          FILE *seed_file = fopen(optarg, "rb");
          if (!seed_file)
            return err("Couldn't read seed file");
          fread(data.seed, sizeof(u64), 4, seed_file);
          fclose(seed_file);
        }
          
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
  u64 *buf_ptr = data.buffer;

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
    if (UNLIKELY(show_seed)) {
      FILE *seed_file = fopen("seed.adam", "wb+");
      if (!seed_file)
        return err("Could not create file for writing seed.");
      fwrite(data.seed, sizeof(u64), 4, seed_file);
      fclose(seed_file);
      printf("\033[1;36mSEED WRITTEN TO: \"seed.adam\"\033[m");
    }
      

    if (UNLIKELY(show_nonce))
      printf("\033[1;36mNONCE:\033[m %llu\n", data.nonce);

  return 0;
}