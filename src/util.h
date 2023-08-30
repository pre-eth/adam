#ifndef UTIL_H
#define UTIL_H

  #include <immintrin.h>    // SIMD
  #include <time.h>         // for clock_t, clock(), CLOCKS_PER_SEC

  #ifdef __AVX512F__
    #define SIMD_LEN         64
    typedef __m512i          reg;
    #define SIMD_SETZERO     _mm512_setzero_si512
    #define SIMD_SET8        _mm512_set1_epi8
    #define SIMD_SETR8       _mm512_setr_epi8
    #define SIMD_ADD8        _mm512_add_epi8
    #define SIMD_SETR64      _mm512_setr_epi64 
    #define SIMD_SET64       _mm512_set1_epi64
    #define SIMD_LOADBITS    _mm512_load_si512
    #define SIMD_STOREBITS   _mm512_store_si512
    #define SIMD_XORBITS     _mm512_xor_si512
  #else
    #define SIMD_LEN         32
    typedef __m256i          reg;
    #define SIMD_SETZERO     _mm256_setzero_si256
    #define SIMD_SET8        _mm256_set1_epi8
    #define SIMD_SETR8       _mm256_setr_epi8
    #define SIMD_ADD8        _mm256_add_epi8
    #define SIMD_SET64       _mm256_set1_epi64x 
    #define SIMD_SETR64      _mm256_setr_epi64x 
    #define SIMD_ADD64       _mm256_add_epi64
    #define SIMD_LOADBITS    _mm256_load_si256
    #define SIMD_STOREBITS   _mm256_store_si256
    #define SIMD_XORBITS     _mm256_xor_si256
  #endif

  typedef __UINT8_TYPE__    u8;
  typedef __UINT16_TYPE__   u16;
  typedef __UINT32_TYPE__   u32;
  typedef __UINT64_TYPE__   u64;
  typedef __uint128_t       u128;

  #define ALIGN(x)          __attribute__ ((aligned (x)))
  #define FORCE_INLINE	    inline __attribute__((always_inline))
  #define CTZ               __builtin_ctz 
  #define FLOOR             __builtin_floor
  #define LIKELY(x)         __builtin_expect((x), 1)
  #define UNLIKELY(x)       __builtin_expect((x), 0)

#endif