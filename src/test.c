#include <pthread.h>
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
    double chseeds[ROUNDS << 2];

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
static u64 fpf_quadrants[4];
static u64 fp_perm_dist[FP_PERM_CAT];

static double tbt_dist[TBT_DF];
static u8 tbt_trials, tbt_successes;
static u64 tbt_pass;

static u64 ham_dist[AVALANCHE_CAT + 1];


static void sac(const u64 *restrict run1, const u64 *restrict run2)
{
    register u16 i = 0;
    do
        ++ham_dist[POPCNT(run1[i] ^ run2[i])];
    while (++i < BUF_SIZE);
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
            rsl->longest_down = rsl->longest_down ^ ((rsl->longest_down ^ curr_down) & -(rsl->longest_down < curr_down));
            curr_down         = 0;
            direction         = 0;
        }
        ++curr_up;
    } else if (num < prev) {
        if (direction != 1) {
            ++rsl->down_runs;
            rsl->longest_up = rsl->longest_up ^ ((rsl->longest_up ^ curr_up) & -(rsl->longest_up < curr_up));
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
                rsl->longest_zero = rsl->longest_zero ^ ((rsl->longest_zero ^ curr_zero) & -(rsl->longest_zero < curr_zero));
                curr_zero         = 0;
                direction         = 1;
            }
            ++curr_one;
        } else {
            if (direction != 0) {
                ++rsl->zero_runs;
                rsl->longest_one = rsl->longest_one ^ ((rsl->longest_one ^ curr_one) & -(rsl->longest_one < curr_one));
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

    // Strict Avalanche Criterion (SAC) test
    // Records the Hamming Distance between this number and the number that
    // was in the same index in the buffer during the previous iteration
    sac(_ptr, sac_run);

    // Topological Binary Test
    // rsl->tbt((u16 *) _ptr);

    register u16 i = 0;
    register double d;
    u64 num;
    do {
        num = _ptr[i];
        rsl->odd += (num & 1);
        rsl->mfreq += POPCNT(num);

        // https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
        rsl->min = num ^ ((rsl->min ^ num) & -(rsl->min < num));
        rsl->max = rsl->max ^ ((rsl->max ^ num) & -(rsl->max < num));

        // Convert this number to float with same logic used for returning FP results
        // Then record float for FP freq distribution in (0.0, 1.0)
        rsl->avg_fp += d = ((double) num / (double) __UINT64_MAX__);
        ++fpfreq_dist[(u8) (d * 10.0)];
        ++fpf_quadrants[(d >= 0.25) + (d >= 0.5) + (d >= 0.75)];

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
    register long int rate   = limit >> 14;
    register short leftovers = limit & (SEQ_SIZE - 1);

    rng_test rsl;
    ent_test ent;

    MEMCPY(&rsl.init_values[0], data->seed, sizeof(u64) * 4);
    rsl.init_values[4] = data->nonce;

    rsl.avg_chseed = rsl.avg_fp = 0.0;
    rsl.mfreq = rsl.up_runs = rsl.longest_up = rsl.down_runs = rsl.longest_down = rsl.one_runs = rsl.perms = rsl.longest_one = rsl.zero_runs = rsl.longest_zero = rsl.odd = 0;

    rsl.sequences       = rate + !!leftovers;
    rsl.expected_chseed = rsl.sequences * (ROUNDS << 2);

    rsl.tbt = (limit >= 800000000) ? &tbt16 : &tbt10;

    const u64 nonce      = data->nonce + 1;
    adam_data sac_runner = adam_setup(adam_seed(data), &nonce);

    run_rng(data);

    rsl.min = rsl.max = data->out[0];
    ent.sccu0         = data->out[0] & 0xFF;

    do {
        run_rng(sac_runner);
        test_loop(&rsl, data->out, data->chseeds, sac_runner->out);
        MEMCPY(sac_runner, data, sizeof(struct adam_data_s));
        sac_runner->nonce += 1;
        run_rng(data);
        leftovers -= (u16) (rate <= 0) << 14;
    } while (LIKELY(--rate > 0) || LIKELY(leftovers > 0));

    adam_results(limit, &rsl, &ent);

    adam_cleanup(sac_runner);
}

static void adam_results(rng_test *rsl)
{
    // First get the ENT results out of the way
    ent_results(rsl->ent);

    register double average_gaplength = 0.0;

    register u16 i = 0;
    register u64 tmp;
    for (; i < BUF_SIZE; ++i) {
        tmp = rsl->ent->freq[i];
        average_gaplength += ((double) gaplengths[i] / (double) (tmp - 1));
        update_lcb(i, rsl->ent->freq);
        update_mcb(i, rsl->ent->freq);
    }

    rsl->avg_gap = average_gaplength / 256.0;
    rsl->avg_fp /= (rsl->sequences << 8);

    rsl->ham_dist    = &ham_dist[0];
    rsl->chseed_dist = &chseed_dist[0];
    rsl->range_dist  = &range_dist[0];
    rsl->fpf_dist    = &fpfreq_dist[0];
    rsl->fpf_quad    = &fpf_quadrants[0];
    rsl->perm_dist   = &fp_perm_dist[0];
    rsl->mcb         = &mcb[0];
    rsl->lcb         = &lcb[0];
}