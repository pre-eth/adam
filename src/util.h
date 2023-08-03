#ifndef UTIL_H
#define UTIL_H

  #include <immintrin.h>

  typedef __UINT8_TYPE__    u8;
  typedef __UINT16_TYPE__   u16;
  typedef __UINT32_TYPE__   u32;
  typedef __UINT64_TYPE__   u64;

  #define ALIGN(x)          __attribute__ ((aligned (x)))
  #define FORCE_INLINE	    inline __attribute__((always_inline))
  #define CLZ               __builtin_clz  
  #define MEMCPY	          __builtin_memcpy
  #define FLOOR             __builtin_floor
  #define SEED64            _rdseed64_step

  #define BIT_LENGTH(n) (64 - __builtin_ctzll(n))

  #define ACCUMULATE(seed, i)\
    _ptr[0  + i] = 0  + seed,\
    _ptr[1  + i] = 1  + seed,\
    _ptr[2  + i] = 2  + seed,\
    _ptr[3  + i] = 3  + seed,\
    _ptr[4  + i] = 4  + seed,\
    _ptr[5  + i] = 5  + seed,\
    _ptr[6  + i] = 6  + seed,\
    _ptr[7  + i] = 7  + seed,\
    _ptr[8  + i] = 8  + seed,\
    _ptr[9  + i] = 9  + seed,\
    _ptr[10 + i] = 10 + seed,\
    _ptr[11 + i] = 11 + seed,\
    _ptr[12 + i] = 12 + seed,\
    _ptr[13 + i] = 13 + seed,\
    _ptr[14 + i] = 14 + seed,\
    _ptr[15 + i] = 15 + seed

  #define SIMD_MASK_PREP(i, j) \
    _ptr[0  + i + j] & 0xFF,\
    _ptr[1  + i + j] & 0xFF,\
    _ptr[2  + i + j] & 0xFF,\
    _ptr[3  + i + j] & 0xFF,\
    _ptr[4  + i + j] & 0xFF,\
    _ptr[5  + i + j] & 0xFF,\
    _ptr[6  + i + j] & 0xFF,\
    _ptr[7  + i + j] & 0xFF,\
    _ptr[8  + i + j] & 0xFF,\
    _ptr[9  + i + j] & 0xFF,\
    _ptr[10 + i + j] & 0xFF,\
    _ptr[11 + i + j] & 0xFF,\
    _ptr[12 + i + j] & 0xFF,\
    _ptr[13 + i + j] & 0xFF,\
    _ptr[14 + i + j] & 0xFF,\
    _ptr[15 + i + j] & 0xFF    

  #define SIMD_XOR_PREP(i, j) \
    (_ptr[0  + i + j] & 0x00FF00) ^ (_ptr[0  + i + j] & 0xFF0000),\
    (_ptr[1  + i + j] & 0x00FF00) ^ (_ptr[1  + i + j] & 0xFF0000),\
    (_ptr[2  + i + j] & 0x00FF00) ^ (_ptr[2  + i + j] & 0xFF0000),\
    (_ptr[3  + i + j] & 0x00FF00) ^ (_ptr[3  + i + j] & 0xFF0000),\
    (_ptr[4  + i + j] & 0x00FF00) ^ (_ptr[4  + i + j] & 0xFF0000),\
    (_ptr[5  + i + j] & 0x00FF00) ^ (_ptr[5  + i + j] & 0xFF0000),\
    (_ptr[6  + i + j] & 0x00FF00) ^ (_ptr[6  + i + j] & 0xFF0000),\
    (_ptr[7  + i + j] & 0x00FF00) ^ (_ptr[7  + i + j] & 0xFF0000),\
    (_ptr[8  + i + j] & 0x00FF00) ^ (_ptr[8  + i + j] & 0xFF0000),\
    (_ptr[9  + i + j] & 0x00FF00) ^ (_ptr[9  + i + j] & 0xFF0000),\
    (_ptr[10 + i + j] & 0x00FF00) ^ (_ptr[10 + i + j] & 0xFF0000),\
    (_ptr[11 + i + j] & 0x00FF00) ^ (_ptr[11 + i + j] & 0xFF0000),\
    (_ptr[12 + i + j] & 0x00FF00) ^ (_ptr[12 + i + j] & 0xFF0000),\
    (_ptr[13 + i + j] & 0x00FF00) ^ (_ptr[13 + i + j] & 0xFF0000),\
    (_ptr[14 + i + j] & 0x00FF00) ^ (_ptr[14 + i + j] & 0xFF0000),\
    (_ptr[15 + i + j] & 0x00FF00) ^ (_ptr[15 + i + j] & 0xFF0000)    

  #ifdef __AVX512F__
    typedef __m512i          reg;
    #define SIMD_INC         64
    #define SIMD_LOADBITS    _mm512_load_si512
    #define SIMD_STOREBITS   _mm512_store_si512
    #define SIMD_SETR8       _mm512_setr_epi8 
    #define SIMD_XORBITS     _mm512_xor_si512
    #define SIMD_ORBITS      _mm512_or_si512
  #else
    typedef __m256i          reg;
    #define SIMD_INC         32
    #define SIMD_LOADBITS    _mm256_load_si256
    #define SIMD_STOREBITS   _mm256_store_si256
    #define SIMD_SETR8       _mm256_setr_epi8 
    #define SIMD_XORBITS     _mm256_xor_si256
    #define SIMD_ORBITS      _mm256_or_si256
  #endif
#endif