#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>

#include "../include/rng.h"
#include "../include/simd.h"
#include "../include/support.h"
#include "../include/test.h"

void get_print_metrics(u16 *center, u16 *indent, u16 *swidth)
{
    struct winsize wsize;
    ioctl(0, TIOCGWINSZ, &wsize);
    *center = (wsize.ws_col >> 1);
    *indent = (wsize.ws_col >> 4);
    *swidth = wsize.ws_col;
}

u8 err(const char *s)
{
    fprintf(stderr, "\033[1;31m%s\033[m\n", s);
    return 1;
}

u64 a_to_u(const char *s, const u64 min, const u64 max)
{
    if (UNLIKELY(s == NULL || s[0] == '-'))
        return min;

    register u8 len  = 0;
    register u64 val = 0;

    for (; s[len] != '\0'; ++len)
        if (UNLIKELY(s[len] < '0' || s[len] > '9'))
            return 0;
    ;

    switch (len) {
    case 20:
        val += 10000000000000000000LU;
    case 19:
        val += (s[len - 19] - '0') * 1000000000000000000LU;
    case 18:
        val += (s[len - 18] - '0') * 100000000000000000LU;
    case 17:
        val += (s[len - 17] - '0') * 10000000000000000LU;
    case 16:
        val += (s[len - 16] - '0') * 1000000000000000LU;
    case 15:
        val += (s[len - 15] - '0') * 100000000000000LU;
    case 14:
        val += (s[len - 14] - '0') * 10000000000000LU;
    case 13:
        val += (s[len - 13] - '0') * 1000000000000LU;
    case 12:
        val += (s[len - 12] - '0') * 100000000000LU;
    case 11:
        val += (s[len - 11] - '0') * 10000000000LU;
    case 10:
        val += (s[len - 10] - '0') * 1000000000LU;
    case 9:
        val += (s[len - 9] - '0') * 100000000LU;
    case 8:
        val += (s[len - 8] - '0') * 10000000LU;
    case 7:
        val += (s[len - 7] - '0') * 1000000LU;
    case 6:
        val += (s[len - 6] - '0') * 100000LU;
    case 5:
        val += (s[len - 5] - '0') * 10000LU;
    case 4:
        val += (s[len - 4] - '0') * 1000LU;
    case 3:
        val += (s[len - 3] - '0') * 100LU;
    case 2:
        val += (s[len - 2] - '0') * 10LU;
    case 1:
        val += (s[len - 1] - '0');
        break;
    }
    return (val >= min || val < max + 1) ? val : 0;
}

static u8 load_seed(u64 *seed, const char *strseed)
{
    FILE *seed_file = fopen(strseed, "rb");
    if (!seed_file)
        return err("Couldn't read seed file");
    fread((void *) seed, sizeof(u64), 4, seed_file);
    fclose(seed_file);
    return 0;
}

static u8 store_seed(const u64 *seed)
{
    char file_name[65];
    printf("File name: \033[1;93m");
    while (!scanf(" %64s", &file_name[0]))
        err("Please enter a valid file name");
    FILE *seed_file = fopen(file_name, "wb+");
    if (!seed_file)
        return err("Couldn't read seed file");
    fwrite((void *) seed, sizeof(u64), 4, seed_file);
    fclose(seed_file);
    return 0;
}

u8 rwseed(u64 *seed, const char *strseed)
{
    if (strseed != NULL)
        return load_seed(seed, strseed);
    return store_seed(seed);
}

u8 rwnonce(u64 *nonce, const char *strnonce)
{
    if (strnonce != NULL)
        *nonce = a_to_u(strnonce, 0, __UINT64_MAX__);
    else
        fprintf(stderr, "\033[1;96mNONCE:\033[m %llu", *nonce);
    return 0;
}

/*
  To make writes more efficient, rather than writing one
  number at a time, 16 numbers are parsed together and then
  written to stdout with 1 fwrite call.
*/
static char bitbuffer[BITBUF_SIZE] ALIGN(SIMD_LEN);

#ifdef __AARCH64_SIMD__
static void print_binary(char *restrict _bptr, u64 num)
{
    // Copying bit masks to high and low halves
    // 72624976668147840 == {128, 64, 32, 16, 8, 4, 2, 1}
    const reg8q masks = SIMD_COMBINE8(SIMD_CREATE8(72624976668147840),
        SIMD_CREATE8(72624976668147840));
    const reg8q zero  = SIMD_SET8('0');

    // Repeat all 8 bytes 8 times each, to fill up r1 with 64 bytes total
    // Then we bitwise AND with masks to turn each byte into a 1 or 0
    reg8q4 r1;
    r1.val[0] = SIMD_COMBINE8(SIMD_MOV8((num >> 56) & 0xFF),
        SIMD_MOV8((num >> 48) & 0xFF));
    r1.val[1] = SIMD_COMBINE8(SIMD_MOV8((num >> 40) & 0xFF),
        SIMD_MOV8((num >> 32) & 0xFF));
    r1.val[2] = SIMD_COMBINE8(SIMD_MOV8((num >> 24) & 0xFF),
        SIMD_MOV8((num >> 16) & 0xFF));
    r1.val[3] = SIMD_COMBINE8(SIMD_MOV8((num >> 8) & 0xFF), SIMD_MOV8(num & 0xFF));

    SIMD_AND4Q8(r1, r1, masks);
    SIMD_CMP4QEQ8(r1, r1, masks);
    SIMD_AND4Q8(r1, r1, SIMD_SET8(1));

    // '0' = 48, '1' = 49. This is how we print the number
    SIMD_ADD4Q8(r1, r1, zero);

    SIMD_STORE8x4(_bptr, r1);
}
#else
static void print_binary(char *restrict _bptr, u64 num)
{
#define BYTE_REPEAT(n) \
    bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n]
#define BYTE_MASKS 128, 64, 32, 16, 8, 4, 2, 1

    u8 bytes[sizeof(u64)];

    MEMCPY(bytes, &num, sizeof(u64));

    const reg zeroes = SIMD_SET8('0');
    const reg masks  = SIMD_SETR8(BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
#ifdef __AVX512F__
        ,
        BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
#endif
    );

    reg r1 = SIMD_SETR8(BYTE_REPEAT(7), BYTE_REPEAT(6), BYTE_REPEAT(5), BYTE_REPEAT(4)
#ifdef __AVX512F__
                                                                            ,
        BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1), BYTE_REPEAT(0)
#endif
    );

    r1 = SIMD_ANDBITS(r1, masks);
    r1 = SIMD_CMPEQ8(r1, masks);
    r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
    r1 = SIMD_ADD8(r1, zeroes);
    SIMD_STOREBITS((reg *) _bptr, r1);

#ifndef __AVX512F__
    r1 = SIMD_SETR8(BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1),
        BYTE_REPEAT(0));
    r1 = SIMD_ANDBITS(r1, masks);
    r1 = SIMD_CMPEQ8(r1, masks);
    r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
    r1 = SIMD_ADD8(r1, zeroes);
    SIMD_STOREBITS((reg *) &_bptr[SIMD_LEN], r1);
#endif
}
#endif

// prints all bits in a buffer as chunks of 1024 bits
static void print_chunks(char *restrict _bptr, u64 *_ptr)
{
#define PRINT_4(i, j) print_binary(_bptr + i, _ptr[j]),           \
                      print_binary(_bptr + 64 + i, _ptr[j + 1]),  \
                      print_binary(_bptr + 128 + i, _ptr[j + 2]), \
                      print_binary(_bptr + 192 + i, _ptr[j + 3])

    register u8 i = 0;
    do {
        PRINT_4(0, i + 0);
        PRINT_4(256, i + 4);
        PRINT_4(512, i + 8);
        PRINT_4(768, i + 12);
        fwrite(_bptr, 1, BITBUF_SIZE, stdout);
    } while ((i += 16 - (i == 240)) < BUF_SIZE - 1);
}

u8 gen_uuid(const u64 higher, const u64 lower, u8 *buf)
{
    // Fill buf with 16 random bytes
    u128 tmp = ((u128) higher << 64) | lower;

    MEMCPY(buf, &tmp, sizeof(u128));

    /*
    CODE AND COMMENT PULLED FROM CRYPTOSYS
    (https://www.cryptosys.net/pki/Uuid.c.html)

    Adjust certain bits according to RFC 4122 section 4.4.
    This just means do the following
    (a) set the high nibble of the 7th byte equal to 4 and
    (b) set the two most significant bits of the 9th byte to 10'B,
        so the high nibble will be one of {8,9,A,B}.
  */
    buf[6] = 0x40 | (buf[6] & 0xf);
    buf[8] = 0x80 | (buf[8] & 0x3f);
    return 0;
}

double stream_ascii(const u64 limit, u64 *seed, u64 *nonce)
{
    /*
    Split limit based on how many calls (if needed)
    we make to print_chunks, which prints the bits of
    an entire buffer (aka the SEQ_SIZE)
  */
    register long int rate   = limit >> 14;
    register short leftovers = limit & (SEQ_SIZE - 1);

    register clock_t start;
    register double duration = 0.0;

    char *restrict _bptr = &bitbuffer[0];

    u64 *buffer;
    adam_connect(&buffer, NULL);

    while (rate > 0) {
        start = clock();
        adam_run(seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;
        print_chunks(_bptr, buffer);
        --rate;
    }

    /*
    Since there are SEQ_SIZE (16384) bits in every
    buffer, adam_bits is designed to print up to SEQ_SIZE
    bits per call, so any leftovers must be processed
    independently.

    Most users probably won't enter powers of 2, especially
    if assessing bits, so this branch has been marked as LIKELY.
  */
    if (LIKELY(leftovers > 0)) {
        register u16 l = 0;
        start          = clock();
        adam_run(seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;

        do {
            print_binary(_bptr, buffer[l >> 6]);
            fwrite(_bptr, 1, 64, stdout);
        } while ((l += 64) < leftovers);
    }
    return duration;
}

double dbl_ascii(const u32 limit, u64 *seed, u64 *nonce, const u32 multiplier, const u8 precision)
{
    double *_buf = (double *) aligned_alloc(SIMD_LEN, limit * sizeof(double));

    register clock_t start = clock();

    if (multiplier > 1)
        adam_fmrun(seed, nonce, _buf, limit, multiplier);
    else
        adam_frun(seed, nonce, _buf, limit);

    register double duration = (double) (clock() - start) / (double) CLOCKS_PER_SEC;

    register u32 i = 0;
    while (limit - i > 8) {
        fprintf(stdout, "%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n",
            precision, _buf[i], precision, _buf[i + 1], precision, _buf[i + 2], precision, _buf[i + 3],
            precision, _buf[i + 4], precision, _buf[i + 5], precision, _buf[i + 6], precision, _buf[i + 7]);
        i += 8;
    }

    while (i < limit)
        fprintf(stdout, "%.*lf\n", precision, _buf[i++]);

    free(_buf);

    return duration;
}

double stream_bytes(const u64 limit, u64 *seed, u64 *nonce)
{
    /*
        Split limit based on how many calls we need to make
        to write the bytes of an entire buffer directly
    */
    const u16 leftovers = limit & (SEQ_SIZE - 1);

    u64 *buffer;

    adam_connect(&buffer, NULL);

    register double duration = 0.0;
    register clock_t start;
    register u64 progress = 0;
    while (LIKELY(progress < limit)) {
        start = clock();
        adam_run(seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;
        fwrite(&buffer[0], sizeof(u64), BUF_SIZE, stdout);
        progress += SEQ_SIZE;
    }

    if (LIKELY(leftovers > 0)) {
        start = clock();
        adam_run(seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;
        fwrite(&buffer[0], sizeof(u64), BUF_SIZE, stdout);
    }

    return duration;
}

double dbl_bytes(const u32 limit, u64 *seed, u64 *nonce, const u32 multiplier)
{
    double *_buf = (double *) aligned_alloc(SIMD_LEN, limit * sizeof(double));

    register clock_t start = clock();

    if (multiplier > 1)
        adam_fmrun(seed, nonce, _buf, limit, multiplier);
    else
        adam_frun(seed, nonce, _buf, limit);

    register double duration = (double) (clock() - start) / (double) CLOCKS_PER_SEC;

    fwrite(&_buf[0], sizeof(double), limit, stdout);

    free(_buf);

    return duration;
}

double get_seq_properties(const u64 limit, rng_test *rsl)
{
    // Connect internal integer and chaotic seed arrays to rng_test
    adam_connect(&rsl->buffer, &rsl->chseeds);

    // Start examination!
    register clock_t start = clock();
    adam_test(limit, rsl);
    return ((double) (clock() - start) / (double) CLOCKS_PER_SEC);
}

static void print_basic_results(const u16 indent, const u64 limit, rng_test *rsl, const u64 *init_values)
{
    const u64 output = rsl->sequences << 8;
    const u32 zeroes = (output << 6) - rsl->mfreq;
    const u32 diff   = (zeroes > rsl->mfreq) ? zeroes - rsl->mfreq : rsl->mfreq - zeroes;

    register u64 bytes = limit >> 3;

    const char *unit;
    if (bytes >= 1000000000UL) {
        bytes /= 1000000000UL;
        unit = "GB";
    } else if (bytes >= 1000000) {
        bytes /= 1000000UL;
        unit = "MB";
    } else if (bytes >= 1000) {
        bytes /= 1000UL;
        unit = "KB";
    }

    printf("\033[1;34m\033[%uC               Sample Size: \033[m%llu BITS (%llu%s)\n", indent, limit, bytes, unit);
    printf("\033[1;34m\033[%uC         Monobit Frequency: \033[m%u ONES, %u ZEROES (+%u %s)\n", indent, rsl->mfreq, zeroes, diff, (zeroes > rsl->mfreq) ? "ZEROES" : "ONES");
    printf("\033[1;34m\033[%uC       Sequences Generated: \033[m%u\n", indent, rsl->sequences);
    printf("\033[2m\033[%uC                    a. u64: \033[m%llu numbers\n", indent, output);
    printf("\033[2m\033[%uC                    b. u32: \033[m%llu numbers\n", indent, output << 1);
    printf("\033[2m\033[%uC                    c. u16: \033[m%llu numbers\n", indent, output << 2);
    printf("\033[2m\033[%uC                    d.  u8: \033[m%llu numbers\n", indent, output << 3);
    printf("\033[1;34m\033[%uC             Minimum Value: \033[m%llu\n", indent, rsl->min);
    printf("\033[1;34m\033[%uC             Maximum Value: \033[m%llu\n", indent, rsl->max);

    const u64 range_exp[5] = {
        (double) output * RANGE1_PROB,
        (double) output * RANGE2_PROB,
        (double) output * RANGE3_PROB,
        (double) output * RANGE4_PROB,
        (double) output * RANGE5_PROB
    };

    register double chi_calc = 0.0;
    register u8 i            = 1;
    for (; i < 5; ++i)
        if (range_exp[i] != 0)
            chi_calc += pow(((double) rsl->range_dist[i] - (double) range_exp[i]), 2) / (double) range_exp[i];

    register u8 suspect_level = 32 - (RANGE_CRITICAL_VALUE <= chi_calc);

    printf("\033[1;34m\033[%uC                     Range: \033[m%llu\n", indent, rsl->max - rsl->min);
    printf("\033[1;34m\033[%uC          Range Chi-Square: \033[m\033[1;%um%1.2lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[2m\033[%uC            a.    [0, 2³²): \033[m%llu (expected %llu)\n", indent, rsl->range_dist[0], range_exp[0]);
    printf("\033[2m\033[%uC            b.  [2³², 2⁴⁰): \033[m%llu (expected %llu)\n", indent, rsl->range_dist[1], range_exp[1]);
    printf("\033[2m\033[%uC            c.  [2⁴⁰, 2⁴⁸): \033[m%llu (expected %llu)\n", indent, rsl->range_dist[2], range_exp[2]);
    printf("\033[2m\033[%uC            d.  [2⁴⁸, 2⁵⁶): \033[m%llu (expected %llu)\n", indent, rsl->range_dist[3], range_exp[3]);
    printf("\033[2m\033[%uC            e.  [2⁵⁶, 2⁶⁴): \033[m%llu (expected %llu)\n", indent, rsl->range_dist[4], range_exp[4]);
    printf("\033[1;34m\033[%uC              Even Numbers: \033[m%llu (%u%%)\n", indent, output - rsl->odd, (u8) (((double) (output - rsl->odd) / (double) output) * 100));
    printf("\033[1;34m\033[%uC               Odd Numbers: \033[m%u (%u%%)\n", indent, rsl->odd, (u8) (((double) rsl->odd / (double) output) * 100));
    printf("\033[1;34m\033[%uC          Zeroes Generated: \033[m%u\n", indent, rsl->zeroes);
    printf("\033[1;34m\033[%uC    256-bit Seed (u64 x 4): \033[m0x%016llX, 0x%016llX,\n", indent, init_values[0], init_values[1]);
    printf("\033[%uC                            0x%016llX, 0x%016llX\n", indent, init_values[2], init_values[3]);
    printf("\033[1;34m\033[%uC              64-bit Nonce: \033[m0x%016llX\n", indent, init_values[4]);
    printf("\033[1;34m\033[%uC        Average Gap Length: \033[m%llu\n", indent, (u64) rsl->avg_gap);
    printf("\033[1;34m\033[%uC      Total Number of Runs: \033[m%u\n", indent, rsl->up_runs + rsl->down_runs);
    printf("\033[2m\033[%uC            a.  Increasing: \033[m%u\n", indent, rsl->up_runs);
    printf("\033[2m\033[%uC            b.  Decreasing: \033[m%u\n", indent, rsl->down_runs);
    printf("\033[2m\033[%uC            c. Longest Run: \033[m%u (INCREASING)\n", indent, rsl->longest_up);
    printf("\033[2m\033[%uC            d. Longest Run: \033[m%u (DECREASING)\n", indent, rsl->longest_down);
}

static void print_ent_results(const u16 indent, const ent_report *ent)
{
    char *chi_str;
    char chi_tmp[6];
    register u8 suspect_level = 32;

    if (ent->pochisq < 0.01) {
        chi_str = "<= 0.01";
        --suspect_level;
    } else if (ent->pochisq > 0.99) {
        chi_str = ">= 0.99";
        --suspect_level;
    } else {
        snprintf(&chi_tmp[0], 5, "%1.2f", ent->pochisq * 100);
        chi_str = &chi_tmp[0];
    }

    printf("\033[1;34m\033[%uC                   Entropy: \033[m%.5lf bits per byte\n", indent, ent->ent);
    printf("\033[1;34m\033[%uC                Chi-Square: \033[m\033[1;%um%1.2lf\033[m (randomly exceeded %s%% of the time) \n", indent, suspect_level, ent->chisq, chi_str);
    printf("\033[1;34m\033[%uC           Arithmetic Mean: \033[m%1.3lf (127.5 = random)\n", indent, ent->mean);
    printf("\033[1;34m\033[%uC  Monte Carlo Value for Pi: \033[m%1.9lf (error: %1.2f%%)\n", indent, ent->montepicalc, ent->monterr);
    if (ent->scc >= -99999)
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[m%1.6f (totally uncorrelated = 0.0).\n", indent, ent->scc);
    else
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[1;31mUNDEFINED\033[m (all values equal!).\n", indent);
}

static void print_chseed_results(const u16 indent, const u64 expected, const u64 *chseed_dist, const double avg_chseed)
{
    register double chi_calc      = 0.0;
    const double expected_chseeds = (expected * 0.2);

    register u8 i = 0;
    for (; i < 5; ++i)
        chi_calc += pow(((double) chseed_dist[i] - expected_chseeds), 2) / expected_chseeds;

    register u8 suspect_level = 32 - (CHSEED_CRITICAL_VALUE <= chi_calc);

    printf("\033[1;34m\033[%uC   Chaotic Seed Chi-Square: \033[m\033[1;%um%1.2lf\n", indent, suspect_level, chi_calc);
    printf("\033[1;34m\033[%uCAverage Chaotic Seed Value: \033[m%1.15lf (ideal = 0.25)\n", indent, avg_chseed / (double) expected);
    printf("\033[2m\033[%uC             a. (0.0, 0.1): \033[m%llu (%llu expected)\n", indent, chseed_dist[0], (u64) expected_chseeds);
    printf("\033[2m\033[%uC             b. [0.1, 0.2): \033[m%llu (%llu expected)\n", indent, chseed_dist[1], (u64) expected_chseeds);
    printf("\033[2m\033[%uC             c. [0.2, 0.3): \033[m%llu (%llu expected)\n", indent, chseed_dist[2], (u64) expected_chseeds);
    printf("\033[2m\033[%uC             d. [0.3, 0.4): \033[m%llu (%llu expected)\n", indent, chseed_dist[3], (u64) expected_chseeds);
    printf("\033[2m\033[%uC             e. [0.4, 0.5): \033[m%llu (%llu expected)\n", indent, chseed_dist[4], (u64) expected_chseeds);
}

void print_seq_results(rng_test *rsl, const u64 limit, const u64 *init_values)
{
    u16 center, indent, swidth;
    get_print_metrics(&center, &indent, &swidth);
    indent <<= 1;

    printf("\033[%uC[RESULTS]\n\n", center - 4);

    print_basic_results(indent, limit, rsl, &init_values[0]);
    print_ent_results(indent, rsl->ent);
    print_chseed_results(indent, rsl->expected_chseed, &rsl->chseed_dist[0], rsl->avg_chseed);
}
