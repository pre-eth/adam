#include <math.h>
#include <stdlib.h>

#include "../include/rng.h"
#include "../include/support.h"
#include "../include/test.h"

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

static u64 tbt_pass, tbt_prop_sum;

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
    rsl->maurer_pass += (p_value >= ALPHA_LEVEL);
    rsl->maurer_fisher += log(p_value);
}

static void tbt(u64 *tbt_array, const u16 *nums)
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
    // Checks the level of compressiiblity of output
    MEMCPY(&rsl->maurer_bytes[maurer_ctr << 11], _ptr, ADAM_BUF_BYTES);
    if (++maurer_ctr == 489) {
        maurer(rsl);
        maurer_ctr = 0;
    }

    // Topological Binary Test
    // Checks for distinct patterns in a certain collection of numbers
    tbt(rsl->tbt_array, (u16 *) _ptr);

    // Strict Avalanche Criterion (SAC) test
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
    register long int rate = limit >> 14;

    rng_test rsl;
    ent_test ent;

    rsl.sequences = rate;

    // General initialization
    MEMCPY(&rsl.init_values[0], data->seed, sizeof(u64) * 4);
    rsl.init_values[4] = data->nonce;

    rsl.avg_chseed = rsl.avg_fp = 0.0;
    rsl.mfreq = rsl.up_runs = rsl.longest_up = rsl.down_runs = rsl.longest_down = 0;
    rsl.one_runs = rsl.perms = rsl.longest_one = rsl.zero_runs = rsl.longest_zero = rsl.odd = 0;

    // Bit Array for representing 2^16 values
    rsl.tbt_array = calloc(0, sizeof(u64) * 1024);

    // SAC and ENT init values
    const u64 nonce      = data->nonce + 1;
    adam_data sac_runner = adam_setup(data->seed, &nonce);

    run_rng(data);
    rsl.min = rsl.max = data->out[0];
    ent.sccu0         = data->out[0] & 0xFF;

    // Maurer test init - calculations were pulled from the NIST STS implementation
    maurer_k           = MAURER_ARR_SIZE - MAURER_Q;
    rsl.maurer_c       = 0.7 - 0.8 / (double) MAURER_L + (4 + 32 / (double) MAURER_L) * pow(maurer_k, -3 / (double) MAURER_L) / 15.0;
    rsl.maurer_std_dev = rsl.maurer_c * sqrt(MAURER_VARIANCE / (double) maurer_k);
    rsl.maurer_bytes   = malloc(MAURER_ARR_SIZE * sizeof(u8));
    MEMCPY(&rsl.maurer_bytes[maurer_ctr++], data->out, ADAM_BUF_BYTES);
    rsl.maurer_mean = rsl.maurer_fisher = 0.0;
    rsl.maurer_pass                     = 0;

    do {
        run_rng(sac_runner);
        test_loop(&rsl, data->out, data->chseeds, sac_runner->out);
        MEMCPY(sac_runner, data, sizeof(struct adam_data_s));
        sac_runner->nonce += 1;
        run_rng(data);
    } while (--rate > 0);

    adam_results(limit, &rsl, &ent);
    free(rsl.tbt_array);
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

    print_avalanche_results(indent, rsl, &ham_dist[0]);
}
}