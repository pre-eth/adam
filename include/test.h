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

  // FOR:   floating point distribution upon converting integer output accordingly
  #define   FPF_CAT                   10
  #define   FPF_CRITICAL_VALUE        21.666
  #define   FPF_PROB                  (1.0 / 10.0)

  // FOR:   floating point permutations test
  #define   FP_PERM_SIZE              5
  #define   FP_PERM_CAT               120             // 5!
  #define   FP_PERM_CRITICAL_VALUE    157.800
  #define   FP_PERM_PROB              (1.0 / 120.0)

  // FOR:   Topological Binary Test (Alcover, Pedro & Guillamón, Antonio & Ruiz, M.D.C.. (2013). A New Randomness Test for Bit Sequences. Informatica (Netherlands). 24. 339-356. 10.15388/Informatica.2013.399.)
  #define   TBT_PROPORTION            0.629
  #define   TBT_M                     16
  #define   TBT_CRITICAL_VALUE        41241
  #define   TBT_SEQ_SIZE              65536
  #define   TBT_BITARRAY_SIZE         1024

  // FOR:   Strict Avalanche Criterion (SAC) Test (Hernandez-Castro, Julio & Sierra, José & Seznec, Andre & Izquierdo, Antonio & Ribagorda, Arturo. (2005). The strict avalanche criterion randomness test. Mathematics and Computers in Simulation. 68. 1-7. 10.1016/j.matcom.2004.09.001.)
  // NOTE:  64 different probabilities needed here so instead of polluting the header, see the function print_avalanche_results() in support.c
  #define   AVALANCHE_CAT             64
  #define   AVALANCHE_CRITICAL_VALUE  93.2168


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