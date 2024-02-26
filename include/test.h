#ifndef TEST_H
#define TEST_H
  #include  "defs.h"
  #include  "ent.h"

	#define   TESTING_BITS    	        8000000ULL
	#define   TESTING_DBL    		        1000ULL
	#define   BITS_TESTING_LIMIT        100000
	#define   DBL_TESTING_LIMIT         1500000

  /*    NOTE: All critical values use an alpha level of 0.01    */

  // FOR:   monobit frequency (bit distribution)
  #define   MFREQ_CRITICAL_VALUE      6.635
  #define   MFREQ_PROB                0.5

  // FOR:   chaotic seed distribution 
  #define   CHSEED_CAT                5
  #define   CHSEED_CRITICAL_VALUE     13.277
  #define   CHSEED_PROB               (1.0 / 5.0)

  // FOR:   range distribution of output values
  #define   RANGE_CAT                 5
  #define   RANGE_CRITICAL_VALUE      13.277
  #define   RANGE1_PROB               2.328306436538696E-10
  #define   RANGE2_PROB               5.937181414550612E-8
  #define   RANGE3_PROB               0.000015199185323
  #define   RANGE4_PROB               0.003891050583657
  #define   RANGE5_PROB               0.996093690626375

  // FOR:   floating point distribution when converting output values
  #define   FPF_CAT                   10
  #define   FPF_CRITICAL_VALUE        21.666
  #define   FPF_PROB                  (1.0 / 10.0)


  typedef struct rng_test {
    u64 *buffer;
    double *chseeds;
    u64 seed[4];
    u64 nonce;
    ent_report *ent;
    u64 sequences;
    u64 mfreq;
    u64 max;
    u64 min;
    u64 odd;
    u64 up_runs;
    u64 longest_up;
    u64 down_runs;
    u64 longest_down;
    u64 one_runs;
    u64 longest_one;
    u64 zero_runs;
    u64 longest_zero;
    double avg_chseed;
    u64 expected_chseed;
    u64 *chseed_dist;
    double avg_gap;
    u64 *range_dist; 
    u8 *mcb; 
    u8 *lcb; 
    u64 *fpf_dist; 
    u64 *fpf_quad;
    double avg_fp;
    double hamming_dist;
  } rng_test;

  void adam_test(const u64 limit, rng_test *rsl);
#endif