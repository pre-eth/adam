#ifndef TEST_H
#define TEST_H
  #include "defs.h"
  #include "ent.h"

  typedef struct rng_test {
    u64 *buffer;
    double *chseeds;
    u64 seed[4];
    u64 nonce;
    ent_report *ent;
    u32 sequences;
    u32 mfreq;
    u64 max;
    u64 min;
    u32 odd;
    u32 zeroes;
    u32 up_runs;
    u32 longest_up;
    u32 down_runs;
    u32 longest_down;
    double avg_chseed;
    u64 expected_chseed;
    u64 *chseed_dist;
    double avg_gap;
    u64 freq_min;
    u64 freq_max;
  } rng_test;

  void adam_test(const u64 limit, rng_test *rsl);
#endif