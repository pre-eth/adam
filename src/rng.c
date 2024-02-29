#include "../include/rng.h"
#include "../include/defs.h"
#include "../include/simd.h"

/*
    The algorithm requires at least the construction of 3 maps of size BUF_SIZE

    This buffer contains 2 of the internal maps used for generating the chaotic
    output. The final output vector is provided by the user, a pointer to an array 
    of at least size 256 * sizeof(u64) bytes.

    static is important because otherwise buffer is initiated with junk that
    removes the deterministic nature of the algorithm
*/
static unsigned long long buffer[BUF_SIZE << 1] ALIGN(SIMD_LEN);

//  Seeds supplied to each iteration of the chaotic function
//  Per each round, 8 consecutive chseeds are supplied
static double chseeds[ROUNDS << 2] ALIGN(SIMD_LEN);

/*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
*/
static u64 IV[8] ALIGN(SIMD_LEN) = {
    0x4265206672756974, 0x66756C20616E6420,
    0x6D756C7469706C79, 0x2C20616E64207265,
    0x706C656E69736820, 0x7468652065617274,
    0x68202847656E6573, 0x697320313A323829
};

static u8 cc; // Counter

static void accumulate(const u64 *restrict seed)
{
    // clang-format off
    // To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
    #define DIV   5.4210109E-20
    #define LIMIT 0.5
    // clang-format on

    IV[0] ^= seed[0];
    IV[1] ^= ~seed[0];
    IV[2] ^= seed[1];
    IV[3] ^= ~seed[1];
    IV[4] ^= seed[2];
    IV[5] ^= ~seed[2];
    IV[6] ^= seed[3];
    IV[7] ^= ~seed[3];

    register u8 i       = 0;
    register u16 offset = cc++;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(DIV * LIMIT);

    dreg4q seeds;
    reg64q4 r1 = SIMD_LOAD64x4(&IV[0]);
    reg64q4 r2;

    do {
        r2 = SIMD_LOAD64x4(&buffer[offset]);
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_CAST4QPD(seeds, r1);
        SIMD_MUL4QPD(seeds, seeds, range);
        SIMD_STORE4PD(&chseeds[i << 3], seeds);
        offset += 8;
    } while (++i < (ROUNDS / 2));

    SIMD_STORE64x4(&IV[0], r1);
#else
    const regd factor = SIMD_SETPD(DIV * LIMIT);

    regd d1;
    reg r1 = SIMD_LOADBITS(&IV[0]);
    reg r2;

    do {
        r2 = SIMD_LOADBITS(&buffer[offset]);
        r1 = SIMD_XOR64(r1, r2);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, factor);
        SIMD_STOREPD((reg *) &chseeds[i], s1);

#ifndef __AVX512F__
        r2 = SIMD_LOADBITS(&buffer[offset + 4]);
        r1 = SIMD_XOR64(r1, r2);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, factor);
        SIMD_STOREPD((reg *) &chseeds[i + 4], s1);
#endif
        i += 8;
        offset += 8;
    } while (i < (ROUNDS << 2));
#endif
}

static void diffuse(u64 *restrict _ptr, const u64 nonce)
{
    // clang-format off

    // For diffusing the buffer
    #define ISAAC_MIX(a,b,c,d,e,f,g,h) { \
        a-=e; f^=h>>9;  h+=a; \
        b-=f; g^=a<<9;  a+=b; \
        c-=g; h^=b>>23; b+=c; \
        d-=h; a^=c<<15; c+=d; \
        e-=a; b^=d>>14; d+=e; \
        f-=b; c^=e<<20; e+=f; \
        g-=c; d^=f>>17; f+=g; \
        h-=d; e^=g<<14; g+=h; \
    }

    // Following logic is adapted from ISAAC64, by Bob Jenkins
    register u64 a, b, c, d, e, f, g, h;
    a = b = c = d = nonce;
    e = f = g = h = ~nonce;

    // Scramble
    register u16 i = 0;
    for (; i < 4; ++i)
        ISAAC_MIX(a, b, c, d, e, f, g, h);

    i = 0;
    do {
       a += _ptr[i + 0]; b += _ptr[i + 1]; c += _ptr[i + 2]; d += _ptr[i + 3];
       e += _ptr[i + 4]; f += _ptr[i + 5]; g += _ptr[i + 6]; h += _ptr[i + 7];

       ISAAC_MIX(a, b, c, d, e, f, g, h);

       _ptr[i + 0] = a; _ptr[i + 1] = b; _ptr[i + 2] = c; _ptr[i + 3] = d;
       _ptr[i + 4] = e; _ptr[i + 5] = f; _ptr[i + 6] = g; _ptr[i + 7] = h;
    } while ((i += 8) < BUF_SIZE);

    // clang-format on
}

static void chaotic_iter(u64 *restrict in, u64 *restrict out, double *restrict chseeds)
{
    const u64 *restrict limit = in + BUF_SIZE;

#ifdef __AARCH64_SIMD__
    const dregq one   = SIMD_SETQPD(1.0);
    const dregq coeff = SIMD_SETQPD(COEFFICIENT);
    const dregq beta  = SIMD_SETQPD(BETA);

    reg64q4 r1, r2;

    // Load 8 seeds at a time
    dreg4q d1 = SIMD_LOAD4PD(chseeds);
    dreg4q d2;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        SIMD_SUB4QPD(d2, one, d1);
        SIMD_MUL4QPD(d2, d2, coeff);
        SIMD_MUL4RQPD(d1, d1, d2);

        // Multiply chaotic result by BETA to obtain int
        SIMD_MUL4QPD(d2, d1, beta);

        // Cast, XOR, and store
        SIMD_CAST4Q64(r1, d2);
        r2 = SIMD_LOAD64x4(in);
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_STORE64x4(out, r1);
        out += 8;
    } while ((in += 8) < limit);
#else
    const regd beta        = SIMD_SETQPD(BETA);
    const regd coefficient = SIMD_SETQPD(COEFFICIENT);
    const regd one         = SIMD_SETQPD(1.0);

    regd d1 = SIMD_LOADPD((regd *) &chseeds[rounds]);
    regd d2;
    reg r1, r2;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d2 = SIMD_SUBPD(one, d1);
        d2 = SIMD_MULPD(d2, coefficient);
        d1 = SIMD_MULPD(d1, d2);

        // Multiply result of chaotic function by beta
        d2 = SIMD_MULPD(d1, beta);

        // Cast, XOR, and store
        r1 = SIMD_CASTPD(d2);
        r2 = SIMD_LOADBITS((reg *) &buffer[idx]);
        r1 = SIMD_XORBITS(r1, r2);
        SIMD_STOREBITS((reg *) &buffer[idx], r1);
    } while ((idx += 8) < limit);
#endif
}

static void apply(u64 *restrict _ptr)
{
    chaotic_iter(_ptr, &buffer[0], &chseeds[0]);
    chaotic_iter(&buffer[0], &buffer[BUF_SIZE], &chseeds[8]);
    chaotic_iter(&buffer[BUF_SIZE], _ptr, &chseeds[16]);
}

static void mix(u64 *restrict _ptr)
{
    const u16 limit = BUF_SIZE;

    u16 map_a = 0;
    u16 map_b = BUF_SIZE;

#ifdef __AARCH64_SIMD__
    reg64q4 r1, r2, r3;
    do {
        r1 = SIMD_LOAD64x4(_ptr);
        r2 = SIMD_LOAD64x4(&buffer[map_a]);
        r3 = SIMD_LOAD64x4(&buffer[map_b]);
        SIMD_3XOR4Q64(r1, r2, r3);
        SIMD_STORE64x4(_ptr, r1);
        _ptr += 8;
        map_a += 8;
        map_b += 8;
    } while (map_a < BUF_SIZE);
#else
    reg r1, r2, r3;
    do {
        r1 = SIMD_LOAD64((reg *) _ptr);
        r2 = SIMD_LOAD64((reg *) &buffer[map_a]);
        r3 = SIMD_LOAD64((reg *) &buffer[map_b]);
        r1 = SIMD_XOR64(r1, r2);
        r1 = SIMD_XOR64(r1, r3);
        SIMD_STOREBITS((reg *) _ptr, r1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[4]);
        r2 = SIMD_LOAD64((reg *) &buffer[map_a + 4]);
        r3 = SIMD_LOAD64((reg *) &buffer[map_b + 4]);
        r1 = SIMD_XOR64(r1, r2);
        r1 = SIMD_XOR64(r1, r3);
        SIMD_STOREBITS((reg *) &_ptr[4], r1);
#endif

        _ptr += 8;
        map_a += 8;
        map_b += 8;
    } while (map_a < BUF_SIZE);
#endif
}

static void reseed(u64 *restrict seed, u64 *restrict nonce)
{
    // clang-format off
    // Slightly modified macro from ISAAC for reseeding ADAM
    #define ISAAC_IND(mm, x) (*(u64 *) ((u8 *) (mm) + (2048 + (x & 1023) + (x & 511))))
    // clang-format on

    // static u8 byte_idx;

    // ++seed[byte_idx];
    // seed[byte_idx + !(byte_idx & 32) - (byte_idx & 32)] += !seed[byte_idx];
    // byte_idx = byte_idx + ((!(byte_idx & 32) - (byte_idx & 32)) * !seed[byte_idx]);

    seed[0] ^= ISAAC_IND(buffer, seed[0]);
    seed[1] ^= ISAAC_IND(buffer, seed[1]);
    seed[2] ^= ISAAC_IND(buffer, seed[2]);
    seed[3] ^= ISAAC_IND(buffer, seed[3]);
    *nonce ^= ISAAC_IND(buffer, *nonce);
}

/*  ALGORITHM ENDS HERE. FOLLOWING IS ALL API RELATED STUFF  */

void adam(unsigned long long *restrict _ptr, unsigned long long *restrict seed, unsigned long long *restrict nonce)
{
    accumulate(seed);
    diffuse(_ptr, *nonce);
    apply(_ptr);
    mix(_ptr);
    reseed(seed, nonce);
}

#ifndef ADAM_MIN_LIB
static void dbl_simd_fill(double *buf, unsigned long long *restrict _ptr, const unsigned amount)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);

    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&_ptr[i]);
        d1.val[0] = SIMD_MULPD(SIMD_CASTPD(r1.val[0]), range);
        d1.val[1] = SIMD_MULPD(SIMD_CASTPD(r1.val[1]), range);
        d1.val[2] = SIMD_MULPD(SIMD_CASTPD(r1.val[2]), range);
        d1.val[3] = SIMD_MULPD(SIMD_CASTPD(r1.val[3]), range);
        SIMD_STORE4PD(&buf[i], d1);
    } while ((i += 8) < amount);
#else
    const regd range = SIMD_SETPD(1.0 / (double) __UINT64_MAX__);

    reg r1;
    regd d1;

    do {
        r1 = SIMD_LOAD64((reg *) &_ptr[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

static void dbl_simd_fill_mult(double *buf, unsigned long long *restrict _ptr, const unsigned amount, const unsigned long long multiplier)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);
    const dregq mult  = SIMD_SETQPD((double) multiplier);
    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&_ptr[i]);
        d1.val[0] = SIMD_MULPD(SIMD_CASTPD(r1.val[0]), range);
        d1.val[1] = SIMD_MULPD(SIMD_CASTPD(r1.val[1]), range);
        d1.val[2] = SIMD_MULPD(SIMD_CASTPD(r1.val[2]), range);
        d1.val[3] = SIMD_MULPD(SIMD_CASTPD(r1.val[3]), range);
        SIMD_MUL4QPD(d1, d1, mult);
        SIMD_STORE4PD(&buf[i], d1);
    } while ((i += 8) < amount);
#else
    const regd range = SIMD_SETPD(1.0 / (double) __UINT64_MAX__);
    const regd mult  = SIMD_SETPD((double) multiplier);

    reg r1;
    regd d1;

    do {
        r1 = SIMD_LOAD64((reg *) &_ptr[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

void adam_frun(double *buf, unsigned long long *restrict _ptr, unsigned long long *restrict seed, unsigned long long *restrict nonce, const unsigned int amount)
{
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        tmp = amount - count;
        out = ((tmp > BUF_SIZE) << 8) | (!(tmp > BUF_SIZE) * tmp);
        adam(_ptr, seed, nonce);
        dbl_simd_fill(&buf[count], _ptr, out);
        count += out;
    }

    const u8 leftovers = amount & 7;

    if (LIKELY(leftovers)) {
        adam(_ptr, seed, nonce);
        register u8 i = 0;
        do {
            buf[count] = (double) buffer[i++] / (double) __UINT64_MAX__;
        } while (++count < amount);
    }
}

void adam_fmrun(double *buf, unsigned long long *restrict _ptr, unsigned long long *seed, unsigned long long *nonce, const unsigned int amount, const unsigned long long multiplier)
{
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        tmp = amount - count;
        out = ((tmp >= BUF_SIZE) << 8) | (!(tmp >= BUF_SIZE) * tmp);
        dbl_simd_fill_mult(&buf[count], _ptr, out, multiplier);
        count += out;
    }

    const u8 leftovers = amount & 7;

    if (LIKELY(leftovers)) {
        register u8 i     = 0;
        const double mult = (double) multiplier;
        do {
            buf[count] = ((double) buffer[i++] / (double) __UINT64_MAX__) * mult;
        } while (++count < amount);
    }
}
#endif