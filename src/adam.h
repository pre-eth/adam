#ifndef ADAM_H
#define ADAM_H

  #include "util.h"

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

  /* ADAM stuff */

  #define SEED64              _rdseed64_step
  
  #define THREAD_EXP          3
  #define THSEED(i, sd)       chdata[i].seed = *sd, *sd += (*sd / 100000000)

  /*
    The PRNG algorithm is based on the construction of three 
    chaotic maps obtained by permuting and shuffling the elements
    of an initial input vector. The permutations are performed 
    using a chaotic function to scramble the used positions. The 
    chaotic function is given by this logistic function
  */
  #define CHAOTIC_FN(x)       (3.9999 * (x) * (1 - (x)))
  
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

    ADAM uses a strict ROUNDS value of 18 because it always uses the 
    maximum double precision value of 15 with the seeds that it 
    generates. I'll let you check the math yourself to prove it :)
  */
  #define ROUNDS              18
  #define ITER                (ROUNDS / 3)

  #define RESEED_ADAM(s)      seed ^= (seed + ((seed << (seed & 31)))) ^ _ptr[seed & 0xFF]

  #define XOR_MAPS(i)         _ptr[0 + i] ^ (_ptr[0 + i + 256]) ^ (_ptr[0 + i + 512]),\
                              _ptr[1 + i] ^ (_ptr[1 + i + 256]) ^ (_ptr[1 + i + 512]),\
                              _ptr[2 + i] ^ (_ptr[2 + i + 256]) ^ (_ptr[2 + i + 512]),\
                              _ptr[3 + i] ^ (_ptr[3 + i + 256]) ^ (_ptr[3 + i + 512])

  // Initiates RNG algorithm with user provided seed and nonce
  // Returns duration of generation
  double adam(u64 *restrict _ptr, const u64 seed, const u64 nonce);

#endif