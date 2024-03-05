#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "../include/ent.h"
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

u8 nearest_space(const char *str, u8 offset)
{
    register u8 a = offset;
    register u8 b = offset;

    while (str[a] && str[a] != ' ')
        --a;

    while (str[b] && str[b] != ' ')
        ++b;

    return (b - offset < offset - a) ? b : a;
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

double stream_ascii(const u64 limit, u64 *restrict buffer, u64 *restrict seed, u64 *restrict nonce)
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

    while (rate > 0) {
        start = clock();
        adam(buffer, seed, nonce);
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
        adam(buffer, seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;

        do {
            print_binary(_bptr, buffer[l >> 6]);
            fwrite(_bptr, 1, 64, stdout);
        } while ((l += 64) < leftovers);
    }
    return duration;
}

double dbl_ascii(const u32 limit, u64 *restrict buffer, u64 *restrict seed, u64 *restrict nonce, const u32 multiplier, const u8 precision)
{
    double *_buf = (double *) aligned_alloc(SIMD_LEN, limit * sizeof(double));

    register clock_t start = clock();

    if (multiplier > 1)
        adam_fmrun(_buf, buffer, seed, nonce, limit, multiplier);
    else
        adam_frun(_buf, buffer, seed, nonce, limit);

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

double stream_bytes(const u64 limit, u64 *restrict buffer, u64 *restrict seed, u64 *restrict nonce)
{
    register double duration = 0.0;
    register clock_t start;
    register u64 progress = 0;
    while (LIKELY(progress < limit)) {
        start = clock();
        adam(buffer, seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;
        fwrite(buffer, sizeof(u64), BUF_SIZE, stdout);
        progress += SEQ_SIZE;
    }

    /*
        Split limit based on how many calls we need to make
        to write the bytes of an entire buffer directly

        We multiply SEQ_SIZE by 4 because we write 4 buffers
        at once
    */
    const u16 leftovers = (limit & ((SEQ_SIZE << 2) - 1)) >> 6;
    if (LIKELY(leftovers > 0)) {
        start = clock();
        adam(buffer, seed, nonce);
        duration += (double) (clock() - start) / (double) CLOCKS_PER_SEC;
        fwrite(buffer, sizeof(u64), leftovers, stdout);
    }

    return duration;
}

double dbl_bytes(const u32 limit, u64 *restrict buffer, u64 *restrict seed, u64 *restrict nonce, const u32 multiplier)
{
    double *_buf = (double *) aligned_alloc(SIMD_LEN, limit * sizeof(double));

    register clock_t start = clock();

    if (multiplier > 1)
        adam_fmrun(_buf, buffer, seed, nonce, limit, multiplier);
    else
        adam_frun(_buf, buffer, seed, nonce, limit);

    register double duration = (double) (clock() - start) / (double) CLOCKS_PER_SEC;

    fwrite(&_buf[0], sizeof(double), limit, stdout);

    free(_buf);

    return duration;
}

static u8 calc_padding(u64 num)
{
    register u8 pad = 0;
    do
        ++pad;
    while ((num /= 10) > 0);
    return pad;
}

static void print_basic_results(const u16 indent, const u64 limit, rng_test *rsl, const u64 *init_values)
{
    const u64 output = rsl->sequences << 8;

    // see if u can do output << 3
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

    const u64 zeroes = (output << 6) - rsl->mfreq;

    const u64 expected_bits  = (double) (output << 6) * MFREQ_PROB;
    register double chi_calc = (pow((double) zeroes - expected_bits, 2) / (double) expected_bits) + (pow((double) rsl->mfreq - expected_bits, 2) / (double) expected_bits);

    register u8 suspect_level = 32 - (MFREQ_CRITICAL_VALUE <= chi_calc);

    printf("\033[1;34m\033[%uC               Sample Size: \033[m%llu BITS (%llu%s)\n", indent, output << 6, bytes, unit);
    printf("\033[1;34m\033[%uC       Sequences Generated: \033[m%llu\n", indent, rsl->sequences);
    printf("\033[2m\033[%uC                    a. u64: \033[m%llu\n", indent, output);
    printf("\033[2m\033[%uC                    b. u32: \033[m%llu\n", indent, output << 1);
    printf("\033[2m\033[%uC                    c. u16: \033[m%llu\n", indent, output << 2);
    printf("\033[2m\033[%uC                    d.  u8: \033[m%llu\n", indent, output << 3);
    printf("\033[1;34m\033[%uC    256-bit Seed (u64 x 4): \033[m0x%016llX, 0x%016llX,\n", indent, init_values[0], init_values[1]);
    printf("\033[%uC                            0x%016llX, 0x%016llX\n", indent, init_values[2], init_values[3]);
    printf("\033[1;34m\033[%uC              64-bit Nonce: \033[m0x%016llX\n", indent, init_values[4]);
    printf("\033[1;34m\033[%uC       Bit Freq Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[2m\033[%uC                   a. Ones: \033[m%llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, rsl->mfreq, rsl->mfreq - expected_bits, expected_bits);
    printf("\033[2m\033[%uC                 b. Zeroes: \033[m%llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, zeroes, zeroes - expected_bits, expected_bits);
    printf("\033[2m\033[%uC   c. Total Number of Runs: \033[m%llu\n", indent, rsl->one_runs + rsl->zero_runs);
    printf("\033[2m\033[%uC           d. Runs Of Ones: \033[m%llu\n", indent, rsl->one_runs);
    printf("\033[2m\033[%uC         e. Runs Of Zeroes: \033[m%llu\n", indent, rsl->zero_runs);
    printf("\033[2m\033[%uC            f. Longest Run: \033[m%llu (ONES)\n", indent, rsl->longest_one);
    printf("\033[2m\033[%uC            g. Longest Run: \033[m%llu (ZEROES)\n", indent, rsl->longest_zero);
}

static void print_byte_results(const u16 indent, rng_test *rsl)
{
    // maurer test and MORE gap info (gaps below 256 and stuff)

    printf("\033[1;34m\033[%uC        Average Gap Length: \033[m%llu (ideal = 256)\n", indent, (u64) rsl->avg_gap);
    printf("\033[1;34m\033[%uC         Most Common Bytes: \033[m0x%02X, 0x%02X, 0x%02X, 0x%02X\n", indent, rsl->mcb[0], rsl->mcb[1], rsl->mcb[2], rsl->mcb[3]);
    printf("\033[1;34m\033[%uC        Least Common Bytes: \033[m0x%02X, 0x%02X, 0x%02X, 0x%02X\n", indent, rsl->lcb[0], rsl->lcb[1], rsl->lcb[2], rsl->lcb[3]);
    printf("\033[1;34m\033[%uC      Total Number of Runs: \033[m%llu\n", indent, rsl->up_runs + rsl->down_runs);
    printf("\033[2m\033[%uC            a.  Increasing: \033[m%llu\n", indent, rsl->up_runs);
    printf("\033[2m\033[%uC            b.  Decreasing: \033[m%llu\n", indent, rsl->down_runs);
    printf("\033[2m\033[%uC            c. Longest Run: \033[m%llu (INCREASING)\n", indent, rsl->longest_up);
    printf("\033[2m\033[%uC            d. Longest Run: \033[m%llu (DECREASING)\n", indent, rsl->longest_down);
}

static void print_range_results(const u16 indent, rng_test *rsl)
{
    const u64 output               = rsl->sequences << 8;
    const u8 pad                   = calc_padding(rsl->range_dist[4]);
    const u64 range_exp[RANGE_CAT] = {
        (double) output * RANGE1_PROB,
        (double) output * RANGE2_PROB,
        (double) output * RANGE3_PROB,
        (double) output * RANGE4_PROB,
        (double) output * RANGE5_PROB
    };

    // Calculate chi-square statistic for distribution of 64-bit numbers
    double delta[5];
    register double chi_calc = 0.0;

    for (u8 i = 0; i < RANGE_CAT; ++i) {
        delta[i] = (double) rsl->range_dist[i] - (double) range_exp[i];
        if (range_exp[i] > 0)
            chi_calc += pow(delta[i], 2) / (double) range_exp[i];
    }

    register u8 suspect_level = 32 - (RANGE_CRITICAL_VALUE <= chi_calc);

    printf("\033[1;34m\033[%uC             Minimum Value: \033[m%llu\n", indent, rsl->min);
    printf("\033[1;34m\033[%uC             Maximum Value: \033[m%llu\n", indent, rsl->max);
    printf("\033[1;34m\033[%uC                     Range: \033[m%llu\n", indent, rsl->max - rsl->min);
    printf("\033[1;34m\033[%uC          Range Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[2m\033[%uC            a.    [0, 2³²): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad + 1, rsl->range_dist[0], (long long) delta[0], range_exp[0]);
    printf("\033[2m\033[%uC            b.  [2³², 2⁴⁰): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad + 1, rsl->range_dist[1], (long long) delta[1], range_exp[1]);
    printf("\033[2m\033[%uC            c.  [2⁴⁰, 2⁴⁸): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad + 1, rsl->range_dist[2], (long long) delta[2], range_exp[2]);
    printf("\033[2m\033[%uC            d.  [2⁴⁸, 2⁵⁶): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad + 1, rsl->range_dist[3], (long long) delta[3], range_exp[3]);
    printf("\033[2m\033[%uC            e.  [2⁵⁶, 2⁶⁴): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad + 1, rsl->range_dist[4], (long long) delta[4], range_exp[4]);
    printf("\033[1;34m\033[%uC              Even Numbers: \033[m%llu (%u%%)\n", indent, output - rsl->odd, (u8) (((double) (output - rsl->odd) / (double) output) * 100));
    printf("\033[1;34m\033[%uC               Odd Numbers: \033[m%llu (%u%%)\n", indent, rsl->odd, (u8) (((double) rsl->odd / (double) output) * 100));
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

    printf("\033[1;34m\033[%uC                   Entropy: \033[m%.5lf %10s per byte)\n", indent, ent->ent, "(bits");
    printf("\033[1;34m\033[%uC                Chi-Square: \033[m\033[1;%um%1.3lf\033[m %15sexceeded %s%% of the time) \n", indent, suspect_level, ent->chisq, "(randomly ", chi_str);
    printf("\033[1;34m\033[%uC           Arithmetic Mean: \033[m%1.3lf%22s\n", indent, ent->mean, "(127.5 = random)");
    printf("\033[1;34m\033[%uC  Monte Carlo Value for Pi: \033[m%1.9lf  (error: %1.2f%%)\n", indent, ent->montepicalc, ent->monterr);
    if (ent->scc >= -99999) {
        double scc = ent->scc;
        if (ent->scc < 0)
            scc *= -1.0;
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[m%1.6f%33s\n", indent, scc, "(totally uncorrelated = 0.0)");
    } else
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[1;31mUNDEFINED\033[m %32s\n", indent, "(all values equal!)");
}

static void print_chseed_results(const u16 indent, const u64 expected, const u64 *chseed_dist, double avg_chseed)
{
    const u64 expected_chseeds = (expected * CHSEED_PROB);

    double delta[5];
    register u8 pad = 1;
    register u8 tmp;
    register double chi_calc = 0.0;
    for (u8 i = 0; i < CHSEED_CAT; ++i) {
        delta[i] = (double) chseed_dist[i] - (double) expected_chseeds;
        chi_calc += pow(delta[i], 2) / (double) expected_chseeds;
        tmp = calc_padding((delta[i] > 0) ? (u64) delta[i] : (u64) (delta[i] * -1.0));
        pad = (tmp > pad) ? tmp : pad;
    }

    register u8 suspect_level = 32 - (CHSEED_CRITICAL_VALUE <= chi_calc);

    printf("\033[1;34m\033[%uC   Chaotic Seed Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[1;34m\033[%uC      Average Chaotic Seed: \033[m%1.15lf (ideal = 0.25)\n", indent, avg_chseed / (double) expected);
    printf("\033[2m\033[%uC             a. (0.0, 0.1): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, chseed_dist[0], pad + 1, (long long) delta[0], expected_chseeds);
    printf("\033[2m\033[%uC             b. [0.1, 0.2): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, chseed_dist[1], pad + 1, (long long) delta[1], expected_chseeds);
    printf("\033[2m\033[%uC             c. [0.2, 0.3): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, chseed_dist[2], pad + 1, (long long) delta[2], expected_chseeds);
    printf("\033[2m\033[%uC             d. [0.3, 0.4): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, chseed_dist[3], pad + 1, (long long) delta[3], expected_chseeds);
    printf("\033[2m\033[%uC             e. [0.4, 0.5): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, chseed_dist[4], pad + 1, (long long) delta[4], expected_chseeds);
}

static void print_fp_results(const u16 indent, rng_test *rsl)
{
    const u64 output         = rsl->sequences << 8;
    register double expected = output * FPF_PROB;

    /*   FREQUENCY   */

    register double chi_calc = 0.0;
    register u8 i            = 0;
    for (; i < FPF_CAT; ++i)
        chi_calc += pow((rsl->fpf_dist[i] - expected), 2) / expected;

    register u8 suspect_level = 32 - (FPF_CRITICAL_VALUE <= chi_calc);

    // Divide frequency values into "quadrants" and report observed vs expected values
    double delta[4];
    register u8 pad = 1;
    register u8 tmp;
    expected = output * 0.25;
    for (u8 i = 0; i < 4; ++i) {
        delta[i] = rsl->fpf_quad[i] - expected;
        tmp      = calc_padding((delta[i] > 0) ? (u64) delta[i] : (u64) (delta[i] * -1.0));
        pad      = (tmp > pad) ? tmp : pad;
    }

    printf("\033[1;34m\033[%uC        FP Freq Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[1;34m\033[%uC          Average FP Value: \033[m%1.15lf (ideal = 0.5)\n", indent, rsl->avg_fp);
    printf("\033[2m\033[%uC            a. (0.0, 0.25): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, rsl->fpf_quad[0], pad + 1, (long long) delta[0], (u64) expected);
    printf("\033[2m\033[%uC            b. [0.25, 0.5): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, rsl->fpf_quad[1], pad + 1, (long long) delta[1], (u64) expected);
    printf("\033[2m\033[%uC            c. [0.5, 0.75): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, rsl->fpf_quad[2], pad + 1, (long long) delta[2], (u64) expected);
    printf("\033[2m\033[%uC            d. [0.75, 1.0): \033[m%llu (\033[1m%+*lli\033[m: exp. %llu)\n", indent, rsl->fpf_quad[3], pad + 1, (long long) delta[3], (u64) expected);

    /*   PERMUTATIONS   */

    expected            = rsl->perms * FP_PERM_PROB;
    chi_calc            = 0;
    i                   = 0;
    double perm_average = 0.0;
    for (; i < FP_PERM_CAT; ++i) {
        perm_average += rsl->perm_dist[i];
        chi_calc += pow((rsl->perm_dist[i] - expected), 2) / expected;
    }

    // Get standard deviation for observed permutation orderings
    perm_average /= FP_PERM_CAT;
    register double std_dev = 0.0;
    for (; i < FP_PERM_CAT; ++i)
        std_dev += pow(((double) rsl->perm_dist[i] - perm_average), 2);

    suspect_level = 32 - (FP_PERM_CRITICAL_VALUE <= chi_calc);
    std_dev       = sqrt(std_dev / (double) FP_PERM_CAT);

    printf("\033[1;34m\033[%uCFP Permutations Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent, suspect_level, chi_calc);
    printf("\033[2m\033[%uC     a. Total Permutations: \033[m%llu\n", indent, rsl->perms);
    printf("\033[2m\033[%uC        b. Average Per Bin: \033[m%llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, (u64) perm_average, (u64) perm_average - (u64) expected, (u64) expected);
    printf("\033[2m\033[%uC     c. Standard Deviation: \033[m%1.2lf\n", indent, std_dev);
}

static void print_avalanche_results(const u16 indent, rng_test *rsl)
{
    // See test.h for more details on these probabilities
    static double expected[AVALANCHE_CAT + 1] = {
        // clang-format off
        5.421010862427522E-20,  3.469446951953614E-18,  1.092875789865388E-16,  2.258609965721802E-15,
        3.444380197725749E-14,  4.133256237270899E-13,  4.064368633316384E-12,  3.367619724747861E-11,
        2.399429053882851E-10,  1.492978077971551E-9,   8.211379428843534E-9,   4.031040810523189E-8,
        1.780376357981075E-7,   7.121505431924302E-7,   2.594262693058138E-6,   8.647542310193795E-6,
        0.000026483098324,      0.000074775807035,      0.000195247940591,      0.000472705540380,
        0.001063587465856,      0.002228468976079,      0.004355643907791,      0.007953784527271,
        0.013587715234088,      0.021740344374540,      0.032610516561811,      0.045896282568475,
        0.060648659108342,      0.075287990617252,      0.087835989053460,      0.096336246058634,
        0.099346753747966,      0.096336246058634,      0.087835989053460,      0.075287990617252,
        0.060648659108342,      0.045896282568475,      0.032610516561811,      0.021740344374540,
        0.013587715234088,      0.007953784527271,      0.004355643907791,      0.002228468976079,
        0.001063587465856,      0.000472705540380,      0.000195247940591,      0.000074775807035,
        0.000026483098324,      8.647542310193795E-6,   2.594262693058138E-6,   7.121505431924302E-7,
        1.780376357981075E-7,   4.031040810523190E-8,   8.211379428843530E-9,   1.492978077971551E-9,
        2.399429053882851E-10,  3.367619724747861E-11,  4.064368633316384E-12,  4.133256237270899E-13,
        3.444380197725749E-14,  2.258609965721803E-15,  1.092875789865388E-16,  3.469446951953614E-16,
        5.421010862427522E-20
        // clang-format on
    };

    const double total_u64 = rsl->sequences << 8;

    register double average, chi_calc;
    average = chi_calc = 0.0;

    double bin_counts[4];
    bin_counts[0] = bin_counts[1] = bin_counts[2] = bin_counts[3] = 0.0;

    double quadrants[4];
    quadrants[0] = quadrants[1] = quadrants[2] = quadrants[3] = 0.0;

    register u8 i = 0;
    do {
        quadrants[i >> 4] += rsl->ham_dist[i];
        expected[i] *= total_u64;
        bin_counts[i >> 4] += (u64) expected[i];
        average += rsl->ham_dist[i] * i;
        chi_calc += pow(((double) rsl->ham_dist[i] - expected[i]), 2) / expected[i];
    } while (++i < (AVALANCHE_CAT + 1));

    register u8 suspect_level = 32 - (AVALANCHE_CRITICAL_VALUE <= chi_calc);
    average                   = average / (double) total_u64;

    register u8 pad = 5;
    register u8 tmp = calc_padding((bin_counts[0] > 0) ? (u64) bin_counts[0] : (u64) (bin_counts[0] * -1.0));
    pad             = pad ^ ((pad ^ tmp) & -(pad < tmp));
    tmp             = calc_padding((bin_counts[1] > 0) ? (u64) bin_counts[1] : (u64) (bin_counts[1] * -1.0));
    pad             = pad ^ ((pad ^ tmp) & -(pad < tmp));
    tmp             = calc_padding((bin_counts[2] > 0) ? (u64) bin_counts[2] : (u64) (bin_counts[2] * -1.0));
    pad             = pad ^ ((pad ^ tmp) & -(pad < tmp));
    tmp             = calc_padding((bin_counts[3] > 0) ? (u64) bin_counts[3] : (u64) (bin_counts[3] * -1.0));
    pad             = pad ^ ((pad ^ tmp) & -(pad < tmp));

    printf("\033[1;34m\033[%uCStrict Avalanche Chi-Square: \033[m\033[1;%um%1.3lf\033[m\n", indent - 1, suspect_level, chi_calc);
    printf("\033[2m\033[%uC  a. Mean Hamming Distance: \033[m%2.3lf (ideal = %u)\n", indent, average, AVALANCHE_CAT >> 1);
    printf("\033[2m\033[%uC                b. [0, 16): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad, (u64) quadrants[0], (u64) quadrants[0] - (u64) bin_counts[0], (u64) bin_counts[0]);
    printf("\033[2m\033[%uC               c. [16, 32): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad, (u64) quadrants[1], (u64) quadrants[1] - (u64) bin_counts[1], (u64) bin_counts[1]);
    printf("\033[2m\033[%uC               d. [32, 48): \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad, (u64) quadrants[2], (u64) quadrants[2] - (u64) bin_counts[2], (u64) bin_counts[2]);
    printf("\033[2m\033[%uC               e. [48, 64]: \033[m%*llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, pad, (u64) quadrants[3], (u64) quadrants[3] - (u64) bin_counts[3], (u64) bin_counts[3]);
}

void get_seq_properties(const u64 limit, unsigned long long *seed, unsigned long long nonce)
{
    // Screen info for pretty printing
    u16 center, indent, swidth;
    get_print_metrics(&center, &indent, &swidth);
    indent <<= 1;

    // Assign an ent_report struct for collecting ENT stats
    rng_test rsl;
    ent_report ent;
    rsl.ent = &ent;

    // Record initial state and assign output vector
    u64 init_values[5];
    init_values[0] = seed[0];
    init_values[1] = seed[1];
    init_values[2] = seed[2];
    init_values[3] = seed[3];
    init_values[4] = nonce;

    // Start examination!
    printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", limit);
    register clock_t start = clock();
    adam_examine(limit, &rsl, seed, nonce);
    register double duration = ((double) (clock() - start) / (double) CLOCKS_PER_SEC);

    // Output the results to the user
    printf("\033[%uC[RESULTS]\n\n", center - 4);
    print_basic_results(indent, limit, &rsl, &init_values[0]);
    print_range_results(indent, &rsl);
    print_ent_results(indent, rsl.ent);
    print_byte_results(indent, &rsl);
    print_chseed_results(indent, rsl.expected_chseed, &rsl.chseed_dist[0], rsl.avg_chseed);
    print_fp_results(indent, &rsl);
    print_avalanche_results(indent, &rsl);

    printf("\n\033[1;33mExamination Complete! (%lfs)\033[m\n\n", duration);
}
