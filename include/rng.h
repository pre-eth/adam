#ifndef RNG_H
#define RNG_H
  #include "../include/defs.h"
  #include "../include/simd.h"

  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u64; 256] with size 256 * 8 = 2048 bytes

    Need to left shift for SEQ_SIZE because there's 64 bits per index
  */
  #define MAGNITUDE           8  
  #define BUF_SIZE            (1U << MAGNITUDE)   
  
  /* 
    ROUNDS must satisfy k = T / 3 where T % 3 = 0, where k is 
    the iterations per each chaotic map. 

    To avoid brute force attacks, ROUNDS must also satisfy the 
    condition: pow([5 x pow(10, c - 1) - 1], ROUNDS) > pow(2, 256),
    where c is the number of digits in the seed.

    Note: The paper says that "In any cryptosystem, for poor keys 
    or limited key space K, the cryptosystem can be easily broken. 
    
    Indeed, given today’s computer speed, it is generally accepted 
    that a key space of size smaller than 2^128."

    However, the paper was written in 2014, and computing power has
    increased even more since then, so instead of 128, 256 is used 
    above as well as the following formula used to calculate K:
      floor(256 / log2(5 x pow(10, c - 1) - 1)) + 1

    The rounds value is supposed to be generated from taking this 
    function 3 times and computing the max, in this case set to 8
    because of the compressed nature of the implementation compared
    to the original implementation
  */
  #define ROUNDS                6

  /*
    The CSPRNG algorithm is based on the construction of three 
    chaotic maps obtained by supplying a set of chaotic seeds and
    partitioning the elements of an initial input vector, with the 
    mixing of all elements in the vector performed between chaotic 
    iterations to produce the final output sequence in place. The 
    chaotic function is given as:

      3.9999 * X * (1 - X)
  */
  #define COEFFICIENT           3.9999      

  /* 
    BETA is derived from the number of digits we'd like to pull from the mantissa
    before converting a chaotic seed output to an integer value
  */
  #define BETA                  10E15

  // To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
  #define RANGE_LIMIT           2.7105054E-20

  /*  The main primitives for using ADAM's algorithm  */

  void accumulate(u64 *restrict out, u64 *work_arr, double *restrict chseeds);
  void diffuse(u64 *restrict out, u64 *restrict mix, const u64 nonce);
  void apply(u64 *restrict out, u64 *restrict state_maps, double *restrict chseeds, u64 *restrict arr);
  void mix(u64 *restrict out, const u64 *restrict state_maps);

#if !defined(__AARCH64_SIMD__) && !defined(__AVX512F__)
  regd mm256_cvtpd_epi64(reg r1);

  #define SIMD_CVTPD  mm256_cvtpd_epi64
#endif
#endif