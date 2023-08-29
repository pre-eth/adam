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
  #define BETA            10E15

  // strength reduction to avoid expensive modulus
  #define REDUCTION(i)    (u64)((1 / ((s - i) >> 6)) & (__UINT64_MAX__ - 1))
  #define POSITION(x, i)  ((((u64) FLOOR((x) * BETA) >> 6) * REDUCTION(i)) >> 6)
  #define JIDX(x, n, j)   ((i + j) + POSITION(x, n))

  register double a, b, c, x;
  register short s = SEQ_SIZE - 1;
  register u16 i, j;
  i = j = 0;
  x = seed;

  reg r1, r2;
  do {
    a = CHAOTIC_FN(x), b = CHAOTIC_FN(a);
    c = CHAOTIC_FN(b), x = CHAOTIC_FN(c);

    r1 = SIMD_SETR64(
                      map_a[JIDX(a, 0, 1)],   map_a[JIDX(b, 64, 2)], 
                      map_a[JIDX(c, 128, 3)], map_a[JIDX(x, 192, 4)]
                  #ifdef __AVX512F__
                    , map_a[JIDX(a = CHAOTIC_FN(x), 256, 5)]
                    , map_a[JIDX(b = CHAOTIC_FN(a), 320, 6)]
                    , map_a[JIDX(c = CHAOTIC_FN(b), 384, 7)]
                    , map_a[JIDX(x = CHAOTIC_FN(c), 448, 8)]
                  #endif
                    );

    // SIMD_LOADBITS causes a seg fault even though map_b is aligned...
    r2 = SIMD_SETR64(
                      map_b[i], map_b[i + 1], map_b[i + 2], map_b[i + 3]
                    #ifdef __AVX512F__
                    , map_b[i + 4], map_b[i + 5], map_b[i + 6], map_b[i + 7]
                    #endif
                    );

    r2 = SIMD_XORBITS(r2, r1);
    SIMD_STOREBITS((reg*) map_b, r2);

    s -= ((u16) SIMD_LEN << 3);
    i += (SIMD_LEN >> 3);
  } while (s > 0);

  return x;
}

FORCE_INLINE static void accumulate(u64 *restrict _ptr, const u64 nonce) {
  register u8 i = 0;
  
  /*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */
  u64 IV[8] ALIGN(SIMD_LEN) = {
    0x4265206672756974UL ^ nonce, 
    0x66756C20616E6420UL ^ ~nonce, 
    0x6D756C7469706C79UL ^ nonce,
    0x2C20616E64207265UL ^ ~nonce, 
    0x706C656E69736820UL ^ nonce,
    0x7468652065617274UL ^ ~nonce, 
    0x68202847656E6573UL ^ nonce,
    0x697320313A323829UL ^ ~nonce
  };

  const reg a = SIMD_LOADBITS((reg*) IV);
  const reg b = SIMD_LOADBITS((reg*) IV + (!!(SIMD_LEN & 63) << 2));

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
  a = b = c = d = e = f = g = h = (_ptr[seed & 0xFF] ^ (GOLDEN_RATIO ^ _ptr[(seed >> (seed & 31)) & 0xFF]));

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

FORCE_INLINE static void apply(u64 *restrict _ptr, double *chseed) {
  register double x = *chseed;

  register u8 i = 0;
  do x = chaotic_iter(_ptr + 256, _ptr, x);
  while (++i < ITER);

  do x = chaotic_iter(_ptr + 512, _ptr + 256, x);
  while (++i < ITER + 6);

  do x = chaotic_iter(_ptr, _ptr + 512, x);
  while (++i < ITER + 12);

  *chseed = x;
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
  double chseed = seed;
  accumulate(_ptr, nonce);
  diffuse(_ptr, nonce);
  apply(_ptr, &chseed);
  mix(_ptr);
  return chseed;
}