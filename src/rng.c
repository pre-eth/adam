#include "../include/rng.h"

#if !defined(__AARCH64_SIMD__) && !defined(__AVX512F__)

// Following code found on https://stackoverflow.com/a/41148578
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

// Remaining filler intrinsics code found on https://stackoverflow.com/a/77376595

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
    d1                      = SIMD_ROUNDPD(d1, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
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

// clang-format off
// For diffusing the buffer - from https://burtleburtle.net/bob/c/isaac64.c
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
// clang-format on


/*     ALGORITHM START     */

void accumulate(u64 *restrict out, u64 *arr, double *restrict chseeds)
{
    register u8 i = 0;

    arr[0] = ~out[4];
    arr[1] = ~out[5];
    arr[2] = ~out[6];
    arr[3] = ~out[7];
    arr[4] = ~out[0];
    arr[5] = ~out[1];
    arr[6] = ~out[2];
    arr[7] = ~out[3];

    // Scramble
    for (; i < 4; ++i) {
        ISAAC_MIX(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7]);
    }
    
    i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(RANGE_LIMIT);

    dreg4q seeds;
    reg64q4 r1 = SIMD_LOAD64x4(&out[0]);
    reg64q4 r2 = SIMD_LOAD64x4(&arr[0]);
    do {
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_ADD4RQ64(r2, r2, r1);
        SIMD_CAST4QPD(seeds, r1);
        SIMD_MUL4QPD(seeds, seeds, range);
        SIMD_STORE4PD(&chseeds[i << 3], seeds);
    } while (++i < (ROUNDS / 2));

    SIMD_STORE64x4(arr, r2);
#else
    const regd range = SIMD_SETPD(RANGE_LIMIT);

    regd d1;
    reg r1 = SIMD_LOADBITS((reg *) &out[0]);
#ifdef __AVX512F__
    reg r2 = SIMD_LOADBITS((reg *) &arr[0]);

    do {
        r1 = SIMD_XORBITS(r1, r2);
        r2 = SIMD_ADD64(r1, r2);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i << 3], d1);
    } while (++i < (ROUNDS / 2));

    SIMD_STOREBITS((reg *) &arr[0], r2);
#else
    reg r2 = SIMD_LOADBITS((reg *) &out[4]);

    reg r3 = SIMD_LOADBITS((reg *) &arr[0]);
    reg r4 = SIMD_LOADBITS((reg *) &arr[4]);

    do {
        r1 = SIMD_XORBITS(r1, r3);
        r3 = SIMD_ADD64(r1, r3);
        d1 = SIMD_CVTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i], d1);

        r2 = SIMD_XORBITS(r2, r4);
        r4 = SIMD_ADD64(r2, r4);
        d1 = SIMD_CVTPD(r2);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i + 4], d1);
    } while ((i += 8) < (ROUNDS << 2));

    SIMD_STOREBITS((reg *) &arr[0], r3);
    SIMD_STOREBITS((reg *) &arr[4], r4);
#endif
#endif
}

void diffuse(u64 *restrict out, const u64 amount, u64 *restrict mix)
{
    register u16 i = 0;

    do {
        mix[0] += out[i + 0];
        mix[1] += out[i + 1];
        mix[2] += out[i + 2];
        mix[3] += out[i + 3];
        mix[4] += out[i + 4];
        mix[5] += out[i + 5];
        mix[6] += out[i + 6];
        mix[7] += out[i + 7];

        ISAAC_MIX(mix[0], mix[1], mix[2], mix[3], mix[4], mix[5], mix[6], mix[7]);

        out[i + 0] = mix[0];
        out[i + 1] = mix[1];
        out[i + 2] = mix[2];
        out[i + 3] = mix[3];
        out[i + 4] = mix[4];
        out[i + 5] = mix[5];
        out[i + 6] = mix[6];
        out[i + 7] = mix[7];
    } while ((i += 8) < amount);
}

static inline void chaotic_iter(u64 *restrict in, u64 *restrict out, double *restrict chseeds, u64 *restrict arr)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq one   = SIMD_SETQPD(1.0);
    const dregq coeff = SIMD_SETQPD(COEFFICIENT);
    const dregq beta  = SIMD_SETQPD(BETA);

    reg64q4 r1;

    // Load 8 consecutive seeds at a time
    dreg4q d1 = SIMD_LOAD4PD(chseeds);
    dreg4q d2;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        SIMD_SUB4QPD(d2, one, d1);
        SIMD_MUL4QPD(d2, d2, coeff);
        SIMD_MUL4RQPD(d1, d1, d2);

        // Multiply chaotic result by BETA to obtain ints
        SIMD_MUL4QPD(d2, d1, beta);

        // Cast, XOR, and store
        SIMD_CAST4Q64(r1, d2);
        SIMD_STORE64x4(&arr[0], r1);

        in[arr[0] & 0xFF] ^= out[i + 0] ^= in[arr[0] & 0xFF];
        in[arr[1] & 0xFF] ^= out[i + 1] ^= in[arr[1] & 0xFF];
        in[arr[2] & 0xFF] ^= out[i + 2] ^= in[arr[2] & 0xFF];
        in[arr[3] & 0xFF] ^= out[i + 3] ^= in[arr[3] & 0xFF];
        in[arr[4] & 0xFF] ^= out[i + 4] ^= in[arr[4] & 0xFF];
        in[arr[5] & 0xFF] ^= out[i + 5] ^= in[arr[5] & 0xFF];
        in[arr[6] & 0xFF] ^= out[i + 6] ^= in[arr[6] & 0xFF];
        in[arr[7] & 0xFF] ^= out[i + 7] ^= in[arr[7] & 0xFF];
    } while ((i += 8) < BUF_SIZE);

    SIMD_STORE4PD(chseeds, d1);
#else
    const regd one   = SIMD_SETPD(1.0);
    const regd coeff = SIMD_SETPD(COEFFICIENT);
    const regd beta  = SIMD_SETPD(BETA);

    regd d1 = SIMD_LOADPD(chseeds);
    regd d2;
    reg r1;

#ifdef __AVX512F__
    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d2 = SIMD_SUBPD(one, d1);
        d2 = SIMD_MULPD(d2, coeff);
        d1 = SIMD_MULPD(d1, d2);

        // Multiply result of chaotic function by beta
        d2 = SIMD_MULPD(d1, beta);

        // Cast, XOR, and store
        r1 = SIMD_CASTPD(d2);
        SIMD_STOREBITS((reg *) &arr[0], r1);

        in[arr[0] & 0xFF] ^= out[i + 0] ^= in[arr[0] & 0xFF];
        in[arr[1] & 0xFF] ^= out[i + 1] ^= in[arr[1] & 0xFF];
        in[arr[2] & 0xFF] ^= out[i + 2] ^= in[arr[2] & 0xFF];
        in[arr[3] & 0xFF] ^= out[i + 3] ^= in[arr[3] & 0xFF];
        in[arr[4] & 0xFF] ^= out[i + 4] ^= in[arr[4] & 0xFF];
        in[arr[5] & 0xFF] ^= out[i + 5] ^= in[arr[5] & 0xFF];
        in[arr[6] & 0xFF] ^= out[i + 6] ^= in[arr[6] & 0xFF];
        in[arr[7] & 0xFF] ^= out[i + 7] ^= in[arr[7] & 0xFF];
    } while ((i += 8) < BUF_SIZE);

    SIMD_STOREPD(chseeds, d1);
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
        SIMD_STOREBITS((reg *) &arr[0], r1);

        // One more run since AVX2 can only do 4 values at a
        // time but we need 8 per iteration
        d3 = SIMD_SUBPD(one, d2);
        d3 = SIMD_MULPD(d3, coeff);
        d2 = SIMD_MULPD(d2, d3);
        d3 = SIMD_MULPD(d2, beta);
        r1 = SIMD_CVT64(d3);
        SIMD_STOREBITS((reg *) &arr[4], r1);

        in[arr[0] & 0xFF] ^= out[i + 0] ^= in[arr[0] & 0xFF];
        in[arr[1] & 0xFF] ^= out[i + 1] ^= in[arr[1] & 0xFF];
        in[arr[2] & 0xFF] ^= out[i + 2] ^= in[arr[2] & 0xFF];
        in[arr[3] & 0xFF] ^= out[i + 3] ^= in[arr[3] & 0xFF];
        in[arr[4] & 0xFF] ^= out[i + 4] ^= in[arr[4] & 0xFF];
        in[arr[5] & 0xFF] ^= out[i + 5] ^= in[arr[5] & 0xFF];
        in[arr[6] & 0xFF] ^= out[i + 6] ^= in[arr[6] & 0xFF];
        in[arr[7] & 0xFF] ^= out[i + 7] ^= in[arr[7] & 0xFF];
    } while ((i += 8) < BUF_SIZE);

    SIMD_STOREPD(chseeds, d1);
    SIMD_STOREPD(&chseeds[4], d2);
#endif
#endif
}

void apply(u64 *restrict out, u64 *restrict state_maps, double *restrict chseeds, u64 *restrict arr)
{
    chaotic_iter(out, &state_maps[0], &chseeds[0], arr);
    chaotic_iter(&state_maps[0], &state_maps[BUF_SIZE], &chseeds[8], arr);
    chaotic_iter(&state_maps[BUF_SIZE], out, &chseeds[16], arr);
}

void mix(u64 *restrict out, const u64 *restrict state_maps)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    reg64q4 r1, r2, r3;
    do {
        r1 = SIMD_LOAD64x4(&out[i]);
        r2 = SIMD_LOAD64x4(&state_maps[i]);
        r3 = SIMD_LOAD64x4(&state_maps[BUF_SIZE + i]);
        SIMD_3XOR4Q64(r1, r2, r3);
        SIMD_STORE64x4(&out[i], r1);
    } while ((i += 8) < BUF_SIZE);
#else
    reg r1, r2;
    do {
        r1 = SIMD_LOADBITS((reg *) &out[i]);
        r2 = SIMD_LOADBITS((reg *) &state_maps[i]);
        r1 = SIMD_XORBITS(r1, r2);
        r2 = SIMD_LOADBITS((reg *) &state_maps[BUF_SIZE + i]);
        r1 = SIMD_XORBITS(r1, r2);
        SIMD_STOREBITS((reg *) &out[i], r1);
#ifndef __AVX512F__
        r1 = SIMD_LOADBITS((reg *) &out[i + 4]);
        r2 = SIMD_LOADBITS((reg *) &state_maps[i + 4]);
        r1 = SIMD_XORBITS(r1, r2);
        r2 = SIMD_LOADBITS((reg *) &state_maps[BUF_SIZE + i + 4]);
        r1 = SIMD_XORBITS(r1, r2);
        SIMD_STOREBITS((reg *) &out[i + 4], r1);
#endif
    } while ((i += 8) < BUF_SIZE);
#endif
}

/*     ALGORITHM END     */