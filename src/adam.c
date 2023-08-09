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
    map_b[i] |= (((map_a[i] >> i) & 1UL)) ^ (((map_a[j] >> i) & 1UL)) << (i & 63);
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

FORCE_INLINE static void apply(u64* restrict _b, u64* restrict _a, double* chseed) {
  double x = *chseed;

  register u8 i = 0;
  do x = chaotic_iter(_b, _a, x);
  while (++i < ITER);


  i = 0;
  x += (double) (x / 10000);
  do x = chaotic_iter(_b, _a, x);
  while (++i < ITER);

  // Last iteration T

  i = 0;
  x += (double) (x / 100000);
  do x = chaotic_iter(_b, _a, x);
  while (++i < ITER);

  *chseed = x;
}

FORCE_INLINE static void mix(u64* restrict _a, u64* restrict _b, u64* restrict _c) {
  register u8 i = 0, j = 128;

  do {
    XOR_MAPS(i + 0),
    XOR_MAPS(i + 8); 
    i += 16;   
  } while (i < (BUF_SIZE >> 1));

  do {
    XOR_MAPS(j + 0),
    XOR_MAPS(j + 8);
    j += (16  - (i == 240));  
  } while (j < (BUF_SIZE - 1));
}

void adam(u64* restrict _ptr) { 
  u8 res;
  u64 seed;
  while (!(res = SEED64(&seed))); 
  seed ^= (seed ^ (GOLDEN_RATIO ^ (seed >> 32)));

  accumulate(_ptr, seed);
  diffuse(_ptr);

  while (!(res = SEED64(&seed))); 
  double x = ((double) (seed / __UINT64_MAX__)) * 0.5;
  
  apply(_ptr + 256, _ptr, &x);
  apply(_ptr + 512, _ptr + 256, &x);
  apply(_ptr, _ptr + 512, &x);

  mix(_ptr, b1, b2);
}

