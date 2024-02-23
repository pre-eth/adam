#include "../include/rng.h"
#include "../include/defs.h"
#include "../include/simd.h"

// To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
#define DIV   5.4210109E-20
#define LIMIT 0.5

// For diffusing the buffer
#define ISAAC_MIX(a, b, c, d, e, f, g, h) \
    {                                     \
        a -= e;                           \
        f ^= h >> 9;                      \
        h += a;                           \
        b -= f;                           \
        g ^= a << 9;                      \
        a += b;                           \
        c -= g;                           \
        h ^= b >> 23;                     \
        b += c;                           \
        d -= h;                           \
        a ^= c << 15;                     \
        c += d;                           \
        e -= a;                           \
        b ^= d >> 14;                     \
        d += e;                           \
        f -= b;                           \
        c ^= e << 20;                     \
        e += f;                           \
        g -= c;                           \
        d ^= f >> 17;                     \
        f += g;                           \
        h -= d;                           \
        e ^= g << 14;                     \
        g += h;                           \
    }

// Slightly modified macro from ISAAC for reseeding ADAM
#define ISAAC_IND(mm, x) (*(u64 *) ((u8 *) (mm) + (4096 + (x & 2047))))
#define ISAAC_RNGSTEP(mx, a, b, mm, m, m2, x, y)                         \
    {                                                                    \
        x      = (m << 24) | (~((m >> 40) ^ __UINT64_MAX__) & 0xFFFFFF); \
        a      = (a ^ (mx)) + m2;                                        \
        m      = (ISAAC_IND(mm, x) + a + b);                             \
        y ^= b = ISAAC_IND(mm, y >> MAGNITUDE) + x;                      \
    }

/*
  The algorithm requires at least the construction of 3 maps of size BUF_SIZE
  Offsets logically represent each individual map, but it's all one buffer.

  This was originally implemented as an array of sizeof(u64) * BUF_SIZE * 3
  bytes, but further optimization and testing found that the security and
  statistical qualities were still preserved so long as the core algorithm
  was not changed, meaning a slightyly modified approach with an extra
  chaotic iteration allowed me to reduce the size of the buffer to just
  sizeof(u64) * 264, where the three "maps" are now logically represented as
  three subsections in the buffer, each of size 88 u64.

  static is important because otherwise buffer is initiated with junk that
  removes the deterministic nature of the algorithm
*/
static unsigned long long buffer[BUF_SIZE * 3] ALIGN(SIMD_LEN);

// Seeds supplied to each iteration of the chaotic function
// Per each round, 8 consecutive chseeds are supplied
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

static void accumulate(u64 *seed)
{
    IV[0] ^= seed[0];
    IV[1] ^= seed[1];
    IV[2] ^= seed[2];
    IV[3] ^= seed[3];
    IV[4] ^= seed[0];
    IV[5] ^= seed[1];
    IV[6] ^= seed[2];
    IV[7] ^= seed[3];

    register u8 i       = 0;
    register u16 offset = BUF_SIZE + (buffer[767] & 0xFF);

#ifdef __AARCH64_SIMD__
    reg64q4 r1 = SIMD_LOAD64x4(&IV[0]);
    reg64q4 r2;

    const dregq range = SIMD_SETQPD(DIV * LIMIT);
    dreg4q seeds;

    do {
        SIMD_CAST4QPD(seeds, r1);
        SIMD_MUL4QPD(seeds, seeds, range);
        SIMD_STORE4PD(&chseeds[i << 3], seeds);
        r2 = SIMD_LOAD64x4(&buffer[offset]);
        SIMD_ADD4RQ64(r1, r1, r2);
        offset += 8;
    } while (++i < (ROUNDS / 2));

    SIMD_STORE64x4(&IV[0], r2);
#else
    const regd factor = SIMD_SETPD(DIV * LIMIT);

    reg r1 = SIMD_LOADBITS(&IV[0]);
    reg r2 = SIMD_LOADBITS(&buffer[BUF_SIZE]);

    do {
        r2 = SIMD_ADD64(r1, r2);
        // shift r1 left by 1, then XOR r1 and r2, then store
        // repeat twice if needed for AVX2

        SIMD_STOREPD((reg *) &chseeds[(i << 2)], s1);
        SIMD_STOREPD((reg *) &chseeds[(i << 2) + 4], s2);
    } while (++i < (ROUNDS >> 1));
#endif
}

static void diffuse(u64 nonce)
{
    u64 a, b, c, d, e, f, g, h;
    a = b = c = d = e = f = g = h = (nonce << 32) | ~(nonce >> 32);

    // Scramble
    register u16 i = 0;
    for (; i < 4; ++i)
        ISAAC_MIX(a, b, c, d, e, f, g, h);

    i = 0;
    do {
        a += buffer[i];
        b += buffer[i + 1];
        c += buffer[i + 2];
        d += buffer[i + 3];
        e += buffer[i + 4];
        f += buffer[i + 5];
        g += buffer[i + 6];
        h += buffer[i + 7];

        ISAAC_MIX(a, b, c, d, e, f, g, h);

        buffer[i]     = a;
        buffer[i + 1] = b;
        buffer[i + 2] = c;
        buffer[i + 3] = d;
        buffer[i + 4] = e;
        buffer[i + 5] = f;
        buffer[i + 6] = g;
        buffer[i + 7] = h;
    } while ((i += 8) < BUF_SIZE);
}

static void chaotic_iter(u16 idx, u16 start, const u8 rounds)
{
    const u16 limit = idx + BUF_SIZE;

#ifdef __AARCH64_SIMD__
    const dregq one   = SIMD_SETQPD(1.0);
    const dregq coeff = SIMD_SETQPD(COEFFICIENT);
    const dregq beta  = SIMD_SETQPD(BETA);

    reg64q4 r1, r2;

    // Load 8 seeds at a time
    dreg4q d1 = SIMD_LOAD4PD(&chseeds[rounds]);
    dreg4q d2;

    do {
        // 3.9999 * X * (1 - X) for all X in the register
        SIMD_SUB4QPD(d2, one, d1);
        SIMD_MUL4QPD(d2, d2, coeff);
        SIMD_MUL4RQPD(d1, d1, d2);

        // Multiply result of chaotic function by BETA
        SIMD_MUL4QPD(d2, d1, beta);

        // Cast, XOR, and store
        SIMD_CAST4Q64(r1, d2);
        r2 = SIMD_LOAD64x4(&buffer[idx]);
        SIMD_XOR4RQ64(r1, r1, r2);
        SIMD_STORE64x4(&buffer[start], r1);
        start += 8;
    } while ((idx += 8) < limit);
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

static void apply()
{
    chaotic_iter(0, 256, 0);
    chaotic_iter(256, 512, 8);
    chaotic_iter(512, 0, 16);
}

static void mix()
{
    const u16 limit = BUF_SIZE;

    u16 dest  = 0;
    u16 map_a = BUF_SIZE;
    u16 map_b = BUF_SIZE << 1;

#ifdef __AARCH64_SIMD__
    reg64q4 r1, r2, r3;
    do {
        r1 = SIMD_LOAD64x4(&buffer[dest]);
        r2 = SIMD_LOAD64x4(&buffer[map_a]);
        r3 = SIMD_LOAD64x4(&buffer[map_b]);
        SIMD_3XOR4Q64(r1, r2, r3);
        SIMD_STORE64x4(&buffer[dest], r1);
        dest += 8;
        map_a += 8;
        map_b += 8;
    } while (dest < limit);
#else
#define XOR_MAPS(i) buffer[0 + i] ^ buffer[0 + i + 256] ^ buffer[0 + i + 512], \
                    buffer[1 + i] ^ buffer[1 + i + 256] ^ buffer[1 + i + 512], \
                    buffer[2 + i] ^ buffer[2 + i + 256] ^ buffer[2 + i + 512], \
                    buffer[3 + i] ^ buffer[3 + i + 256] ^ buffer[3 + i + 512]

    reg r1, r2;

    do {
        r1 = SIMD_SETR64(XOR_MAPS(i)
#ifdef __AVX512F__
                             ,
            XOR_MAPS(i + 4)
#endif
        );

        r2 = SIMD_SETR64(XOR_MAPS(i + (SIMD_LEN >> 3))
#ifdef __AVX512F__
                             ,
            XOR_MAPS(i + (SIMD_LEN >> 3) + 4)
#endif
        );

        SIMD_STOREBITS((reg *) &buffer[i], r1);
        SIMD_STOREBITS((reg *) &buffer[i + (SIMD_LEN >> 3)], r2);

        i += (SIMD_LEN >> 2);
    } while (i < BUF_SIZE);
#endif
}

static void reseed(u64 *seed, u64 *nonce)
{
    // static u8 byte_idx;

    // const u8 next = byte_idx + !(byte_idx & 32) - (byte_idx & 32);

    // ++seed[byte_idx];
    // seed[next] += !seed[byte_idx];
    // byte_idx += (!seed[byte_idx] & !(byte_idx & 32)) - (byte_idx & 32) * !seed[byte_idx];

    seed[0] ^= ISAAC_IND(buffer, seed[0]);
    seed[1] ^= ISAAC_IND(buffer, seed[1]);
    seed[2] ^= ISAAC_IND(buffer, seed[2]);
    seed[3] ^= ISAAC_IND(buffer, seed[3]);
    *nonce = ISAAC_IND(buffer, *nonce) + 1;
}

void adam_run(unsigned long long *seed, unsigned long long *nonce)
{
    accumulate(seed);
    diffuse(*nonce);
    apply();
    mix();
    reseed(seed, nonce);
}
{
    adam_run(seed, nonce);
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);

    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&buffer[i]);
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
        r1 = SIMD_LOAD64((reg *) &buffer[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &buffer[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

static void dbl_simd_fill_mult(double *buf, u64 *seed, u64 *nonce, u16 amount, const u64 multiplier)
{
    adam_run(seed, nonce);
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);
    const dregq mult  = SIMD_SETQPD((double) multiplier);
    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&buffer[i]);
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
        r1 = SIMD_LOAD64((reg *) &buffer[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &buffer[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

void adam_run(unsigned long long *seed, unsigned long long *nonce)
{
    register u64 _nonce = *nonce;
    cc += _nonce ^ (seed[~_nonce & 3] ^ buffer[256 + (cc & 7)]);

    accumulate(seed);
    diffuse(_nonce);
    apply(0, DET(cc, 0));
    mix(0, 88, 176);
    apply(88, DET(cc, 1));
    mix(88, 0, 176);
    apply(176, DET(cc, 2));
    mix(176, 0, 88);
    apply(0, DET(cc, 3));

    ISAAC_RNGSTEP(~(aa ^ (aa << 21)), aa, bb, buffer, buffer[256], buffer[260],
        seed[0], _nonce);
    ISAAC_RNGSTEP(aa ^ (aa >> 5), aa, bb, buffer, buffer[257], buffer[261],
        seed[1], _nonce);
    ISAAC_RNGSTEP(aa ^ (aa << 12), aa, bb, buffer, buffer[258], buffer[262],
        seed[2], _nonce);
    ISAAC_RNGSTEP(aa ^ (aa >> 33), aa, bb, buffer, buffer[259], buffer[263],
        seed[3], _nonce);

    *nonce = _nonce;
}

void adam_frun(unsigned long long *seed, unsigned long long *nonce, double *buf, const unsigned int amount)
{
    const u8 leftovers = amount & 7;
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        tmp = amount - count;
        out = ((tmp > BUF_SIZE) << 8) | (!(tmp > BUF_SIZE) * tmp);
        dbl_simd_fill(&buf[count], seed, nonce, out);
        count += out;
    }

    if (LIKELY(amount & 7)) {
        adam_run(seed, nonce);
        register u8 i = 0;
        do {
            buf[count] = (double) buffer[i++] / (double) __UINT64_MAX__;
        } while (++count < amount);
    }
}

void adam_fmrun(unsigned long long *seed, unsigned long long *nonce, double *buf, const unsigned int amount, const unsigned long long multiplier)
{
    const u8 leftovers = amount & 7;
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        tmp = amount - count;
        out = ((tmp >= BUF_SIZE) << 8) | (!(tmp >= BUF_SIZE) * tmp);
        dbl_simd_fill_mult(&buf[count], seed, nonce, out, multiplier);
        count += out;
    }

    if (LIKELY(amount & 7)) {
        adam_run(seed, nonce);
        register u8 i     = 0;
        const double mult = (double) multiplier;
        do {
            buf[count] = ((double) buffer[i++] / (double) __UINT64_MAX__) * mult;
        } while (++count < amount);
    }
}

void adam_connect(unsigned long long **_ptr, double **_chptr)
{
    *_ptr = &buffer[0];

    if (_chptr != (void *) 0)
        *_chptr = &chseeds[0];
}
