#include "../include/rng.h"
#include "../include/simd.h"

// clang-format off
// For diffusion - from https://burtleburtle.net/bob/c/isaac64.c
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

void initialize(const u64 *restrict seed, const u64 nonce, u64 *restrict out, u64 *restrict mix)
{
    /*
        8 64-bit IV's that correspond to the verse:
        "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"

        Mix IV's with different configurations of seed values
    */
    out[0] = 0x4265206672756974 ^ seed[0];
    out[1] = 0x66756C20616E6420 ^ ((seed[1] << (nonce & 63)) | (seed[3] >> (64 - (nonce & 63))));
    out[2] = 0x6D756C7469706C79 ^ seed[1];
    out[3] = 0x2C20616E64207265 ^ ((seed[0] << 32) | (seed[2] >> 32));
    out[4] = 0x706C656E69736820 ^ seed[2];
    out[5] = 0x7468652065617274 ^ ((seed[2] << (nonce & 63)) | (seed[0] >> (64 - (nonce & 63))));
    out[6] = 0x68202847656E6573 ^ seed[3];
    out[7] = 0x697320313A323829 ^ ((seed[3] << 32) | (seed[1] >> 32));

    // Initialize intermediate chaotic mix
    mix[0] = ~out[4] ^ nonce;
    mix[1] = ~out[5] ^ (nonce + 1);
    mix[2] = ~out[6] ^ (nonce + 2);
    mix[3] = ~out[7] ^ (nonce + 3);
    mix[4] = ~out[0] ^ (nonce + 4);
    mix[5] = ~out[1] ^ (nonce + 5);
    mix[6] = ~out[2] ^ (nonce + 6);
    mix[7] = ~out[3] ^ (nonce + 7);
}

void accumulate(u64 *restrict out, u64 *restrict mix, double *restrict chseeds)
{
    register u8 i = 0;

    // Scramble
    for (; i < ROUNDS; ++i) {
        ISAAC_MIX(mix[0], mix[1], mix[2], mix[3], mix[4], mix[5], mix[6], mix[7]);
    }

    i = 0;

#ifdef __AARCH64_SIMD__
    const reg64q4 mr = SIMD_LOAD64x4(mix);
    reg64q4 r1       = SIMD_LOAD64x4(out);
    reg64q4 r2;

    do {
        SIMD_XAR64RQ(r2, r1, mr, 32);
        SIMD_ADD4RQ64(r1, r1, r2);
    } while (++i < ROUNDS);

    const dregq range = SIMD_SETQPD(RANGE_LIMIT);
    dreg4q seeds;
    SIMD_CAST4QPD(seeds, r2);
    SIMD_MUL4QPD(seeds, seeds, range);
    SIMD_STORE4PD(chseeds, seeds);

    SIMD_STORE64x4(mix, r1);
#elif __AVX512F_
    const reg mr1 = SIMD_LOADBITS((reg *) mix)
        reg r1
        = SIMD_LOADBITS((reg *) out)
            reg r2;

    do {
        r2 = SIMD_XORBITS(mr, r1);
        r2 = SIMD_ROTR64(r2, 32)
            r1
            = SIMD_ADD64(r1, r2);
    } while (++i < ROUNDS);

    const regd range = SIMD_SETPD(RANGE_LIMIT);
    regd seeds       = SIMD_CVTPD(r2);
    seeds            = SIMD_MULPD(seeds, range);
    SIMD_STOREPD(chseeds, seeds);
    SIMD_STOREBITS((reg *) mix, r1);
#else
    const reg mr1 = SIMD_LOADBITS((reg *) mix);
    const reg mr2 = SIMD_LOADBITS((reg *) &mix[4]);

    reg r1 = SIMD_LOADBITS((reg *) out);
    reg r2 = SIMD_LOADBITS((reg *) &out[4]);

    reg r3, r4;

    do {
        r3 = SIMD_XORBITS(mr1, r1);
        r3 = SIMD_ORBITS(SIMD_RSHIFT64(r3, 32), SIMD_LSHIFT64(r3, 32));
        r1 = SIMD_ADD64(r1, r3);

        r4 = SIMD_XORBITS(mr2, r2);
        r4 = SIMD_ORBITS(SIMD_RSHIFT64(r4, 32), SIMD_LSHIFT64(r4, 32));
        r2 = SIMD_ADD64(r2, r4);
    } while (++i < ROUNDS);

    const regd range = SIMD_SETPD(RANGE_LIMIT);
    regd seeds       = SIMD_CVT64(r3);
    seeds            = SIMD_MULPD(seeds, range);
    SIMD_STOREPD(chseeds, seeds);

    seeds = SIMD_CVT64(r4);
    seeds = SIMD_MULPD(seeds, range);
    SIMD_STOREPD(&chseeds[4], seeds);

    SIMD_STOREBITS((reg *) mix, r3);
    SIMD_STOREBITS((reg *) &mix[4], r4);
#endif
}

void diffuse(u64 *restrict out, u64 *restrict mix, const u64 nonce)
{
    register u16 i = 0;

    do {
        mix[0] += nonce;
        mix[1] += nonce;
        mix[2] += nonce;
        mix[3] += nonce;
        mix[4] += nonce;
        mix[5] += nonce;
        mix[6] += nonce;
        mix[7] += nonce;

        ISAAC_MIX(mix[0], mix[1], mix[2], mix[3], mix[4], mix[5], mix[6], mix[7]);

        out[i + 0] = mix[0];
        out[i + 1] = mix[1];
        out[i + 2] = mix[2];
        out[i + 3] = mix[3];
        out[i + 4] = mix[4];
        out[i + 5] = mix[5];
        out[i + 6] = mix[6];
        out[i + 7] = mix[7];
    } while ((i += 8) < BUF_SIZE);
}

void apply(u64 *restrict out, double *restrict chseeds)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq one   = SIMD_SETQPD(1.0);
    const dregq coeff = SIMD_SETQPD(COEFFICIENT);
    const reg64q mask = SIMD_SETQ64(0xFFFFFFFFFFFFF); // 2^52 bits

    reg64q4 r1, r2;

    // Load 8 consecutive seeds at a time
    dreg4q d1 = SIMD_LOAD4PD(chseeds);
    dreg4q d2;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        SIMD_SUB4QPD(d2, one, d1);
        SIMD_MUL4QPD(d2, d2, coeff);
        SIMD_MUL4RQPD(d1, d1, d2);

        // Load data at current offset
        r1 = SIMD_LOAD64x4(&out[i]);

        // Reinterpret results of d1 as binary floating point
        // Add the mantissa value to r1
        SIMD_REINTERP_ADD64(r2, d1, r1);

        // Store
        SIMD_STORE64x4(&out[i], r1);
    } while ((i += 8) < BUF_SIZE);

    SIMD_STORE4PD(chseeds, d1);
#else
    const regd one   = SIMD_SETPD(1.0);
    const regd coeff = SIMD_SETPD(COEFFICIENT);
    const reg mask   = SIMD_SET64(0xFFFFFFFFFFFFF);

    regd d1 = SIMD_LOADPD(chseeds);
    reg r1;

#ifdef __AVX512F__
    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d1 = SIMD_SUBPD(one, d1);
        d1 = SIMD_MULPD(d1, coeff);
        d1 = SIMD_MULPD(d1, d1);

        // Load data at current offset
        r1 = SIMD_LOADBITS((reg *) &out[i])

            // Reinterpret results of d1 as binary floating point
            // Add the mantissa value to r1
            r1
            = SIMD_ADD64(r1, SIMD_ANDBITS(SIMD_CVT64(d2), mask));

        // Store
        SIMD_STOREBITS((reg *) &out[i], r1);
    } while ((i += 8) < BUF_SIZE);

    SIMD_STOREPD(chseeds, d1);
#else
    reg r2;
    regd d2 = SIMD_LOADPD(&chseeds[4]);

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        d1 = SIMD_SUBPD(one, d1);
        d1 = SIMD_MULPD(d1, coeff);
        d1 = SIMD_MULPD(d1, d1);
        d2 = SIMD_SUBPD(one, d2);
        d2 = SIMD_MULPD(d2, coeff);
        d2 = SIMD_MULPD(d2, d2);

        // Load data at current offset
        r1 = SIMD_LOADBITS((reg *) &out[i]);
        r2 = SIMD_LOADBITS((reg *) &out[i + 4]);

        // Reinterpret results of d1 as binary floating point
        // Add the mantissa value to r1
        r1 = SIMD_ADD64(r1, SIMD_ANDBITS(SIMD_CVTPD(d1), mask));
        r2 = SIMD_ADD64(r2, SIMD_ANDBITS(SIMD_CVTPD(d2), mask));

        SIMD_STOREBITS((reg *) &out[i], r1);
        SIMD_STOREBITS((reg *) &out[i + 4], r2);
    } while ((i += 8) < BUF_SIZE);

    SIMD_STOREPD(chseeds, d1);
    SIMD_STOREPD(&chseeds[4], d2);
#endif
#endif
}

void mix(u64 *restrict out, const u64 *restrict mix)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    reg64q4 mr = SIMD_LOAD64x4(mix);
    reg64q4 r1, r2;

    do {
        r1 = SIMD_LOAD64x4(&out[i]);
        SIMD_XAR64RQ(r2, r1, mr, 32);
        SIMD_ADD4RQ64(mr, mr, r1);
        SIMD_3XOR4Q64(r1, r2, mr);
        SIMD_STORE64x4(&out[i], r1);
    } while ((i += 8) < BUF_SIZE);
#else
    reg mr = SIMD_LOADBITS((reg *) mix);

    reg r1, r2;

    do {
        r1 = SIMD_LOADBITS((reg *) &out[i]);
        r2 = SIMD_XORBITS(r1, mr);
        r2 = SIMD_ORBITS(SIMD_RSHIFT64(r2, 32), SIMD_LSHIFT64(r2, 32));
        mr = SIMD_ADD64(mr, r1);
        r1 = SIMD_XORBITS(r1, r2);
        r1 = SIMD_XORBITS(r1, mr);
        SIMD_STOREBITS((reg *) &out[i], r1);
#ifndef __AVX512F__
        r1 = SIMD_LOADBITS((reg *) &out[i + 4]);
        r2 = SIMD_XORBITS(r1, mr);
        r2 = SIMD_ORBITS(SIMD_LSHIFT64(r2, 32), SIMD_RSHIFT64(r2, 32));
        mr = SIMD_ADD64(mr, r1);
        r1 = SIMD_XORBITS(r1, r2);
        r1 = SIMD_XORBITS(r1, mr);
        SIMD_STOREBITS((reg *) &out[i + 4], r1);
#endif
    } while ((i += 8) < BUF_SIZE);
#endif
}

u64 generate(u64 *restrict out, u8 *restrict idx, double *restrict chseeds)
{
    chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);

    const u64 m = (u64) CHMANT32(*idx, chseeds) << 32;

    chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);

    const u8 offset = out[*idx] & 0x7F;
    const u8 a      = *idx + 1 + offset;
    const u8 b      = *idx + 128 + offset;

    const u64 num = out[a] ^ out[b] ^ (m | CHMANT32(*idx, chseeds));
    out[*idx] ^= out[a] ^ out[b];

    *idx += 1;

    return num;
}

/*     ALGORITHM END     */
