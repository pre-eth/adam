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
  #define CHAOTIC_FN(x)   (3.9999F * x * (1 - x))
  
  #define MAGNITUDE     10  
  
  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u32; 1024], but logically it is still
    [u8; 1024] - the values for the other chaotic maps are stored 
    in the different 8-bit slices of each u32 value
  */
  #define BUF_SIZE      (1UL << MAGNITUDE)     
  #define SEQ_SIZE      (BUF_SIZE << 3)

  // All seeds must be be at least 7 digits
  #define DEFAULT_SEED   0.3456789F
  
  // ROUNDS must satsify k = T / 3 where T % 3 = 0. 
  // k is the iterations per chaotic map. ADAM allows
  // any ROUNDS value r such that 6 <= r <= 24
  #define ROUNDS        9

  // Per Bob's original implementation
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

  // Returns seed at the end of iteration so it can be used to start another iteration
  FORCE_INLINE static float chaotic_iter(u32* restrict _ptr, float seed, u8 k);

  /* Internal functions for the ADAM RNG */

  // Produces initial vector
  FORCE_INLINE static void accumulate(u32* restrict _ptr, u64 seed);

  // Performs ISAAC mix logic on contents and creates the first chaotic map
  FORCE_INLINE void diffuse(u32* restrict _ptr, float seed, u8 iter);

  // Applies (ROUNDS / 3) iterations twice to create the next two chaotic maps
  FORCE_INLINE void apply(u32* restrict _ptr, float seed, u8 iter);

  // XOR's the three chaotic maps to create final output vector
  FORCE_INLINE void mix(u32* restrict _ptr);

  /* User facing functions for the ADAM CLI */

  // Initiate RNG algorithm - returns the seed for the generated buffer
  FORCE_INLINE u64 generate(u32* restrict _ptr, float seed, u8 rounds);

  // Initiate RNG algorithm with user provided uint64_t seed
  FORCE_INLINE void sgenerate(u64 seed, u32* restrict _ptr, float chseed, u8 rounds);
#endif