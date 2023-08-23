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

FORCE_INLINE static double chaotic_iter(u64 *map_b, u64 *map_a, const double seed) {
  /* 
    BETA is derived from the length of the mantissa 
    ADAM uses the max double accuracy length of 15 to minimize ROUNDS
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
    map_b[i >> 6] ^= map_a[j >> 6];
  } while (++i < SEQ_SIZE - 2);

  return x;
}

FORCE_INLINE static void accumulate(u64 *restrict _ptr, const u64 seed) {
  register u8 i = 0;
  
  const reg a = SIMD_SET64(seed);
  const reg b = SIMD_SET64(seed);

  do {
    SIMD_STOREBITS((reg*) (_ptr + i), a);
    SIMD_STOREBITS((reg*) (_ptr + i + (SIMD_LEN >> 3)), b);
    SIMD_STOREBITS((reg*) (_ptr + i + (SIMD_LEN >> 2)), a); 
    SIMD_STOREBITS((reg*) (_ptr + i + (SIMD_LEN >> 1)), b);         
    i += SIMD_LEN - (i == BUF_SIZE - SIMD_LEN);
  } while (i < BUF_SIZE - 1);
}

FORCE_INLINE static void diffuse(u64 *restrict _ptr, u64 seed) {
  register u8 i = 0;

  u64 a, b, c, d, e, f, g, h;
  a = b = c = d = e = f = g = h = (_ptr[seed & 0xFF] ^ (GOLDEN_RATIO ^ _ptr[(seed >> 32) & 0xFF]));

  // Scramble it
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);

  do {
    a += _ptr[i];     b += _ptr[i + 1]; c += _ptr[i + 2]; d += _ptr[i + 3];
    e += _ptr[i + 4]; f += _ptr[i + 5]; g += _ptr[i + 6]; h += _ptr[i + 7];

    ISAAC_MIX(a, b, c, d, e, f, g, h);

    _ptr[i]     = a; _ptr[i + 1] = b; _ptr[i + 2] = c; _ptr[i + 3] = d;
    _ptr[i + 4] = e; _ptr[i + 5] = f; _ptr[i + 6] = g; _ptr[i + 7] = h;

  } while ((i += 8 - (i == 248)) < BUF_SIZE - 1);

  u64 *pp, *p2, *pend, *r;
  r = pp = _ptr;
  pend = p2 = pp + (BUF_SIZE >> 1);

  register u64 x, y;

  for (;pp < pend;) {
    ISAAC_STEP(~(a^(a<<21)), a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>5)  , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a<<12) , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>33) , a, b, _ptr, pp, p2, r, x);
  }

  p2 = pp;
  pend = &_ptr[BUF_SIZE];
  for (;p2 < pend;) {
    ISAAC_STEP(~(a^(a<<21)), a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>5)  , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a<<12) , a, b, _ptr, pp, p2, r, x);
    ISAAC_STEP(  a^(a>>33) , a, b, _ptr, pp, p2, r, x);
  } 
}

FORCE_INLINE static void apply(u64 *restrict _b, u64 *restrict _a, double *restrict chseed) {
  double x = *chseed;

  register u8 i = 0;
  do x = chaotic_iter(_b, _a, x);
  while (++i < ITER);

  // i = 0;
  // x += (double) (x / 100000);
  // do x = chaotic_iter(_b, _a, x);
  // while (++i < ITER);

  // i = 0;
  // x += (double) (x / 1000000);
  // do x = chaotic_iter(_b, _a, x);
  // while (++i < ITER);

  // Some testing revealed that x sometimes exceeds 0.5, which 
  // violates the algorithm, so this is a corrective measure
  *chseed = x - (0.5 * (double)(x >= 0.5));
}

FORCE_INLINE static void mix(u64 *restrict _ptr) {
  register u8 i = 0, j = 128;

  do {
    XOR_MAPS(i + 0),
    XOR_MAPS(i + 8); 
  } while ((i += 16) < (BUF_SIZE >> 1));

  do {
    XOR_MAPS(j + 0),
    XOR_MAPS(j + 8);
    j += (16  - (j == 240));  
  } while (j < (BUF_SIZE - 1));
}

double adam(u64 *restrict _ptr, const double seed, const u64 nonce) {
  accumulate(_ptr, nonce);
  diffuse(_ptr, nonce);

  double chseed = seed;
  
  apply(_ptr + 256, _ptr, &chseed);
  apply(_ptr + 512, _ptr + 256, &chseed);
  apply(_ptr, _ptr + 512, &chseed);

  mix(_ptr);

  return chseed;
}