#include "../include/rng.h"
#include "../include/defs.h"
#include "../include/simd.h"

// To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random double D
#define DIV 5.4210109E-20
#define LIMIT 0.5

#define ISAAC_MIX(a, b, c, d, e, f, g, h) \
  {                                       \
    a -= e;                               \
    f ^= h >> 9;                          \
    h += a;                               \
    b -= f;                               \
    g ^= a << 9;                          \
    a += b;                               \
    c -= g;                               \
    h ^= b >> 23;                         \
    b += c;                               \
    d -= h;                               \
    a ^= c << 15;                         \
    c += d;                               \
    e -= a;                               \
    b ^= d >> 14;                         \
    d += e;                               \
    f -= b;                               \
    c ^= e << 20;                         \
    e += f;                               \
    g -= c;                               \
    d ^= f >> 17;                         \
    f += g;                               \
    h -= d;                               \
    e ^= g << 14;                         \
    g += h;                               \
  }

// Slightly modified versions of macros from ISAAC for reseeding ADAM
#define ISAAC_IND(mm, x) (*(u64 *)((u8 *)(mm) + ((x) % 2040)))
#define ISAAC_RNGSTEP(mx, a, b, mm, m, m2, x, y)                \
  {                                                             \
    x = (m << 24) | (~((m >> 40) ^ __UINT64_MAX__) & 0xFFFFFF); \
    a = (a ^ (mx)) + m2;                                        \
    m = (ISAAC_IND(mm, x) + a + b);                             \
    y ^= b = ISAAC_IND(mm, y >> MAGNITUDE) + x;                 \
  }

#ifndef __AARCH64_SIMD__
#define XOR_MAPS(i) buffer[0 + i] ^ buffer[0 + i + 256] ^ buffer[0 + i + 512], \
                    buffer[1 + i] ^ buffer[1 + i + 256] ^ buffer[1 + i + 512], \
                    buffer[2 + i] ^ buffer[2 + i + 256] ^ buffer[2 + i + 512], \
                    buffer[3 + i] ^ buffer[3 + i + 256] ^ buffer[3 + i + 512]
#endif

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
static u64 buffer[BUF_SIZE + 8] ALIGN(SIMD_LEN);

// Seeds supplied to each iteration of the chaotic function
// Per each round, 8 consecutive chseeds are supplied
static double chseeds[ROUNDS << 2] ALIGN(SIMD_LEN);

// State variables
static u64 aa, bb;

#ifdef __AARCH64_SIMD__
static void accumulate(const u64 *seed)
{
  /*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */
  u64 IV[8] ALIGN(SIMD_LEN) = {
    0x4265206672756974ULL ^ seed[2], 0x66756C20616E6420ULL ^ ~seed[1],
    0x6D756C7469706C79ULL ^ seed[1], 0x2C20616E64207265ULL ^ ~seed[3],
    0x706C656E69736820ULL ^ seed[3], 0x7468652065617274ULL ^ ~seed[0],
    0x68202847656E6573ULL ^ seed[0], 0x697320313A323829ULL ^ ~seed[2]
  };

  reg64q4 r1 = SIMD_LOAD64x4(&IV[0]);
  reg64q4 r2 = SIMD_LOAD64x4(&buffer[256]);

  const dregq range = SIMD_SETQPD(DIV * LIMIT);
  dreg4q seeds;

  register u8 i = 0;
  do {
    SIMD_ADD4RQ64(r2, r1, r2);

    r1.val[0] = vrax1q_u64(r1.val[0], r2.val[0]);
    r1.val[1] = vrax1q_u64(r1.val[1], r2.val[1]);
    r1.val[2] = vrax1q_u64(r1.val[2], r2.val[2]);
    r1.val[3] = vrax1q_u64(r1.val[3], r2.val[3]);
    SIMD_CAST4QPD(seeds, r1);
    SIMD_MUL4QPD(seeds, seeds, range);
    SIMD_STORE4PD(&chseeds[i << 3], seeds);
  } while (++i < (ROUNDS >> 1));
}
#else
static void accumulate(const u64 *seed)
{
  /*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */
  u64 IV[8] ALIGN(SIMD_LEN) = {
    0x4265206672756974ULL ^ seed[2], 0x66756C20616E6420ULL ^ ~seed[1],
    0x6D756C7469706C79ULL ^ seed[1], 0x2C20616E64207265ULL ^ ~seed[3],
    0x706C656E69736820ULL ^ seed[3], 0x7468652065617274ULL ^ ~seed[0],
    0x68202847656E6573ULL ^ seed[0], 0x697320313A323829ULL ^ ~seed[2]
  };

  reg r1, r2;

  register u16 i;

  u64 *_ptr = &buffer[0];

  i = 0;
  r1 = SIMD_LOADBITS((reg *)IV);
#ifndef __AVX512F__
  r2 = SIMD_LOADBITS((reg *)&IV[4]);
  reg r3;
#endif

  do {
    SIMD_STOREBITS((reg *)&_ptr[i], r1);
#ifdef __AVX512F__
    r2 = SIMD_ADD64(r1, r1);
    r2 = SIMD_XORBITS(r1, r2);
    SIMD_STOREBITS((reg *)&_ptr[i + 8], r2);
    r1 = SIMD_ADD64(r1, r2);
#else
    SIMD_STOREBITS((reg *)&_ptr[i + 4], r2);
    r3 = SIMD_ADD64(r1, r1);
    r3 = SIMD_XORBITS(r1, r3);
    SIMD_STOREBITS((reg *)&_ptr[i + 8], r3);
    r1 = SIMD_ADD64(r1, r3);
    r3 = SIMD_ADD64(r2, r2);
    r3 = SIMD_XORBITS(r2, r3);
    SIMD_STOREBITS((reg *)&_ptr[i + 12], r3);
    r2 = SIMD_ADD64(r2, r3);
#endif
  } while ((i += 16) < BUF_SIZE);

  r1 = SIMD_STOREBITS((reg *)IV);
#ifndef __AVX512F__
  r2 = SIMD_STOREBITS((reg *)&IV[4]);
#endif

  // Calculation methodology for this part works best with AVX2
  const __m256d factor = SIMD_SETPD(DIV * LIMIT);

  __m256d s1, s2;
  i = 0;
  do {
    IV[0] += buffer[IV[7] & 0xFF], IV[1] += buffer[IV[6] & 0xFF];
    IV[2] += buffer[IV[5] & 0xFF], IV[3] += buffer[IV[4] & 0xFF];
    IV[4] += buffer[IV[3] & 0xFF], IV[5] += buffer[IV[2] & 0xFF];
    IV[6] += buffer[IV[1] & 0xFF], IV[7] += buffer[IV[0] & 0xFF];

    s1 = _mm256_setr_pd((double)(IV[0] ^ IV[4]), (double)(IV[1] ^ IV[5]),
        (double)(IV[2] ^ IV[6]), (double)(IV[3] ^ IV[7]));

    IV[0] += buffer[IV[7] & 0xFF], IV[1] += buffer[IV[6] & 0xFF];
    IV[2] += buffer[IV[5] & 0xFF], IV[3] += buffer[IV[4] & 0xFF];
    IV[4] += buffer[IV[3] & 0xFF], IV[5] += buffer[IV[2] & 0xFF];
    IV[6] += buffer[IV[1] & 0xFF], IV[7] += buffer[IV[0] & 0xFF];

    s2 = _mm256_setr_pd((double)(IV[2] ^ IV[6]), (double)(IV[3] ^ IV[7]),
        (double)(IV[0] ^ IV[4]), (double)(IV[1] ^ IV[5]));

    s1 = _mm256_mul_pd(s1, factor);
    s2 = _mm256_mul_pd(s2, factor);

    _mm256_store_pd((reg *)&chseeds[(i << 2)], s1);
    _mm256_store_pd((reg *)&chseeds[(i << 2) + 4], s2);
  } while (++i < (ROUNDS >> 1));
}
#endif

static void diffuse(const u64 nonce)
{
  u64 a, b, c, d, e, f, g, h;
  a = b = c = d = e = f = g = h = nonce;

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

    buffer[i] = a;
    buffer[i + 1] = b;
    buffer[i + 2] = c;
    buffer[i + 3] = d;
    buffer[i + 4] = e;
    buffer[i + 5] = f;
    buffer[i + 6] = g;
    buffer[i + 7] = h;
  } while ((i += 8) < BUF_SIZE);
}

#ifdef __AARCH64_SIMD__
static void apply(u16 idx, const u8 rounds)
{
  const dregq one = SIMD_SETQPD(1.0);
  const dregq coeff = SIMD_SETQPD(COEFFICIENT);
  const dregq beta = SIMD_SETQPD(BETA);

  reg64q4 r1, r2;

  // Load 8 seeds at a time
  dreg4q d1 = SIMD_LOAD4PD(&chseeds[rounds]);
  dreg4q d2;

  const u16 limit = idx + 88;
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
    SIMD_STORE64x4(&buffer[idx], r1);
  } while ((idx += 8) < limit);
}
#else
static void apply(void)
{
  const regd beta = SIMD_SETQPD(BETA), coefficient = SIMD_SETQPD(3.9999),
             one = SIMD_SETQPD(1.0);

  const reg mask = SIMD_SET64(0xFFUL), inc = SIMD_SET64(4UL);

  u64 *map_a = &buffer[0], *map_b = &buffer[BUF_SIZE];

  register u8 rounds = 0;
  register u16 i;

  regd d1, d2, d3;

  reg r1, r2, scale;

  u64 arr[8] ALIGN(SIMD_LEN);
chaotic_iter:
  d1 = SIMD_LOADPD(&chseeds[rounds]);
  i = 0;
  scale = SIMD_SETR64(1UL, 2UL, 3UL, 4UL);

  do {
    // 3.9999 * X * (1 - X) for all X in the register
    d2 = SIMD_SUBPD(one, d1);
    d2 = SIMD_MULPD(d2, coefficient);
    d1 = SIMD_MULPD(d1, d2);

    // Multiply result of chaotic function by beta
    d2 = SIMD_MULPD(d1, beta);

    // Cast to u64, add the scaling factor
    // Mask so idx stays in range of buffer
    r1 = SIMD_CASTPD(d2);
    r1 = SIMD_ANDBITS(r1, mask);
    r1 = SIMD_ADD64(r1, scale);
    SIMD_STOREBITS((reg *)arr, r1);

    map_b[i] ^= map_a[arr[0]];
    map_b[i + 1] ^= map_a[arr[1]];
    map_b[i + 2] ^= map_a[arr[2]];
    map_b[i + 3] ^= map_a[arr[3]];

    scale = SIMD_ADD64(scale, inc);
  } while ((i += 4) < BUF_SIZE);

  rounds += 4;

  if (rounds < (ROUNDS << 2)) {
    i = (rounds == (ITER << 3));
    map_a += BUF_SIZE;
    // reset to start if the third iteration is about to begin
    // otherwise add 256 just like above
    map_b += (!i << 8) - (i << 9);
    i = 0;
    goto chaotic_iter;
  }
}
#endif

static void mix(u16 dest, u16 map_a, u16 map_b)
{
  const u16 limit = dest + 88;

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

    SIMD_STOREBITS((reg *)&buffer[i], r1);
    SIMD_STOREBITS((reg *)&buffer[i + (SIMD_LEN >> 3)], r2);

    i += (SIMD_LEN >> 2);
  } while (i < BUF_SIZE);
#endif
}

static void dbl_simd_fill(double *buf, u64 *seed, u64 *nonce, u16 amount)
{
  adam_run(seed, nonce);
  register u16 i = 0;

#ifdef __AARCH64_SIMD__
  const dregq range = SIMD_SETQPD(1.0 / (double)__UINT64_MAX__);

  reg64q4 r1;
  dreg4q d1;

  do {
    r1 = SIMD_LOAD64x4(&buffer[i]);
    d1.val[0] = SIMD_MULPD(vcvtq_f64_u64(r1.val[0]), range);
    d1.val[1] = SIMD_MULPD(vcvtq_f64_u64(r1.val[1]), range);
    d1.val[2] = SIMD_MULPD(vcvtq_f64_u64(r1.val[2]), range);
    d1.val[3] = SIMD_MULPD(vcvtq_f64_u64(r1.val[3]), range);
    SIMD_STORE4PD(&buf[i], d1);
  } while ((i += 8) < amount);
#else
  const regd range = SIMD_SETPD(DIV * LIMIT);

  reg r1;
  regd d1;

  do {
    r1 = SIMD_LOAD64((reg *)&buffer[i]);
    d1 = SIMD_CASTPD(r1);
    d1 = SIMD_MULPD(d1, max);
    SIMD_STOREPD((dreg *)&buf[i], d1);

#ifndef __AVX512F__
    r1 = SIMD_LOAD64((reg *)&buffer[i + 4]);
    d1 = SIMD_CASTPD(r1);
    d1 = SIMD_MULPD(d1, max);
    SIMD_STOREPD((dreg *)&buf[i + 4], d1);
#endif
  } while ((i += 8) < amount);
#endif
}

static void dbl_simd_fill_mult(double *buf, u64 *seed, u64 *nonce, u16 amount, const u64 multiplier)
{
  adam_run(seed, nonce);
  register u16 i = 0;

#ifdef __AARCH64_SIMD__
  const dregq range = SIMD_SETQPD(1.0 / (double)__UINT64_MAX__);
  const dregq mult = SIMD_SETQPD((double)multiplier);
  reg64q4 r1;
  dreg4q d1;

  do {
    r1 = SIMD_LOAD64x4(&buffer[i]);
    d1.val[0] = SIMD_MULPD(vcvtq_f64_u64(r1.val[0]), range);
    d1.val[1] = SIMD_MULPD(vcvtq_f64_u64(r1.val[1]), range);
    d1.val[2] = SIMD_MULPD(vcvtq_f64_u64(r1.val[2]), range);
    d1.val[3] = SIMD_MULPD(vcvtq_f64_u64(r1.val[3]), range);
    SIMD_MUL4QPD(d1, d1, mult);
    SIMD_STORE4PD(&buf[i], d1);
  } while ((i += 8) < amount);
#else
  const regd range = SIMD_SETPD(DIV * LIMIT);
  const regd mult = SIMD_SETPD((double)multiplier);

  reg r1;
  regd d1;

  do {
    r1 = SIMD_LOAD64((reg *)&buffer[i]);
    d1 = SIMD_CASTPD(r1);
    d1 = SIMD_MULPD(d1, max);
    d1 = SIMD_MULPD(d1, mult);
    SIMD_STOREPD((dreg *)&buf[i], d1);

#ifndef __AVX512F__
    r1 = SIMD_LOAD64((reg *)&buffer[i + 4]);
    d1 = SIMD_CASTPD(r1);
    d1 = SIMD_MULPD(d1, max);
    d1 = SIMD_MULPD(d1, mult);
    SIMD_STOREPD((dreg *)&buf[i + 4], d1);
#endif
  } while ((i += 8) < amount);
#endif
}

void adam_run(unsigned long long *seed, unsigned long long *nonce)
{
  register u64 _nonce = *nonce;

  accumulate(seed);
  diffuse(_nonce);
  apply(0, 0);
  mix(0, 88, 176);
  apply(88, 8);
  mix(88, 176, 0);
  apply(176, 16);
  mix(176, 0, 88);
  apply(0, 24);

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
      buf[count] = (double)buffer[i++] / (double)__UINT64_MAX__;
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
    register u8 i = 0;
    const double mult = (double)multiplier;
    do {
      buf[count] = ((double)buffer[i++] / (double)__UINT64_MAX__) * mult;
    } while (++count < amount);
  }
}

void adam_connect(unsigned long long **_ptr, double **_chptr)
{
  *_ptr = &buffer[0];

  if (_chptr != (void *)0)
    *_chptr = &chseeds[0];
}
