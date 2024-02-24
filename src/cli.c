#include <getopt.h>
#include <stdio.h>

#include "../include/api.h"
#include "../include/support.h"
#include "../include/test.h"

#define STRINGIZE(a) #a
#define STRINGIFY(a) STRINGIZE(a)

#define MAJOR        1
#define MINOR        4
#define PATCH        0

#define OPTSTR       ":hvdfbxoap:m:w:e:r:u::s::n::"
#define ARG_COUNT    16

//  Pointer to RNG buffer and state
static adam_data data;

//  Number of bits in results (8, 16, 32, 64)
static u8 width;

//  Print hex?
static bool hex;

//  Print octal?
static bool octal;

//  Number of results to return to user (max 1000)
static u16 results;

// Flag for floating point output
static u8 dbl_mode;

//  Number of decimal places for floating point output (default 15)
static u8 precision;

// Multiplier to scale floating point results, if the user wants
// This returns doubles within range (0, mult)
static u64 mult;

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

    register u8 i = 0;

    register u16 running_length = indent;

    printf("\n\033[%uC", indent);

    for (; i < SUMM_PIECES - 1; ++i) {
        printf("%s ", pieces[i]);
        // add one for space
        running_length += sizes[i] + 1;
        if (running_length + sizes[i + 1] + 1 >= swidth) {
            // add 5 for "adam " to create hanging indent for succeeding lines
            printf("\n\033[%uC", indent + 5);
            running_length = indent + 5;
        }
    }
    printf("%s", pieces[SUMM_PIECES - 1]);

    printf("\n\n");
}

static u8 nearest_space(const char *str, u8 offset)
{
    register u8 a = offset;
    register u8 b = offset;

    while (str[a] && str[a] != ' ')
        --a;

    while (str[b] && str[b] != ' ')
        ++b;

    return (b - offset < offset - a) ? b : a;
}

static u8 help(void)
{
    u16 center, indent, swidth;
    get_print_metrics(&center, &indent, &swidth);

    const u8 CENTER      = center - 4;
    const u8 INDENT      = indent - 1;                    // subtract 1 because it is half of width for arg (ex. "-d")
    const u8 HELP_INDENT = INDENT + INDENT + 1;           // total indent for help descriptions if they have to go to next line
    const u8 HELP_WIDTH  = swidth - HELP_INDENT - indent; // max length for help description in COL 2 before it needs to wrap

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
        "Get the seed for the generated buffer (no parameter), or provide your own. Seeds are reusable but should be kept secret",
        "Get the nonce for the generated buffer (no parameter), or provide your own. Nonces should be unique per seed and kept secret",
        "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 1000)",
        "The amount of numbers to generate and return, written to stdout. Must be within max limit for current width (see -d)",
        "Dump entire buffer using the specified width (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)",
        "Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64",
        "Just bits... literally",
        "Write an ASCII or binary sample of bits/doubles to file for external assessment. You can choose a multiplier to output up to 1GB of bits, or 1 billion doubles (with optional scaling factor), at a time",
        "Examine a sample of 1000000 bits (1 Mb) with the ENT framework and log some statistical properties of the output sequence. You can choose a multiplier within [1, " STRINGIFY(BITS_TESTING_LIMIT) "] to examine up to 1GB at a time",
        "Print numbers in hexadecimal format with leading prefix",
        "Print numbers in octal format with leading prefix",
        "Enable floating point mode to generate doubles in (0, 1) instead of integers",
        "The number of decimal places to display when printing doubles. Must be within [1, 15]. Default is 15",
        "Multiplier for randomly generated doubles, such that they fall in the range (0, <MULTIPLIER>)"
    };

    const u8 lengths[ARG_COUNT] = { 25, 33, 119, 124, 109, 116, 91, 81, 22, 200, 197, 55, 49, 76, 100, 93 };

    register short len;
    register u16 line_width;
    for (u8 i = 0; i < ARG_COUNT; ++i) {
        line_width = nearest_space(ARGSHELP[i], HELP_WIDTH);
        printf("\n\033[%uC\033[1;33m-%c\033[m\033[%uC%.*s", INDENT, ARGS[i], INDENT, line_width, ARGSHELP[i]);
        len = lengths[i] - line_width;
        while (len > 0) {
            ARGSHELP[i] += line_width;
            line_width = nearest_space(ARGSHELP[i], HELP_WIDTH);
            printf("\n\033[%uC%.*s", HELP_INDENT, line_width, ARGSHELP[i]);
            len -= line_width;
        }
    }

    putchar('\n');

    return 0;
}

static void print_int()
{
    const u64 num = adam_int(&data, width);
    if (hex)
        printf("0x%llx", num);
    else if (octal)
        printf("0o%llo", num);
    else
        printf("%llu", num);
}

static void print_dbl()
{
    const double d = adam_dbl(&data, mult);
    printf("%.*lf", precision, d);
}

static u8 dump_buffer()
{
    void (*write_fn)() = (!dbl_mode) ? &print_int : &print_dbl;

    write_fn();

    while (--results > 0) {
        printf("\n");
        write_fn();
    }

    putchar('\n');

    return 0;
}

static u8 uuid(const char *strlimit)
{
    register u16 limit = a_to_u(optarg, 1, 1000);
    if (!limit)
        return err("Invalid amount specified. Value must be within range [1, 1000]");

    u8 buf[16];

    register u16 i = 0;

    u64 lower, upper;

    do {
        lower = adam_int(&data, 64);
        upper = adam_int(&data, 64);
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

static u8 assessf(bool ascii_mode)
{
    u32 output_mult;
    fprintf(stderr, "\033[mSequence Size (x 1000): \033[1;33m");
    while (!scanf(" %u", &output_mult) || output_mult < 1 || output_mult > DBL_TESTING_LIMIT) {
        err("Output multiplier must be between [1, " STRINGIFY(DBL_TESTING_LIMIT) "]");
        fprintf(stderr, "\033[mSequence Size (x 1000): \033[1;33m");
    }

    const u64 limit = TESTING_DBL * mult;

    register double duration;
    if (ascii_mode)
        duration = dbl_ascii(limit, &data.seed[0], &data.nonce, mult, precision);
    else
        duration = dbl_bytes(limit, &data.seed[0], &data.nonce, mult);

    fprintf(stderr,
        "\n\033[0mGenerated \033[36m%llu\033[m doubles in \033[36m%lfs\033[m\n",
        limit, duration);

    return duration;
}

static u8 assess()
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

    if (dbl_mode)
        return assessf(c == '1');

    u32 mult;
    fprintf(stderr, "\033[mSequence Size (x 1000000): \033[1;33m");
    while (!scanf(" %u", &mult) || mult < 1 || mult > BITS_TESTING_LIMIT) {
        err("Output multiplier must be between [1, " STRINGIFY(BITS_TESTING_LIMIT) "]");
        fprintf(stderr, "\033[mSequence Size (x 1000000): \033[1;33m");
    }

    register u64 limit = ASSESS_UNIT * mult;

    if (c == '1')
        duration = stream_ascii(limit, &data.seed[0], &data.nonce);
    else
        duration = stream_bytes(limit, &data.seed[0], &data.nonce);

    fprintf(stderr,
        "\n\033[0mGenerated \033[36m%llu\033[m bits in \033[36m%lfs\033[m\n",
        limit, duration);

    return 0;
}

static u8 examine(const char *strlimit)
{
    // Initialize properties
    rng_test rsl;
    ent_report ent;
    rsl.ent = &ent;

    // Record initial state and connect internal state to rsl_test
    u64 init_values[5];
    init_values[0] = rsl.seed[0] = data.seed[0];
    init_values[1] = rsl.seed[1] = data.seed[1];
    init_values[2] = rsl.seed[2] = data.seed[2];
    init_values[3] = rsl.seed[3] = data.seed[3];
    init_values[4] = rsl.nonce = data.nonce;

    // Check for and validate multiplier
    register u64 limit = EXAMINE_UNIT;
    if (strlimit != NULL)
        limit *= a_to_u(strlimit, 1, BITS_TESTING_LIMIT);

    printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", limit);
    register double duration = get_seq_properties(limit, &rsl);

    print_seq_results(&rsl, limit, &init_values[0]);

    printf("\n\033[1;33mExamination Complete! (%lfs)\033[m\n\n", duration);

    return 0;
}

int main(int argc, char **argv)
{
    adam_setup(&data, NULL, NULL);

    results   = 1;
    hex       = false;
    octal     = false;
    dbl_mode  = false;
    precision = 15;
    width     = 64;
    mult      = 0;

    register char opt;
    while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (opt) {
        case 'h':
            return help();
        case 'v':
            puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
            return 0;
        case 'a':
            return assess();
        case 'b':
            stream_bytes(__UINT64_MAX__, &data.seed[0], &data.nonce);
            return 0;
        case 'x':
            hex = true;
            continue;
        case 'o':
            octal = true;
            continue;
        case 'w':
            width = a_to_u(optarg, 8, 32);
            if (UNLIKELY(width != 8 || width != 16 || width != 32))
                return err("Width must be either 8, 16, 32");
            continue;
        case 'r':
            results = a_to_u(optarg, 1, ADAM_BUF_SIZE * (64 / width));
            if (!results)
                return err("Invalid number of results specified for desired width");
            continue;
        case 's':
            rwseed(&data.seed[0], optarg);
            continue;
        case 'n':
            rwnonce(&data.nonce, optarg);
            continue;
        case 'u':
            return uuid(optarg);
        case 'd':
            results = ADAM_BUF_SIZE * (64 / width);
            dump_buffer();
            return 0;
        case 'f':
            dbl_mode = true;
            continue;
        case 'p':
            dbl_mode  = true;
            precision = a_to_u(optarg, 1, 15);
            if (!precision)
                return err("Floating point precision must be between [1, 15]");
            continue;
        case 'm':
            dbl_mode = true;
            mult     = a_to_u(optarg, 1, __UINT64_MAX__);
            if (!mult)
                return err("Floating point scaling factor must be between [1, " STRINGIFY(__UINT64_MAX__) "]");
            continue;
        case 'e':
            return examine(optarg);
        default:
            return err("Option is invalid or missing required argument");
        }
    }

    dump_buffer();

    return 0;
}