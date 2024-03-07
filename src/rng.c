#include "../include/defs.h"
#include "../include/simd.h"
#include "../include/rng.h"

void accumulate(u64 *restrict seed, u64 *restrict IV, u64 *restrict work_buffer, double *restrict chseeds, const u64 cc)
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
    register u16 offset = cc & 255;
#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(DIV * LIMIT);

    dreg4q seeds;
    reg64q4 r1 = SIMD_LOAD64x4(&IV[0]);
    reg64q4 r2 = SIMD_LOAD64x4(&work_buffer[offset]);

    do {
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_CAST4QPD(seeds, r1);
        SIMD_MUL4QPD(seeds, seeds, range);
        SIMD_STORE4PD(&chseeds[i << 3], seeds);
        SIMD_ADD4RQ64(r1, r1, r2);
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

static void chaotic_iter(u64 *restrict in, u64 *restrict out, double *restrict chseeds)
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
    reg r1, r2, r3;
    do {
        r1 = SIMD_LOAD64((reg *) &_ptr[i]);
        r2 = SIMD_LOAD64((reg *) &buffer[i]);
        r3 = SIMD_LOAD64((reg *) &buffer[BUF_SIZE + i]);
        r1 = SIMD_XOR64(r1, r2);
        r1 = SIMD_XOR64(r1, r3);
        SIMD_STOREBITS((reg *) _ptr, r1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[i + 4]);
        r2 = SIMD_LOAD64((reg *) &buffer[i + 4]);
        r3 = SIMD_LOAD64((reg *) &buffer[BUF_SIZE + i + 4]);
        r1 = SIMD_XOR64(r1, r2);
        r1 = SIMD_XOR64(r1, r3);
        SIMD_STOREBITS((reg *) &_ptr[i + 4], r1);
#endif
        _ptr += 8;
        map_a += 8;
        map_b += 8;
    } while (map_a < BUF_SIZE);
#endif
}

void reseed(u64 *restrict seed, u64 *restrict work_buffer, u64 *restrict nonce, u64 *restrict cc)
{
    // clang-format off
    // Slightly modified macro from ISAAC for reseeding ADAM
    #define ISAAC_IND(mm, x) (*(u64 *) ((u8 *) (mm) + (2048 + (x & 1015))))

    cc      += (*nonce >> (*nonce & 15));
    seed[0] ^= ~ISAAC_IND(work_buffer, seed[0]);
    seed[1] ^= ~ISAAC_IND(work_buffer, seed[1]);
    seed[2] ^= ~ISAAC_IND(work_buffer, seed[2]);
    seed[3] ^= ~ISAAC_IND(work_buffer, seed[3]);
    *nonce  ^= ~ISAAC_IND(work_buffer, *nonce);
    // clang-format on
}