#ifndef TEST_H
#define TEST_H
  #include "defs.h"
  #include "ent.h"
                         
  typedef struct rng_data rng_data;

  typedef struct rng_test {
    rng_data *data;
    ent_report *ent;
    u64 init_values[7];
    u32 mfreq;
    u64 max;
    u64 min;
    u32 odd;
    u32 zeroes;
    u32 up_runs;
    u32 longest_up;
    u32 down_runs;
    u32 longest_down;
    u64 *poker_dist;
    double avg_chseed;
    u64 *chseed_dist;
    double avg_gap;
    u64 *permutations;
  } rng_test;

  void adam_test(const u64 limit, rng_test *rsl);
#endif