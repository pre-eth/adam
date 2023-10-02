#ifndef UTIL_H
#define UTIL_H
  #include <time.h>           // for clock_t, clock(), CLOCKS_PER_SEC
  
  #ifdef __AARCH64_SIMD__
    #include <arm_acle.h>     // CORE
    #include <arm_neon.h>     // SIMD

    typedef uint8_t           u8;
    typedef uint16_t          u16;
    typedef uint32_t          u32;
    typedef uint64_t          u64;
    typedef __uint128_t       u128;

    typedef uint8x16_t        reg8q;
    typedef uint8x16x4_t      reg8q4;
    typedef uint64x2_t        reg64q;    
    typedef uint64x2x4_t      reg64q4;
    typedef float64x2_t       dregq;
    typedef float64x2x2_t     dreg2q;
    typedef float64x2x4_t     dreg4q;

    #define SIMD_LOAD8x4      vld1q_u8_x4
    #define SIMD_STORE8x4     vst1q_u8_x4
    #define SIMD_MOV8         vmov_n_u8
    #define SIMD_CREATE8      vcreate_u8
    #define SIMD_COMBINE8     vcombine_u8
    #define SIMD_SET8         vdupq_n_u8
    #define SIMD_SETQ8        vld4q_dup_u8
    #define SIMD_ADD8         vaddq_u8
    #define SIMD_CMPEQ8       vceqq_u8
    #define SIMD_AND8         vandq_u8
    #define SIMD_LOAD64       vld1q_u64
    #define SIMD_LOAD4Q64     vld4q_u64
    #define SIMD_LOAD64x4     vld1q_u64_x4
    #define SIMD_STORE64x4    vst1q_u64_x4
    #define SIMD_SET64        vdupq_n_u64
    #define SIMD_SETQ64       vld4q_dup_u64
    #define SIMD_ADD64        vaddq_u64
    #define SIMD_AND64        vandq_u64
    #define SIMD_XOR64        veorq_u64
    #define SIMD_CAST64       vreinterpretq_u64_f64
    #define SIMD_LOAD2PD      vld2q_f64
    #define SIMD_STORE2PD     vst1q_f64_x2
    #define SIMD_LOAD4PD      vld1q_f64_x4
    #define SIMD_CREATEPD     vcreate_f64
    #define SIMD_COMBINEPD    vcombine_f64
    #define SIMD_SETPD        vdup_n_f64
    #define SIMD_SETQPD       vdupq_n_f64
    #define SIMD_SET4PD       vld4q_dup_f64
    #define SIMD_ADDPD        vaddq_f64
    #define SIMD_SUBPD        vsubq_f64
    #define SIMD_MULPD        vmulq_f64
    #define SIMD_SMULPD       vmulq_n_f64

    #define SIMD_ADD4Q8(s1, s2, s3) \
      s1.val[0] = SIMD_ADD8(s2.val[0], s3), \
      s1.val[1] = SIMD_ADD8(s2.val[1], s3), \
      s1.val[2] = SIMD_ADD8(s2.val[2], s3), \
      s1.val[3] = SIMD_ADD8(s2.val[3], s3)    

    #define SIMD_CMP4QEQ8(s1, s2, s3) \
        s1.val[0] = SIMD_CMPEQ8(s2.val[0], s3), \
        s1.val[1] = SIMD_CMPEQ8(s2.val[1], s3), \
        s1.val[2] = SIMD_CMPEQ8(s2.val[2], s3), \
        s1.val[3] = SIMD_CMPEQ8(s2.val[3], s3)    

    #define SIMD_AND4Q8(s1, s2, s3) \
      s1.val[0] = SIMD_AND8(s2.val[0], s3), \
      s1.val[1] = SIMD_AND8(s2.val[1], s3), \
      s1.val[2] = SIMD_AND8(s2.val[2], s3), \
      s1.val[3] = SIMD_AND8(s2.val[3], s3)       

    #define SIMD_ADD4Q64(s1, s2, s3) \
      s1.val[0] = SIMD_ADD64(s2.val[0], s3), \
      s1.val[1] = SIMD_ADD64(s2.val[1], s3), \
      s1.val[2] = SIMD_ADD64(s2.val[2], s3), \
      s1.val[3] = SIMD_ADD64(s2.val[3], s3)

    #define SIMD_ADD4RQ64(s1, s2, s3) \
      s1.val[0] = SIMD_ADD64(s2.val[0], s3.val[0]), \
      s1.val[1] = SIMD_ADD64(s2.val[1], s3.val[1]), \
      s1.val[2] = SIMD_ADD64(s2.val[2], s3.val[2]), \
      s1.val[3] = SIMD_ADD64(s2.val[3], s3.val[3])      

    #define SIMD_AND4Q64(s1, s2, s3) \
      s1.val[0] = SIMD_AND64(s2.val[0], s3), \
      s1.val[1] = SIMD_AND64(s2.val[1], s3), \
      s1.val[2] = SIMD_AND64(s2.val[2], s3), \
      s1.val[3] = SIMD_AND64(s2.val[3], s3)        

    #define SIMD_XOR4RQ64(s1, s2, s3) \
      s1.val[0] = SIMD_XOR64(s2.val[0], s3.val[0]), \
      s1.val[1] = SIMD_XOR64(s2.val[1], s3.val[1]), \
      s1.val[2] = SIMD_XOR64(s2.val[2], s3.val[2]), \
      s1.val[3] = SIMD_XOR64(s2.val[3], s3.val[3])      

    #define SIMD_CAST4Q64(s1, s2) \
      s1.val[0] = SIMD_CAST64(s2.val[0]), \
      s1.val[1] = SIMD_CAST64(s2.val[1]), \
      s1.val[2] = SIMD_CAST64(s2.val[2]), \
      s1.val[3] = SIMD_CAST64(s2.val[3])

    #define SIMD_3XOR4Q64(s1, s2, s3) \
      s3.val[0] = veor3q_u64(s1.val[0], s2.val[0], s3.val[0]), \
      s3.val[1] = veor3q_u64(s1.val[1], s2.val[1], s3.val[1]), \
      s3.val[2] = veor3q_u64(s1.val[2], s2.val[2], s3.val[2]), \
      s3.val[3] = veor3q_u64(s1.val[3], s2.val[3], s3.val[3])            

    #define SIMD_SCALARMUL2PD(s1, d) \
      s1.val[0] = SIMD_SMULPD(s1.val[0], d), \
      s1.val[1] = SIMD_SMULPD(s1.val[1], d)

    #define SIMD_SCALARMUL4PD(s1, s2, d) \
      s1.val[0] = SIMD_SMULPD(s2.val[0], d), \
      s1.val[1] = SIMD_SMULPD(s2.val[1], d), \
      s1.val[2] = SIMD_SMULPD(s2.val[2], d), \
      s1.val[3] = SIMD_SMULPD(s2.val[3], d)

    #define SIMD_SUB4QPD(s1, s2, s3) \
      s1.val[0] = SIMD_MULPD(s2, s3.val[0]), \
      s1.val[1] = SIMD_MULPD(s2, s3.val[1]), \
      s1.val[2] = SIMD_MULPD(s2, s3.val[2]), \
      s1.val[3] = SIMD_MULPD(s2, s3.val[3])

    #define SIMD_MUL4RQPD(s1, s2, s3) \
      s1.val[0] = SIMD_MULPD(s2.val[0], s3.val[0]), \
      s1.val[1] = SIMD_MULPD(s2.val[1], s3.val[1]), \
      s1.val[2] = SIMD_MULPD(s2.val[2], s3.val[2]), \
      s1.val[3] = SIMD_MULPD(s2.val[3], s3.val[3])
  #else
    #include <immintrin.h>    // AVX/AVX2

    typedef __UINT8_TYPE__    u8;
    typedef __UINT16_TYPE__   u16;
    typedef __UINT32_TYPE__   u32;
    typedef __UINT64_TYPE__   u64;
    typedef __uint128_t       u128;

    #ifdef __AVX512F__
      #define SIMD_LEN         64
      typedef __m512i          reg;
      typedef __m512d          regd;
      #define SIMD_SETZERO     _mm512_setzero_si512
      #define SIMD_SET8        _mm512_set1_epi8
      #define SIMD_SETR8       _mm512_setr_epi8
      #define SIMD_ADD8        _mm512_add_epi8
      #define SIMD_CMPEQ8      _mm512_cmpeq_epi8 
      #define SIMD_SETR64      _mm512_setr_epi64 
      #define SIMD_SET64       _mm512_set1_epi64
      #define SIMD_ADD64       _mm512_add_epi64
      #define SIMD_LOADBITS    _mm512_load_si512
      #define SIMD_STOREBITS   _mm512_store_si512
      #define SIMD_ANDBITS     _mm512_and_si512
      #define SIMD_XORBITS     _mm512_xor_si512
      #define SIMD_CASTPD      _mm512_castpd_si512
      #define SIMD_LOADPD      _mm512_load_pd
      #define SIMD_STOREPD     _mm512_store_pd
      #define SIMD_SETPD       _mm512_set1_pd
      #define SIMD_SETRPD      _mm512_setr_pd
      #define SIMD_ADDPD       _mm512_add_pd
      #define SIMD_SUBPD       _mm512_sub_pd
      #define SIMD_MULPD       _mm512_mul_pd
    #else
      #define SIMD_LEN         32
      typedef __m256i          reg;
      typedef __m256d          regd;
      #define SIMD_SETZERO     _mm256_setzero_si256
      #define SIMD_SET8        _mm256_set1_epi8
      #define SIMD_SETR8       _mm256_setr_epi8
      #define SIMD_ADD8        _mm256_add_epi8
      #define SIMD_CMPEQ8      _mm256_cmpeq_epi8 
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
    #endif
  #endif

  #define ALIGN(x)          __attribute__ ((aligned (x)))
  #define FORCE_INLINE	    inline __attribute__((always_inline))
  #define CTZ               __builtin_ctz 
  #define FLOOR             __builtin_floor
  #define MEMCPY            __builtin_memcpy
  #define LIKELY(x)         __builtin_expect((x), 1)
  #define UNLIKELY(x)       __builtin_expect((x), 0)
#endif