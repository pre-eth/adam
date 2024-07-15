#include "../include/simd.h"    
#include "../include/rng.h"

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
    const dregq range = SIMD_SETQPD(RANGE_LIMIT);
    dreg4q seeds;
    
    const reg64q4 mr = SIMD_LOAD64x4(mix); 
    reg64q4 r1 = SIMD_LOAD64x4(out);
    reg64q4 r2;

    do {
        SIMD_XAR64RQ(r2, r1, mr, 32);
        SIMD_ADD4RQ64(r1, r1, r2);
    } while (++i < ROUNDS);

    SIMD_CAST4QPD(seeds, r2);
    SIMD_MUL4QPD(seeds, seeds, range);
    SIMD_STORE4PD(chseeds, seeds);
    
    SIMD_STORE64x4(mix, r1);
#else
#ifdef __AVX512F__
    const __m256d range = SIMD_SETPD(RANGE_LIMIT);

    __m256d d1;
    const __m256i r1 = SIMD_LOADBITS((__m256i *) mix);
    __m256i r2 = SIMD_LOADBITS((__m256i *) out);

    do {
        r1 = SIMD_ROTR64(r1, 8);
        r1 = SIMD_XORBITS(r1, r2);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&chseeds[i << 3], d1);
    } while (++i < ROUNDS);

    SIMD_STOREBITS((reg *) arr, r1);
#else
    register a, b, c, d, e, f, g, h;

    a = out[0];
    b = out[1];
    c = out[2];
    d = out[3];
    e = out[4];
    f = out[5];
    g = out[6];
    h = out[7];

    do {
        a = _rotr64(a, 8) ^ arr[0];
        b = _rotr64(b, 8) ^ arr[1];
        c = _rotr64(c, 8) ^ arr[2];
        d = _rotr64(d, 8) ^ arr[3];
        e = _rotr64(e, 8) ^ arr[4];
        f = _rotr64(f, 8) ^ arr[5];
        g = _rotr64(g, 8) ^ arr[6];
        h = _rotr64(h, 8) ^ arr[7];

        chseeds[i + 0] = (double) a * RANGE_LIMIT;
        chseeds[i + 1] = (double) b * RANGE_LIMIT;
        chseeds[i + 2] = (double) c * RANGE_LIMIT;
        chseeds[i + 3] = (double) d * RANGE_LIMIT;
        chseeds[i + 4] = (double) e * RANGE_LIMIT;
        chseeds[i + 5] = (double) f * RANGE_LIMIT;
        chseeds[i + 6] = (double) g * RANGE_LIMIT;
        chseeds[i + 7] = (double) h * RANGE_LIMIT;
    } while ((i += 8) < (ROUNDS << 2));

    arr[0] = a;
    arr[1] = b;
    arr[2] = c;
    arr[3] = d;
    arr[4] = e;
    arr[5] = f;
    arr[6] = g;
    arr[7] = h;
#endif
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
    const dregq mask = SIMD_SETQPD(0xFFFFFFFFFFFFF); // 2^52 bits

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

        XOR_ASSIGN(in, out, arr, 0);
        XOR_ASSIGN(in, out, arr, 1);
        XOR_ASSIGN(in, out, arr, 2);
        XOR_ASSIGN(in, out, arr, 3);
        XOR_ASSIGN(in, out, arr, 4);
        XOR_ASSIGN(in, out, arr, 5);
        XOR_ASSIGN(in, out, arr, 6);
        XOR_ASSIGN(in, out, arr, 7);
    } while ((i += 8) < 8);
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

        XOR_ASSIGN(in, out, arr, 0);
        XOR_ASSIGN(in, out, arr, 1);
        XOR_ASSIGN(in, out, arr, 2);
        XOR_ASSIGN(in, out, arr, 3);
        XOR_ASSIGN(in, out, arr, 4);
        XOR_ASSIGN(in, out, arr, 5);
        XOR_ASSIGN(in, out, arr, 6);
        XOR_ASSIGN(in, out, arr, 7);
    } while ((i += 8) < BUF_SIZE);
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
    const u64 elem = out[*idx];

    chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);
    
    const u64 m = (u64) CHMANT32(*idx, chseeds) << 32;

    chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);
   
    const u8 a = *idx + 1 + (elem & 0x7F);
    const u8 b = *idx + 128 + (elem & 0x7F);

    const u64 num = out[a] ^ out[b] ^ (m | CHMANT32(*idx, chseeds));

    out[*idx] ^= out[a] ^ out[b];

    *idx += 1;

    return num;
}

/*     ALGORITHM END     */
