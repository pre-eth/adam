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
    #define SIMD_ANDBITS     _mm512_and_si512
    #define SIMD_XORBITS     _mm512_xor_si512
  #else
    #define SIMD_LEN         32
    typedef __m256i          reg;
    typedef __m256d          regd;
    #define SIMD_SETZERO     _mm256_setzero_si256
    #define SIMD_SET8        _mm256_set1_epi8
    #define SIMD_SETR8       _mm256_setr_epi8
    #define SIMD_ADD8        _mm256_add_epi8
    #define SIMD_SET64       _mm256_set1_epi64x 
    #define SIMD_SETR64      _mm256_setr_epi64x 
    #define SIMD_ADD64       _mm256_add_epi64
    #define SIMD_LOADBITS    _mm256_load_si256
    #define SIMD_STOREBITS   _mm256_store_si256
    #define SIMD_ANDBITS     _mm256_and_si256
    #define SIMD_XORBITS     _mm256_xor_si256
    #define SIMD_CASTPD      _mm256_castpd_si256
    #define SIMD_LOADPD      _mm256_load_pd
    #define SIMD_STOREPD     _mm256_store_pd
    #define SIMD_SETPD       _mm256_set1_pd
    #define SIMD_SETRPD      _mm256_setr_pd
    #define SIMD_ADDPD       _mm256_add_pd
    #define SIMD_SUBPD       _mm256_sub_pd
    #define SIMD_MULPD       _mm256_mul_pd
    #define SIMD_ANDPD       _mm256_and_pd
    #define SIMD_ORPD        _mm256_or_pd
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

  #define LONG_TO_BYTES(num,a,b,c,d,e,f,g,h) \
    a = num & 0xFF, \
    b = (num >> 8)  & 0xFF, \
    c = (num >> 16) & 0xFF, \
    d = (num >> 24) & 0xFF, \
    e = (num >> 32) & 0xFF, \
    f = (num >> 40) & 0xFF, \
    g = (num >> 48) & 0xFF, \
    h = (num >> 56) & 0xFF

#endif