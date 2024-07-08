#ifndef SIMD_H
#define SIMD_H  
  #ifdef __AARCH64_SIMD__
    #include <arm_acle.h>     // CORE
    #include <arm_neon.h>     // SIMD

    typedef uint8x16_t        reg8q;
    typedef uint8x16x4_t      reg8q4;
    typedef uint64x2_t        reg64q;    
    typedef uint64x2x4_t      reg64q4;
    typedef float64x2_t       dregq;
    typedef float64x2x4_t     dreg4q;

    #define SIMD_LEN          64
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
    #define SIMD_LOAD64x4     vld1q_u64_x4
    #define SIMD_STORE64x4    vst1q_u64_x4
    #define SIMD_SET64        vdup_n_u64
    #define SIMD_SETQ64       vdupq_n_u64
    #define SIMD_ADD64        vaddq_u64
    #define SIMD_XOR64        veorq_u64
    #define SIMD_XAR64        vxarq_u64
    #define SIMD_STOREPD      vst1q_f64
    #define SIMD_STORE4PD     vst1q_f64_x4
    #define SIMD_LOAD4PD      vld1q_f64_x4
    #define SIMD_CASTPD       vcvtq_f64_u64
    #define SIMD_SETQPD       vdupq_n_f64
    #define SIMD_SUBPD        vsubq_f64
    #define SIMD_MULPD        vmulq_f64

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

    #define SIMD_ADD4RQ64(s1, s2, s3) \
      s1.val[0] = SIMD_ADD64(s2.val[0], s3.val[0]), \
      s1.val[1] = SIMD_ADD64(s2.val[1], s3.val[1]), \
      s1.val[2] = SIMD_ADD64(s2.val[2], s3.val[2]), \
      s1.val[3] = SIMD_ADD64(s2.val[3], s3.val[3])      

    #define SIMD_3XOR4Q64(s1, s2, s3) \
      s1.val[0] = veor3q_u64(s1.val[0], s2.val[0], s3.val[0]), \
      s1.val[1] = veor3q_u64(s1.val[1], s2.val[1], s3.val[1]), \
      s1.val[2] = veor3q_u64(s1.val[2], s2.val[2], s3.val[2]), \
      s1.val[3] = veor3q_u64(s1.val[3], s2.val[3], s3.val[3])            
    
    #define SIMD_CAST4QPD(s1, s2) \
      s1.val[0] = SIMD_CASTPD(s2.val[0]), \
      s1.val[1] = SIMD_CASTPD(s2.val[1]), \
      s1.val[2] = SIMD_CASTPD(s2.val[2]), \
      s1.val[3] = SIMD_CASTPD(s2.val[3])

    #define SIMD_XAR64RQ(s1, s2, s3, n, m) \
      s1.val[0] = vxarq_u64(s2.val[0], s3.val[0], n), \
      s1.val[1] = vxarq_u64(s2.val[1], s3.val[1], m), \
      s1.val[2] = vxarq_u64(s2.val[2], s3.val[2], n), \
      s1.val[3] = vxarq_u64(s2.val[3], s3.val[3], m)  \

    #define SIMD_SUB4QPD(s1, s2, s3) \
      s1.val[0] = SIMD_SUBPD(s2, s3.val[0]), \
      s1.val[1] = SIMD_SUBPD(s2, s3.val[1]), \
      s1.val[2] = SIMD_SUBPD(s2, s3.val[2]), \
      s1.val[3] = SIMD_SUBPD(s2, s3.val[3])

    #define SIMD_MUL4QPD(s1, s2, s3) \
      s1.val[0] = SIMD_MULPD(s2.val[0], s3), \
      s1.val[1] = SIMD_MULPD(s2.val[1], s3), \
      s1.val[2] = SIMD_MULPD(s2.val[2], s3), \
      s1.val[3] = SIMD_MULPD(s2.val[3], s3)

    #define SIMD_MUL4RQPD(s1, s2, s3) \
      s1.val[0] = SIMD_MULPD(s2.val[0], s3.val[0]), \
      s1.val[1] = SIMD_MULPD(s2.val[1], s3.val[1]), \
      s1.val[2] = SIMD_MULPD(s2.val[2], s3.val[2]), \
      s1.val[3] = SIMD_MULPD(s2.val[3], s3.val[3])

    #define SIMD_REINTERP_ADD64(s1, d1, s2) \
        s1.val[0] = SIMD_ADD64(vreinterpretq_u64_f64(d1.val[0]), s2.val[0]); \
        s1.val[1] = SIMD_ADD64(vreinterpretq_u64_f64(d1.val[1]), s2.val[1]); \
        s1.val[2] = SIMD_ADD64(vreinterpretq_u64_f64(d1.val[2]), s2.val[2]); \
        s1.val[3] = SIMD_ADD64(vreinterpretq_u64_f64(d1.val[3]), s2.val[3]);
  #else
    #include <immintrin.h>    // AVX/AVX2

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
      #define SIMD_ROTR64      _mm512_ror_epi64 
      #define SIMD_LOADBITS    _mm512_load_si512
      #define SIMD_STOREBITS   _mm512_store_si512
      #define SIMD_ANDBITS     _mm512_and_si512
      #define SIMD_XORBITS     _mm512_xor_si512
      #define SIMD_CVTPD       _mm512_cvtepi64_pd 
      #define SIMD_LOADPD      _mm512_load_pd
      #define SIMD_STOREPD     _mm512_store_pd
      #define SIMD_STOREUPD    _mm512_storeu_pd
      #define SIMD_SETPD       _mm512_set1_pd
      #define SIMD_SETRPD      _mm512_setr_pd
      #define SIMD_ADDPD       _mm512_add_pd
      #define SIMD_SUBPD       _mm512_sub_pd
      #define SIMD_MULPD       _mm512_mul_pd
      #define SIMD_CVT64       _mm512_cvtpd_epi64 
    #else
      #define SIMD_LEN         32
      typedef __m256i          reg;
      typedef __m256d          regd;
      #define SIMD_SETZERO     _mm256_setzero_si256
      #define SIMD_SET8        _mm256_set1_epi8
      #define SIMD_SETR8       _mm256_setr_epi8
      #define SIMD_ADD8        _mm256_add_epi8
      #define SIMD_CMPEQ8      _mm256_cmpeq_epi8 
      #define SIMD_BLEND16     _mm256_blend_epi16
      #define SIMD_SET64       _mm256_set1_epi64x 
      #define SIMD_SETR64      _mm256_setr_epi64x 
      #define SIMD_ADD64       _mm256_add_epi64
      #define SIMD_SUB64       _mm256_sub_epi64
      #define SIMD_RSHIFT64    _mm256_srli_epi64
      #define SIMD_LSHIFT64    _mm256_slli_epi64
      #define SIMD_LOADBITS    _mm256_load_si256
      #define SIMD_STOREBITS   _mm256_store_si256
      #define SIMD_ANDBITS     _mm256_and_si256
      #define SIMD_XORBITS     _mm256_xor_si256
      #define SIMD_ORBITS      _mm256_or_si256
      #define SIMD_CASTBITS    _mm256_castpd_si256
      #define SIMD_LOADPD      _mm256_load_pd
      #define SIMD_STOREPD     _mm256_store_pd
      #define SIMD_SETPD       _mm256_set1_pd
      #define SIMD_SETRPD      _mm256_setr_pd
      #define SIMD_ADDPD       _mm256_add_pd
      #define SIMD_SUBPD       _mm256_sub_pd
      #define SIMD_MULPD       _mm256_mul_pd
      #define SIMD_CASTPD      _mm256_castsi256_pd 
      #define SIMD_ROUNDPD     _mm256_round_pd 
    #endif

    #define BYTE_REPEAT(n)     bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n]
    #define BYTE_MASKS         128, 64, 32, 16, 8, 4, 2, 1
  #endif
#endif
