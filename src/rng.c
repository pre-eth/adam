#include "../include/rng.h"

#if !defined(__AARCH64_SIMD__) && !defined(__AVX512F__)
// Following code courtesy of https://stackoverflow.com/a/41148578
regd mm256_cvtpd_epi64(reg r1)
{
    const regd factor = SIMD_SETPD(0x0010000000000000);
    const regd fix1   = SIMD_SETPD(19342813113834066795298816.0);
    const regd fix2   = SIMD_SETPD(19342813118337666422669312.0);

    reg xH, xL;
    regd d1;

    xH = SIMD_RSHIFT64(r1, 32);
    xH = SIMD_ORBITS(xH, SIMD_CASTBITS(fix1));          //  2^84
    xL = SIMD_BLEND16(r1, SIMD_CASTBITS(factor), 0xCC); //  2^52
    d1 = SIMD_SUBPD(SIMD_CASTPD(xH), fix2);             //  2^84 + 2^52
    d1 = SIMD_ADDPD(d1, SIMD_CASTPD(xL));
    return d1;
}

// Remaining code courtesy of https://stackoverflow.com/a/77376595
static reg double_to_int64(regd x)
{
    x = SIMD_ADDPD(x, SIMD_SETPD(0x0018000000000000));
    return SIMD_SUB64(
        SIMD_CASTBITS(x),
        SIMD_CASTBITS(SIMD_SETPD(0x0018000000000000)));
}

// Only works for inputs in the range: [-2^51, 2^51]
static regd int64_to_double(reg x)
{
    x = SIMD_ADD64(x, SIMD_CASTBITS(SIMD_SETPD(0x0018000000000000)));
    return SIMD_SUBPD(SIMD_CASTPD(x), SIMD_SETPD(0x0018000000000000));
}

static reg mm256_cvtepi64_pd(regd d1)
{
    d1 = SIMD_ROUNDPD(d1, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    const regd k2_32inv_dbl = SIMD_SETPD(1.0 / 4294967296.0); // 1 / 2^32
    const regd k2_32_dbl    = SIMD_SETPD(4294967296.0);       // 2^32

    // Multiply by inverse instead of dividing.
    const regd v_hi_dbl = SIMD_MULPD(d1, k2_32inv_dbl);
    // Convert to integer.
    const reg v_hi = double_to_int64(v_hi_dbl);
    // Convert high32 integer to double and multiply by 2^32.
    const regd v_hi_int_dbl = SIMD_MULPD(int64_to_double(v_hi), k2_32_dbl);
    // Subtract that from the original to get the remainder.
    const regd v_lo_dbl = SIMD_SUBPD(d1, v_hi_int_dbl);
    // Convert to low32 integer.
    const reg v_lo = double_to_int64(v_lo_dbl);
    // Reconstruct integer from shifted high32 and remainder.
    return SIMD_ADD64(SIMD_LSHIFT64(v_hi, 32), v_lo);
}

#define SIMD_CVT64 mm256_cvtepi64_pd

#endif

/*     ALGORITHM START     */

void accumulate(u64 *restrict seed, u64 *restrict IV, u64 *restrict work_buffer, double *restrict chseeds, const u64 cc)
{
    IV[0] ^= seed[0];
    IV[1] ^= ~seed[0];
    IV[2] ^= seed[1];
    IV[3] ^= ~seed[1];
    IV[4] ^= seed[2];
    IV[5] ^= ~seed[2];
    IV[6] ^= seed[3];
    IV[7] ^= ~seed[3];

    register u8 i      = 0;
    register u8 offset = ((cc & 0xFF) >> 2) << 2;
#ifdef __AARCH64_SIMD__
    // To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
    const dregq range = SIMD_SETQPD(2.7105054E-20);

    dreg4q seeds;
    reg64q4 r1 = SIMD_LOAD64x4(&IV[0]);
    reg64q4 r2 = SIMD_LOAD64x4(&work_buffer[offset]);

    do {
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_CAST4QPD(seeds, r1);
        SIMD_MUL4QPD(seeds, seeds, range);
        SIMD_STORE4PD(&chseeds[i << 3], seeds);
        SIMD_ADD4RQ64(r2, r1, r2);
    } while (++i < (ROUNDS / 2));

    SIMD_STORE64x4(&IV[0], r2);
#else
    // To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
    const regd range = SIMD_SETPD(2.7105054E-20);

    regd d1;
    reg r1 = SIMD_LOADBITS((reg *) &IV[0]);
#ifdef __AVX512F__
    reg r2 = SIMD_LOADBITS((reg *) &work_buffer[offset]);

    do {
        r1 = SIMD_XORBITS(r1, r2);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i << 3], d1);
        r2 = SIMD_ADD64(r1, r2);
    } while (++i < (ROUNDS / 2));

    SIMD_STOREBITS((reg *) &IV[0], r2);
#else
    reg r2 = SIMD_LOADBITS((reg *) &IV[4]);

    reg r3 = SIMD_LOADBITS((reg *) &work_buffer[offset]);
    reg r4 = SIMD_LOADBITS((reg *) &work_buffer[offset + 4]);

    do {
        r1 = SIMD_XORBITS(r1, r3);
        d1 = SIMD_CVTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i], d1);

        r2 = SIMD_XORBITS(r2, r4);
        d1 = SIMD_CVTPD(r2);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i + 4], d1);

        r3 = SIMD_ADD64(r1, r3);
        r4 = SIMD_ADD64(r2, r4);
    } while ((i += 8) < (ROUNDS << 2));

    SIMD_STOREBITS((reg *) &IV[0], r3);
    SIMD_STOREBITS((reg *) &IV[4], r4);
#endif
#endif
}

void diffuse(u64 *restrict _ptr, const u64 nonce)
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
static void chaotic_iter(u64 *restrict in, u64 *restrict out, const double *restrict chseeds)
{
#ifdef __AARCH64_SIMD__
    const dregq one   = SIMD_SETQPD(1.0);
    const dregq coeff = SIMD_SETQPD(COEFFICIENT);
    const dregq beta  = SIMD_SETQPD(BETA);

    reg64q4 r1, r2;

    // Load 8 consecutive seeds at a time
    dreg4q d1 = SIMD_LOAD4PD(chseeds);
    dreg4q d2;

    register u16 i = 0;
    do {
        // 3.9999 * X * (1 - X) for all X in the register
        SIMD_SUB4QPD(d2, one, d1);
        SIMD_MUL4QPD(d2, d2, coeff);
        SIMD_MUL4RQPD(d1, d1, d2);

        // Multiply chaotic result by BETA to obtain ints
        SIMD_MUL4QPD(d2, d1, beta);

        // Cast, XOR, and store
        SIMD_CAST4Q64(r1, d2);
        r2 = SIMD_LOAD64x4(&in[i]);
        SIMD_XOR4RQ64(r1, r2, r1);
        SIMD_STORE64x4(&out[i], r1);
    } while ((i += 8) < BUF_SIZE);
#else
    const regd one   = SIMD_SETPD(1.0);
    const regd coeff = SIMD_SETPD(COEFFICIENT);
    const regd beta  = SIMD_SETPD(BETA);

    regd d1 = SIMD_LOADPD(chseeds);
    regd d2;
    reg r1, r2;

    register u16 i = 0;

#ifdef __AVX512F__
    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d2 = SIMD_SUBPD(one, d1);
        d2 = SIMD_MULPD(d2, coeff);
        d1 = SIMD_MULPD(d1, d2);

        // Multiply result of chaotic function by beta
        d2 = SIMD_MULPD(d1, beta);

        // Cast, XOR, and store
        r1 = SIMD_CASTBITS(d2);
        r2 = SIMD_LOADBITS((reg *) &in[i]);
        r1 = SIMD_XORBITS(r2, r1);
        SIMD_STOREBITS((reg *) &out[i], r1);
    } while ((i += 8) < BUF_SIZE);
#else
    d2 = SIMD_LOADPD(&chseeds[4]);
    regd d3;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d3 = SIMD_SUBPD(one, d1);
        d3 = SIMD_MULPD(d3, coeff);
        d1 = SIMD_MULPD(d1, d3);

        // Multiply result of chaotic function by beta
        d3 = SIMD_MULPD(d1, beta);

        // Cast, XOR, and store
        r1 = SIMD_CVT64(d3);
        r2 = SIMD_LOADBITS((reg *) &in[i]);
        r1 = SIMD_XORBITS(r2, r1);
        SIMD_STOREBITS((reg *) &out[i], r1);

        // 3.9999 * X * (1 - X) for all X in the register
        d3 = SIMD_SUBPD(one, d2);
        d3 = SIMD_MULPD(d3, coeff);
        d2 = SIMD_MULPD(d2, d3);

        // Multiply result of chaotic function by beta
        d3 = SIMD_MULPD(d2, beta);

        // Cast, XOR, and store
        r1 = SIMD_CVT64(d3);
        r2 = SIMD_LOADBITS((reg *) &in[i + 4]);
        r1 = SIMD_XORBITS(r2, r1);
        SIMD_STOREBITS((reg *) &out[i + 4], r1);
    } while ((i += 8) < BUF_SIZE);
#endif
#endif
}

void apply(u64 *restrict _ptr, u64 *restrict work_buffer, double *restrict chseeds)
{
    chaotic_iter(_ptr, &work_buffer[0], &chseeds[0]);
    chaotic_iter(&work_buffer[0], &work_buffer[BUF_SIZE], &chseeds[8]);
    chaotic_iter(&work_buffer[BUF_SIZE], _ptr, &chseeds[16]);
}

void mix(u64 *restrict _ptr, const u64 *restrict work_buffer)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    reg64q4 r1, r2, r3;
    do {
        r1 = SIMD_LOAD64x4(&_ptr[i]);
        r2 = SIMD_LOAD64x4(&work_buffer[i]);
        r3 = SIMD_LOAD64x4(&work_buffer[BUF_SIZE + i]);
        SIMD_3XOR4Q64(r1, r2, r3);
        SIMD_STORE64x4(&_ptr[i], r1);
    } while ((i += 8) < BUF_SIZE);
#else
    reg r1, r2;
    do {
        r1 = SIMD_LOADBITS((reg *) &_ptr[i]);
        r2 = SIMD_LOADBITS((reg *) &work_buffer[i]);
        r1 = SIMD_XORBITS(r1, r2);
        r2 = SIMD_LOADBITS((reg *) &work_buffer[BUF_SIZE + i]);
        r1 = SIMD_XORBITS(r1, r2);
        SIMD_STOREBITS((reg *) &_ptr[i], r1);
#ifndef __AVX512F__
        r1 = SIMD_LOADBITS((reg *) &_ptr[i + 4]);
        r2 = SIMD_LOADBITS((reg *) &work_buffer[i + 4]);
        r1 = SIMD_XORBITS(r1, r2);
        r2 = SIMD_LOADBITS((reg *) &work_buffer[BUF_SIZE + i + 4]);
        r1 = SIMD_XORBITS(r1, r2);
        SIMD_STOREBITS((reg *) &_ptr[i + 4], r1);
#endif
    } while ((i += 8) < BUF_SIZE);
#endif
}

void reseed(u64 *restrict seed, u64 *restrict work_buffer, u64 *restrict nonce, u64 *restrict cc)
{
    // clang-format off

    // Slightly modified macro from ISAAC for reseeding ADAM
    #define ISAAC_IND(mm, x) (*(u64 *) ((u8 *) (mm) + (2040 + (x & 2047))))

    *cc     += (*nonce >> (*nonce & 63));
    seed[0] ^= ~ISAAC_IND(work_buffer, seed[0]);
    seed[1] ^= ~ISAAC_IND(work_buffer, seed[1]);
    seed[2] ^= ~ISAAC_IND(work_buffer, seed[2]);
    seed[3] ^= ~ISAAC_IND(work_buffer, seed[3]);
    *nonce  ^= (*nonce + ISAAC_IND(work_buffer, *nonce));

    // clang-format on
}