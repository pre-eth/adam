#include <math.h>
#include <stdlib.h>

#include "../include/rng.h"
#include "../include/test.h"
#include "../include/util.h"

static void adam_results(const u64 limit, rng_test *rsl);

static u64 gaps[__UINT8_MAX__ + 1];
static u64 gaplengths[__UINT8_MAX__ + 1];

static u64 sat_range[SP_CAT + 1];
static u64 sat_dist[SP_DIST];

static u64 maurer_arr[1U << MAURER_L];
static u64 maurer_ctr;

static u64 *tbt_array;

static double vnt_fisher, vnt_fisher_gb;

static u16 wh_gb_ctr;
static double wh_fisher, wh_fisher_mb, wh_fisher_gb;

static u64 ham_dist[ADAM_WORD_BITS + 1];

static void sat_point(const u8 *nums)
{
    // Bitarray to track presence of 2^4 values
    static u16 num_range;
    static u64 ctr;
 
    register u16 i = 0, j = 0;
    do {
        // 2 4-bit quantities per 8-bits
        // so we only increment <i> every other iteration
        num_range |= (1U << ((nums[i] >> ((j & 1) << 2)) & 15));

        ++ctr;

        // Ranges derived from probability table in paper
        if (num_range == __UINT16_MAX__) {
            // Log the actual saturation point
            if (ctr > SP_OBS_MAX) {
                ++sat_dist[49];
            } else {
                ++sat_dist[ctr - SP_OBS_MIN];
            }
            
            if (ctr >= SP_OBS_MIN && ctr < 39) {
                ++sat_range[0];
            } else if (ctr >= 39 && ctr < 46) {
                ++sat_range[1];
            } else if (ctr >= 46 && ctr < 54) {
                ++sat_range[2];
            } else if (ctr >= 54 && ctr <= SP_OBS_MAX) {
                ++sat_range[3];
            } else if (ctr > SP_OBS_MAX) {
                ++sat_range[4];
            }
            
            ctr = num_range = 0;
        }

        i += j;
        j = !j;
    } while (i < 8);
}

static void maurer(maurer_test *mau)
{
    register u32 i = 0;
    for (; i < MAURER_Q; ++i) {
        maurer_arr[mau->bytes[i]] = i;
    }

    register double sum = 0.0;

    i = MAURER_Q;
    for (; i < MAURER_Q + MAURER_K; ++i) {
        sum += log(i - maurer_arr[mau->bytes[i]]) / log(2);
        maurer_arr[mau->bytes[i]] = i;
    }

    // These 3 lines were pulled from the NIST STS implementation
    const double phi     = sum / MAURER_K;
    const double x       = fabs(phi - MAURER_EXPECTED) / (sqrt(2) * mau->std_dev);
    const double p_value = erfc(x);

    mau->mean += phi;
    mau->pass += (p_value >= ALPHA_LEVEL);
    mau->fisher += log(p_value);
}

static void tbt(const u16 *nums, tb_test *topo)
{
    static u32 ctr;

    // Checks if this 16-bit pattern has been recorded
    tbt_array[nums[0] >> 6] |= 1ULL << (nums[0] & 63);
    tbt_array[nums[1] >> 6] |= 1ULL << (nums[1] & 63);
    tbt_array[nums[2] >> 6] |= 1ULL << (nums[2] & 63);
    tbt_array[nums[3] >> 6] |= 1ULL << (nums[3] & 63);

    // TBT_SEQ_SIZE iterations before we can update our test metrics
    ctr += 4;
    if (ctr == TBT_SEQ_SIZE) {
        register u32 i, different;
        i = different = 0;
        do {
            different += POPCNT(tbt_array[i]);
        } while (++i < TBT_ARR_SIZE);

        ++topo->trials;
        topo->prop_sum += different;
        topo->pass_rate += (different >= TBT_CRITICAL_VALUE);

        MEMSET(&tbt_array[0], 0, sizeof(u64) * TBT_ARR_SIZE);
        ctr = 0;
    }
}

static void vnt(const u32 *nums, vn_test *von)
{
    register u32 i    = 0;
    register u32 prev = nums[0];

    register double numer, denom, avg;
    numer = denom = avg = 0.0;

    do {
        avg += (double) nums[i];
        numer += pow((double) prev - (double) nums[i], 2);
        prev = nums[i];
    } while (++i < VNT_N);

    avg /= VNT_N;

    i = 0;
    do {
        denom += pow((double) nums[i] - avg, 2);
    } while (++i < VNT_N);

    numer *= VNT_N;
    denom *= (VNT_N - 1);

    const double stat    = ((numer / denom) - VNT_MEAN) / VNT_STD_DEV;
    const double p_value = po_zscore(stat);
    von->pass_rate += (p_value > ALPHA_LEVEL);
    vnt_fisher += log(p_value);
}

static void walsh_test(const u32 *nums, wh_test *walsh)
{
    static u32 walsh_arr[4];
    static u16 idx;
    static double sum;

    walsh_arr[(idx & 1) << 2] = nums[0];
    walsh_arr[((idx & 1) << 2) + 1] = nums[1];

    idx += 2;

    if (!(idx & 3)) {
        const u8 start = (idx >> 2);

        // 32-bit work units, but process 128-bits at a time for statistic
        register double stat = wh_transform(start, walsh_arr[0], 0)
                             + wh_transform(start, walsh_arr[1], 32)
                             + wh_transform(start, walsh_arr[2], 64)
                             + wh_transform(start, walsh_arr[3], 96);

        stat /= WH_STD_DEV;
        walsh->pass_num += (stat >= WH_LOWER_BOUND && stat <= WH_UPPER_BOUND);
        sum += pow(stat, 2);
    }   

    // If we've compiled 128 statistics, obtain a p-value
    // If idx == 512 (WH_DF << 2), we have processed 128 * 128 bits
    if (idx == (WH_DF << 2)) {
        const double p_value = cephes_igamc(WH_DF / 2, sum / 2);

        // Add to Fisher method accumulator and record in p-value dist
        wh_fisher += log(p_value);
        walsh->dist[(u8) (p_value * 10.0)]++;
        walsh->pass_seq += (sum <= WH_CRITICAL_VALUE);

        // Reset counters
        idx = 0;
        sum = 0.0;
    }
}

static void fp_max8(const u32 *nums, u64 *max_runs, u64 *max_dist)
{
    static double fp_max_arr[FP_MAX_CAT];
    static u16 idx;

    fp_max_arr[idx] = (double) nums[0] / (double) __UINT32_MAX__;
    fp_max_arr[idx + 1] = (double) nums[1] / (double) __UINT32_MAX__;

    idx += 2;

    if (idx == FP_MAX_CAT) {
        register u8 max = 0;    
        register u8 i = 1;

        do {
            max  = (fp_max_arr[i] > fp_max_arr[max]) ? i : max;
        } while (++i < FP_MAX_CAT);

        ++max_dist[max];
        *max_runs += 1;
        idx = 0;
    }
}

static void fp_perm(const double num, u64 *perms, u64 *perm_dist)
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

        ++perm_dist[f];
        perm_idx = 0;
    }
}

static void update_mcb_lcb(const u8 idx, const u64 *freq, u64 *mcb, u64 *lcb)
{
    // Most Common Bytes
    register short i = 3;
    while (i >= 0 && freq[idx] > freq[mcb[i]]) {
        mcb[i + 1] = mcb[i];
        --i;
    }
    mcb[i + 1] = idx;

    // Least Common Bytes
    i = 3;
    while (i >= 0 && freq[idx] < freq[lcb[i]]) {
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

    register u8 i = 0;
    do {
        byte = (num >> i) & 0xFF;
        dist = (++ctr - gaps[byte]);
        gaplengths[byte] += dist;
        gaps[byte] = ctr;
    } while ((i += 8) < 64);
}

static void tally_runs(const u64 num, basic_test *basic)
{
    // 0 = up, 1 = down, -1 = init
    static short direction = -1;

    static u64 curr_up, curr_down, prev;

    if (num > prev) {
        if (direction != 0) {
            ++basic->up_runs;
            basic->longest_down = MAX(basic->longest_down, curr_down);
            curr_down           = 0;
            direction           = 0;
        }
        ++curr_up;
    } else if (num < prev) {
        if (direction != 1) {
            ++basic->down_runs;
            basic->longest_up = MAX(basic->longest_up, curr_up);
            curr_up           = 0;
            direction         = 1;
        }
        ++curr_down;
    }
    prev = num;
}

static void tally_bitruns(const u64 num, mfreq_test *mfreq)
{
    // 1 = 0, 2 = 1, 0 = init
    static u8 direction;
    static u64 curr_zero, curr_one;

    register u8 i = 0;
    do {
        if ((num >> i) & 1) {
            if (direction != 1) {
                ++mfreq->one_runs;
                mfreq->longest_zero = MAX(mfreq->longest_zero, curr_zero);
                curr_zero           = 0;
                direction           = 1;
            }
            ++curr_one;
        } else {
            if (direction != 2) {
                ++mfreq->zero_runs;
                mfreq->longest_one = MAX(mfreq->longest_one, curr_one);
                curr_one           = 0;
                direction          = 2;
            }
            ++curr_zero;
        }
    } while (++i < 64);
}

static void run_all(rng_test *rsl, adam_data data, adam_data sac) 
{
    // Maurer Universal Test
    // Checks the level of compressiiblity of output, assuming 1MB of
    // bytes have been accumulated
    if (++maurer_ctr == TESTING_BITS / ADAM_WORD_BITS) {
        maurer(rsl->mau);
        maurer_ctr = 0;

        /*
            This counter is also used for the wh_fisher_mb accumulator,
            which is used to compile the p_values generated by wh_fisher.

            We do this to make sure p-value accuracy doesn't deteriorate
            too quickly for larger sequences, so we employ the Fisher method
            twice, this time per MB
        */
        wh_fisher *= -2.0;
        register double wh_pvalue = cephes_igamc(TESTING_BITS >> 14, wh_fisher / 2);
        wh_fisher_mb += log(wh_pvalue);
        wh_fisher = 0.0;

        // Von Neumann Ratio Test
        vnt((u32 *) rsl->mau->bytes, rsl->von);

        // Counter to determine whether we need to scale even more for (>= 1GB) sequences
        if (++wh_gb_ctr == 1000) {
            wh_fisher_mb *= -2.0;
            wh_pvalue = cephes_igamc(1000, wh_fisher_mb / 2);
            wh_fisher_gb += log(wh_pvalue);
            wh_fisher_mb = 0.0;
            wh_gb_ctr    = 0;

            vnt_fisher *= -2.0;
            const double vnt_pvalue = cephes_igamc(1000, vnt_fisher / 2);
            vnt_fisher_gb += log(vnt_pvalue);
            vnt_fisher = 0.0;
        }
    }

    const u64 num = adam_int(data, UINT64);

    // First, write to Maurer array, and record some basic info
    MEMCPY(&rsl->mau->bytes[maurer_ctr << 3], &num, 8);
    rsl->range->odd += (num & 1);
    rsl->mfreq->mfreq += POPCNT(num);
    rsl->range->min = MIN(rsl->range->min, num);
    rsl->range->max = MAX(rsl->range->max, num);

    // Record range that this number falls in
    ++rsl->range->range_dist[(num >= __UINT32_MAX__) + (num >= (1ULL << 40)) + (num >= (1ULL << 48)) + (num >= (1ULL << 56))];

    // Convert this number to float with same logic used for returning FP results
    // Then record float for FP freq distribution in (0.0, 1.0)
    register double d;
    rsl->fp->avg_fp += (d = ((double) num / (double) __UINT64_MAX__));
    ++rsl->fp->fpf_dist[(u8) (d * 10.0)];
    ++rsl->fp->fpf_quad[(d >= 0.25) + (d >= 0.5) + (d >= 0.75)];

    // 32-bit floating point max-of-T test with T = 8
    fp_max8((u32 *) &num, &rsl->fp->fp_max_runs, &rsl->fp->fp_max_dist[0]);

    // Collect this floating point value into the permutations tuple
    // Once the tuple reaches 5 elements, the permutation is recorded
    fp_perm(d, &rsl->fp->perms, rsl->fp->fp_perms);

    // Tracks the amount of runs AND longest run, both increasing and decreasing
    tally_runs(num, rsl->basic);

    // Tracks the amount of runs AND longest runs for the bits in this number
    tally_bitruns(num, rsl->mfreq);

    // Checks gap lengths
    gap_lengths(num);

    // Saturation Point Test
    // Determines index where all 2^4 values have appeared at least once
    sat_point((u8 *) &num);

    // Topological Binary Test
    // Checks for distinct patterns in a certain collection of numbers
    tbt((u16 *) &num, rsl->topo);

    // Strict Avalanche Criterion (SAC) Test
    // Records the Hamming Distance between this number and the number that
    // was in the same index in the buffer during the previous iteration
    ++ham_dist[POPCNT(num ^ adam_int(sac, UINT64))];

    // Walsh-Hadamard Transform Test
    // Related to the frequency and autocorrelation test, computes a
    // test statistic per u128 from a transformed binary sequence
    walsh_test((u32 *) &num, rsl->walsh);

    // Calls into ENT framework, updating all the stuff there
    ent_loop((const u8 *) &num);
}

void adam_examine(const u64 limit, adam_data data)
{
    // General initialization
    basic_test basic = { 0 };
    basic.sequences  = limit / ADAM_WORD_BITS; 

    adam_record(data, basic.seed, basic.nonce);

    // Number range related testing
    range_test range = { 0 };

    // Bit frequency related stuff
    mfreq_test mfreq = { 0 };

    // Floating point test related stuff
    fp_test fp = { 0 };

    // Topological Binary init
    tb_test topo   = { 0 };
    topo.total_u16 = limit >> 4;

    // Bitarray for representing 2^16 values
    tbt_array = calloc(TBT_ARR_SIZE, sizeof(u64));
    if (tbt_array == NULL) {
        err("Could not allocate memory for topological binary test");
        return;
    }

    // Von Neumann Ratio init
    vn_test von = { 0 };
    von.trials  = limit / TESTING_BITS;

    const u64 init_num = adam_int(data, UINT64);

    // ENT init values
    ent_test ent = { 0 };
    ent.sccu0    = init_num & 0xFF;

    range.min = range.max = init_num;

    // Maurer test init - calculations were pulled from the NIST STS implementation
    maurer_test mau = { 0 };
    mau.trials      = limit / TESTING_BITS;
    mau.std_dev     = MAURER_C * sqrt(MAURER_VARIANCE / (double) MAURER_K);
    mau.bytes       = malloc(MAURER_ARR_SIZE * sizeof(u8));
    if (mau.bytes == NULL) {
        err("Could not allocate memory for Maurer test");
        return;
    }
 
    MEMCPY(mau.bytes, &init_num, ADAM_WORD_SIZE);

    // SAC test init
    const u32 old = basic.nonce[2];
    basic.nonce[2] ^= (1ULL << (basic.nonce[2] & 63));
    adam_data sac_runner = adam_setup(basic.seed, basic.nonce);
    if (sac_runner == NULL) {
        err("Could not allocate memory for strict avalanche test");
        return;
    }
    basic.nonce[2] = old;

    // Walsh-Hadamard Test init
    wh_test walsh = { 0 };
    walsh.trials  = limit / TESTING_BITS;

    // Aggregation struct
    rng_test rsl;
    rsl.basic = &basic;
    rsl.range = &range;
    rsl.mfreq = &mfreq;
    rsl.fp    = &fp;
    rsl.mau   = &mau;
    rsl.topo  = &topo;
    rsl.von   = &von;
    rsl.walsh = &walsh;
    rsl.ent   = &ent;

    // Start testing!
    register long long rate = basic.sequences;
    
    do {
        run_all(&rsl, data, sac_runner);
    } while (--rate > 0);

    adam_results(limit, &rsl);
    free(tbt_array);
    free(mau.bytes);
    adam_cleanup(sac_runner);
}

static void adam_results(const u64 limit, rng_test *rsl)
{
    // Screen info for pretty printing
    u16 center, indent, swidth;
    get_print_metrics(&center, &indent, &swidth);
    indent += (indent >> 1);

    // First get the ENT results out of the way
    ent_results(rsl->ent);

    register u16 i = 0;
    for (; i < BUF_SIZE; ++i) {
        rsl->basic->avg_gap += ((double) gaplengths[i] / (double) (rsl->ent->freq[i] - 1));
        update_mcb_lcb(i, rsl->ent->freq, rsl->basic->mcb, rsl->basic->lcb);
    }

    // Now print all the results per test / category of stuff

    print_basic_results(indent, limit, rsl->basic);
    print_mfreq_results(indent, rsl->basic->sequences, rsl->mfreq);

    rsl->basic->avg_gap /= 256.0;
    print_byte_results(indent, rsl->basic);

    print_range_results(indent, rsl->basic->sequences, rsl->range);
    print_ent_results(indent, rsl->ent);

    rsl->fp->avg_fp /= (rsl->basic->sequences);
    print_fp_results(indent, rsl->basic->sequences, rsl->fp);

    print_sp_results(indent, rsl, &sat_dist[0], &sat_range[0]);

    rsl->mau->fisher *= -2.0;
    print_maurer_results(indent, rsl->mau);

    print_tbt_results(indent, rsl->topo);

    // Von Neumann results
    if (rsl->basic->sequences >= ((TESTING_BITS / ADAM_WORD_BITS) * 1000)) {
        rsl->von->fisher  = vnt_fisher_gb * -2.0;
        rsl->von->p_value = cephes_igamc(rsl->von->trials / 1000, rsl->von->fisher / 2);
    } else {
        rsl->von->fisher  = vnt_fisher * -2.0;
        rsl->von->p_value = cephes_igamc(rsl->von->trials, rsl->von->fisher / 2);
    }
    print_vnt_results(indent, rsl->von);

    // SAC test results
    print_avalanche_results(indent, rsl->basic, &ham_dist[0]);

    // Walsh-Hademard test results
    if (rsl->basic->sequences >= ((TESTING_BITS / ADAM_WORD_BITS) * 1000)) {
        rsl->walsh->fisher  = wh_fisher_gb * -2.0;
        rsl->walsh->p_value = cephes_igamc(rsl->walsh->trials / 1000, rsl->walsh->fisher / 2);
    } else {
        rsl->walsh->fisher  = wh_fisher_mb * -2.0;
        rsl->walsh->p_value = cephes_igamc(rsl->walsh->trials, rsl->walsh->fisher / 2);
    }
    print_wht_results(indent, rsl->walsh);
}

#define Z_TABLE_SIZE 390

// Positive side only, from 0.00 - 3.99
static double z_table[Z_TABLE_SIZE] = {
    .50000, .50399, .50798, .51197, .51595, .51994, .52392, .52790, .53188, .53586,
    .53983, .54380, .54776, .55172, .55567, .55962, .56356, .56749, .57142, .57535,
    .57926, .58317, .58706, .59095, .59483, .59871, .60257, .60642, .61026, .61409,
    .61791, .62172, .62552, .62930, .63307, .63683, .64058, .64431, .64803, .65173,
    .65542, .65910, .66276, .66640, .67003, .67364, .67724, .68082, .68439, .68793,
    .69146, .69497, .69847, .70194, .70540, .70884, .71226, .71566, .71904, .72240,
    .72575, .72907, .73237, .73565, .73891, .74215, .74537, .74857, .75175, .75490,
    .75804, .76115, .76424, .76730, .77035, .77337, .77637, .77935, .78230, .78524,
    .78814, .79103, .79389, .79673, .79955, .80234, .80511, .80785, .81057, .81327,
    .81594, .81859, .82121, .82381, .82639, .82894, .83147, .83398, .83646, .83891,
    .84134, .84375, .84614, .84849, .85083, .85314, .85543, .85769, .85993, .86214,
    .86433, .86650, .86864, .87076, .87286, .87493, .87698, .87900, .88100, .88298,
    .88493, .88686, .88877, .89065, .89251, .89435, .89617, .89796, .89973, .90147,
    .90320, .90490, .90658, .90824, .90988, .91149, .91309, .91466, .91621, .91774,
    .91924, .92073, .92220, .92364, .92507, .92647, .92785, .92922, .93056, .93189,
    .93319, .93448, .93574, .93699, .93822, .93943, .94062, .94179, .94295, .94408,
    .94520, .94630, .94738, .94845, .94950, .95053, .95154, .95254, .95352, .95449,
    .95543, .95637, .95728, .95818, .95907, .95994, .96080, .96164, .96246, .96327,
    .96407, .96485, .96562, .96638, .96712, .96784, .96856, .96926, .96995, .97062,
    .97128, .97193, .97257, .97320, .97381, .97441, .97500, .97558, .97615, .97670,
    .97725, .97778, .97831, .97882, .97932, .97982, .98030, .98077, .98124, .98169,
    .98214, .98257, .98300, .98341, .98382, .98422, .98461, .98500, .98537, .98574,
    .98610, .98645, .98679, .98713, .98745, .98778, .98809, .98840, .98870, .98899,
    .98928, .98956, .98983, .99010, .99036, .99061, .99086, .99111, .99134, .99158,
    .99180, .99202, .99224, .99245, .99266, .99286, .99305, .99324, .99343, .99361,
    .99379, .99396, .99413, .99430, .99446, .99461, .99477, .99492, .99506, .99520,
    .99534, .99547, .99560, .99573, .99585, .99598, .99609, .99621, .99632, .99643,
    .99653, .99664, .99674, .99683, .99693, .99702, .99711, .99720, .99728, .99736,
    .99744, .99752, .99760, .99767, .99774, .99781, .99788, .99795, .99801, .99807,
    .99813, .99819, .99825, .99831, .99836, .99841, .99846, .99851, .99856, .99861,
    .99865, .99869, .99874, .99878, .99882, .99886, .99889, .99893, .99896, .99900,
    .99903, .99906, .99910, .99913, .99916, .99918, .99921, .99924, .99926, .99929,
    .99931, .99934, .99936, .99938, .99940, .99942, .99944, .99946, .99948, .99950,
    .99952, .99953, .99955, .99957, .99958, .99960, .99961, .99962, .99964, .99965,
    .99966, .99968, .99969, .99970, .99971, .99972, .99973, .99974, .99975, .99976,
    .99977, .99978, .99978, .99979, .99980, .99981, .99981, .99982, .99983, .99983,
    .99984, .99985, .99985, .99986, .99986, .99987, .99987, .99988, .99988, .99989,
    .99989, .99990, .99990, .99990, .99991, .99991, .99992, .99992, .99992, .99992,
    .99993, .99993, .99993, .99994, .99994, .99994, .99994, .99995, .99995, .99995
};

double po_zscore(double z_score)
{
    const bool neg = (z_score < 0.0);
    if (neg) {
        z_score *= -1.0;
    }

    const u16 coord_row = (u16) (z_score * 10);
    const u16 coord_col = (u16) (z_score * 100) - (coord_row * 10);
    const u16 coord     = (10 * coord_row) + coord_col;

    if (coord >= Z_TABLE_SIZE) {
        return 0.0;
    }

    register double p_value = z_table[coord];

    if (!neg) {
        p_value = 1.0 - p_value;
    }

    return p_value;
}

/*   
    FOLLOWING CODE UNTIL END IS FROM THE CEPHES C MATH LIBRARY:
    https://www.netlib.org/cephes/
*/

// 2**-53
static double MACHEP = 1.11022302462515654042E-16;

// log(MAXNUM)
static double MAXLOG = 7.09782712893383996732224E2;

// 2**1024*(1-MACHEP)
static double MAXNUM = 1.7976931348623158E308;

static double big    = 4.503599627370496E15;
static double biginv = 2.22044604925031308085E-16;

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

    do {
        ans = ans * x + *p++;
    } while (--i);

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

    do {
        ans = ans * x + *p++;
    } while (--i);

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
        if ((i & 1) == 0) {
            sgngam = -1;
        } else {
            sgngam = 1;
        }
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
            if (u == 0.0) {
                goto lgsing;
            }
            z /= u;
            p += 1.0;
            u = x + p;
        }
        if (z < 0.0) {
            sgngam = -1;
            z      = -z;
        } else {
            sgngam = 1;
        }
        if (u == 2.0) {
            return (log(z));
        }
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
    if (x > 1.0E8) {
        return q;
    }

    p = 1.0 / (x * x);
    if (x >= 1000.0) {
        q += ((7.9365079365079365079365e-4 * p
                  - 2.7777777777777777777778e-3)
                     * p
                 + 0.0833333333333333333333)
            / x;
    } else {
        q += cephes_polevl(p, (double *) A, 4) / x;
    }

    return q;
}

static double cephes_igam(double a, double x)
{
    double ans, ax, c, r;

    if ((x <= 0) || (a <= 0)) {
        return 0.0;
    }

    if ((x > 1.0) && (x > a)) {
        return 1.E0 - cephes_igamc(a, x);
    }

    /* Compute  x**a * exp(-x) / gamma(a)  */
    ax = a * log(x) - x - cephes_lgam(a);
    if (ax < -MAXLOG) {
        return 0.0;
    }

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

    if ((x <= 0) || (a <= 0)) {
        return (1.0);
    }

    if ((x < 1.0) || (x < a)) {
        return (1.e0 - cephes_igam(a, x));
    }

    ax = a * log(x) - x - cephes_lgam(a);

    if (ax < -MAXLOG) {
        return 0.0;
    }

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
        } else {
            t = 1.0;
        }
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
