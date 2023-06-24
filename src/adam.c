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

FORCE_INLINE static float chaotic_iter(u32* restrict _ptr, float seed, u8 k) {
  /* 
    According to the paper, the variable c is derived from: 
      floor(log10(M)) + 3
    Here M is the amount of bits in the buffer, which for ADAM is 
    8192, and log10(8192) = 3.9, so c = floor(3.9) + 3 = 6. Then, 
      BETA = pow(10, 6)
  */
  #define BETA          1000000 

  float x = seed;
  u16 s = SEQ_SIZE - 1;
  u8 i, j;
  i = j = 0;

  do {
    x = CHAOTIC_FN(x);
    j = i + 1 + ((u32) FLOOR(x * BETA) % s);
    --s;
    _ptr[i >> 3] |= (((_ptr[i >> 3] >> ((i & 7) + k)) & 1UL) ^ ((_ptr[j >> 3] >> ((j & 7) + k)) & 1UL)) << ((i & 7) + k);
  } while (++i < BUF_SIZE - 2);

  return x;
}

FORCE_INLINE static void accumulate(u32* restrict _ptr, u64 seed) {
  u8 i = 0;
  u8 j = 4;
  
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

FORCE_INLINE static void diffuse(u32* restrict _ptr, float* chseed, u8 iter) {
  #define SEED16    _rdseed16_step
  #define POINT5    1056964608 // 0.5F aka 00111111000000000000000000000000

  u8 i = 0, j = 512;

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

  u8 res;
  u16 seed;
  while (!(res = SEED16(&seed)));  
  
  u32 s;
  MEMCPY(chseed, &s, sizeof(float));

  s = (seed ^ s) % POINT5;

  float x;
  MEMCPY(&s, &x, sizeof(u32));

  do {
    chaotic_iter(_ptr, x, 0, 0);
    x = FLOOR(x * DEFAULT_SEED * BETA) / BETA;
  } while (--iter > 0);

  *chseed = x;
}

  do s = chaotic_iter(_ptr, s, 0);
  while (--iter > 0);

  float* _fptr = &seed;
  *_fptr = s;
}

FORCE_INLINE u64 generate(u32* restrict _ptr, float chseed, u8 rounds) {
  #define SEED64    _rdseed64_step
   
  const u8 iter = rounds / 3;

  u8 res;
  u64 seed;
  while (!(res = SEED64(&seed))); 
  
  seed ^= (seed ^ (GOLDEN_RATIO ^ (seed >> 32)));

  accumulate(_ptr, seed);
  diffuse(_ptr, chseed, iter);
  apply(_ptr, chseed, iter);
  mix(_ptr);

  return seed;
}

FORCE_INLINE void sgenerate(u64 seed, u32* restrict _ptr, float chseed, u8 rounds) {
  const u8 iter = rounds / 3;

  accumulate(_ptr);
  diffuse(_ptr, chseed, iter);
  apply(_ptr, chseed, iter);
  mix(_ptr);
}