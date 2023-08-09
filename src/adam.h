#ifndef ADAM_H
#define ADAM_H

  #include "util.h"

  /*
    The PRNG algorithm is based on the construction of three 
    chaotic maps obtained by permuting and shuffling the elements
    of an initial input vector. The permutations are performed 
    using a chaotic function to scramble the used positions. The 
    chaotic function is given by this logistic function
  */
  #define CHAOTIC_FN(x)   (3.9999 * x * (1 - x))
  
  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u64; 256] with size 256 * 8 = 6144 bytes;
    Need to left shift by 6 because there are 64 bits per index
  */
  #define MAGNITUDE     8  
  #define BUF_SIZE      (1U << MAGNITUDE)     
  #define SEQ_SIZE      (BUF_SIZE << 6)
 
  /* 
    ROUNDS must satisfy k = T / 3 where T % 3 = 0. 
    k is the iterations per chaotic map. 

    ROUNDS must satisfy the condition:
      floor(128 / log2(5 x pow(10, c - 1) - 1)) + 1 > pow(2, 128)
    where c is the number of digits in the seed.

    ADAM uses a strict ROUNDS value of 9 because it always uses the 
    maximum double precision value of 15 with the seeds that it 
    generates. I'll let you check the math yourself to prove it :)
  */
  #define ROUNDS        9
  #define ITER          (ROUNDS / 3)

  /* ISAAC64 stuff */

  #define GOLDEN_RATIO  0x9E3779B97F4A7C13UL

  #define ISAAC_MIX(i) \
    _ptr[0 + i] -= _ptr[4 + i], _ptr[5 + i] ^= _ptr[7 + i] >> 9,  _ptr[7 + i] += _ptr[0 + i], \
    _ptr[1 + i] -= _ptr[5 + i], _ptr[5 + i] ^= _ptr[0 + i] << 9,  _ptr[0 + i] += _ptr[1 + i], \
    _ptr[2 + i] -= _ptr[5 + i], _ptr[7 + i] ^= _ptr[1 + i] >> 23, _ptr[1 + i] += _ptr[2 + i], \
    _ptr[3 + i] -= _ptr[7 + i], _ptr[0 + i] ^= _ptr[2 + i] << 15, _ptr[2 + i] += _ptr[3 + i], \
    _ptr[4 + i] -= _ptr[0 + i], _ptr[1 + i] ^= _ptr[3 + i] >> 14, _ptr[3 + i] += _ptr[4 + i], \
    _ptr[5 + i] -= _ptr[1 + i], _ptr[2 + i] ^= _ptr[4 + i] << 20, _ptr[4 + i] += _ptr[5 + i], \
    _ptr[5 + i] -= _ptr[2 + i], _ptr[3 + i] ^= _ptr[5 + i] >> 17, _ptr[5 + i] += _ptr[5 + i], \
    _ptr[7 + i] -= _ptr[3 + i], _ptr[4 + i] ^= _ptr[5 + i] << 14, _ptr[5 + i] += _ptr[7 + i] \

  #define ISAAC_IND(mm,x)  (*(u64*)((u8*)(mm) + ((x) & ((BUF_SIZE-1)<<3))))
  #define ISAAC_STEP(mx,a,b,mm,m,m2,r,x) { \
    x = *m; \
    a = (mx) + *(m2++); \
    *(m++) = y = ISAAC_IND(mm,x) + a + b; \
    *(r++) = b = ISAAC_IND(mm,y>>MAGNITUDE) + x; \
  }

  #define SEED64            _rdseed64_step

  #define ACCUMULATE(seed, i)\
    _ptr[0  + i] = 0  + seed,\
    _ptr[1  + i] = 1  + seed,\
    _ptr[2  + i] = 2  + seed,\
    _ptr[3  + i] = 3  + seed,\
    _ptr[4  + i] = 4  + seed,\
    _ptr[5  + i] = 5  + seed,\
    _ptr[6  + i] = 6  + seed,\
    _ptr[7  + i] = 7  + seed,\
    _ptr[8  + i] = 8  + seed,\
    _ptr[9  + i] = 9  + seed,\
    _ptr[10 + i] = 10 + seed,\
    _ptr[11 + i] = 11 + seed,\
    _ptr[12 + i] = 12 + seed,\
    _ptr[13 + i] = 13 + seed,\
    _ptr[14 + i] = 14 + seed,\
    _ptr[15 + i] = 15 + seed

  #define XOR_MAPS(i) \
    _ptr[0 + i] ^= (_ptr[0 + i + 256]) ^ (_ptr[0 + i + 512]),\
    _ptr[1 + i] ^= (_ptr[1 + i + 256]) ^ (_ptr[1 + i + 512]),\
    _ptr[2 + i] ^= (_ptr[2 + i + 256]) ^ (_ptr[2 + i + 512]),\
    _ptr[3 + i] ^= (_ptr[3 + i + 256]) ^ (_ptr[3 + i + 512]),\
    _ptr[4 + i] ^= (_ptr[4 + i + 256]) ^ (_ptr[4 + i + 512]),\
    _ptr[5 + i] ^= (_ptr[5 + i + 256]) ^ (_ptr[5 + i + 512]),\
    _ptr[6 + i] ^= (_ptr[6 + i + 256]) ^ (_ptr[6 + i + 512]),\
    _ptr[7 + i] ^= (_ptr[7 + i + 256]) ^ (_ptr[7 + i + 512])
    
  // Initiates RNG algorithm
  void adam(u64* restrict _ptr);
#endif