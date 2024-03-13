#ifndef TEST_H
#define TEST_H
  #include  "defs.h"
  #include  "ent.h"
  #include  "api.h"

  /*    NOTE: All critical values use an alpha level of 0.01    */

  #define   GAMMA_2DF                 1
  #define   GAMMA_4DF                 2
  #define   GAMMA_5DF                 1.329340388179137
  #define   GAMMA_8DF                 6
  #define   GAMMA_10DF                24
  #define   GAMMA_120DF               1.386831185456898E80

  // FOR:   Range distribution of output values
  #define   RANGE_CAT                 5
  #define   RANGE_CRITICAL_VALUE      13.277
  #define   RANGE_PROB1               2.328306436538696E-10
  #define   RANGE_PROB2               5.937181414550612E-8
  #define   RANGE_PROB3               0.000015199185323
  #define   RANGE_PROB4               0.003891050583657
  #define   RANGE_PROB5               0.996093690626375

  // FOR:   Monobit frequency (bit distribution)
  #define   MFREQ_CRITICAL_VALUE      6.635
  #define   MFREQ_PROB                0.5

  // FOR:   Chaotic seed distribution 
  #define   CHSEED_CAT                5
  #define   CHSEED_CRITICAL_VALUE     13.277
  #define   CHSEED_PROB               (1.0 / 5.0)

  // FOR:   Floating point distribution upon converting integer output accordingly
  #define   FPF_CAT                   10
  #define   FPF_CRITICAL_VALUE        21.666
  #define   FPF_PROB                  (1.0 / 10.0)

  // FOR:   Floating point permutations test
  #define   FP_PERM_SIZE              5
  #define   FP_PERM_CAT               120             // 5!
  #define   FP_PERM_CRITICAL_VALUE    157.800
  #define   FP_PERM_PROB              (1.0 / 120.0)

  // FOR:   Floating point max of 8 test
  #define   FP_MAX_CAT                8
  #define   FP_MAX_PROB               0.125
  #define   FP_MAX_CRITICAL_VALUE     18.475

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
    the inputs by 1 bit, so a copy of the adam_data struct that is passed to the test runner is assigned
    the same seed and nonce, but the nonce is incremented by 1. The resulting output between both is
    compared for the test, and as the original struct gets reseeded, its internal state is duplicated
    to the copy struct and the nonce is incremented again. So we get to conduct the SAC test for EACH
    iteration of the testing loop and collect all the results for the end!
    
    An interesting thing of note is that the binomial distribution with parameters B(0.5, n) will
    average out to n/2. So we use this knowledge to check how the distribution of Hamming distances
    we have recorded matches the given distribution.
    
    Additionally, the mean Hamming distance across all measurements, the distribution, and the
    standard deviation of the observed distances is also reported as well.

    Hernandez-Castro, Julio & Sierra, José & Seznec, Andre & Izquierdo, Antonio & Ribagorda, Arturo. (2005).
    The strict avalanche criterion randomness test. Mathematics and Computers in Simulation. 68. 1-7. 
    10.1016/j.matcom.2004.09.001.
  */ 
  #define   AVALANCHE_CAT             4
  #define   AVALANCHE_CRITICAL_VALUE  11.345

  /*
    FOR:   16-bit Topological Binary Test (TBT)

    The 16-bit TBT looks for the number of distinct 16-bit patterns per 2^16 patterns, where a "pattern"
    is a 16-bit block of bits. Since we provide 1024 such patterns per iteration (each iteration of
    ADAM produces 1024 u16), we count the number of distinct 16-bit patterns over 64 RNG iterations. If
    the number of distinct numbers found is greater than or equal to TBT_CRITICAL_VALUE, then we fail to 
    reject the null hypothesis.

    The pass rate, average number of distinct patterns, and overall proportion are reported in addition
    to the target metrics from the original paper. While the paper specifies the discrete probability 
    function allowing us to obtain a p-value, actually using the function for calculations proves very
    difficult due to the large values involved, especially the Stirling Numbers of the Second Kind. So
    only the collected results of each discrete outcome is reported.

    Alcover, Pedro & Guillamón, Antonio & Ruiz, M.D.C.. (2013). A New Randomness Test for Bit Sequences.
    Informatica (Netherlands). 24. 339-356. 10.15388/Informatica.2013.399.
  */ 
  #define   TBT_M                     16
  #define   TBT_SEQ_SIZE              (1UL << TBT_M)
  #define   TBT_CRITICAL_VALUE        41241
  #define   TBT_PROPORTION            0.629

  #define   SP_CAT                    5
  #define   SP_DIST                   50
  #define   SP_EXPECTED               51
  #define   SP_K                      4
  #define   SP_OBS_MIN                16
  #define   SP_OBS_MAX                64
  #define   SP_CRITICAL_VALUE         13.277
  #define   SP_PROB1                  0.193609
  #define   SP_PROB2                  0.179686
  #define   SP_PROB3                  0.196007
  #define   SP_PROB4                  0.195881
  #define   SP_PROB5                  0.234818

  typedef struct rng_test {
    u64 init_values[5];
    u64 sequences;
    u64 max;
    u64 min;
    u64 odd;
    u64 up_runs;
    u64 longest_up;
    u64 down_runs;
    u64 longest_down;
    double avg_gap; 
    u64 mfreq;
    u64 one_runs;
    u64 longest_one;
    u64 zero_runs;
    u64 longest_zero;
    double avg_fp;
    u64 perms;
    u16 *tbt_array;
    u64 tbt_pass;
    u64 tbt_prop;
    double avg_chseed;
    u64 expected_chseed;
    u64 fp_max_runs;
  } rng_test;

  void adam_examine(const u64 limit, adam_data data);
#endif