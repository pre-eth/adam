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

FORCE_INLINE static double chaotic_iter(u64* map_b, u64* map_a, const double seed) {
  /* 
    BETA is derived from the length of the mantissa 
    ADAM uses the max length of 15 to minimize ROUNDS
  */
  #define BETA          10E15

  register double x = seed;
  register u16 s = SEQ_SIZE - 1;
  register u16 i, j;
  i = j = 0;

  do {
    x = CHAOTIC_FN(x);
    j = i + 1 + ((u64) FLOOR(x * BETA) % s);
    --s;
    map_b[i] |= (((map_a[i] >> i) & 1UL)) ^ (((map_b[j] >> i) & 1UL)) << (i & 63);
  } while (++i < SEQ_SIZE - 2);

  return x;
}

FORCE_INLINE static void accumulate(u64* restrict _ptr, u64 seed) {
  register u8 i = 0;
  
  do {
    ACCUMULATE(((seed + (i << 6)) >> 56),  ((i << 6) + 0)),
    ACCUMULATE(((seed + (i << 6)) >> 48),  ((i << 6) + 16)),
    ACCUMULATE(((seed + (i << 6)) >> 40),  ((i << 6) + 32)),
    ACCUMULATE(((seed + (i << 6)) >> 32),  ((i << 6) + 48));
  } while (++i < 4);
}

FORCE_INLINE static void diffuse(u64* restrict _ptr) {
  register u8 i = 0;

  do {
    ISAAC_MIX(0  + i),
    ISAAC_MIX(8  + i),
    ISAAC_MIX(16 + i),
    ISAAC_MIX(24 + i),
    ISAAC_MIX(32 + i),
    ISAAC_MIX(40 + i),
    ISAAC_MIX(48 + i),
    ISAAC_MIX(56 + i);
    i += (64 - (i == 192));
  } while (i < BUF_SIZE - 1);

  u64 *pp, *p2, *pend, *r;
  pp = _ptr;

  register u64 a = 0, b = 1, x, y;

  for (pp = _ptr, pend = p2 = pp + BUF_SIZE; pp < pend;) {
    ISAAC_STEP(~(a^(a<<21)), a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>5)  , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a<<12) , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>33) , a, b, _ptr, pp, p2, r, x);
  }
}

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
  u16 i = 0, j = 1024;

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

