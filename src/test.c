#include "../include/test.h"
#include "../include/rng.h"

static u64 copy[BUF_SIZE];
static u64 range_dist[RANGE_CAT];
static u64 chseed_dist[CHSEED_CAT];
static u64 gaps[256];
static u64 gaplengths[256];
static u8 lcb[5], mcb[5];
static u64 fpfreq_dist[FPF_CAT];
static u64 fpf_quadrants[4];
static double hamming_distance;

static void ham(const u64 num)
{
    static u8 i;
    hamming_distance += POPCNT(copy[i] ^ num);
    ++i;
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

static void chseed_unif(double *chseeds, double *avg_chseed)
{
    register u8 i = 0, idx;
    do {
        idx = (chseeds[i] >= 0.1) + (chseeds[i] >= 0.2) + (chseeds[i] >= 0.3) + (chseeds[i] >= 0.4);
        *avg_chseed += chseeds[i];
        ++chseed_dist[idx];
    } while (++i < (ROUNDS << 2));
}

static void test_loop(rng_test *rsl)
{
    // Chaotic seeds all occur within (0.0, 0.5).
    // This function tracks their distribution and we check the uniformity at the end.
    chseed_unif(rsl->chseeds, &rsl->avg_chseed);

    register u16 i = 0;
    u64 num;
    do {
        num = rsl->buffer[i];
        ++range_dist[(num >= __UINT32_MAX__) + (num >= (1ULL << 40)) + (num >= (1ULL << 48)) + (num >= (1ULL << 56))];

        // https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
        rsl->min = num ^ ((rsl->min ^ num) & -(rsl->min < num));
        rsl->max = rsl->max ^ ((rsl->max ^ num) & -(rsl->max < num));

        rsl->odd += (num & 1);
        rsl->zeroes += (num < 0);
        rsl->mfreq += POPCNT(num);

        // Tracks the amount of runs AND longest run, both increasing and decreasing
        tally_runs(num, rsl);

        // Checks gap lengths
        gap_lengths(num);

        // Calls into ENT framework, updating all the stuff there
        ent_test((u8 *) &num);
    } while (++i < BUF_SIZE);
}

void adam_test(const u64 limit, rng_test *rsl)
{
    register long int rate   = limit >> 14;
    register short leftovers = limit & (SEQ_SIZE - 1);

    rsl->avg_chseed = 0.0;
    rsl->mfreq = rsl->zeroes = rsl->up_runs = rsl->longest_up = rsl->down_runs = rsl->longest_down = rsl->odd = 0;

    rsl->sequences       = rate + !!(leftovers);
    rsl->expected_chseed = rsl->sequences * (ROUNDS << 2);

    adam_run(rsl->seed, &rsl->nonce);

    rsl->min = rsl->max = rsl->buffer[0];
    rsl->ent->sccu0     = rsl->buffer[0] & 0xFF;

    do {
        test_loop(rsl);
        adam_run(rsl->seed, &rsl->nonce);
        leftovers -= (u16) (rate <= 0) << 14;
    } while (LIKELY(--rate > 0) || LIKELY(leftovers > 0));

    ent_results(rsl->ent);

    register double average_gaplength = 0.0;

    register u16 i = 0;

    register u64 tmp;
    rsl->freq_max = rsl->freq_min = rsl->ent->freq[0];
    for (; i < BUF_SIZE; ++i) {
        tmp = rsl->ent->freq[i];
        average_gaplength += ((double) gaplengths[i] / (double) (tmp - 1));
        rsl->freq_min = tmp ^ ((rsl->freq_min ^ tmp) & -(rsl->freq_min < tmp));
        rsl->freq_max = rsl->freq_max ^ ((rsl->freq_max ^ tmp) & -(rsl->freq_max < tmp));
    }

    rsl->avg_gap = average_gaplength / 256.0;

    rsl->chseed_dist = &chseed_dist[0];
    rsl->range_dist  = &range_dist[0];
}
