#include "adam.h"

/*
  To avoid expensive modulus for all the required chaotic iterations, this
  function implements the modular reduction algorithm described in:

 "Computing Mod with a Variable Lookup Table" by Mark A. Will & Ryan K. L. Ko 
  from the Cyber Security Lab at The University of Waikato, New Zealand

  FORCE_INLINE static mod_reduction(u32 integer, u16 mod) {
    const u8 mod_width = 32 - CLZ(mod);
    const u8 int_width = 32 - CLZ(integer);
    const u8 num = (u8) (int_width / mod_width);
    
    u8 n = num - 1;
    while (n > 0) {

    }
  }
*/ 

FORCE_INLINE static double chaotic_iter(u32* restrict _ptr, double seed, u8 k, u8 factor) {
  /* 
    According to the paper, the variable c is derived from: 
      floor(log10(M)) + 3
    Here M is the amount of bits in the buffer, which for ADAM is 
    8192, and log10(8192) = 3.9, so c = floor(3.9) + 3 = 6. Then, 
      BETA = pow(10, 6)
  */
  #define BETA          10E15

  double x = seed;
  u16 s = SEQ_SIZE - 1;
  u16 i, j;
  i = j = 0;

  // u16 m = (u16) 1 << factor;

  do {
    x = CHAOTIC_FN(x);
    j = i + 1 + ((u64) FLOOR(x * BETA) % s);
    --s;
    _ptr[i >> 3] |= ((_ptr[i >> 3] >> ((i & 7) + factor) & 1UL)) ^ ((_ptr[j >> 3] >> ((i & 7) + factor) & 1UL)) << ((i & 7) + k);
  } while (++i < SEQ_SIZE - 2);

  return x;
}

FORCE_INLINE static void accumulate(u32* restrict _ptr, u64 seed) {
  u8 i = 0, j = 4;
  
  do {
    ACCUMULATE(((seed + i) >> 56),  (i + 0)),
    ACCUMULATE(((seed + i) >> 48),  (i + 16)),
    ACCUMULATE(((seed + i) >> 40),  (i + 32)),
    ACCUMULATE(((seed + i) >> 32),  (i + 48)),
    ACCUMULATE(((seed + i) >> 24),  (i + 64)),
    ACCUMULATE(((seed + i) >> 16),  (i + 80)),
    ACCUMULATE(((seed + i) >> 8),   (i + 96)),
    ACCUMULATE(((seed + i) & 0xFF), (i + 112));
  } while (++i < 4);

  do {
    ACCUMULATE(((seed + j) >> 56),  (j + 0)),
    ACCUMULATE(((seed + j) >> 48),  (j + 16)),
    ACCUMULATE(((seed + j) >> 40),  (j + 32)),
    ACCUMULATE(((seed + j) >> 32),  (j + 48)),
    ACCUMULATE(((seed + j) >> 24),  (j + 64)),
    ACCUMULATE(((seed + j) >> 16),  (j + 80)),
    ACCUMULATE(((seed + j) >> 8),   (j + 96)),
    ACCUMULATE(((seed + j) & 0xFF), (j + 112));
  } while (++j < 8);
}

FORCE_INLINE static void diffuse(u32* restrict _ptr, double* chseed, u8 iter) {
  u16 i = 0, j = 1024;

  do {
    ISAAC_MIX(0  + i),
    ISAAC_MIX(8  + i),
    ISAAC_MIX(16 + i),
    ISAAC_MIX(24 + i),
    ISAAC_MIX(32 + i),
    ISAAC_MIX(40 + i),
    ISAAC_MIX(48 + i),
    ISAAC_MIX(56 + i);
    i += 64;
  } while (i < (BUF_SIZE >> 1));

  do {
    ISAAC_MIX(0  + j),
    ISAAC_MIX(8  + j),
    ISAAC_MIX(16 + j),
    ISAAC_MIX(24 + j),
    ISAAC_MIX(32 + j),
    ISAAC_MIX(40 + j),
    ISAAC_MIX(48 + j),
    ISAAC_MIX(56 + j);
    j += 64;
  } while (j < BUF_SIZE);

  double x = *chseed;

  do {
    x = chaotic_iter(_ptr, x, 0, 0);
    x = (*chseed + iter);
  } while (--iter > 0);

  *chseed = x;
}

FORCE_INLINE void apply(u32* restrict _ptr, double chseed, u8 iter) {
  u8 i = iter;

  double x = chaotic_iter(_ptr, chseed, 8, 0);

  // Need to add 4 from now on bc diffuse() uses chseed + 1, chseed + 2, and chseed + 3
  x = chseed + 4.0;
  --i;

  do {
    x = chaotic_iter(_ptr, x, 8, 8);
    x = chseed + 4.0 + i;
  } while (--i > 0);

  // Last iteration T

  i = iter;

  x = chaotic_iter(_ptr, x, 16, 8);
  x = chseed + 8.0;
  --i;

  do {
    x = chaotic_iter(_ptr, x, 16, 16);
    x = chseed + 8.0 + i;
  } while (--i > 0);
}

FORCE_INLINE void mix(u32* restrict _ptr) {
  u16 i = 0, j = 512;

  reg tmp1, tmp2, tmp3, tmp4;
  do {
    tmp1 = SIMD_SETR8(
      SIMD_MASK_PREP(0, i),
      SIMD_MASK_PREP(16, i)
    );

    tmp2 = SIMD_SETR8(
      SIMD_XOR_PREP(0, i),
      SIMD_XOR_PREP(16, i)
    );

    tmp1 = SIMD_XORBITS(tmp1, tmp2);
    tmp2 = SIMD_LOADBITS((reg*) &_ptr[i]);
    tmp2 = SIMD_ORBITS(tmp2, tmp1);

    SIMD_STOREBITS((reg*) &_ptr[i], tmp2);
  } while ((i += SIMD_INC) < (BUF_SIZE >> 1));

  do {
    tmp1 = SIMD_SETR8(
      SIMD_MASK_PREP(0, j),
      SIMD_MASK_PREP(16, j)
    );
    
    tmp2 = SIMD_SETR8(
      SIMD_XOR_PREP(0, j),
      SIMD_XOR_PREP(16, j)
    );

    tmp1 = SIMD_XORBITS(tmp1, tmp2);
    tmp2 = SIMD_LOADBITS((reg*) &_ptr[j]);
    tmp2 = SIMD_ORBITS(tmp2, tmp1);

    SIMD_STOREBITS((reg*) &_ptr[j], tmp2);
  } while ((j += SIMD_INC) < BUF_SIZE);
}

void generate(u32* restrict _ptr) { 
  const u8 iter = ROUNDS / 3;

  u8 res;
  u64 seed;
  while (!(res = SEED64(&seed))); 
  seed ^= (seed ^ (GOLDEN_RATIO ^ (seed >> 32)));

  accumulate(_ptr, seed);

  while (!(res = SEED64(&seed))); 
  double x = ((double) (seed / __UINT64_MAX__)) * 0.5;
  
  diffuse(_ptr, &x, iter);
  apply(_ptr, x, iter);
  mix(_ptr);
}

