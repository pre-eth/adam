#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "../include/adam.h"
#include "../include/support.h"

#define STRINGIZE(a) #a
#define STRINGIFY(a) STRINGIZE(a)

#define MAJOR 1
#define MINOR 4
#define PATCH 0

#define OPTSTR ":hvdfbxop:w:a:e:r::u::s::n::"
#define ARG_COUNT 15

typedef struct rng_cli {
  //  Pointer to RNG buffer and state
  rng_data *data;

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
} rng_cli;

static void print_summary(const u16 swidth, const u16 indent)
{
#define SUMM_PIECES 9

  const char *pieces[SUMM_PIECES] = {
    "\033[1madam \033[m[-h|-v|-b|-x|-o|-d|-f]", "[-w \033[1mwidth\033[m]",
    "[-a \033[1mmultiplier\033[m]", "[-e \033[1mmultiplier\033[m]",
    "[-p \033[1mprecision\033[m]", "[-r \033[1mresults\033[m]", "[-s[\033[3mseed?\033[m]]",
    "[-n[\033[3mnonce?\033[m]]", "[-u[\033[3mamount?\033[m]]"
  };

  const u8 sizes[SUMM_PIECES + 1] = { 25, 10, 15, 15, 14, 14, 11, 12, 13, 0 };

  register u8 i = 0, running_length = indent;

  printf("\n\033[%uC", indent);

  for (; i < SUMM_PIECES; ++i) {
    printf("%s ", pieces[i]);
    // add one for space
    running_length += sizes[i] + 1;
    if (running_length + sizes[i + 1] + 1 >= swidth) {
      // add 5 for "adam " to create hanging indent for
      // succeeding lines
      printf("\n\033[%uC", indent + 5);
      running_length = 0;
    }
  }

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
    'p'
  };

  const char *ARGSHELP[ARG_COUNT] = {
    "Get command summary and all available options",
    "Version of this software (v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")",
    "Get the seed for the generated buffer (no parameter), or provide "
    "your own. Seeds are reusable but should be kept secret.",
    "Get the nonce for the generated buffer (no parameter), or provide "
    "your own. Nonces should ALWAYS be unique and secret",
    "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 128)",
    "The amount of numbers to generate and return, written to stdout. Must be within [1, 1000]",
    "Dump entire buffer using the specified width (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)",
    "Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64",
    "Just bits... literally",
    "Write an ASCII or binary sample of 1000000 bits (1 Mb) to file for external assessment with your favorite tests. "
    "You can choose a multiplier within [1, " STRINGIFY(TESTING_LIMIT) "] to output up to 1GB at a time",
    "Examine a sample of 1000000 bits (1 Mb) with the ENT framework as well as some other in-house statistical tests. "
    "You can choose a multiplier within [1, " STRINGIFY(TESTING_LIMIT) "] to examine up to 1GB at a time",
    "Print numbers in hexadecimal format with leading prefix",
    "Print numbers in octal format with leading prefix",
    "Enable floating point mode to generate doubles in (0, 1) instead of integers",
    "The number of decimal places to display when printing doubles. Must be within [1, 15]. Default is 15"
  };
  const u8 lengths[ARG_COUNT] = { 25, 33, 120, 118, 108, 89, 95, 81, 22, 187, 178, 55, 49, 83, 100 };

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

static u8 set_width(rng_cli *cli, const char *strwidth)
{
  cli->width = a_to_u(strwidth, 8, 32);
  if (UNLIKELY(cli->width & (cli->width - 1)))
    return 1;
  return 0;
}

static void print_int(rng_cli *cli)
{
  u64 num;
  adam_get(&num, cli->data, cli->width, NULL);
  if (cli->hex)
    printf("0x%llx", num);
  else if (cli->octal)
    printf("0o%llo", num);
  else
    printf("%llu", num);
}

static void print_dbl(rng_cli *cli)
{
  double d;
  adam_get(&d, cli->data, cli->width, NULL);
  printf("%.*lf", cli->precision, d);
}

static u8 dump_buffer(rng_cli *cli)
{
  rng_data *data = cli->data;
  void (*write_fn)(rng_cli *cli) = (!data->dbl_mode) ? &print_int : &print_dbl;

  write_fn(cli);

  while (--cli->results > 0) {
    printf(",\n");
    write_fn(cli);
  }

  putchar('\n');

  return 0;
}

static u8 uuid(const char *strlimit, rng_data *data)
{
  register u8 limit = a_to_u(optarg, 1, __UINT64_MAX__);
  if (!limit)
    return err("Invalid amount specified. Value must be within "
               "range [1, 128]");

  u8 buf[16];

  register u8 i = 0;

  u64 lower, upper;

  do {
    adam_get(&upper, data, 64, NULL);
    adam_get(&lower, data, 64, NULL);
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

static u8 assess(const char *strlimit, rng_cli *cli)
{
  const u16 limit = a_to_u(strlimit, 1, TESTING_LIMIT);
  if (!limit)
    return err(
        "Multiplier must be within range [1, " STRINGIFY(TESTING_LIMIT) "]");

  const u64 total = limit * TESTING_BITS;
  register double duration = 0.0;

  char file_name[65];
  printf("File name: \033[1;93m");
  while (!scanf(" %64s", &file_name[0]))
    err("Please enter a valid file name");

  char c = '2';
get_file_type:
  fprintf(stderr, "\033[mSample type (1 = ASCII, 2 = BINARY): \033[1;33m");
  scanf(" %c", &c);
  if (c == '1') {
    freopen(file_name, "w+", stdout);
    duration = stream_ascii(total, cli->data);
  } else if (c == '2') {
    freopen(file_name, "wb+", stdout);
    duration = stream_bytes(total, cli->data);
  } else {
    err("Value must be 1 or 2");
    goto get_file_type;
  }

  /*
    Based off some observed benchmarks, keeping track of CPU time
    adds around latency to the entire program, so this should be
    accounted for with <duration> to give a more accurate measure of
    how much cpu time the number generation really took.

    0.77 was chosen because to generate 1GB of random data, there is
    ~0.15s of latency added on average, increasing the generation time
    from 0.47s to around 0.62s on Mac. So this ratio was used to adjust
    the duration calculations.

    TODO: confirm offset validity on linux
  */
  duration *= 0.77;

  fprintf(stderr,
      "\n\033[0mGenerated \033[36m%llu\033[m bits in \033[36m%lfs\033[m\n",
      total, duration);

  return 0;
}

int main(int argc, char **argv)
{
  rng_data data;
  adam_init(&data, false);

  rng_cli cli;
  cli.results = 1;
  cli.data = &data;
  cli.hex = false;
  cli.octal = false;
  cli.precision = 0;
  cli.width = 64;

  register char opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
    switch (opt) {
    case 'h':
      return help();
    case 'v':
      puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
      return 0;
    case 'a':
      return assess(optarg, &cli);
    case 'b':
      stream_bytes(__UINT64_MAX__, &data);
      return 0;
    case 'x':
      cli.hex = true;
      continue;
    case 'o':
      cli.octal = true;
      continue;
    case 'w':
      if (set_width(&cli, optarg))
        return err("Width must be either 8, 16, 32");
      continue;
    case 'r':
      cli.results = a_to_u(optarg, 1, 1000);
      if (cli.results == 0)
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
      cli.results = BUF_SIZE * (64 / cli.width);
      dump_buffer(&cli);
      return 0;
    case 'f':
      data.dbl_mode = true;
      cli.precision = 15;
      continue;
    case 'p':
      data.dbl_mode = true;
      cli.precision = a_to_u(optarg, 1, 15);
      continue;
    case 'e':
      return examine(optarg, &data);
    default:
      return err("Option is invalid or missing required argument");
    }
  }

  // #include <time.h>

  // register clock_t start = clock();
  // adam_bench(&data, 1000000000ULL);
  // double duration = (double)(clock() - start) / (double)CLOCKS_PER_SEC;
  // printf("Time: %lfs\n", duration);
  // printf("diffuse(): %lfs\n", b);
  // printf("apply(): %lfs\n", c);
  // printf("mix(): %lfs\n", d);

  dump_buffer(&cli);

  return 0;
}