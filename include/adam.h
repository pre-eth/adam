#ifndef ADAM_H
#define ADAM_H
  #include "defs.h"

  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u64; 256] with size 256 * 8 = 6144 bytes;
    Need to left shift by 6 because there are 64 bits per index
  */
  #define MAGNITUDE           8  
  #define BUF_SIZE            (1U << MAGNITUDE)     
  #define SEQ_SIZE            (BUF_SIZE << 6)  

  /* ISAAC64 stuff */

  #define GOLDEN_RATIO        0x9E3779B97F4A7C13UL

  #define ISAAC_MIX(a, b, c, d, e, f, g, h) { \
    a-=e; f^=h>>9;  h+=a; \
    b-=f; g^=a<<9;  a+=b; \
    c-=g; h^=b>>23; b+=c; \
    d-=h; a^=c<<15; c+=d; \
    e-=a; b^=d>>14; d+=e; \
    f-=b; c^=e<<20; e+=f; \
    g-=c; d^=f>>17; f+=g; \
    h-=d; e^=g<<14; g+=h; \
  }

  // Slightly modified versions of macros from ISAAC for reseeding ADAM
  #define ISAAC_IND(mm, x)    (*(u64*)((u8*)(mm) + ((x) & ((BUF_SIZE-1)<<3))))
  #define ISAAC_RNGSTEP(mx, a, b, mm, m, m2, x, y) { \
    x = (m << 24) | ((~(m >> 40) ^ __UINT64_MAX__)  & 0xFFFFFF);  \
    a = (a^(mx)) + m2; \
    m = ~(ISAAC_IND(mm,x) + a + b); \
    y ^= b = ISAAC_IND(mm,y>>MAGNITUDE) + x; \
  }

  /* ADAM stuff */

  /*
    The PRNG algorithm is based on the construction of three 
    chaotic maps obtained by permuting and shuffling the elements
    of an initial input vector. The permutations are performed 
    using a chaotic function to scramble the used positions. The 
    chaotic function is given by this logistic function:

      3.9999 * (x) * (1 - (x))
  */
  #define COEFFICIENT           3.9999      
  
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
    above as well as the following formula used to calculate k:
      floor(256 / log2(5 x pow(10, c - 1) - 1)) + 1

    The rounds value is supposed to be generated from taking this 
    function 3 times and computing the max, in this case 6
  */
  #define ROUNDS                6
  #define ITER                  (ROUNDS / 3)

  /* 
    BETA is derived from the length of the significant digits
    ADAM uses the max double accuracy length of 15
  */
  #define BETA                  10E15 

  // Data for RNG process
  typedef struct rng_data {
    u64 *buffer;                //  Where we store the results
    u64 seed[4];                //  256-bit seed/key
    u64 nonce;                  //  64-bit nonce
    u64 aa;                     //  State variable 1
    u64 bb;                     //  State variable 2
    double *restrict chseeds;   //  Where we store seeds for each round of chaotic function
  } ALIGN(SIMD_LEN) rng_data;

  void adam_init(rng_data *data);
  void adam(rng_data *data);
#endif
