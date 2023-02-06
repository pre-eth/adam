#ifndef UTIL_H
#define UTIL_H
    #include <sys/ioctl.h>
    #include <immintrin.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <unistd.h>
    #define u8   uint8_t
    #define u16  uint16_t
    #define u32  uint32_t
    #define u64  uint64_t
    #define u128 __uint128_t
    #define i8   int8_t
    #define i16  int16_t
    #define i32  int32_t
    #define i64  int64_t

    #ifdef __AVX512F__
        typedef __m512i         reg;
        typedef __m512d         dreg; 
        #define REG_SETR32      _mm512_setr_epi32 
        #define REG_SET32       _mm512_set1_epi32 
        #define REG_SETR64      _mm512_setr_epi64 
        #define REG_SET64       _mm512_set1_epi64 
        #define REG_LOADBITS    _mm512_load_si512
        #define REG_STOREBITS   _mm512_store_si512
        #define REG_LOADUBITS   _mm512_loadu_si512
        #define REG_STOREUBITS  _mm512_storeu_si512
        #define REG_LOAD64      _mm512_load_epi64
        #define REG_STORE64     _mm512_store_epi64
        #define REG_ADD32       _mm512_add_epi32
        #define REG_SUB32       _mm512_sub_epi32
        #define REG_ADD64       _mm512_add_epi64
        #define REG_SUB64       _mm512_sub_epi64
        #define REG_SRLV64      _mm512_srlv_epi64
        #define REG_MBLEND64    _mm512_mask_blend_epi64
        #define REG_PERM64      _mm512_permutexvar_epi64
    #else
        typedef __m256i         reg;
        typedef __m256d         dreg;  
        #define REG_SETR32      _mm256_setr_epi32 
        #define REG_SET32       _mm256_set1_epi32 
        #define REG_SETR64      _mm256_setr_epi64x 
        #define REG_SET64       _mm256_set1_epi64x 
        #define REG_LOADBITS    _mm256_load_si256
        #define REG_STOREBITS   _mm256_store_si256
        #define REG_LOADUBITS   _mm256_loadu_si256
        #define REG_STOREUBITS  _mm256_storeu_si256
        #define REG_ADD32       _mm256_add_epi32
        #define REG_SUB32       _mm256_sub_epi32
        #define REG_ADD64       _mm256_add_epi64
        #define REG_SUB64       _mm256_sub_epi64
        #define REG_BLEND32     _mm256_blend_epi32
        #define REG_SLLV64      _mm256_sllv_epi64 
        #define REG_SRLV64      _mm256_srlv_epi64
        #define REG_PERM2X128   _mm256_permute2x128_si256
        #define REG_PERM8X32    _mm256_permutevar8x32_epi32
        #define REG_EXT64       _mm256_extract_epi64
        #define REG_INS64       _mm256_insert_epi64
    #endif

    // http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
    #define ISPOW2(n) n && !(n & (n - 1))
    #define COMPUTEPOW2(n) 1 << n

    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
    #define FINDMIN(x, y) y + ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1)));
    #define FINDMAX(x, y) x - ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1)));

    // http://graphics.stanford.edu/~seander/bithacks.html#SwappingValuesXOR
    #define SWAP(x, y) ((x) ^ (y)) && ((y) ^= (x) ^= (y), (x) ^= (y));

    // http://graphics.stanford.edu/~seander/bithacks.html#ConditionalNegate
    #define IFNEGATE(n, c) n = (n ^ -(c)) + (c);

    // http://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
    #define BITSETCLEAR(w, m, c) (w & ~m) | (-c & m)

    // above clears if false, this retains original value
    #define BITSET(w, m, c) w |= (-c & m)

    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
    #define ABSMASK(i) const int m = i >> (sizeof(int) * 8) - 1
    #define ABS(n, i) ABSMASK(i); n = (i + m) ^ m

    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
    static inline u8 log2(u64 x) {
        static const char LogTable256[256] = {
            #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
                -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
                LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
        };

        u64 tt;
        if (tt = x >> 56) 
            return 56 + LogTable256[tt];
        if (tt = x >> 48) 
            return 48 + LogTable256[tt];
        if (tt = x >> 40) 
            return 40 + LogTable256[tt];
        if (tt = x >> 32)
            return 32 + LogTable256[tt];
        if (tt = x >> 24) 
            return 24 + LogTable256[tt];
        if (tt = x >> 16) 
            return 16 + LogTable256[tt];
        if (tt = x >> 8)
            return 8 + LogTable256[tt];

        return LogTable256[x];
    }

    // https://stackoverflow.com/questions/25892665/performance-of-log10-function-returning-an-int
    static inline u8 log10(u64 x) {                            
        static const u8 guess[64] = {
            0,  0,  0,  0,  1,  1,  1,  2,  2,  2,
            3,  3,  3,  3,  4,  4,  4,  5,  5,  5,
            6,  6,  6,  6,  7,  7,  7,  8,  8,  8,
            9,  9,  9,  9,  10, 10, 10, 11, 11, 11,
            12, 12, 12, 12, 13, 13, 13, 14, 14, 14,
            15, 15, 15, 15, 16, 16, 16, 17, 17, 17,
            18, 19, 19, 19
        }; 
        static const u64 tenToThe[20] = {
            1, 10, 100, 1000, 10000, 100000, 
            1000000, 10000000, 100000000, 1000000000,
            10000000000, 100000000000, 1000000000000,
            10000000000000, 100000000000000, 1000000000000000,
            10000000000000000, 100000000000000000, 1000000000000000000,
            10000000000000000000
        };
        IFNEGATE(x, x < 0)
        u8 ln = log2(x);
        // printf("LN IS %d\n", ln);
        u8 digits = guess[ln];
        return digits + (digits <= 20) << (x < tenToThe[digits - 1]);
    }

    inline void getScreenDimensions(u16* h, u16* w) {
        struct winsize wsize;
        ioctl(0, TIOCGWINSZ, &wsize);
        *h = wsize.ws_row;
        *w =  wsize.ws_col;
    }
    
    // adapted from https://stackoverflow.com/a/57112610/3772283
    static inline u64 x_to_i(const char* hex_str) {
        u64 res{0};
        char c;
        while (c = *hex_str++) {
            char v = (c & 0xF) + (c >> 6) | ((c >> 3) & 0x8);
            res = (res << 4) | (u64) v;
        }
        return res;
    }
    /*  Adapted from https://tombarta.wordpress.com/2008/04/23/specializing-atoi/
        
        THIS IS FOR PARSING NUMERICAL ASCII USER INPUT
        Makes the following assumptions:

        - Negative numbers can show up, but positive numbers never have a leading + character.
        - Leading 0s never show up. 
        - Size of string/number is known beforehand
        - Max size of string is 10 digits. User shouldn't need to supply ASCII 64-bit nums
        - Validation is performed by checking against the [min, max) range provided

        FAILURE is denoted by returning the min value
    */
    static inline u64 a_to_u(const char* s, u64 min = 0, u64 max = UINT64_MAX) {
        u64 val{0}; 
        u8 len = strlen(s);
//12115082318202757305
//12115082309612822713
        switch (len) { 
            case 20:    val += 10000000000000000000LLU;
            case 19:    val += (s[len-19] - '0') * 1000000000000000000LLU;
            case 18:    val += (s[len-18] - '0') * 100000000000000000LLU;
            case 17:    val += (s[len-17] - '0') * 10000000000000000LLU;
            case 16:    val += (s[len-16] - '0') * 1000000000000000LLU;
            case 15:    val += (s[len-15] - '0') * 100000000000000LLU;
            case 14:    val += (s[len-14] - '0') * 10000000000000LLU;
            case 13:    val += (s[len-13] - '0') * 1000000000000LLU;
            case 12:    val += (s[len-12] - '0') * 100000000000LLU;
            case 11:    val += (s[len-11] - '0') * 10000000000LLU;
            case 10:    val += (s[len-10] - '0') * 1000000000LLU;
            case  9:    val += (s[len- 9] - '0') * 100000000LLU;
            case  8:    val += (s[len- 8] - '0') * 10000000LLU;
            case  7:    val += (s[len- 7] - '0') * 1000000LLU;
            case  6:    val += (s[len- 6] - '0') * 100000LLU;
            case  5:    val += (s[len- 5] - '0') * 10000LLU;
            case  4:    val += (s[len- 4] - '0') * 1000LLU;
            case  3:    val += (s[len- 3] - '0') * 100LLU;
            case  2:    val += (s[len- 2] - '0') * 10LLU;
            case  1:    val += (s[len- 1] - '0');
                break;
            default:
                return min;
        }
        return BITSET(val, min, (val < min || val > max));
    }

    static inline i64 a_to_i(const char* s, i64 min = 0, i64 max = INT64_MAX) {
        i64 val{0}; 
        i8 sign{1};
        bool isSigned = (*s == '-');
        BITSET(sign, -1, isSigned); // s-= BITSET(sign, -1, (*s == '-'))
        s += isSigned;
        u8 len = strlen(s);
        switch (len) { 
            case 20:    val += 10000000000000000000LL;
            case 19:    val += (s[len-19] - '0') * 1000000000000000000LL;
            case 18:    val += (s[len-18] - '0') * 100000000000000000LL;
            case 17:    val += (s[len-17] - '0') * 10000000000000000LL;
            case 16:    val += (s[len-16] - '0') * 1000000000000000LL;
            case 15:    val += (s[len-15] - '0') * 100000000000000LL;
            case 14:    val += (s[len-14] - '0') * 10000000000000LL;
            case 13:    val += (s[len-13] - '0') * 1000000000000LL;
            case 12:    val += (s[len-12] - '0') * 100000000000LL;
            case 11:    val += (s[len-11] - '0') * 10000000000LL;
            case 10:    val += (s[len-10] - '0') * 1000000000LL;
            case  9:    val += (s[len- 9] - '0') * 100000000LL;
            case  8:    val += (s[len- 8] - '0') * 10000000LL;
            case  7:    val += (s[len- 7] - '0') * 1000000LL;
            case  6:    val += (s[len- 6] - '0') * 100000LL;
            case  5:    val += (s[len- 5] - '0') * 10000LL;
            case  4:    val += (s[len- 4] - '0') * 1000LL;
            case  3:    val += (s[len- 3] - '0') * 100LL;
            case  2:    val += (s[len- 2] - '0') * 10LL;
            case  1:    val += (s[len- 1] - '0');
                val *= sign;
                break;
            default:
                return min;
        }
        return BITSET(val, min, (val < min || val > max));
    }
    

    static inline unsigned short trng16() {
        unsigned short val;
        int c{0};
        while (!c)
            c = _rdrand16_step(&val);
        return val;
    }

    static inline unsigned int trng32() {
        unsigned int val;
        int c{0};
        while (!c)
            c = _rdrand32_step(&val);
        return val;
    }

    static inline unsigned long long trng64() {
        unsigned long long val;
        int c{0};
        while (!c)
            c =  _rdrand64_step(&val);
        return val;
    }

    static inline long page_size() {
        return sysconf(_SC_PAGESIZE);
    }

#endif