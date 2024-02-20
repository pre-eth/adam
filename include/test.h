#ifndef TEST_H
#define TEST_H
  #include  "defs.h"
  #include  "ent.h"

	#define   ASSESS_UNIT    	        1000000ULL
	#define   EXAMINE_UNIT    	      1000000ULL
	#define   TESTING_DBL    		      1000ULL
	#define   BITS_TESTING_LIMIT      8000
	#define   DBL_TESTING_LIMIT       1500000

  #define   CHSEED_CRITICAL_VALUE   13.277

  #define   RANGE_CRITICAL_VALUE    13.277
  #define   RANGE1_PROB             2.328306436538696E-10
  #define   RANGE2_PROB             5.937181414550612E-8
  #define   RANGE3_PROB             0.000015199185323
  #define   RANGE4_PROB             0.003891050583657
  #define   RANGE5_PROB             0.996093690626375

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
    u64 *range_dist; 
  } rng_test;

  void adam_test(const u64 limit, rng_test *rsl);
#endif