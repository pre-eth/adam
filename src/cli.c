#include <getopt.h>
#include <math.h>
#include <stdio.h>

#include "../include/adam.h"
#include "../include/support.h"
#include "../include/test.h"

#define STRINGIZE(a) #a
#define STRINGIFY(a) STRINGIZE(a)

#define MAJOR 1
#define MINOR 4
#define PATCH 0

#define OPTSTR ":hvdfbxoap:m:w:e:r:u::s::n::"
#define ARG_COUNT 16

typedef struct rng_cli {
  //  Pointer to RNG buffer and state
  adam_data *data;

  //  Number of bits in results (8, 16, 32, 64)
  u8 width;

  //  Number of decimal places for floating point output (default 15)
  u8 precision;

  //  Print hex?
  bool hex;

  //  Print octal?
  bool octal;

  //  Number of results to return to user (max 1000)
  u16 results;

  // Multiplier to scale floating point results, if the user wants
  // This returns doubles within range (0, mult)
  u64 mult;
} rng_cli;

static void print_summary(const u16 swidth, const u16 indent)
{
#define SUMM_PIECES 11

  const char *pieces[SUMM_PIECES] = {
    "\033[1madam\033[m [-h|-v|-b]", "[-s[\033[3mseed?\033[m]]", "[-n[\033[3mnonce?\033[m]]", "[-dxof]",
    "[-w \033[1mwidth\033[m]", "[-m \033[1mmultiplier\033[m]", "[-p \033[1mprecision\033[m]",
    "[-a \033[1mmultiplier\033[m]", "[-e \033[1mmultiplier\033[m]", "[-r \033[1mresults\033[m]",
    "[-u[\033[3mamount?\033[m]]"
  };

  const u8 sizes[SUMM_PIECES] = { 15, 11, 12, 7, 11, 15, 14, 15, 15, 12, 13 };

  register u8 i = 0, running_length = indent;

  printf("\n\033[%uC", indent);

  for (; i < SUMM_PIECES - 1; ++i) {
    printf("%s ", pieces[i]);
    // add one for space
    running_length += sizes[i] + 1;
    if (running_length + sizes[i + 1] + 1 >= swidth) {
      // add 5 for "adam " to create hanging indent for
      // succeeding lines
      printf("\n\033[%uC", indent + 5);
      running_length = indent + 5;
    }
  }
  printf("%s", pieces[SUMM_PIECES - 1]);

  printf("\n\n");
}

static u8 help(void)
{
  u16 center, indent, swidth;
  get_print_metrics(&center, &indent, &swidth);

  const u8 CENTER = center - 4;
  // subtract 1 because it is half of width for arg (ex. "-d")
  const u8 INDENT = indent - 1;
  // total indent for help descriptions if they have to go to next line
  const u8 HELP_INDENT = INDENT + INDENT + 2;
  // max length for help description in COL 2 before it needs to wrap
  const u8 HELP_WIDTH = swidth - (HELP_INDENT << 1);

  print_summary(swidth, INDENT);

  printf("\033[%uC[OPTIONS]\n", CENTER);

  const char ARGS[ARG_COUNT] = {
    'h',
    'v',
    's',
    'n',
    'u',
    'r',
    'd',
    'w',
    'b',
    'a',
    'e',
    'x',
    'o',
    'f',
    'p',
    'm'
  };

  const char *ARGSHELP[ARG_COUNT] = {
    "Get command summary and all available options",
    "Version of this software (v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")",
    "Get the seed for the generated buffer (no parameter), or provide "
    "your own. Seeds are reusable but should be kept secret",
    "Get the nonce for the generated buffer (no parameter), or provide "
    "your own. Nonces should be unique per seed and kept secret",
    "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 1000)",
    "The amount of numbers to generate and return, written to stdout. Must be within max limit for current width (see -d)",
    "Dump entire buffer using the specified width (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)",
    "Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64",
    "Just bits... literally",
    "Write an ASCII or binary sample of bits/doubles to file for external assessment. You can choose a multiplier to output up to 1GB, or 1 billion doubles (with optional scaling factor), at a time",
    "Examine a sample of 1000000 bits (1 Mb) with the ENT framework and log some properties of the output sequence. "
    "You can choose a multiplier within [1, " STRINGIFY(BITS_TESTING_LIMIT) "] to examine up to 1GB at a time",
    "Print numbers in hexadecimal format with leading prefix",
    "Print numbers in octal format with leading prefix",
    "Enable floating point mode to generate doubles in (0, 1) instead of integers",
    "The number of decimal places to display when printing doubles. Must be within [1, 15]. Default is 15",
    "Multiplier for randomly generated doubles, such that they fall in the range (0, <MULTIPLIER>)"
  };
  const u8 lengths[ARG_COUNT] = { 25, 33, 119, 124, 109, 116, 91, 81, 22, 191, 185, 55, 49, 83, 100, 93 };

  register short len;
  for (u8 i = 0; i < ARG_COUNT; ++i) {
    printf("\033[%uC\033[1;33m-%c\033[m\033[%uC%.*s\n", INDENT, ARGS[i], INDENT, HELP_WIDTH, ARGSHELP[i]);
    len = lengths[i] - HELP_WIDTH;
    while (len > 0) {
      ARGSHELP[i] += HELP_WIDTH + (ARGSHELP[i][HELP_WIDTH] == ' ');
      printf("\033[%uC%.*s\n", HELP_INDENT, HELP_WIDTH, ARGSHELP[i]);
      len -= HELP_WIDTH;
    }
  }
  return 0;
}

static void print_int(rng_cli *cli)
{
  register u64 num = adam_int(cli->data, cli->width);
  if (cli->hex)
    printf("0x%llx", num);
  else if (cli->octal)
    printf("0o%llo", num);
  else
    printf("%llu", num);
}

static void print_dbl(rng_cli *cli)
{
  double d = adam_dbl(cli->data, cli->mult);
  printf("%.*lf", cli->precision, d);
}

static u8 dump_buffer(rng_cli *cli)
{
  adam_data *data = cli->data;
  void (*write_fn)(rng_cli *cli) = (!data->dbl_mode) ? &print_int : &print_dbl;

  write_fn(cli);

  while (--cli->results > 0) {
    printf("\n");
    write_fn(cli);
  }

  putchar('\n');

  return 0;
}

static u8 uuid(const char *strlimit, adam_data *data)
{
  register u16 limit = a_to_u(optarg, 1, 1000);
  if (!limit)
    return err("Invalid amount specified. Value must be within "
               "range [1, 1000]");

  u8 buf[16];

  register u16 i = 0;

  u64 lower, upper;

  do {
    lower = adam_int(data, 64);
    upper = adam_int(data, 64);
    gen_uuid(upper, lower, &buf[0]);

    // Print the UUID
    printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%"
           "02x%02x%02x%02x",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
        buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14],
        buf[15]);

    putchar('\n');
  } while (++i < limit);

  return 0;
}

static u8 assessf(bool ascii_mode, rng_cli *cli)
{
  u32 mult;
  fprintf(stderr, "\033[mSequence Size (x 1000): \033[1;33m");
  while (!scanf(" %u", &mult) || mult < 1 || mult > DBL_TESTING_LIMIT) {
    err("Output multiplier must be between [1, " STRINGIFY(DBL_TESTING_LIMIT) "]");
    fprintf(stderr, "\033[mSequence Size (x 1000): \033[1;33m");
  }

  u64 scale_factor = 1;
  fprintf(stderr, "\033[mScaling factor: \033[1;33m");
  while (!scanf(" %llu", &scale_factor)) {
    err("Scaling factor must be within range [1, " STRINGIFY(__UINT64_MAX__) "]");
    fprintf(stderr, "\033[mScaling factor: \033[1;33m");
  }

  const u64 limit = TESTING_DBL * mult;

  register double duration;
  if (ascii_mode)
    duration = dbl_ascii(limit, &cli->data->seed[0], &cli->data->nonce, scale_factor, cli->precision);
  else
    duration = dbl_bytes(limit, &cli->data->seed[0], &cli->data->nonce, scale_factor);

  fprintf(stderr,
      "\n\033[0mGenerated \033[36m%llu\033[m doubles in \033[36m%lfs\033[m\n",
      limit, duration);

  return duration;
}

static u8 assess(rng_cli *cli)
{
  char file_name[65];
  fprintf(stderr, "File name: \033[1;33m");
  while (!scanf(" %64s", &file_name[0]))
    err("Please enter a valid file name");

  register double duration = 0.0;

  char c = '0';

  fprintf(stderr, "\033[mOutput type (1 = ASCII, 2 = BINARY): \033[1;33m");
  while (!scanf(" %c", &c) || (c != '1' && c != '2')) {
    err("Value must be 1 or 2");
    fprintf(stderr, "\033[mOutput type (1 = ASCII, 2 = BINARY): \033[1;33m");
  }

  freopen(file_name, (c == '1') ? "w+" : "wb+", stdout);

  if (cli->data->dbl_mode)
    return assessf((c == '1'), cli);

  u32 mult;
  fprintf(stderr, "\033[mSequence Size (x 1000000): \033[1;33m");
  while (!scanf(" %u", &mult) || mult < 1 || mult > BITS_TESTING_LIMIT) {
    err("Output multiplier must be between [1, " STRINGIFY(BITS_TESTING_LIMIT) "]");
    fprintf(stderr, "\033[mSequence Size (x 1000000): \033[1;33m");
  }

  register u64 limit = TESTING_BITS * mult;

  if (c == '1')
    duration = stream_ascii(limit, &cli->data->seed[0], &cli->data->nonce);
  else
    duration = stream_bytes(limit, &cli->data->seed[0], &cli->data->nonce);

  fprintf(stderr,
      "\n\033[0mGenerated \033[36m%llu\033[m bits in \033[36m%lfs\033[m\n",
      limit, duration);

  return 0;
}

static void print_basic_results(const u16 indent, const u64 limit, rng_test *rsl, const u64 *init_values)
{
  const u64 output = rsl->sequences << 8;
  const u32 zeroes = (output << 6) - rsl->mfreq;
  const u32 diff = (zeroes > rsl->mfreq) ? zeroes - rsl->mfreq : rsl->mfreq - zeroes;

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

  printf("\033[1;34m\033[%uC              Total Output: \033[m%llu u64 (%llu u32 | %llu u16 | %llu u8)\n", indent, output, output << 1, output << 2, output << 3);
  printf("\033[1;34m\033[%uC       Sequences Generated: \033[m%u\n", indent, rsl->sequences);
  printf("\033[1;34m\033[%uC               Sample Size: \033[m%llu BITS (%llu%s)\n", indent, limit, bytes, unit);
  printf("\033[1;34m\033[%uC         Monobit Frequency: \033[m%u ONES, %u ZEROES (+%u %s)\n", indent, rsl->mfreq, zeroes, diff, (zeroes > rsl->mfreq) ? "ZEROES" : "ONES");
  printf("\033[1;34m\033[%uC             Minimum Value: \033[m%llu\n", indent, rsl->min);
  printf("\033[1;34m\033[%uC             Maximum Value: \033[m%llu\n", indent, rsl->max);

  const u64 range_exp[5] = {
    (double)output * RANGE1_PROB,
    (double)output * RANGE2_PROB,
    (double)output * RANGE3_PROB,
    (double)output * RANGE4_PROB,
    (double)output * RANGE5_PROB
  };

  register double chi_calc = 0.0;
  register u8 i = 1;
  for (; i < 5; ++i)
    if (range_exp[i] != 0)
      chi_calc += pow(((double)rsl->range_dist[i] - (double)range_exp[i]), 2) / (double)range_exp[i];

  register u8 suspect_level = 32 - (RANGE_CRITICAL_VALUE <= chi_calc);

  printf("\033[1;34m\033[%uC          Range Chi Square: \033[m\033[1;%um%1.2lf\n", indent, suspect_level, chi_calc);
  printf("\033[1;34m\033[%uC                     Range: \033[m%llu\n", indent, rsl->max - rsl->min);
  printf("\033[2m\033[%uC            a.    [0, 2³²): \033[m%u (expected %llu)\n", indent, rsl->range_dist[0], range_exp[0]);
  printf("\033[2m\033[%uC            b.  [2³², 2⁴⁰): \033[m%u (expected %llu)\n", indent, rsl->range_dist[1], range_exp[1]);
  printf("\033[2m\033[%uC            c.  [2⁴⁰, 2⁴⁸): \033[m%u (expected %llu)\n", indent, rsl->range_dist[2], range_exp[2]);
  printf("\033[2m\033[%uC            d.  [2⁴⁸, 2⁵⁶): \033[m%u (expected %llu)\n", indent, rsl->range_dist[3], range_exp[3]);
  printf("\033[2m\033[%uC            e.  [2⁵⁶, 2⁶⁴): \033[m%u (expected %llu)\n", indent, rsl->range_dist[4], range_exp[4]);
  printf("\033[1;34m\033[%uC              Even Numbers: \033[m%llu (%u%%)\n", indent, output - rsl->odd, (u8)(((double)(output - rsl->odd) / (double)output) * 100));
  printf("\033[1;34m\033[%uC               Odd Numbers: \033[m%u (%u%%)\n", indent, rsl->odd, (u8)(((double)rsl->odd / (double)output) * 100));
  printf("\033[1;34m\033[%uC          Zeroes Generated: \033[m%u\n", indent, rsl->zeroes);
  printf("\033[1;34m\033[%uC    256-bit Seed (u64 x 4): \033[m0x%16llX, 0x%16llX,\n", indent, init_values[0], init_values[1]);
  printf("\033[%uC                            0x%16llX, 0x%16llX\n", indent, init_values[2], init_values[3]);
  printf("\033[1;34m\033[%uC              64-bit Nonce: \033[m0x%16llX\n", indent, init_values[4]);
  printf("\033[1;34m\033[%uC        Average Gap Length: \033[m%llu\n", indent, (u64)rsl->avg_gap);
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

  if (ent->pochisq < 0.001) {
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
  register double chi_calc = 0.0;
  const double expected_chseeds = (expected * 0.2);

  register u8 i = 0;
  for (; i < 5; ++i)
    chi_calc += pow(((double)chseed_dist[i] - expected_chseeds), 2) / expected_chseeds;

  register u8 suspect_level = 32 - (CHSEED_CRITICAL_VALUE <= chi_calc);

  printf("\033[1;34m\033[%uC   Chaotic Seed Chi-Square: \033[m\033[1;%um%1.2lf\n", indent, suspect_level, chi_calc);
  printf("\033[1;34m\033[%uCAverage Chaotic Seed Value: \033[m%1.15lf (ideal = 0.25)\n", indent, avg_chseed / (double)expected);
  printf("\033[2m\033[%uC             a. (0.0, 0.1): \033[m%llu (%llu expected)\n", indent, chseed_dist[0], (u64)expected_chseeds);
  printf("\033[2m\033[%uC             b. [0.1, 0.2): \033[m%llu (%llu expected)\n", indent, chseed_dist[1], (u64)expected_chseeds);
  printf("\033[2m\033[%uC             c. [0.2, 0.3): \033[m%llu (%llu expected)\n", indent, chseed_dist[2], (u64)expected_chseeds);
  printf("\033[2m\033[%uC             d. [0.3, 0.4): \033[m%llu (%llu expected)\n", indent, chseed_dist[3], (u64)expected_chseeds);
  printf("\033[2m\033[%uC             e. [0.4, 0.5): \033[m%llu (%llu expected)\n", indent, chseed_dist[4], (u64)expected_chseeds);
}

static u8 examine(const char *strlimit, adam_data *data)
{
  // Initialize properties
  rng_test rsl;
  ent_report ent;
  rsl.ent = &ent;

  // Record initial state and connect internal state to rsl_test
  u64 init_values[5];
  init_values[0] = rsl.seed[0] = data->seed[0];
  init_values[1] = rsl.seed[1] = data->seed[1];
  init_values[2] = rsl.seed[2] = data->seed[2];
  init_values[3] = rsl.seed[3] = data->seed[3];
  init_values[4] = rsl.nonce = data->nonce;

  // Check for and validate multiplier
  register u64 limit = TESTING_BITS;
  if (strlimit != NULL)
    limit *= a_to_u(strlimit, 1, BITS_TESTING_LIMIT);

  printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", limit);
  register double duration = get_seq_properties(limit, &rsl);

  // Rest of this is just formatting and printing the results
  u16 center, indent, swidth;
  get_print_metrics(&center, &indent, &swidth);
  indent <<= 1;

  printf("\033[%uC[RESULTS]\n\n", center - 4);

  print_basic_results(indent, limit, &rsl, &init_values[0]);
  print_ent_results(indent, &ent);
  print_chseed_results(indent, rsl.expected_chseed, &rsl.chseed_dist[0], rsl.avg_chseed);

  printf("\n\033[1;33mExamination Complete! (%lfs)\033[m\n", duration);

  return 0;
}

int main(int argc, char **argv)
{
  adam_data data;
  adam_setup(&data, false, NULL, NULL);

  rng_cli cli;
  cli.results = 1;
  cli.data = &data;
  cli.hex = false;
  cli.octal = false;
  cli.precision = 15;
  cli.width = 64;
  cli.mult = 0;

  register char opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
    switch (opt) {
    case 'h':
      return help();
    case 'v':
      puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
      return 0;
    case 'a':
      return assess(&cli);
    case 'b':
      stream_bytes(__UINT64_MAX__, &data.seed[0], &data.nonce);
      return 0;
    case 'x':
      cli.hex = true;
      continue;
    case 'o':
      cli.octal = true;
      continue;
    case 'w':
      cli.width = a_to_u(optarg, 8, 32);
      if (UNLIKELY(cli.width & (cli.width - 1)))
        return err("Width must be either 8, 16, 32");
      continue;
    case 'r':
      cli.results = a_to_u(optarg, 1, ADAM_BUF_SIZE * (64 / cli.width));
      if (!cli.results)
        return err("Invalid number of results specified for desired width");
      continue;
    case 's':
      rwseed(&data.seed[0], optarg);
      continue;
    case 'n':
      rwnonce(&data.nonce, optarg);
      continue;
    case 'u':
      return uuid(optarg, &data);
    case 'd':
      cli.results = ADAM_BUF_SIZE * (64 / cli.width);
      dump_buffer(&cli);
      return 0;
    case 'f':
      data.dbl_mode = true;
      continue;
    case 'p':
      data.dbl_mode = true;
      cli.precision = a_to_u(optarg, 1, 15);
      if (!cli.precision)
        return err("Floating point precision must be between [1, 15]");
      continue;
    case 'm':
      data.dbl_mode = true;
      cli.mult = a_to_u(optarg, 1, __UINT64_MAX__);
      if (!cli.mult)
        return err("Floating point scaling factor must be between [1, " STRINGIFY(__UINT64_MAX__) "]");
      continue;
    case 'e':
      return examine(optarg, &data);
    default:
      return err("Option is invalid or missing required argument");
    }
  }

  dump_buffer(&cli);

  return 0;
}