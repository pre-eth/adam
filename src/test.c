#include <math.h>
#include <stdlib.h>

#include "../include/rng.h"
#include "../include/test.h"
#include "../include/util.h"

// Redefinition of API struct here so we can access internals
struct adam_data_s {
    // 256-bit seed
    u64 seed[4];

    // 64-bit nonce
    u64 nonce;

    // 8 64-bit initialization vectors part of internal state
    u64 IV[8];

    // Output vector - 256 64-bit integers = 2048 bytes
    u64 out[BUF_SIZE] ALIGN(ADAM_ALIGNMENT);

    // Work maps - sizeof(u64) * 512 = 4096 bytes
    u64 work_buffer[BUF_SIZE << 1] ALIGN(ADAM_ALIGNMENT);

    // The seeds supplied to each iteration of the chaotic function
    double chseeds[ROUNDS << 2] ALIGN(ADAM_ALIGNMENT);

    // Counter
    u64 cc;

    //  Current index in buffer (as bytes)
    u16 buff_idx;
};

static u64 range_dist[RANGE_CAT];
static u64 chseed_dist[CHSEED_CAT];

static u64 gaps[256];
static u64 gaplengths[256];
static u8 lcb[5], mcb[5];

static u64 fpfreq_dist[FPF_CAT];
static u64 fpfreq_quadrants[4];
static u64 fp_perm_dist[FP_PERM_CAT];
static u64 fp_max_dist[FP_MAX_CAT];
static u64 fp_max_runs;

static u64 sat_range[SP_CAT + 1];
static u64 sat_dist[SP_DIST];

static u64 maurer_arr[1U << MAURER_L];
static u64 maurer_ctr;
static double maurer_k;

static u64 *tbt_array;
static u64 tbt_pass, tbt_prop_sum;

static u64 wh_pass_seq, wh_pass_num;
static u64 wh_pdist[10];
static double wh_fisher, wh_fisher_mb;

static u64 ham_dist[65];

static void sat_point(const u8 *nums)
{
    // Bitarray to track presence of 2^4 values
    static u16 num_range;
    static u64 ctr;

    register u16 i = 0, j = 0;
    do {
        // 2 4-bit quantities per 8 -bits
        // so we only increment <i> every other iteration
        num_range |= (1U << ((nums[i] >> ((j & 1) << 2)) & 15));

        ++ctr;

        // Ranges derived from probability table in paper
        if (num_range == __UINT16_MAX__) {
            // Log the actual saturation point
            if (ctr > SP_OBS_MAX)
                ++sat_dist[49];
            else
                ++sat_dist[ctr - SP_OBS_MIN];

            if (ctr >= SP_OBS_MIN && ctr < 39)
                ++sat_range[0];
            else if (ctr >= 39 && ctr < 46)
                ++sat_range[1];
            else if (ctr >= 46 && ctr < 54)
                ++sat_range[2];
            else if (ctr >= 54 && ctr <= SP_OBS_MAX)
                ++sat_range[3];
            else if (ctr > SP_OBS_MAX)
                ++sat_range[4];

            ctr = num_range = 0;
        }

        i += j;
        j = !j;
    } while (i < ADAM_BUF_BYTES);
}

static void maurer(rng_test *rsl)
{
    register u32 i = 0;
    for (; i < MAURER_Q; ++i)
        maurer_arr[rsl->maurer_bytes[i]] = i;

    register double sum = 0.0;

    i = 0;
    for (int i = MAURER_Q; i < MAURER_Q + maurer_k; ++i) {
        sum += log(i - maurer_arr[rsl->maurer_bytes[i]]) / log(2);
        maurer_arr[rsl->maurer_bytes[i]] = i;
    }

    // These 3 lines were pulled from the NIST STS implementation
    const double phi     = sum / maurer_k;
    const double x       = fabs(phi - MAURER_EXPECTED) / (sqrt(2) * rsl->maurer_std_dev);
    const double p_value = erfc(x);

    rsl->maurer_mean += phi;
    rsl->maurer_pass += (p_value > ALPHA_LEVEL);
    rsl->maurer_fisher += log(p_value);
}

static void tbt(const u16 *nums)
{
    static u8 ctr;

    register u16 i = 0;

    // Checks if this 10-bit pattern has been recorded
    // Each run gives us 1024 u16 which is the TBT_SEQ_SIZE
    do
        tbt_array[nums[i] >> 6] |= 1ULL << (nums[i] & 63);
    while (++i < (ADAM_BUF_BYTES >> 1));

    // 65536 / 1024 u16 per iteration = 64 iterations
    if (++ctr == 64) {
        register u16 different;
        i = different = 0;
        do
            different += POPCNT(tbt_array[i]);
        while (++i < 1024);

        tbt_prop_sum += different;
        tbt_pass += (different >= TBT_CRITICAL_VALUE);

        const double proportion = different / TBT_SEQ_SIZE;

        MEMSET(&tbt_array[0], 0, sizeof(u64) * 1024);
        ctr = 0;
    }
}

static void wh_test(const u32 *nums)
{
    register double stat, sum = 0.0;

    // 32-bit work units, but process 128-bits at a time for statistic
    for (u16 i = 0; i < (BUF_SIZE << 1); i += 4) {
        stat = wh_transform(i, nums[i]) + wh_transform(i, nums[i + 1]) + wh_transform(i, nums[i + 2]) + wh_transform(i, nums[i + 3]);
        wh_pass_num += (stat >= WH_LOWER_BOUND && stat <= WH_UPPER_BOUND);
        sum += pow(stat, 2);
    }

    // Now we've compiled 128 statistics, obtain a p-value
    const double p_value = cephes_igamc(WH_DF / 2, sum / 2);
    wh_pass_seq += (sum >= 0 && sum <= WH_CRITICAL_VALUE);

    // Add to Fisher method accumulator and record in p-value dist
    wh_fisher += log(p_value);
    ++wh_pdist[(u8) (p_value * 10.0)];
}

static void sac(const u64 *restrict run1, const u64 *restrict run2)
{
    register u16 i = 0;
    do
        ++ham_dist[POPCNT(run1[i] ^ run2[i])];
    while (++i < BUF_SIZE);
}

static void fp_max8(const u32 *nums)
{
    register u16 idx = 1;
    register u16 max, count;
    max = count = 0;

    register double d;
    register double last = (double) nums[0] / (double) __UINT32_MAX__;
    do {
        d    = (double) nums[idx] / (double) __UINT32_MAX__;
        max  = (d > last) ? (idx & 7) : max;
        last = d;
        ++idx;
        if (++count == FP_MAX_CAT) {
            ++fp_max_dist[max];
            ++fp_max_runs;
            max = count = 0;
            last        = (double) nums[idx] / (double) __UINT32_MAX__;
            ++idx;
        }
    } while (idx < (BUF_SIZE << 1));
}

static void fp_perm(const double num, u64 *perms)
{
    static int perm_idx;
    static double tuple[FP_PERM_SIZE + 1];

    tuple[++perm_idx] = num;
    if (perm_idx == FP_PERM_SIZE) {
        *perms += 1;

        register u64 f = 0;
        register u8 s;
        double d;
        while (perm_idx > 1) {
            s = perm_idx;
            switch (perm_idx) {
            case 5:
                s = (tuple[4] > tuple[s]) ? 4 : s;
            case 4:
                s = (tuple[3] > tuple[s]) ? 3 : s;
            case 3:
                s = (tuple[2] > tuple[s]) ? 2 : s;
            case 2:
                s = (tuple[1] > tuple[s]) ? 1 : s;
                break;
            default:
                break;
            }

            f               = (perm_idx * f) + s - 1;
            d               = tuple[perm_idx];
            tuple[perm_idx] = tuple[s];
            tuple[s]        = d;

            --perm_idx;
        }

        ++fp_perm_dist[f];
        perm_idx = 0;
    }
}

static void update_mcb(const u8 idx, const u64 *freq)
{
    register short i = 3;
    while (freq[idx] > freq[mcb[i]] && i >= 0) {
        mcb[i + 1] = mcb[i];
        --i;
    }
    mcb[i + 1] = idx;
}

static void update_lcb(const u8 idx, const u64 *freq)
{
    register short i = 3;
    while (freq[idx] < freq[lcb[i]] && i >= 0) {
        lcb[i + 1] = lcb[i];
        --i;
    }
    lcb[i + 1] = idx;
}

static void gap_lengths(u64 num)
{
    static u64 ctr;

    register u8 byte;
    register u64 dist;

    do {
        byte = num & 0xFF;
        dist = (++ctr - gaps[byte]);
        gaplengths[byte] += dist;
        gaps[byte] = ctr;
        num >>= 8;
    } while (num > 0);
}

static void tally_runs(const u64 num, rng_test *rsl)
{
    // 0 = up, 1 = down, -1 = init
    static short direction = -1;

    static u64 curr_up, curr_down, prev;

    if (num > prev) {
        if (direction != 0) {
            ++rsl->up_runs;
            rsl->longest_down = MAX(rsl->longest_down, curr_down);
            curr_down         = 0;
            direction         = 0;
        }
        ++curr_up;
    } else if (num < prev) {
        if (direction != 1) {
            ++rsl->down_runs;
            rsl->longest_up = MAX(rsl->longest_up, curr_up);
            curr_up         = 0;
            direction       = 1;
        }
        ++curr_down;
    }
    prev = num;
}

static void tally_bitruns(u64 num, rng_test *rsl)
{
    // 0 = 0, 1 = 1, -1 = init
    static short direction = -1;
    static u64 curr_zero, curr_one;

    do {
        if (num & 1) {
            if (direction != 1) {
                ++rsl->one_runs;
                rsl->longest_zero = MAX(rsl->longest_zero, curr_zero);
                curr_zero         = 0;
                direction         = 1;
            }
            ++curr_one;
        } else {
            if (direction != 0) {
                ++rsl->zero_runs;
                rsl->longest_one = MAX(rsl->longest_one, curr_one);
                curr_one         = 0;
                direction        = 0;
            }
            ++curr_zero;
        }
    } while (num >>= 1);
}

static void chseed_unif(const double *restrict chseeds, double *avg_chseed)
{
    register u8 i = 0, idx;
    do {
        idx = (chseeds[i] >= 0.1) + (chseeds[i] >= 0.2) + (chseeds[i] >= 0.3) + (chseeds[i] >= 0.4);
        *avg_chseed += chseeds[i];
        ++chseed_dist[idx];
    } while (++i < (ROUNDS << 2));
}

static void test_loop(rng_test *rsl, u64 *restrict _ptr, const double *restrict chseeds, const u64 *sac_run)
{
    // Chaotic seeds all occur within (0.0, 0.5).
    // This function tracks their distribution and we check the uniformity at the end.
    chseed_unif(chseeds, &rsl->avg_chseed);

    // Saturation Point Test
    // Determines index where all 2^4 values have appeared at least once
    sat_point((u8 *) _ptr);

    // Maurer Universal Test
    // Checks the level of compressiiblity of output, assuming 1MB of
    // bytes have been accumulated
    MEMCPY(&rsl->maurer_bytes[maurer_ctr << 11], _ptr, ADAM_BUF_BYTES);
    if (++maurer_ctr == TESTING_BITS / SEQ_SIZE) {
        maurer(rsl);
        maurer_ctr = 0;

        /*
            This counter is also used for the wh_fisher_mb accumulator,
            which is used to compile the p_values generated by wh_fisher.

            We do this to make sure p-value accuracy doesn't deteriorate
            too quickly for larger sequences, so we employ the Fisher method
            twice, this time per MB
        */
        wh_fisher *= -2;
        const double wh_pvalue = cephes_igamc(TESTING_BITS / SEQ_SIZE, wh_fisher / 2);

        wh_fisher_mb += log(wh_pvalue);
        wh_fisher = 0.0;
    }

    // Topological Binary Test
    // Checks for distinct patterns in a certain collection of numbers
    tbt((u16 *) _ptr);

    // Walsh-Hadamard Transform Test
    // Related to the frequency and autocorellation test, computes a
    // test statistic per u32 from a transformed binary sequence
    wh_test((u32 *) _ptr);

    // Strict Avalanche Criterion (SAC) Test
    // Records the Hamming Distance between this number and the number that
    // was in the same index in the buffer during the previous iteration
    sac(_ptr, sac_run);

    // 32-bit floating point max-of-T test with T = 8
    fp_max8((u32 *) _ptr);

    register u16 i = 0;
    register double d;
    u64 num;
    do {
        num = _ptr[i];
        rsl->odd += (num & 1);
        rsl->mfreq += POPCNT(num);

        // https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
        rsl->min = MIN(rsl->min, num);
        rsl->max = MAX(rsl->max, num);

        // Convert this number to float with same logic used for returning FP results
        // Then record float for FP freq distribution in (0.0, 1.0)
        rsl->avg_fp += d = ((double) num / (double) __UINT64_MAX__);
        ++fpfreq_dist[(u8) (d * 10.0)];
        ++fpfreq_quadrants[(d >= 0.25) + (d >= 0.5) + (d >= 0.75)];

        // Collect this floating point value into the permutations tuple
        // Once the tuple reaches 5 elements, the permutation is recorded
        fp_perm(d, &rsl->perms);

        // Record range that this number falls in
        ++range_dist[(num >= __UINT32_MAX__) + (num >= (1ULL << 40)) + (num >= (1ULL << 48)) + (num >= (1ULL << 56))];

        // Tracks the amount of runs AND longest run, both increasing and decreasing
        tally_runs(num, rsl);

        // Tracks the amount of runs AND longest runs for the bits in this number
        tally_bitruns(num, rsl);

        // Checks gap lengths
        gap_lengths(num);

        // Calls into ENT framework, updating all the stuff there
        ent_loop((const u8 *) &num);
    } while (++i < BUF_SIZE);
}

static void adam_results(const u64 limit, rng_test *rsl, ent_test *ent);

static void run_rng(adam_data data)
{
    accumulate(data->seed, data->IV, data->work_buffer, data->chseeds, data->cc);
    diffuse(data->out, data->nonce);
    apply(data->out, data->work_buffer, data->chseeds);
    mix(data->out, data->work_buffer);
    reseed(data->seed, data->work_buffer, &data->nonce, &data->cc);
}

void adam_examine(const u64 limit, adam_data data)
{
    rng_test rsl;
    ent_test ent;

    rsl.sequences = limit >> 14;

    // General initialization
    MEMCPY(&rsl.init_values[0], data->seed, sizeof(u64) * 4);
    rsl.init_values[4] = data->nonce;

    rsl.avg_chseed = rsl.avg_fp = 0.0;
    rsl.mfreq = rsl.up_runs = rsl.longest_up = rsl.down_runs = rsl.longest_down = 0;
    rsl.one_runs = rsl.perms = rsl.longest_one = rsl.zero_runs = rsl.longest_zero = rsl.odd = 0;

    // Bit Array for representing 2^16 values
    tbt_array = calloc(0, sizeof(u64) * 1024);

    // SAC and ENT init values
    const u64 nonce      = data->nonce + 1;
    adam_data sac_runner = adam_setup(data->seed, &nonce);

    run_rng(data);
    rsl.min = rsl.max = data->out[0];
    ent.sccu0         = data->out[0] & 0xFF;

    // Maurer test init - calculations were pulled from the NIST STS implementation
    maurer_k           = MAURER_ARR_SIZE - MAURER_Q;
    rsl.maurer_pass    = 0;
    rsl.maurer_c       = 0.7 - 0.8 / (double) MAURER_L + (4 + 32 / (double) MAURER_L) * pow(maurer_k, -3 / (double) MAURER_L) / 15.0;
    rsl.maurer_std_dev = rsl.maurer_c * sqrt(MAURER_VARIANCE / (double) maurer_k);
    rsl.maurer_bytes   = malloc(MAURER_ARR_SIZE * sizeof(u8));
    MEMCPY(&rsl.maurer_bytes[maurer_ctr++], data->out, ADAM_BUF_BYTES);
    rsl.maurer_mean = rsl.maurer_fisher = 0.0;

    register long long rate = rsl.sequences;
    do {
        run_rng(sac_runner);
        test_loop(&rsl, data->out, data->chseeds, sac_runner->out);
        MEMCPY(sac_runner, data, sizeof(struct adam_data_s));
        sac_runner->nonce ^= (1ULL << (data->nonce & 63));
        run_rng(data);
    } while (--rate > 0);

    adam_results(limit, &rsl, &ent);
    free(tbt_array);
    free(rsl.maurer_bytes);
    adam_cleanup(sac_runner);
}

static void adam_results(const u64 limit, rng_test *rsl, ent_test *ent)
{
    // Screen info for pretty printing
    u16 center, indent, swidth;
    get_print_metrics(&center, &indent, &swidth);
    indent <<= 1;

    // First get the ENT results out of the way
    ent_results(ent);

    // Gap related info
    register double average_gaplength = 0.0;

    register u16 i = 0;
    register u64 tmp;
    for (; i < BUF_SIZE; ++i) {
        tmp = ent->freq[i];
        average_gaplength += ((double) gaplengths[i] / (double) (tmp - 1));
        update_lcb(i, ent->freq);
        update_mcb(i, ent->freq);
    }

    print_basic_results(indent, rsl, limit);
    print_mfreq_results(indent, rsl);

    rsl->avg_gap = average_gaplength / 256.0;
    print_byte_results(indent, rsl, &mcb[0], &lcb[0]);

    print_range_results(indent, rsl, &range_dist[0]);
    print_ent_results(indent, ent);
    print_chseed_results(indent, rsl->sequences * (ROUNDS << 2), &chseed_dist[0], rsl->avg_chseed);

    rsl->avg_fp /= (rsl->sequences << 8);
    rsl->fp_max_runs = fp_max_runs;
    print_fp_results(indent, rsl, &fpfreq_dist[0], &fpfreq_quadrants[0], &fp_perm_dist[0], &fp_max_dist[0]);

    print_sp_results(indent, rsl, &sat_dist[0], &sat_range[0]);

    rsl->maurer_fisher *= -2.0;
    print_maurer_results(indent, rsl, limit / TESTING_BITS);

    print_tbt_results(indent, rsl->sequences >> 6, tbt_prop_sum, tbt_pass);

    wh_fisher_mb *= -2.0;
    print_wht_results(indent, wh_fisher_mb, wh_pass_seq, wh_pass_num, limit / TESTING_BITS, &wh_pdist[0]);

    print_avalanche_results(indent, rsl, &ham_dist[0]);
}

/*    FOLLOWING CODE IS FROM THE CEPHES C MATH LIBRARY    */

// 2**-53
static double MACHEP = 1.11022302462515654042E-16;

// log(MAXNUM)
static double MAXLOG = 7.09782712893383996732224E2;

// 2**1024*(1-MACHEP)
static double MAXNUM = 1.7976931348623158E308;

static double big    = 4.503599627370496e15;
static double biginv = 2.22044604925031308085e-16;

/*
    A[]: Stirling's formula expansion of log gamma
    B[], C[]: log gamma function between 2 and 3
*/
static u16 A[] = {
    0x6661, 0x2733, 0x9850, 0x3f4a,
    0xe943, 0xb580, 0x7fbd, 0xbf43,
    0x5ebb, 0x20dc, 0x019f, 0x3f4a,
    0xa5a1, 0x16b0, 0xc16c, 0xbf66,
    0x554b, 0x5555, 0x5555, 0x3fb5
};
static u16 B[] = {
    0x6761, 0x8ff3, 0x8901, 0xc095,
    0xb93e, 0x355b, 0xf234, 0xc0e2,
    0x89e5, 0xf890, 0x3d73, 0xc114,
    0xdb51, 0xf994, 0xbc82, 0xc131,
    0xf20b, 0x0219, 0x4589, 0xc13a,
    0x055e, 0x5418, 0x0c67, 0xc12a
};
static u16 C[] = {
    /*  0x0000,0x0000,0x0000,0x3ff0  */
    0x12b2, 0x1cf3, 0xfd0d, 0xc075,
    0xd757, 0x7b89, 0xaa0d, 0xc0d0,
    0x4c9b, 0xb974, 0xeb84, 0xc10a,
    0x0043, 0x7195, 0x6286, 0xc131,
    0xf34c, 0x892f, 0x5255, 0xc143,
    0xe14a, 0x6a11, 0xce4b, 0xc13e
};

#define MAXLGM 2.556348E305

static double cephes_polevl(double x, double *coef, int N)
{
    double ans;
    int i;
    double *p;

    p   = coef;
    ans = *p++;
    i   = N;

    do
        ans = ans * x + *p++;
    while (--i);

    return ans;
}

static double cephes_p1evl(double x, double *coef, int N)
{
    double ans;
    double *p;
    int i;

    p   = coef;
    ans = x + *p++;
    i   = N - 1;

    do
        ans = ans * x + *p++;
    while (--i);

    return ans;
}

/* Logarithm of gamma function */
static double cephes_lgam(double x)
{
    double p, q, u, w, z;
    int i;

    int sgngam = 1;

    if (x < -34.0) {
        q = -x;
        w = cephes_lgam(q); /* note this modifies sgngam! */
        p = floor(q);
        if (p == q) {
        lgsing:
            goto loverf;
        }
        i = (int) p;
        if ((i & 1) == 0)
            sgngam = -1;
        else
            sgngam = 1;
        z = q - p;
        if (z > 0.5) {
            p += 1.0;
            z = p - q;
        }
        z = q * sin(PI * z);
        if (z == 0.0)
            goto lgsing;
        /*      z = log(PI) - log( z ) - w;*/
        z = log(PI) - log(z) - w;
        return z;
    }

    if (x < 13.0) {
        z = 1.0;
        p = 0.0;
        u = x;
        while (u >= 3.0) {
            p -= 1.0;
            u = x + p;
            z *= u;
        }
        while (u < 2.0) {
            if (u == 0.0)
                goto lgsing;
            z /= u;
            p += 1.0;
            u = x + p;
        }
        if (z < 0.0) {
            sgngam = -1;
            z      = -z;
        } else
            sgngam = 1;
        if (u == 2.0)
            return (log(z));
        p -= 2.0;
        x = x + p;
        p = x * cephes_polevl(x, (double *) B, 5) / cephes_p1evl(x, (double *) C, 6);

        return log(z) + p;
    }

    if (x > MAXLGM) {
    loverf:
        return sgngam * MAXNUM;
    }

    q = (x - 0.5) * log(x) - x + log(sqrt(2 * PI));
    if (x > 1.0e8)
        return q;

    p = 1.0 / (x * x);
    if (x >= 1000.0)
        q += ((7.9365079365079365079365e-4 * p
                  - 2.7777777777777777777778e-3)
                     * p
                 + 0.0833333333333333333333)
            / x;
    else
        q += cephes_polevl(p, (double *) A, 4) / x;

    return q;
}

static double cephes_igam(double a, double x)
{
    double ans, ax, c, r;

    if ((x <= 0) || (a <= 0))
        return 0.0;

    if ((x > 1.0) && (x > a))
        return 1.e0 - cephes_igamc(a, x);

    /* Compute  x**a * exp(-x) / gamma(a)  */
    ax = a * log(x) - x - cephes_lgam(a);
    if (ax < -MAXLOG)
        return 0.0;

    ax = exp(ax);

    /* power series */
    r   = a;
    c   = 1.0;
    ans = 1.0;

    do {
        r += 1.0;
        c *= x / r;
        ans += c;
    } while (c / ans > MACHEP);

    return ans * ax / a;
}

double cephes_igamc(double a, double x)
{
    double ans, ax, c, yc, r, t, y, z;
    double pk, pkm1, pkm2, qk, qkm1, qkm2;

    if ((x <= 0) || (a <= 0))
        return (1.0);

    if ((x < 1.0) || (x < a))
        return (1.e0 - cephes_igam(a, x));

    ax = a * log(x) - x - cephes_lgam(a);

    if (ax < -MAXLOG)
        return 0.0;

    ax = exp(ax);

    /* continued fraction */
    y    = 1.0 - a;
    z    = x + y + 1.0;
    c    = 0.0;
    pkm2 = 1.0;
    qkm2 = x;
    pkm1 = x + 1.0;
    qkm1 = z * x;
    ans  = pkm1 / qkm1;

    do {
        c += 1.0;
        y += 1.0;
        z += 2.0;
        yc = y * c;
        pk = pkm1 * z - pkm2 * yc;
        qk = qkm1 * z - qkm2 * yc;
        if (qk != 0) {
            r   = pk / qk;
            t   = fabs((ans - r) / r);
            ans = r;
        } else
            t = 1.0;
        pkm2 = pkm1;
        pkm1 = pk;
        qkm2 = qkm1;
        qkm1 = qk;
        if (fabs(pk) > big) {
            pkm2 *= biginv;
            pkm1 *= biginv;
            qkm2 *= biginv;
            qkm1 *= biginv;
        }
    } while (t > MACHEP);

    return ans * ax;
}