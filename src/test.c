/*
  This is a collection of miscellaneous heuristics implemented
  by me, just to log some properties of the generated sequence(s).
  It's nowhere near as important as the feedback from tried and true
  testing suites, but it's good for some quick information about the
  sequences you generate

  Additionally, the ENT framework is integrated into this collection,
  for a total of 12 pieces of information (plus initial state) that
  are returned to the user:

    - monobit frequency
    - presence of zeroes
    - max and min values
    - parity: even and odd number totals
    - chaotic seed distribution + chi-square
    - runs: total # of runs and longest run (increasing/decreasing)
    - average gap length
    - entropy                     (ENT)
    - chi-square                  (ENT)
    - arithmetic mean             (ENT)
    - monte carlo value for pi    (ENT)
    - serial correlation          (ENT)
*/

#include "../include/test.h"
#include "../include/adam.h"

static u64 chseed_dist[5];
static u64 gaps[BUF_SIZE + 1];
static u64 gaplengths[BUF_SIZE];

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
    const u8 flag = (direction != 0);
    rsl->up_runs += flag;
    if (flag) {
      rsl->longest_down = rsl->longest_down ^ ((rsl->longest_down ^ curr_down) & -(rsl->longest_down < curr_down));
      curr_down = 0;
    }
    ++curr_up;
    direction = 0;
  } else if (num < prev) {
    const u8 flag = (direction != 1);
    rsl->down_runs += flag;
    if (flag) {
      rsl->longest_up = rsl->longest_up ^ ((rsl->longest_up ^ curr_up) & -(rsl->longest_up < curr_up));
      curr_up = 0;
    }
    ++curr_down;
    direction = 1;
  }
  prev = num;
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

void test_loop(rng_test *rsl, double *duration)
{
  // Chaotic seeds all occur within (0.0, 0.5).
  // This function tracks their distribution and we check the uniformity at the end.
  chseed_unif(rsl->data->chseeds, &rsl->avg_chseed);

  register u16 i = 0;
  u64 num;
  do {
    num = adam_get(rsl->data, 64, duration);

    // https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
    rsl->min = num ^ ((rsl->min ^ num) & -(rsl->min < num));
    rsl->max = rsl->max ^ ((rsl->max ^ num) & -(rsl->max < num));

    rsl->odd += (num & 1);
    rsl->zeroes += (num == 0);
    rsl->mfreq += POPCNT(num);

    // Tracks the amount of runs AND longest run, both increasing and decreasing
    tally_runs(num, rsl);

    // Checks gap lengths
    gap_lengths(num);

    // Calls into ENT framework, updating all the stuff there
    ent_test((u8 *)&num);
  } while (++i < BUF_SIZE);
}

void adam_test(const u64 limit, rng_test *rsl, double *duration)
{
  rng_data *data = rsl->data;
  u64 buffer[BUF_SIZE] ALIGN(64);

  adam_fill(data, (void *)&buffer[0], SEQ_BYTES, duration);

  rsl->ent->sccu0 = buffer[0] & 0xFF;

  rsl->avg_chseed = 0.0;

  rsl->mfreq = rsl->zeroes = rsl->up_runs = rsl->longest_up = rsl->down_runs = rsl->longest_down = rsl->odd = 0;
  rsl->min = rsl->max = buffer[0];

  MEMCPY(&rsl->init_values[0], &data->seed, sizeof(u64) << 2);
  rsl->init_values[4] = data->nonce;
  rsl->init_values[5] = data->aa;
  rsl->init_values[6] = data->bb;

  register long int rate = limit >> 14;
  register short leftovers = limit & (SEQ_SIZE - 1);

  do {
    test_loop(rsl, duration);
    adam_fill(data, (void *)&buffer[0], SEQ_BYTES, duration);
    leftovers -= (u16)(rate <= 0) << 14;
  } while (LIKELY(--rate > 0) || LIKELY(leftovers > 0));

  ent_results(rsl->ent);

  register double average_gaplength = 0.0;

  register u16 i = 0;
  for (; i < BUF_SIZE; ++i)
    average_gaplength += ((double)gaplengths[i] / (double)(rsl->ent->freq[i] - 1));

  rsl->avg_gap = ((double)average_gaplength) / 256.0;

  rsl->chseed_dist = &chseed_dist[0];
}

