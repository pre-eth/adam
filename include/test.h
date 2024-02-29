#ifndef TEST_H
#define TEST_H
  #include  "defs.h"

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

  /*
    FOR:    Strict Avalanche Criterion (SAC) Test

    64 different probabilities needed here so instead of polluting the header, see the function
    print_avalanche_results() in support.c.

    These probabilities were calculated from the binomial distribution using probability 0.5 as per
    the paper. We have 64 degrees of freedom for the 64 bits in each value. If at least 32 bits have
    not changed, then the test fails for that run. So we do this for the entire stream size: tally the
    Hamming distance between u64 numbers in the same buffer position per run, compute the expected
    count per bin for the size of this stream, then do a chi-square test for goodness of fit with the
    binomial distribution. The avalanche effect measures the difference in output when you change
    the inputs by 1 bit. We increment the seed internally per iteration so we are never changing
    the input more than 1 bit, thus we can naturally perform the strict avalanche criterion (SAC)
    test while examining a bit sequence!
    
    An interesting thing of note is that the binomial distribution with parameters B(0.5, n) will
    average out to n/2. So we use this knowledge to check how the distribution of Hamming distances
    we have recorded matches the given distribution.
    
    Additionally, the mean Hamming distance across all measurements, the distribution, and the
    standard deviation of the observed distances is also reported as well.

    Hernandez-Castro, Julio & Sierra, José & Seznec, Andre & Izquierdo, Antonio & Ribagorda, Arturo. (2005).
    The strict avalanche criterion randomness test. Mathematics and Computers in Simulation. 68. 1-7. 
    10.1016/j.matcom.2004.09.001.
  */ 
  #define   AVALANCHE_CAT             64
  #define   AVALANCHE_CRITICAL_VALUE  93.2168


  typedef struct rng_test {
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
    u64 *restrict chseed_dist;
    double avg_gap;
    u64 *restrict range_dist; 
    u8 *restrict mcb; 
    u8 *restrict lcb; 
    u64 *restrict fpf_dist; 
    u64 *restrict fpf_quad;
    double avg_fp;
    u64 *restrict ham_dist;
    u64 perms;
    u64 *restrict perm_dist;
    u32 tbt_m;
  } rng_test;

  void adam_examine(const u64 limit, rng_test *rsl, unsigned long long *seed, unsigned long long nonce);
#endif