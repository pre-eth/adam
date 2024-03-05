#ifndef TEST_H
#define TEST_H
  #include  "defs.h"
  #include  "ent.h"
  #include  "api.h"

  /*    NOTE: All critical values use an alpha level of 0.01    */

  // FOR:   range distribution of output values
  #define   RANGE_CAT                 5
  #define   RANGE_CRITICAL_VALUE      13.277
  #define   RANGE1_PROB               2.328306436538696E-10
  #define   RANGE2_PROB               5.937181414550612E-8
  #define   RANGE3_PROB               0.000015199185323
  #define   RANGE4_PROB               0.003891050583657
  #define   RANGE5_PROB               0.996093690626375

  // FOR:   monobit frequency (bit distribution)
  #define   MFREQ_CRITICAL_VALUE      6.635
  #define   MFREQ_PROB                0.5

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

  // FOR:   chaotic seed distribution 
  #define   CHSEED_CAT                5
  #define   CHSEED_CRITICAL_VALUE     13.277
  #define   CHSEED_PROB               (1.0 / 5.0)

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
    FOR:   10-bit and 16-bit Topological Binary Test (TBT)

    The 10-bit TBT is done on sequences that are less than 100MB. This is because the chi-square test 
    cannot be applied effectively, due to having less than 5 items per bin. But otherwise, the 16-bit 
    test is performed on larger sequences that DO meet this criteria.

    We once again use the binomial distribution to compute the number of successes in a run of 10 (just an
    arbitrary choice but it can likely be experimented with), as the topological binary test is another
    Bernoulli trial: if the number of different 10-bit/16-bit patterns is less than the respective 
    critical value, then the sequence is not sufficiently random. The paper DOES have an exact distribution
    function, but having to calculate the Stirling numbers for large values of k (since the -e option
    allows for testing up to 100GB) is unfeasible. So I adapted the approach here a bit :P 

    The probabilities calculated for k successes in a run of 10 have been put in print_tbt_results() in
    support.c, just like what was done for the SAC test above

    Additionally, we take the critical value in each case as the expected value and model the observed
    numbers of different patterns as a Poisson distribution. We use the calculated 10-bit and 16-bit 
    probabilities to check if the range of recorded successes is as expected.

    Alcover, Pedro & Guillamón, Antonio & Ruiz, M.D.C.. (2013). A New Randomness Test for Bit Sequences.
    Informatica (Netherlands). 24. 339-356. 10.15388/Informatica.2013.399.
  */ 
  #define   TBT_CAT                   10
  #define   TBT_M1                    10
  #define   TBT_SEQ_SIZE              (1U << TBT_M1)
  #define   TBT_CRITICAL_VALUE        624
  #define   TBT_PROPORTION            0.609
  #define   TBT_M2                    16
  #define   TBT_SEQ_SIZE16            (1UL << TBT_M2)
  #define   TBT_CRITICAL_VALUE16      41241
  #define   TBT_PROPORTION16          0.629
  #define   TBT_BITARRAY_SIZE         1024

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
    void (*tbt)(u16 *);
    u64 tbt_pass;
    u64 tbt_prop;
    double avg_chseed;
    u64 expected_chseed;
  } rng_test;

  void adam_examine(const u64 limit, adam_data data);
#endif