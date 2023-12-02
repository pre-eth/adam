#include <stdio.h> // for output
#include <unistd.h> // for sleep()

#include "../include/adam.h"
#include "../include/cli.h"
#include "../include/support.h"

u8 cli_init(rng_cli *cli)
{
  cli->fmt = "%llu";
  cli->width = 64;
  cli->results = 1;
  // cli->precision = 0;

  return 0;
}

u8 version()
{
  return puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
}

FORCE_INLINE static void print_summary(const u16 swidth, const u16 indent)
{
#define SUMM_PIECES 8

  const char *pieces[SUMM_PIECES] = {
    "\033[1madam \033[m[-h|-v|-l|-b|-x|-o]", "[-w \033[1mwidth\033[m]",
    "[-a \033[1mmultiplier?\033[m]", "[-e \033[1mmultiplier?\033[m]",
    "[-r \033[3mresults?\033[m]", "[-s \033[3mseed?\033[m]",
    "[-n \033[3mnonce?\033[m]", "[-u \033[3mamount?\033[m]"
  };

  const u8 sizes[SUMM_PIECES + 1] = { 21, 10, 16, 16, 13, 10, 11, 12, 0 };

  u8 i = 0, running_length = indent;

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

  putchar('\n'), putchar('\n');
}

u8 help()
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

  const char ARGS[ARG_COUNT] = { 'h', 'v', 's', 'n', 'u', 'r',
    'w', 'b', 'a', 'e', 'l', 'x', 'o' };
  const char *ARGSHELP[ARG_COUNT] = {
    "Get command summary and all available options",
    "Version of this software (" STRINGIFY(MAJOR) "." STRINGIFY(
        MINOR) "." STRINGIFY(PATCH) ")",
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
    "You can choose a multiplier within [1, 8000] to output up to 1GB at a time",
    "Live stream of continuously generated numbers",
    "Print numbers in hexadecimal format with leading prefix",
    "Print numbers in octal format with leading prefix"
  };
  const u8 lengths[ARG_COUNT] = { 25, 32, 119, 117, 108, 107,
    81, 22, 187, 178, 45, 55, 49 };

  register short len;
  for (u8 i = 0; i < ARG_COUNT; ++i) {
    printf("\033[%uC\033[1;33m-%c\033[m\033[%uC%.*s\n", INDENT, ARGS[i], INDENT,
        HELP_WIDTH, ARGSHELP[i]);
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
  const u8 max = SEQ_SIZE >> CTZ(cli->width);
  cli->results -= (cli->results > max) * (cli->results - max);
  return 0;
}

u8 set_results(rng_cli *cli, const char *strsl)
{
  cli->results = (strsl == NULL) ? SEQ_SIZE >> CTZ(cli->width) : a_to_u(optarg, 1, SEQ_SIZE >> CTZ(cli->width));
  return 0;
}

u8 print_buffer(rng_cli *cli)
{
  register u64 mask;

  if (cli->width == 64)
    mask = __UINT64_MAX__ - 1;
  else
    mask = (1UL << cli->width) - 1;

  rng_data *data = cli->data;

  // Need to do this for default precision because
  // we can't rely on overflow arithmetic :(
  register u8 inc = (cli->width == 64), idx = 0;

  printf(cli->fmt, data->buffer[idx] & mask);

  while (--cli->results > 0) {
    printf(",\n");
    data->buffer[idx] >>= cli->width;
    idx += (inc || !data->buffer[idx]);
    printf(cli->fmt, data->buffer[idx] & mask);
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

  adam(data);
  u64 *restrict _ptr = &data->buffer[0];
  register u8 i = 0;

  do {
    gen_uuid(&data->buffer[(i & 0x7F) << 1], &buf[0]);

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
  u16 limit = a_to_u(strlimit, 1, TESTING_LIMIT);
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
      "\n\033[0;1mWrote \033[36m%llu\033[m bits in \033[36m%lfs\033[m\n",
      total, duration);

  return 0;
}

u8 infinite(rng_data *data)
{
/*
  There are 256 numbers per buffer. But we only need 75 to print one
  iteration. So 75 * 3 = 225. 256 - 225 = 31. Thus, for each buffer 31
  numbers will be left over. To avoid waste, these numbers ARE used, but
  only when the index upon exiting the loop is 225, meaning the index
  started from 0 for this iteration.

  If the 31 numbers are leftover and printed, the index is appropriately
  changed to 31 before the goto statement trigger, starting the loop with
  i = 31. Meaning when we exit the loop this time, our index value is at
  255! (31 + 75 * 3) - 1. We subtract one to avoid overflow. Since the
  index is NOT 225 in this case, the statement to print leftovers won't
  happen, and the next iteration will start clean.

  This pattern is cyclical as you may be able to tell - one round there will
  be leftovers, next round there won't. On and on until the program exits.
*/
#define LIVE_ITER (BUF_SIZE - 31)

  const char *ADAM_ASCII = {
    "%s\033[38;2;173;58;0m/"
    "\033[38;2;255;107;33m@@@@@@\033[38;2;173;58;0m\\\033[0m"
    "%s\033[38;2;173;58;0m/"
    "(\033[38;2;255;107;33m@@\033[38;2;173;58;0m((((((((((((((((\033["
    "38;2;"
    "255;107;33m@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((((((((((((((((((("
    "(\033["
    "38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;173;58;0m(((((((((((((((((((((((((((((\033[38;2;255;"
    "107;33m#"
    "@##@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((((((((((\033[38;2;"
    "255;"
    "107;33m@#####&\033[38;2;173;58;0m(((((((((((((((\033[38;2;255;107;"
    "33m@@@"
    "@\033[0m"
    "%s\033[38;2;173;58;0m((((((((((((((\033[38;2;255;107;33m#@@\033["
    "38;2;"
    "173;58;0m((((\033[38;2;255;107;33m@@@@@\033[38;2;173;58;0m(((((((("
    "(((((("
    "\033[38;2;255;107;33m@@\033[0m"
    "%s\033[38;2;173;58;0m((((((((\033[38;2;255;107;33m@@\033[38;2;173;"
    "58;0m("
    "((((((((\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((((((((\033["
    "38;2;"
    "255;107;33m@&#@\033[38;2;173;58;0m(((((((((((\033[38;2;255;107;"
    "33m#@@"
    "\033[0m"
    "%s\033[38;2;173;58;0m(\033[38;2;255;107;33m@@@\033[38;2;173;58;0m("
    "(("
    "\033[38;2;255;107;33m##\033[38;2;173;58;0m((((((((\033[38;2;255;"
    "107;33m@"
    "@\033[38;2;173;58;0m((((((((((((((\033[38;2;255;107;33m##@##\033["
    "38;2;"
    "173;58;0m(((((((((\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((\033[38;2;255;107;"
    "33m##"
    "\033[38;2;173;58;0m(((\033[38;2;255;107;33m@#####\033[38;2;173;58;"
    "0m(((("
    "(\033[38;2;255;107;33m######\033[38;2;173;58;0m((((\033[38;2;255;"
    "107;"
    "33m###@\033[38;2;173;58;0m(((((((((\033[38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((\033[38;2;255;"
    "107;33m@&"
    "#########\033[38;2;173;58;0m((((\033[38;2;255;107;33m###@@##\033["
    "38;2;"
    "173;58;0m(((\033[38;2;255;107;33m@###@\033[38;2;173;58;0m(((((((("
    "\033["
    "38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((((((\033[38;2;255;"
    "107;"
    "33m#&@@@#########@\033[38;2;173;58;0m(((((\033[38;2;255;107;33m@@"
    "\033["
    "38;2;173;58;0m((((\033[38;2;255;107;33m#####@\033[38;2;173;58;0m(("
    "((((("
    "\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@@@@\033[38;2;173;58;0m((((\033[38;2;255;"
    "107;33m&"
    "#\033[38;2;173;58;0m(\033[38;2;255;107;33m################&@@@####"
    "@###@"
    "\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m@&&\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((\033[38;2;255;107;"
    "33m####"
    "#\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m###########@&####"
    "\033["
    "38;2;173;58;0m((((\033[38;2;255;107;33m@##@&\033[0m"
    "%s\033[38;2;173;58;0m(\033[38;2;255;107;33m@\033[38;2;173;58;0m((("
    "(\033["
    "38;2;255;107;33m@####@######@@###\033[38;2;173;58;0m(((\033[38;2;"
    "255;"
    "107;33m@######@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m@#"
    "\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((\033[38;2;255;"
    "107;33m##"
    "############@@##\033[38;2;173;58;0m((((((((((((((((((\033[38;2;"
    "255;107;"
    "33m@\033[0m"
    "%s\033[38;2;255;107;33m@\033[0m"
    "%s\033[38;2;173;58;0m/"
    "\033[38;2;255;107;33m@\033[38;2;173;58;0m((\033[38;2;255;107;33m@#"
    "##"
    "\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m###@@@@@@@@@@@@@&@#"
    "#@"
    "\033[38;2;173;58;0m((\033[38;2;255;107;33m@&\033[0m"
    "%s\033[38;2;255;107;33m#@\033[0m"
    "%s\033[38;2;173;58;0m(((\033[38;2;255;107;33m################@@@##"
    "######"
    "#\033[0m"
    "%s\033[38;2;173;58;0m/"
    "\033[38;2;255;107;33m#\033[38;2;173;58;0m)\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((\033[38;2;255;107;"
    "33m####"
    "\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m####&###\033[38;2;"
    "173;58;"
    "0m(((((((\033[38;2;255;107;33m##@\033[0m"
    "%s\033[38;2;255;107;33m@&@\033[0m"
    "%s\033[38;2;255;107;33m.@\033[38;2;173;58;0m((((\033[38;2;255;107;"
    "33m###"
    "###########@#@######@@@@@#####\033[0m"
    "%s\033[38;2;255;107;33m@&@\033[0m"
    "%s\033[38;2;255;107;33m@@@@\033[38;2;173;58;0m((\033[38;2;255;107;"
    "33m@##"
    "##@\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m#####@@@#\033["
    "38;2;"
    "173;58;0m(((((((\033[38;2;255;107;33m&#@@\033[38;2;173;58;"
    "0m\\\033[0m"
    "%s\033[38;2;255;107;33m@@#@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((\033[38;2;255;107;"
    "33m@#####"
    "@@#######@#################@\033[0m"
    "%s\033[38;2;255;107;33m@@##@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((\033[38;2;255;107;"
    "33m@####"
    "#\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m######@###@@@@@"
    "\033[0m"
    "%s\033[38;2;255;107;33m@@#@##&#\033[0m"
    "%s\033[38;2;255;107;33m@@\033[38;2;173;58;0m((\033[38;2;255;107;"
    "33m@@"
    "\033[38;2;173;58;0m((((\033[38;2;255;107;33m@##########&\033[38;2;"
    "173;"
    "58;0m((((\033[38;2;255;107;33m###@\033[0m"
    "%s\033[38;2;255;107;33m@&&##@\033[38;2;173;58;0m))\033[38;2;255;"
    "107;33m@"
    "#@@\033[0m"
    "%s\033[38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((\033[38;2;255;107;"
    "33m&##"
    "#####\033[38;2;173;58;0m((\033[38;2;255;107;33m#@############&"
    "\033[38;2;"
    "173;58;0m)))))))\033[0m"
    "%s\033[38;2;255;107;33m@#\033[38;2;173;58;0m(((((\033[38;2;255;"
    "107;33m#@"
    "##############@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m#@#@"
    "\033[0m"
    "%s\033[38;2;255;107;33m@@@@@@&@#\033[38;2;173;58;0m((((\033[38;2;"
    "255;"
    "107;33m##@@@@@#\033[38;2;173;58;0m((((((\033[38;2;255;107;33m#&#@#"
    "&\033["
    "0m"
    "%s\033[38;2;255;107;33m####\033[0m"
    "%s\033[38;2;255;107;33m#&#\033[38;2;173;58;0m(\033[38;2;255;107;"
    "33m@@@@@"
    "\033[0m%s%s"
  };

  // set window dimensions for live stream
  printf("\033[8;29;64t");
  // no buffering
  setbuf(stdout, NULL);

  char lines[40][100];

  register u8 i = 0;
  adam(data);
  u64 *restrict _ptr = &data->buffer[0];
live_adam:
  do {
    snprintf(lines[0], 96, "%llu%llu%llu%llu%llu%llu", GET_3(i + 0),
        GET_3(i + 3));
    snprintf(lines[1], 50, "%llu%llu%llu", GET_3(i + 6));
    snprintf(lines[2], 39, "%llu%llu%llu", GET_3(i + 9));
    snprintf(lines[3], 32, "%llu%llu%llu", GET_3(i + 12));
    snprintf(lines[4], 29, "%llu%llu", GET_2(i + 15));
    snprintf(lines[5], 26, "%llu%llu", GET_2(i + 17));
    snprintf(lines[6], 16, "%llu%llu", GET_2(i + 19));
    snprintf(lines[7], 15, "%llu", GET_1(i + 21));
    snprintf(lines[8], 17, "%llu%llu", GET_2(i + 22));
    snprintf(lines[9], 17, "%llu%llu", GET_2(i + 24));
    snprintf(lines[10], 11, "%llu%llu", GET_2(i + 26));
    snprintf(lines[11], 14, "%llu", GET_1(i + 28));
    snprintf(lines[12], 18, "%llu%llu", GET_2(i + 29));
    snprintf(lines[13], 20, "%llu%llu", GET_2(i + 31));
    snprintf(lines[14], 21, "%llu%llu", GET_2(i + 33));
    snprintf(lines[15], 7, "%llu", GET_1(i + 35));
    snprintf(lines[16], 18, "%llu%llu", GET_2(i + 36));
    snprintf(lines[17], 8, "%llu", GET_1(i + 38));
    snprintf(lines[18], 18, "%llu%llu", GET_2(i + 39));
    snprintf(lines[19], 15, "%llu", GET_1(i + 41));
    snprintf(lines[20], 17, "%llu%llu", GET_2(i + 42));
    snprintf(lines[21], 13, "%llu", GET_1(i + 44));
    snprintf(lines[22], 15, "%llu", GET_1(i + 45));
    snprintf(lines[23], 8, "%llu", GET_1(i + 46));
    snprintf(lines[24], 16, "%llu", GET_1(i + 47));
    snprintf(lines[25], 5, "%llu", GET_1(i + 48));
    snprintf(lines[26], 21, "%llu%llu", GET_2(i + 49));
    snprintf(lines[27], 3, "%llu", GET_1(i + 51));
    snprintf(lines[28], 22, "%llu%llu", GET_2(i + 52));
    snprintf(lines[29], 5, "%llu", GET_1(i + 54));
    snprintf(lines[30], 19, "%llu%llu", GET_2(i + 55));
    snprintf(lines[31], 6, "%llu", GET_1(i + 57));
    snprintf(lines[32], 16, "%llu%llu", GET_2(i + 58));
    snprintf(lines[33], 4, "%llu", GET_1(i + 60));
    snprintf(lines[34], 29, "%llu%llu", GET_2(i + 61));
    snprintf(lines[35], 31, "%llu%llu", GET_2(i + 63));
    snprintf(lines[36], 38, "%llu%llu%llu", GET_3(i + 65));
    snprintf(lines[37], 3, "%llu", GET_1(i + 68));
    snprintf(lines[38], 26, "%llu%llu", GET_2(i + 69));
    snprintf(lines[39], 65, "%llu%llu%llu%llu", GET_3(i + 71), GET_1(i + 74));
    printf(ADAM_ASCII, lines[0], lines[1], lines[2], lines[3], lines[4],
        lines[5], lines[6], lines[7], lines[8], lines[9], lines[10],
        lines[11], lines[12], lines[13], lines[14], lines[15], lines[16],
        lines[17], lines[18], lines[19], lines[20], lines[21], lines[22],
        lines[23], lines[24], lines[25], lines[26], lines[27], lines[28],
        lines[29], lines[30], lines[31], lines[32], lines[33], lines[34],
        lines[35], lines[36], lines[37], lines[38], lines[39]);
    sleep(1);
    fwrite("\033[2J\r", 1, 5, stdout);
  } while ((i += 75 - (i == 181)) < LIVE_ITER);

  const u8 leftovers = (i == 225);

  if (leftovers) {
    snprintf(lines[0], 96, "%llu%llu%llu%llu%llu%llu", GET_3(i + 0),
        GET_3(i + 3));
    snprintf(lines[1], 50, "%llu%llu%llu", GET_3(i + 6));
    snprintf(lines[2], 39, "%llu%llu%llu", GET_3(i + 9));
    snprintf(lines[3], 32, "%llu%llu%llu", GET_3(i + 12));
    snprintf(lines[4], 29, "%llu%llu", GET_2(i + 15));
    snprintf(lines[5], 26, "%llu%llu", GET_2(i + 17));
    snprintf(lines[6], 16, "%llu%llu", GET_2(i + 19));
    snprintf(lines[7], 15, "%llu", GET_1(i + 21));
    snprintf(lines[8], 17, "%llu%llu", GET_2(i + 22));
    snprintf(lines[9], 17, "%llu%llu", GET_2(i + 24));
    snprintf(lines[10], 11, "%llu%llu", GET_2(i + 26));
    snprintf(lines[11], 14, "%llu", GET_1(i + 28));
    snprintf(lines[12], 18, "%llu%llu", GET_2(i + 29));
  }
  i = ((leftovers) << 5) - (leftovers);
  adam(data);
  goto live_adam;

  return 0;
}
