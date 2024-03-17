#ifndef TEST_H
#define TEST_H
  #include  "defs.h"
  #include  "ent.h"
  #include  "api.h"

  /*    NOTE: All critical values use an alpha level of 0.01    */
  #define   ALPHA_LEVEL               0.01

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

  // FOR:   Floating point max of T test (T = 8)
  #define   FP_MAX_CAT                8
  #define   FP_MAX_PROB               0.125
  #define   FP_MAX_CRITICAL_VALUE     18.475

  /*
    FOR:   4-bit Saturation Point Test

    The Saturation Point Test is similar to the Coupon Collector's Test, where it sees how long of a
    sequence is required to get all the numbers in a set (in our case, 2^4 values), and logs the index 
    where all such values appear (which is called the saturation point, or SP). It can be applied to
    a variable width of integers and analyze short sequences as opposed to requiring longer sequences 
    like some other tests here. The implementation differs a bit from what's described in the paper for 
    efficiency's sake, including p-value calculation as the author passes a value of 2 to the igamc()
    function used for calculating the p-value, where we can go one step further and directly pass 2.5
    since we have a version that accepts double inputs. However, the test still works as expected and 
    ADAM meets all the expected values, including hovering right around the expected SP (50 vs 51).

    We check all 4-bit quantities per iteration (so basically ADAM_BUF_BYTES * 2), record the SP value
    and then do a chi-square test for goodness of fit over the observed ranges of the SP values. The
    probabilities were directly provided by the author in the paper.

    Additionally, the raw chi-square, average SP, and overall distribution of values are reported.

    F. Sulak, “A New Statistical Randomness Test: Saturation Point Test”, IJISS, vol. 2, no. 3,
    pp. 81–85, 2013.
  */
  #define   SP_CAT                    5
  #define   SP_DIST                   50
  #define   SP_EXPECTED               51
  #define   SP_K                      4
  #define   SP_OBS_MIN                (1U << SP_K)
  #define   SP_OBS_MAX                64
  #define   SP_CRITICAL_VALUE         13.277
  #define   SP_PROB1                  0.193609
  #define   SP_PROB2                  0.179686
  #define   SP_PROB3                  0.196007
  #define   SP_PROB4                  0.195881
  #define   SP_PROB5                  0.234818

  /*
    FOR:   8-bit Maurer Universal Statistical Test

    The Maurer Universal Statistic is a great alternative/complement to other classic pseudorandom
    statistical tests like the frequency, runs, autocorrelation, and serial tests, but offers 2 very
    useful advantages over them. According to the paper:

    1.  Rather than being tailored to detecting a specific type of statistical defect, the test is able 
        to detect any one of the very general class of statistical defects that can be modeled by an
        ergodic stationary source with finite memory, which includes all those detected by the tests
        discussed in the previous section and can reasonably be argued to comprise the possible defects
        that could realistically occur in a practical implementation of a random bit generator.

    2.  The test measures the actual amount by which the security of a cipher system would be reduced if
        the tested generator G were used as the key source, i.e., it measures the effective key size of a
        cipher system with key source G. Therefore, statistical defects are weighted according to the
        potential damage they would cause in a cryptographic application. 

    The reason why I included this test is to provide more in-house tests for judging the cryptographic
    qualities of ADAM specifically, as opposed to some of the other tests which are just property reporting
    and traditional PRNG tests that don't necessarily deduce anything about security.

    We analyze each 1MB sequence per the requested examination size, collect each p-value from the subtests
    using Fisher's method, and then get a final p-value from the Fisher's method statistic at the very end
    when reporting all test results and printing them. This makes the implementation even stronger, allowing
    us to conduct a meta-analysis of all the subtests to comb through the output sequence.

    Maurer, U.M. A universal statistical test for random bit generators. J. Cryptology 5, 89–105 (1992).
    https://doi.org/10.1007/BF00193563
  */
  #define   MAURER_ARR_SIZE           1001472
  #define   MAURER_L                  8
  #define   MAURER_Q                  2048
  #define   MAURER_Y                  2.58
  #define   MAURER_EXPECTED           7.1836656 
  #define   MAURER_VARIANCE           3.2386622

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

  /*
    FOR:    32-bit Walsh-Hadamard Transform Test

    The Walsh-Hadamard Transform (WHT) is a concept I had trouble understanding for a while especially
    since I don't have a background in stats or math, and I wasn't really familiar with the Discrete
    Fourier Transform either. But it seems like the basic idea is this: the WHT is a way to map the output
    of one function to a binary domain of [-1, 1] through a set of "basis functions." This transform is
    used a lot in areas like compression, quantum computing, and for our purposes, pseudorandom generation
    and cryptography. All the details and inner workings of the WHT aren't necessary to understand the
    test, but this is the foundational concept.

    This test works on 32-bit blocks per output sequence, where each sequence is size ADAM_SEQ_SIZE. Each
    block is converted to binary form, and the 32 bits fed to the basis function:

      F(x) = 1 - 2x (where x is 0 or 1)

    This maps the sequence to another binary sequence where each value is -1 or 1, and all those values are
    used in a summation AFTER each term is multplied with -1^(i * j), where i is the ith 32-bit block (aka
    the index of the currently transformed 32-bit number) and j is the index of the currently transformed
    bit. The summation result for the ith block (aka its Walsh-Hadamard transform) is then squared and
    added to an accumulator, whose final value is used to get a p-value for this sequence's test.

    We combine all the p-values using Fisher's method just like above with the Maurer test, and report a 
    final p-value with the rest of the examination results.

    This test is used to detects a general class of randomness defects like frequency and autocorrelation
    failure. Another goal of this test is to answer to the question if the tested sequence is produced by
    some binary function. Additionally, it's used in testing several cryptographic criteria like correlation
    immunity, frequency balance, and strict avalanche. The Walsh-Hadamard is also used in a fast correlation
    attack for a general class of cryptosystems (a correlation attack assumes the existence of correlations
    between linear combinations of internal and output bits; fast correlation is based on precomputing data).

    Additonally the number of passing sequences and numbers and distribution of p-values are also reported.

  */
  #define WH_DF               512   
  #define WH_CRITICAL_VALUE   588.29779   
  #define WH_STD_DEV          5.6568542495   
  #define WH_UPPER_BOUND 	    2.5758
  #define WH_LOWER_BOUND 	    -2.5758  

  /*
    FOR:    64-bit Strict Avalanche Criterion (SAC) Test

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
    
    Additionally, the mean Hamming distance and observed distribution are reported as well.

    Hernandez-Castro, Julio & Sierra, José & Seznec, Andre & Izquierdo, Antonio & Ribagorda, Arturo. (2005).
    The strict avalanche criterion randomness test. Mathematics and Computers in Simulation. 68. 1-7. 
    10.1016/j.matcom.2004.09.001.
  */ 
  #define   AVALANCHE_CAT             4
  #define   AVALANCHE_CRITICAL_VALUE  11.345

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
    double avg_chseed;
    u64 fp_max_runs;
    double maurer_stat;
    u8 *maurer_bytes;
    u64 maurer_pass;
    double maurer_mean;
    double maurer_c;
    double maurer_std_dev;
    double maurer_fisher;
  } rng_test;

  // https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
  #define MIN(a, b) (b ^ ((a ^ b) & -(a < b)))
  #define MAX(a, b) (a ^ ((a ^ b) & -(a < b)))

  void adam_examine(const u64 limit, adam_data data);
  double cephes_igamc(double a, double x);
#endif