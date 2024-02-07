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
  rng_data *data;             //  Pointer to RNG buffer and state
  u8 width;                   //  Number of bits in results (8, 16, 32, 64)
  u8 precision;               //  Number of decimal places for floating point output (default 15 for double, 7 for float)
  bool hex;                   //  Print hex?
  bool octal;                 //  Print octal?
  u16 results;                //  Number of results to return to user (varies based on width, max 2048 u8)
} rng_cli;

static void print_summary(const u16 swidth, const u16 indent)
{
#define SUMM_PIECES 8

  const char *pieces[SUMM_PIECES] = {
    "\033[1madam \033[m[-h|-v|-b|-x|-o]", "[-w \033[1mwidth\033[m]",
    "[-a \033[1mmultiplier\033[m]", "[-e \033[1mmultiplier\033[m]",
    "[-r \033[3mresults?\033[m]", "[-s \033[3mseed?\033[m]",
    "[-n \033[3mnonce?\033[m]", "[-u \033[3mamount?\033[m]"
  };

  const u8 sizes[SUMM_PIECES + 1] = { 19, 10, 15, 15, 13, 10, 11, 12, 0 };

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

u8 help()
{
#define ARG_COUNT 15
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
    'w',
    'b',
    'a',
    'e',
    'x',
    'o',
  };

  const char *ARGSHELP[ARG_COUNT] = {
    "Get command summary and all available options",
    "Version of this software (" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) ")",
    "Get the seed for the generated buffer (no parameter) or provide "
    "your own. Seeds are reusable but should be kept secret.",
    "Get the nonce for the generated buffer (no parameter) or provide "
    "your own. Nonces should ALWAYS be unique and secret",
    "Generate a universally unique identifier (UUID). Optionally "
    "specify a number of UUID's to generate (max 128)",
    "Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8). "
    "No argument dumps entire buffer",
    "Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64",
    "Just bits... literally",
    "Write an ASCII or binary sample of 1000000 bits (1 Mb) to file for external assessment with your favorite tests. "
    "You can choose a multiplier within [1, 8000] to output up to 1GB at a time",
    "Examine a sample of 1000000 bits (1 Mb) with the ENT framework as well as some other in-house statistical tests. "
    "You can choose a multiplier within [1, 8000] to examine up to 1GB at a time",
    "Print numbers in hexadecimal format with leading prefix",
    "Print numbers in octal format with leading prefix"
  };
  const u8 lengths[ARG_COUNT] = { 25, 32, 119, 117, 108, 107,
    81, 22, 187, 178, 55, 49 };

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

u8 set_width(rng_cli *cli, const char *strwidth)
{
  cli->width = a_to_u(strwidth, 8, 32);
  if (UNLIKELY(cli->width & (cli->width - 1)))
    return 1;

  /*
    This line will basically "floor" results to the max value of results
    possible for this new precision in case it exceeds the possible limit
    This can be avoided by ordering your arguments so that -w comes first
  */
  const u8 max = BUF_SIZE * (64 >> CTZ(cli->width));
  cli->results -= (cli->results > max) * (cli->results - max);

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

u8 dump_buffer(rng_cli *cli)
{
  rng_data *data = cli->data;
  void (*write_fn)(rng_cli *cli) = (!data->dbl_mode) ? &print_int : &print_dbl;

  write_fn(cli);

  // index = 0 means old buffer has finished printing,
  // and new buffer has been generated.
  while (--cli->results > 0) {
    printf(",\n");
    write_fn(cli);
  }

  putchar('\n');

  return 0;
}

u8 uuid(const char *strlimit, rng_data *data)
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

u8 assess(const char *strlimit, rng_cli *cli)
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

  fprintf(stderr,
      "\n\033[0;1mGenerated \033[36m%llu\033[m bits in \033[36m%lfs\033[m\n",
      total, duration);

  return 0;
}

u8 cli_runner(int argc, char **argv)
{
  rng_data data;
  rng_cli cli;

  adam_init(&data, false);
  cli.results = 1;
  cli.data = &data;
  cli.hex = false;
  cli.octal = false;
  cli.precision = 15;
  cli.width = 64;

  register char opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
    switch (opt) {
    case 'h':
      return help();
    case 'v':
      return puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
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
      // Returns all results if no argument provided
      cli.results = (optarg == NULL) ? BUF_SIZE * (64 / cli.width) : a_to_u(optarg, 1, BUF_SIZE * (64 / cli.width));
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
      data.dbl_mode = true;
      continue;
    case 'f':
      data.dbl_mode = true;
      cli.width = 32;
      cli.precision = 7;
      continue;
    case 'p':
      data.dbl_mode = true;
      cli.precision = a_to_u(optarg, 2, (cli.width == 32) ? 7 : 15);
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