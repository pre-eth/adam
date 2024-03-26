#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "../include/simd.h"
#include "../include/util.h"

double wh_transform(const u16 idx, const u32 test, const u8 offset)
{
    const u8 limit = offset + 32;
    register u32 nummask = test;
    register u8 ctr       = offset;
    register double final = 0;
    int val;
    do {
        val = (1 - ((nummask & 1) << 1));
        final += (double) val * pow(-1.0, (double) POPCNT(idx & ctr));
        nummask >>= 1;
    } while (++ctr < limit);

    return final;
}

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
        return err("Couldn't open seed file");
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
static void print_binary(u8 *restrict _bptr, u64 num)
{
    // Copying bit masks to high and low halves
    // 72624976668147840 == {128, 64, 32, 16, 8, 4, 2, 1}
    const reg8q masks = SIMD_COMBINE8(
        SIMD_CREATE8(72624976668147840),
        SIMD_CREATE8(72624976668147840));
    const reg8q zero = SIMD_SET8('0');

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
static void print_binary(u8 *restrict _bptr, u64 num)
{
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
void print_ascii_bits(u64 *_ptr, const u64 limit)
{
#define PRINT_4(i, j) print_binary((u8 *) &bitbuffer[i], _ptr[j]),           \
                      print_binary((u8 *) &bitbuffer[64 + i], _ptr[j + 1]),  \
                      print_binary((u8 *) &bitbuffer[128 + i], _ptr[j + 2]), \
                      print_binary((u8 *) &bitbuffer[192 + i], _ptr[j + 3])

    register u64 i = 0;
    do {
        PRINT_4(0, i + 0);
        PRINT_4(256, i + 4);
        PRINT_4(512, i + 8);
        PRINT_4(768, i + 12);
        fwrite(&bitbuffer[0], 1, BITBUF_SIZE, stdout);
        i += 16;
    } while (limit - i >= 16);

    do {
        print_binary((u8 *) &bitbuffer[0], _ptr[i]);
        fwrite(&bitbuffer[0], 1, 64, stdout);
    } while (++i < limit);
}

static u8 calc_padding(u64 num)
{
    register u8 pad = 0;
    do
        ++pad;
    while ((num /= 10) > 0);
    return pad;
}

void print_basic_results(const u16 indent, const u64 limit, const basic_test *rsl)
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

    printf("\033[1;34m\033[%uC               Output Size: \033[m%llu BITS (%llu%s)\n", indent, output << 6, bytes, unit);
    printf("\033[1;34m\033[%uC       Sequences Generated: \033[m%llu\n", indent, rsl->sequences);
    printf("\033[2m\033[%uC                    a. u64: \033[m%llu\n", indent, output);
    printf("\033[2m\033[%uC                    b. u32: \033[m%llu\n", indent, output << 1);
    printf("\033[2m\033[%uC                    c. u16: \033[m%llu\n", indent, output << 2);
    printf("\033[2m\033[%uC                    d.  u8: \033[m%llu\n", indent, output << 3);
    printf("\033[1;34m\033[%uC    256-bit Seed (u64 x 4): \033[m0x%016llX, 0x%016llX,\n", indent, rsl->init_values[0], rsl->init_values[1]);
    printf("\033[%uC                            0x%016llX, 0x%016llX\n", indent, rsl->init_values[2], rsl->init_values[3]);
    printf("\033[1;34m\033[%uC              64-bit Nonce: \033[m0x%016llX\n", indent, rsl->init_values[4]);
}

void print_mfreq_results(const u16 indent, const u64 output, const mfreq_test *rsl)
{
    const u64 zeroes = (output << 6) - rsl->mfreq;

    const u64 expected_bits  = (double) (output << 6) * MFREQ_PROB;
    register double chi_calc = (pow((double) zeroes - expected_bits, 2) / (double) expected_bits) + (pow((double) rsl->mfreq - expected_bits, 2) / (double) expected_bits);

    const u8 suspect_level = 32 - (MFREQ_CRITICAL_VALUE <= chi_calc);

    const double p_value = cephes_igamc(1, chi_calc / 2);

    printf("\033[1;34m\033[%uC         Monobit Frequency:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\n", indent, chi_calc, MFREQ_CRITICAL_VALUE);
    printf("\033[2m\033[%uC                   b. Ones:\033[m %llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, rsl->mfreq, rsl->mfreq - expected_bits, expected_bits);
    printf("\033[2m\033[%uC                 c. Zeroes:\033[m %llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, zeroes, zeroes - expected_bits, expected_bits);
    printf("\033[2m\033[%uC   d. Total Number of Runs:\033[m %llu\n", indent, rsl->one_runs + rsl->zero_runs);
    printf("\033[2m\033[%uC           e. Runs Of Ones:\033[m %llu\n", indent, rsl->one_runs);
    printf("\033[2m\033[%uC         f. Runs Of Zeroes:\033[m %llu\n", indent, rsl->zero_runs);
    printf("\033[2m\033[%uC            g. Longest Run:\033[m %llu (ONES)\n", indent, rsl->longest_one);
    printf("\033[2m\033[%uC            h. Longest Run:\033[m %llu (ZEROES)\n", indent, rsl->longest_zero);
}

void print_byte_results(const u16 indent, const basic_test *rsl)
{
    printf("\033[1;34m\033[%uC        Average Gap Length:\033[m %llu (ideal = 256)\n", indent, (u64) rsl->avg_gap);
    printf("\033[1;34m\033[%uC         Most Common Bytes:\033[m 0x%02llX, 0x%02llX, 0x%02llX, 0x%02llX\n", indent, rsl->mcb[0], rsl->mcb[1], rsl->mcb[2], rsl->mcb[3]);
    printf("\033[1;34m\033[%uC        Least Common Bytes:\033[m 0x%02llX, 0x%02llX, 0x%02llX, 0x%02llX\n", indent, rsl->lcb[0], rsl->lcb[1], rsl->lcb[2], rsl->lcb[3]);
    printf("\033[1;34m\033[%uC      Total Number of Runs:\033[m %llu\n", indent, rsl->up_runs + rsl->down_runs);
    printf("\033[2m\033[%uC            a.  Increasing:\033[m %llu\n", indent, rsl->up_runs);
    printf("\033[2m\033[%uC            b.  Decreasing:\033[m %llu\n", indent, rsl->down_runs);
    printf("\033[2m\033[%uC            c. Longest Run:\033[m %llu (INCREASING)\n", indent, rsl->longest_up);
    printf("\033[2m\033[%uC            d. Longest Run:\033[m %llu (DECREASING)\n", indent, rsl->longest_down);
}

void print_range_results(const u16 indent, const u64 output, const range_test *rsl)
{
    const u8 pad                     = calc_padding(rsl->range_dist[4]);
    const double expected[RANGE_CAT] = {
        (double) output * RANGE_PROB1,
        (double) output * RANGE_PROB2,
        (double) output * RANGE_PROB3,
        (double) output * RANGE_PROB4,
        (double) output * RANGE_PROB5
    };

    // Calculate chi-square statistic for distribution of 64-bit numbers
    double delta[5];
    register double chi_calc = 0.0;

    for (u8 i = 0; i < RANGE_CAT; ++i) {
        delta[i] = (double) rsl->range_dist[i] - expected[i];
        if (expected[i] > 0)
            chi_calc += pow(delta[i], 2) / expected[i];
    }

    const u8 suspect_level = 32 - (RANGE_CRITICAL_VALUE < chi_calc);

    const double p_value = cephes_igamc(RANGE_CAT / 2, chi_calc / 2);

    printf("\033[1;34m\033[%uC             Minimum Value:\033[m %llu\n", indent, rsl->min);
    printf("\033[1;34m\033[%uC             Maximum Value:\033[m %llu\n", indent, rsl->max);
    printf("\033[1;34m\033[%uC                     Range:\033[m %llu\n", indent, rsl->max - rsl->min);
    printf("\033[1;34m\033[%uC        Range Distribution:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\n", indent, chi_calc, RANGE_CRITICAL_VALUE);
    printf("\033[2m\033[%uC            b.    [0, 2³²):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad + 1, rsl->range_dist[0], (u64) expected[0], (long long) delta[0]);
    printf("\033[2m\033[%uC            c.  [2³², 2⁴⁰):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad + 1, rsl->range_dist[1], (u64) expected[1], (long long) delta[1]);
    printf("\033[2m\033[%uC            d.  [2⁴⁰, 2⁴⁸):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad + 1, rsl->range_dist[2], (u64) expected[2], (long long) delta[2]);
    printf("\033[2m\033[%uC            e.  [2⁴⁸, 2⁵⁶):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad + 1, rsl->range_dist[3], (u64) expected[3], (long long) delta[3]);
    printf("\033[2m\033[%uC            f.  [2⁵⁶, 2⁶⁴):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad + 1, rsl->range_dist[4], (u64) expected[4], (long long) delta[4]);
    printf("\033[1;34m\033[%uC              Even Numbers:\033[m %llu (%u%%)\n", indent, output - rsl->odd, (u8) (((double) (output - rsl->odd) / (double) output) * 100));
    printf("\033[1;34m\033[%uC               Odd Numbers:\033[m %llu (%u%%)\n", indent, rsl->odd, (u8) (((double) rsl->odd / (double) output) * 100));
}

void print_ent_results(const u16 indent, const ent_test *rsl)
{
    char *chi_str;
    char chi_tmp[6];

    const u8 suspect_level = 32 - (rsl->pochisq <= ALPHA_LEVEL);

    printf("\033[1;34m\033[%uC                   Entropy:\033[m %.5lf %9s per byte)\n", indent, rsl->ent, "(bits");
    printf("\033[1;34m\033[%uC            ENT Chi-Square:\033[m %1.3lf\033[m  %14s\033[1;%um%1.2lf\033[m)\n", indent, rsl->chisq, "(p-value = ", suspect_level, rsl->pochisq);
    printf("\033[1;34m\033[%uC           Arithmetic Mean:\033[m %1.3lf%21s\n", indent, rsl->mean, "(127.5 = random)");
    printf("\033[1;34m\033[%uC  Monte Carlo Value for Pi:\033[m %1.9lf (error: %1.2f%%)\n", indent, rsl->montepicalc, rsl->monterr);

    if (rsl->scc >= -99999) {
        double scc = rsl->scc;
        if (rsl->scc < 0)
            scc *= -1.0;
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[m%1.6f%32s\n", indent, scc, "(totally uncorrelated = 0.0)");
    } else
        printf("\033[1;34m\033[%uC        Serial Correlation: \033[1;31mUNDEFINED\033[m %32s\n", indent, "(all values equal!)");
}

void print_chseed_results(const u16 indent, const basic_test *rsl)
{
    const u64 expected_chseeds = (rsl->chseed_exp * CHSEED_PROB);

    double delta[5];
    register double chi_calc = 0.0;
    for (u8 i = 0; i < CHSEED_CAT; ++i) {
        delta[i] = (double) rsl->chseed_dist[i] - (double) expected_chseeds;
        chi_calc += pow(delta[i], 2) / (double) expected_chseeds;
    }

    const u8 suspect_level = 32 - (CHSEED_CRITICAL_VALUE < chi_calc);
    const double p_value   = cephes_igamc(CHSEED_CAT / 2, chi_calc / 2);

    printf("\033[1;34m\033[%uC Chaotic Seed Distribution:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\n", indent, chi_calc, CHSEED_CRITICAL_VALUE);
    printf("\033[2m\033[%uC   b. Average Chaotic Seed:\033[m %1.15lf (ideal = 0.25)\n", indent, rsl->avg_chseed / (double) rsl->chseed_exp);
    printf("\033[2m\033[%uC             c. (0.0, 0.1):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->chseed_dist[0], expected_chseeds, (long long) delta[0]);
    printf("\033[2m\033[%uC             d. [0.1, 0.2):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->chseed_dist[1], expected_chseeds, (long long) delta[1]);
    printf("\033[2m\033[%uC             e. [0.2, 0.3):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->chseed_dist[2], expected_chseeds, (long long) delta[2]);
    printf("\033[2m\033[%uC             f. [0.3, 0.4):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->chseed_dist[3], expected_chseeds, (long long) delta[3]);
    printf("\033[2m\033[%uC             g. [0.4, 0.5):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->chseed_dist[4], expected_chseeds, (long long) delta[4]);
}

void print_fp_results(const u16 indent, const u64 output, const fp_test *rsl)
{
    register double expected = output * FPF_PROB;

    /*   FREQUENCY   */

    register double chi_calc = 0.0;
    register u8 i            = 0;
    for (; i < FPF_CAT; ++i)
        chi_calc += pow((rsl->fpf_dist[i] - expected), 2) / expected;

    register u8 suspect_level = 32 - (FPF_CRITICAL_VALUE < chi_calc);

    // Divide frequency values into "quadrants" and report observed vs expected values
    double delta[4];
    expected = (double) output * 0.25;
    for (u8 i = 0; i < 4; ++i)
        delta[i] = rsl->fpf_quad[i] - expected;

    register double p_value = cephes_igamc(FPF_CAT / 2, chi_calc / 2);

    printf("\033[1;34m\033[%uC              FP Frequency:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\n", indent, chi_calc, FPF_CRITICAL_VALUE);
    printf("\033[2m\033[%uC       b. Average FP Value:\033[m %1.15lf (ideal = 0.5)\n", indent, rsl->avg_fp);
    printf("\033[2m\033[%uC            c. (0.0, 0.25):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->fpf_quad[0], (u64) expected, (long long) delta[0]);
    printf("\033[2m\033[%uC            d. [0.25, 0.5):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->fpf_quad[1], (u64) expected, (long long) delta[1]);
    printf("\033[2m\033[%uC            e. [0.5, 0.75):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->fpf_quad[2], (u64) expected, (long long) delta[2]);
    printf("\033[2m\033[%uC            f. [0.75, 1.0):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, rsl->fpf_quad[3], (u64) expected, (long long) delta[3]);

    /*   PERMUTATIONS   */

    expected       = rsl->perms * FP_PERM_PROB;
    chi_calc       = 0;
    i              = 0;
    double average = 0.0;
    for (; i < FP_PERM_CAT; ++i) {
        average += rsl->fp_perms[i];
        chi_calc += pow(((double) rsl->fp_perms[i] - expected), 2) / expected;
    }

    // Get standard deviation for observed permutation orderings
    average /= FP_PERM_CAT;
    register double std_dev = 0.0;
    for (; i < FP_PERM_CAT; ++i)
        std_dev += pow(((double) rsl->fp_perms[i] - average), 2);

    suspect_level = 32 - (FP_PERM_CRITICAL_VALUE < chi_calc);
    std_dev       = sqrt(std_dev / (double) FP_PERM_CAT);

    p_value = cephes_igamc(FP_PERM_CAT / 2, chi_calc / 2);

    printf("\033[1;34m\033[%uC           FP Permutations:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %1.3lf)\n", indent, chi_calc, FP_PERM_CRITICAL_VALUE);
    printf("\033[2m\033[%uC     b. Total Permutations:\033[m %llu\n", indent, rsl->perms);
    printf("\033[2m\033[%uC        c. Average Per Bin:\033[m %llu (\033[1m%+lli\033[m: exp. %llu)\n", indent, (u64) average, (u64) average - (u64) expected, (u64) expected);
    printf("\033[2m\033[%uC     d. Standard Deviation:\033[m %1.3lf\n", indent, std_dev);

    /*   MAX-OF-8   */

    expected = (rsl->fp_max_runs) * FP_MAX_PROB;
    chi_calc = 0;
    i        = 0;
    average  = 0.0;

    register u8 most_pos, least_pos;
    most_pos = least_pos = 0;
    for (; i < FP_MAX_CAT; ++i) {
        most_pos  = (rsl->fp_max_dist[most_pos] > rsl->fp_max_dist[i]) ? most_pos : i;
        least_pos = (rsl->fp_max_dist[least_pos] < rsl->fp_max_dist[i]) ? least_pos : i;
        chi_calc += pow(((double) rsl->fp_max_dist[i] - expected), 2) / expected;
    }

    suspect_level = 32 - (FP_MAX_CRITICAL_VALUE < chi_calc);

    p_value = cephes_igamc(FP_MAX_CAT / 2, chi_calc / 2);

    printf("\033[1;34m\033[%uC               FP Max-of-8:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %1.3lf)\n", indent, chi_calc, FP_MAX_CRITICAL_VALUE);
    printf("\033[2m\033[%uC             b. Total Sets:\033[m %llu\n", indent, rsl->fp_max_runs);
    printf("\033[2m\033[%uC   c. Most Common Position:\033[m %u (exp. %llu : \033[1m%+lli\033[m)\n", indent, most_pos + 1, (u64) expected, rsl->fp_max_dist[most_pos] - (u64) expected);
    printf("\033[2m\033[%uC  d. Least Common Position:\033[m %u (exp. %llu : \033[1m%+lli\033[m)\n", indent, least_pos + 1, (u64) expected, rsl->fp_max_dist[least_pos] - (u64) expected);
}

void print_sp_results(const u16 indent, const rng_test *rsl, const u64 *sat_dist, const u64 *sat_range)
{
    const u64 output              = sat_range[0] + sat_range[1] + sat_range[2] + sat_range[3] + sat_range[4];
    const double expected[SP_CAT] = {
        (double) output * SP_PROB1,
        (double) output * SP_PROB2,
        (double) output * SP_PROB3,
        (double) output * SP_PROB4,
        (double) output * SP_PROB5
    };

    register double average, chi_calc;
    average = chi_calc = 0.0;

    register u8 i      = 0;
    register u64 count = 0;
    do {
        count += sat_dist[i];
        average += sat_dist[i] * (i + 16);
    } while (++i < SP_DIST);

    average /= count;

    register u64 tmp;
    tmp = i = 0;
    for (; i < SP_CAT; ++i) {
        chi_calc += pow(((double) sat_range[i] - expected[i]), 2) / expected[i];
        tmp = MAX(tmp, sat_range[i]);
    }

    const u8 suspect_level = 32 - (SP_CRITICAL_VALUE < chi_calc);

    const u8 pad         = calc_padding(tmp);
    const double p_value = cephes_igamc(SP_CAT / 2.0, chi_calc / 2.0);

    printf("\033[1;34m\033[%uC     Saturation Point Test:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\n", indent, chi_calc, SP_CRITICAL_VALUE);
    printf("\033[2m\033[%uCb. Average Saturation Point:\033[m %llu (ideal = %u)\n", indent - 1, (u64) average, SP_EXPECTED);
    printf("\033[2m\033[%uC               c. [16, 39):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, sat_range[0], (u64) expected[0], sat_range[0] - (u64) expected[0]);
    printf("\033[2m\033[%uC               d. [39, 46):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, sat_range[1], (u64) expected[1], sat_range[1] - (u64) expected[1]);
    printf("\033[2m\033[%uC               e. [46, 55):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, sat_range[2], (u64) expected[2], sat_range[2] - (u64) expected[2]);
    printf("\033[2m\033[%uC               f. [54, 64]:\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, sat_range[3], (u64) expected[3], sat_range[3] - (u64) expected[3]);
    printf("\033[2m\033[%uC              g. [65, INF):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, sat_range[4], (u64) expected[4], sat_range[4] - (u64) expected[4]);
}

void print_maurer_results(const u16 indent, maurer_test *rsl)
{
    const double lower_bound = MAURER_EXPECTED - (MAURER_Y * rsl->std_dev);
    const double upper_bound = MAURER_EXPECTED + (MAURER_Y * rsl->std_dev);
    const double pass_rate   = (double) rsl->pass / (double) rsl->trials;

    // we don't explicitly halve the df value because chi-square for fisher method
    // value uses 2K df, so after halving it becomes the original value k
    const double final_pvalue = cephes_igamc(rsl->trials, rsl->fisher / 2);
    const u8 suspect_level    = 32 - (final_pvalue <= ALPHA_LEVEL);

    rsl->mean /= rsl->trials;

    printf("\033[1;34m\033[%uCMaurer Universal Statistic:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, final_pvalue);
    printf("\033[2m\033[%uCa. Raw Fisher's Method Value:\033[m %1.3lf\n", indent - 2, rsl->fisher);
    printf("\033[2m\033[%uC                   b. Mean:\033[m %1.7lf (exp. %1.7lf : %+1.7lf)\n", indent, rsl->mean, MAURER_EXPECTED, rsl->mean - MAURER_EXPECTED);
    printf("\033[2m\033[%uC              c. Pass Rate:\033[m %llu/%llu (%llu%%)\n", indent, rsl->pass, rsl->trials, (u64) (pass_rate * 100.0));
    printf("\033[2m\033[%uC                      d. C:\033[m %1.3lf\n", indent, MAURER_C);
    printf("\033[2m\033[%uC     e. Standard Deviation:\033[m %1.7lf\n", indent, rsl->std_dev);
    printf("\033[2m\033[%uC            f. Lower Bound:\033[m %1.7lf\n", indent, lower_bound);
    printf("\033[2m\033[%uC            g. Upper Bound:\033[m %1.7lf\n", indent, upper_bound);
}

void print_tbt_results(const u16 indent, const tb_test *topo)
{
    const double proportion    = ((double) topo->prop_sum / (double) topo->total_u16);
    const u16 average_distinct = ((double) topo->prop_sum / (double) topo->trials);
    const u8 pass_rate         = ((double) topo->pass_rate / (double) topo->trials) * 100;

    printf("\033[1;34m\033[%uC   Topological Binary Test:\033[m %llu/%llu (%u%%)\n", indent, topo->pass_rate, topo->trials, pass_rate);
    printf("\033[2m\033[%uCa. Average Distinct Patterns:\033[m %u (cv = %u)\n", indent - 2, average_distinct, TBT_CRITICAL_VALUE);
    printf("\033[2m\033[%uC             b. Proportion:\033[m %.3lf (min. %.3f)\n", indent, proportion, TBT_PROPORTION);
}

void print_vnt_results(const u16 indent, const vn_test *von)
{
    const double seq_pass_rate = (double) von->pass_rate / (double) von->trials;
    const u8 suspect_level     = 32 - (von->p_value <= ALPHA_LEVEL);

    printf("\033[1;34m\033[%uC    Von Neumann Ratio Test:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, von->p_value);
    printf("\033[2m\033[%uCa. Raw Fisher's Method Value:\033[m %1.3lf\n", indent - 2, von->fisher);
    printf("\033[2m\033[%uC              b. Pass Rate:\033[m %llu/%llu (%llu%%)\n", indent, von->pass_rate, von->trials, (u64) (seq_pass_rate * 100.0));
}

void print_avalanche_results(const u16 indent, const basic_test *rsl, const u64 *ham_dist)
{
    // See test.h for more details on these probabilities
    static double expected[65] = {
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

    const u64 output = rsl->sequences << 8;

    register double average = 0.0;

    double bin_counts[4];
    bin_counts[0] = bin_counts[1] = bin_counts[2] = bin_counts[3] = 0.0;

    double quadrants[4];
    quadrants[0] = quadrants[1] = quadrants[2] = quadrants[3] = 0.0;

    register u8 i = 0;
    do {
        quadrants[i >> 4] += ham_dist[i];
        expected[i] *= output;
        bin_counts[i >> 4] += (u64) expected[i];
        average += ham_dist[i] * i;
    } while (++i < 65);

    register double chi_calc = 0.0;
    i                        = 0;
    do
        chi_calc += pow((quadrants[i] - bin_counts[i]), 2) / bin_counts[i];
    while (++i < AVALANCHE_CAT);

    register u8 suspect_level = 32 - (AVALANCHE_CRITICAL_VALUE <= chi_calc);
    average                   = average / (quadrants[0] + quadrants[1] + quadrants[2] + quadrants[3]);

    // Figure out the right amount of padding based on length of expected bin counts
    register u8 pad = 5;
    register u8 tmp = calc_padding((bin_counts[0] > 0) ? (u64) bin_counts[0] : (u64) (bin_counts[0] * -1.0));
    pad             = MAX(pad, tmp);
    tmp             = calc_padding((bin_counts[1] > 0) ? (u64) bin_counts[1] : (u64) (bin_counts[1] * -1.0));
    pad             = MAX(pad, tmp);
    tmp             = calc_padding((bin_counts[2] > 0) ? (u64) bin_counts[2] : (u64) (bin_counts[2] * -1.0));
    pad             = MAX(pad, tmp);
    tmp             = calc_padding((bin_counts[3] > 0) ? (u64) bin_counts[3] : (u64) (bin_counts[3] * -1.0));
    pad             = MAX(pad, tmp);

    const double p_value = cephes_igamc(AVALANCHE_CAT / 2, chi_calc / 2.0);

    printf("\033[1;34m\033[%uCStrict Avalanche Criterion:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, p_value);
    printf("\033[2m\033[%uC         a. Raw Chi-Square:\033[m %1.3lf (cv = %.3lf)\033[m\n", indent, chi_calc, AVALANCHE_CRITICAL_VALUE);
    printf("\033[2m\033[%uC  b. Mean Hamming Distance:\033[m %2.3lf (ideal = %u)\n", indent, average, 32);
    printf("\033[2m\033[%uC               c.  [0, 16):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, (u64) quadrants[0], (u64) bin_counts[0], (u64) quadrants[0] - (u64) bin_counts[0]);
    printf("\033[2m\033[%uC               d. [16, 32):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, (u64) quadrants[1], (u64) bin_counts[1], (u64) quadrants[1] - (u64) bin_counts[1]);
    printf("\033[2m\033[%uC               e. [32, 48):\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, (u64) quadrants[2], (u64) bin_counts[2], (u64) quadrants[2] - (u64) bin_counts[2]);
    printf("\033[2m\033[%uC               f. [48, 64]:\033[m %*llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, pad, (u64) quadrants[3], (u64) bin_counts[3], (u64) quadrants[3] - (u64) bin_counts[3]);
}

void print_wht_results(const u16 indent, const wh_test *walsh)
{
    const u64 num_total        = walsh->trials * 489;
    const double u32_pass_rate = (double) walsh->pass_num / (double) (num_total << 7);
    const double seq_pass_rate = (double) walsh->pass_seq / (double) num_total;
    const u64 expected         = (double) num_total * 0.1;

    const u8 suspect_level = 32 - (walsh->p_value <= ALPHA_LEVEL);

    printf("\033[1;34m\033[%uC         WH Transform Test:\033[m\033[1;%um %1.2lf\033[m\n", indent, suspect_level, walsh->p_value);
    printf("\033[2m\033[%uCa. Raw Fisher's Method Value:\033[m %1.3lf\n", indent - 2, walsh->fisher);
    printf("\033[2m\033[%uC              b. Pass Rate:\033[m %llu/%llu (%llu%% : SEQUENCES)\n", indent, walsh->pass_seq, num_total, (u64) (seq_pass_rate * 100.0));
    printf("\033[2m\033[%uC              c. Pass Rate:\033[m %llu/%llu (%llu%% : U128)\n", indent, walsh->pass_num, num_total << 7, (u64) (u32_pass_rate * 100.0));
    printf("\033[2m\033[%uC             d. (0.0, 0.2):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, walsh->dist[0], expected, walsh->dist[0] - expected);
    printf("\033[2m\033[%uC             e. [0.2, 0.4):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, walsh->dist[1], expected, walsh->dist[1] - expected);
    printf("\033[2m\033[%uC             f. [0.4, 0.6):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, walsh->dist[2], expected, walsh->dist[2] - expected);
    printf("\033[2m\033[%uC             g. [0.6, 0.8):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, walsh->dist[3], expected, walsh->dist[3] - expected);
    printf("\033[2m\033[%uC             h. [0.8, 1.0):\033[m %llu (exp. %llu : \033[1m%+lli\033[m)\n", indent, walsh->dist[4], expected, walsh->dist[4] - expected);
}