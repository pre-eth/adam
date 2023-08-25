#include <threads.h>

#include "adam.h"

typedef struct thread_data {
  u64 *start;
  u64 *end;
  double seed;
} thdata;

FORCE_INLINE static double chaotic_iter(u64 *map_b, u64 *map_a, const double seed) {
  /* 
    BETA is derived from the length of the mantissa 
    ADAM uses the max double accuracy length of 15 to minimize ROUNDS
  */
  #define BETA          10E15

  register double x = seed;
  register u16 s = (SEQ_SIZE) - 1;
  register u16 i, j = 0;
  i = j = 0;

  // rewrite with SIMD
  do {
    x = CHAOTIC_FN(x);
    j = i + 1 + (((u64) FLOOR(x * BETA) % s) >> 6);
    s -= 64 - (s == 1983);
    map_b[i++] ^= map_a[j];
  } while (s > 0);

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
  a = b = c = d = e = f = g = h = (_ptr[seed & 0xFF] ^ (GOLDEN_RATIO ^ _ptr[(seed >> 37) & 0xFF]));

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

static double multi_thread(thdata *data) {
  register double x = data->seed;
  register u8 i = 0;
  do {
    x = chaotic_iter(data->end, data->start, x);
    
    // Some testing revealed that x sometimes exceeds 0.5, which 
    // violates the algorithm, so this is a corrective measure
    x -= (double)(x >= 0.5) * 0.5;
  } while (++i < ITER);
  
  return x;
}

/* FORCE_INLINE static void apply(u64 *restrict _ptr, double *restrict chseed) {
  // Number of threads we will use
  #define THREAD_COUNT  (1 << THREAD_EXP)

  // Divide sequence bits by thread count, then divide that by 64 
  // to get the increment value for _ptr and ctz to get shift count
  #define THREAD_INC    CTZ((SEQ_SIZE / THREAD_COUNT) >> 6)

  register u8 i = 0;
  register u16 start = 0, end = BUF_SIZE; 

  thrd_t threads[THREAD_COUNT];
  thdata chdata[THREAD_COUNT];

  THSEED(0, chseed),
  THSEED(1, chseed),
  THSEED(2, chseed),
  THSEED(3, chseed),
  THSEED(4, chseed),
  THSEED(5, chseed),
  THSEED(6, chseed),
  THSEED(7, chseed); 

  eval_fn:
    for(; i < THREAD_COUNT; ++i) {
      chdata[i].start = _ptr + start  + (i << THREAD_INC);
      chdata[i].end   = _ptr + end + (i << THREAD_INC);
      thrd_create(&threads[i], multi_thread, chdata);
    }

    i = 0;
    for(; i < THREAD_COUNT; ++i)
      thrd_join(threads[i], &chdata[i].seed);
    
    i = 0;
    end += start += 256;
    end *= (end <= 512);
    if (start <= 512) goto eval_fn;
} */

FORCE_INLINE static void apply(u64 *restrict _b, u64 *restrict _a, double *restrict chseed) {
  register double x = *chseed;

  register u8 i = 0;
  do x = chaotic_iter(_b, _a, x);
  while (++i < ITER);

  // Some testing revealed that x sometimes exceeds 0.5, which 
  // violates the algorithm, so this is a corrective measure
  *chseed = x - (0.5 * (double)(x >= 0.5));
}

FORCE_INLINE static void mix(u64 *restrict _ptr) {
  register u8 i = 0;

  reg a, b;

  do {
    a = SIMD_SETR64(
      XOR_MAPS(i)
      #ifdef __AVX512F__
        , XOR_MAPS(i + 4)
      #endif
    );
    SIMD_STOREBITS((reg*) (_ptr + i), a);   
    b = SIMD_SETR64(
      XOR_MAPS(i + (SIMD_LEN >> 3))
      #ifdef __AVX512F__
        , XOR_MAPS(i + (SIMD_LEN >> 3) + 4)
      #endif
    );
    SIMD_STOREBITS((reg*) (_ptr + i + (SIMD_LEN >> 3)), b);    
    i += (SIMD_LEN >> 2) - (i == BUF_SIZE - (SIMD_LEN >> 2));
  } while (i < (BUF_SIZE - 1));
}

double adam(u64 *restrict _ptr, const double seed, const u64 nonce) {
  // double chseed = seed;
  // accumulate(_ptr, nonce);
  // diffuse(_ptr, nonce);
  // apply(_ptr, &chseed);
  // mix(_ptr);

  accumulate(_ptr, nonce);
  diffuse(_ptr, nonce);

  double chseed = seed;
  
  apply(_ptr + 256, _ptr, &chseed);
  apply(_ptr + 512, _ptr + 256, &chseed);
  apply(_ptr, _ptr + 512, &chseed);

  mix(_ptr);

  return chseed;
}