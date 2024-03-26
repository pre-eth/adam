#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/api.h"
#include "../include/test.h"
#include "../include/util.h"

#define STRINGIZE(a) #a
#define STRINGIFY(a) STRINGIZE(a)

#define MAJOR        0
#define MINOR        9
#define PATCH        0

#define OPTSTR       ":hvdfbxoap:m:w:e:r:u::s::n::"
#define ARG_COUNT    16

//  Number of bits in results (8, 16, 32, 64)
static u8 width;

//  Print hex?
static bool hex;

//  Print octal?
static bool octal;

//  Number of results to return to user (max 1000)
static u16 results;

//  Flag for floating point output
static bool dbl_mode;

//  Number of decimal places for floating point output (default 15)
static u8 precision;

//  Multiplier to scale floating point results, if the user wants
//  This returns doubles within range (0, mult)
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
        "Just bits... literally. Pass the -f flag beforehand to stream random doubles instead of integers",
        "Write an ASCII or binary sample of bits/doubles to file for external assessment. You can choose a multiplier to output up to 100GB of bits, or 1 billion doubles (with optional scaling factor), at a time",
        "Examine a sample of 1MB with the ENT framework and some other statistical tests to reveal properties of the output sequence. You can choose a multiplier within [1, " STRINGIFY(BITS_TESTING_LIMIT) "] to examine up to 100GB at a time",
        "Print numbers in hexadecimal format with leading prefix",
        "Print numbers in octal format with leading prefix",
        "Enable floating point mode to generate doubles in (0.0, 1.0) instead of integers",
        "How many decimal places to print when printing doubles. Must be within [1, 15]. Default is 15",
        "Multiplier for randomly generated doubles, such that they fall in the range (0, <MULTIPLIER>)"
    };

    const u8 lengths[ARG_COUNT] = { 45, 33, 119, 124, 109, 116, 91, 81, 96, 202, 204, 55, 49, 80, 93, 93 };

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

static void print_int(adam_data data)
{
    const u64 num = adam_int(data, width, false);
    if (hex)
        printf("0x%llx", num);
    else if (octal)
        printf("0o%llo", num);
    else
        printf("%llu", num);
}

static void print_dbl(adam_data data)
{
    const double d = adam_dbl(data, mult, false);
    printf("%.*lf", precision, d);
}

static u8 dump_buffer(adam_data data)
{
    void (*write_fn)(adam_data) = (!dbl_mode) ? &print_int : &print_dbl;

    write_fn(data);

    while (--results > 0) {
        printf("\n");
        write_fn(data);
    }

    putchar('\n');

    adam_cleanup(data);

    return 0;
}

static u8 uuid(adam_data data, const char *strlimit)
{
    register u16 limit = a_to_u(optarg, 1, 1000);
    if (!limit)
        return err("Invalid amount specified. Value must be within range [1, 1000]");

    u8 buf[16];

    register u16 i = 0;

    register u64 lower, upper;

    do {
        lower = adam_int(data, 64, false);
        upper = adam_int(data, 64, false);
        gen_uuid(upper, lower, &buf[0]);

        // Print the UUID
        printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%"
               "02x%02x%02x%02x",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
            buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14],                                buf[15]);

        putchar('\n');
    } while (++i < limit);

    return 0;
}

static u8 assessf(adam_data data, const u64 limit, bool ascii_mode)
{
    double *_buf = aligned_alloc(ADAM_ALIGNMENT, limit * sizeof(double));
    if (_buf == NULL)
        return 1;

    //  We allow higher assess fill values for testing than supported by the API
    //  so for the larger user provided values we need to split fills
    if (limit > ADAM_FILL_MAX) {
        adam_dfill(data, _buf, mult, limit - ADAM_FILL_MAX);
        adam_dfill(data, &_buf[limit - ADAM_FILL_MAX], mult, ADAM_FILL_MAX);
    } else {
        adam_dfill(data, _buf, mult, limit);
    }

    if (ascii_mode) {
        register u32 i = 0;
        while (limit - i > 8) {
            fprintf(stdout, "%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n",
                precision, _buf[i], precision, _buf[i + 1], precision, _buf[i + 2], precision, _buf[i + 3],
                precision, _buf[i + 4], precision, _buf[i + 5], precision, _buf[i + 6], precision, _buf[i + 7]);
            i += 8;
        }

        while (i < limit)
            fprintf(stdout, "%.*lf\n", precision, _buf[i++]);
    } else
        fwrite(&_buf[0], sizeof(double), limit, stdout);

    free(_buf);
    return 0;
}

static void streamf(adam_data data)
{
    double *_buf = aligned_alloc(ADAM_ALIGNMENT, ADAM_BUF_SIZE * sizeof(double));
    if (_buf == NULL)
        return;

    register u64 written = 0;

    while (written < __UINT64_MAX__) {
        adam_dfill(data, _buf, 0, ADAM_BUF_SIZE);
        fwrite(&_buf[0], sizeof(double), ADAM_BUF_SIZE, stdout);
        written += ADAM_BUF_SIZE;
    }
}

static u8 assess(adam_data data)
{
    // clang-format off
    #define ASSESS_PROMPT(var, msg, fmt, cond, error)    \
    {                                                    \
        while (true) {                                   \
            fprintf(stderr, "\033[m" msg " \033[1;33m"); \
            if (!scanf(fmt, var) || cond) {              \
                err(error);                              \
                continue;                                \
            }                                            \
            break;                                       \
        }                                                \
    }
    // clang-format on

    char file_name[65];
    ASSESS_PROMPT(&file_name[0], "File name:", " %64s", false, "Please enter a valid file name");

    char c = '0';
    ASSESS_PROMPT(&c, "Output type (1 = ASCII, 2 = BINARY):", " %c", (c != '1' && c != '2'), "Value must be 1 or 2");

    const bool file_mode = (c == '1');
    freopen(file_name, file_mode ? "w+" : "wb+", stdout);

    u32 output_mult;
    register u64 limit;

    if (dbl_mode) {
        ASSESS_PROMPT(&output_mult, "Sequence Size (x 1000):", " %lu", (output_mult < 1 || output_mult > DBL_TESTING_LIMIT), "Output multiplier must be between [1, " STRINGIFY(DBL_TESTING_LIMIT) "]");
        limit = TESTING_DBL * output_mult;
        if (assessf(data, limit, file_mode))
            return err("Could not allocate enough space for adam -a");
    } else {
        ASSESS_PROMPT(&output_mult, "Sequence Size (x 1MB):", " %lu", (output_mult < 1 || output_mult > BITS_TESTING_LIMIT), "Output multiplier must be between [1, " STRINGIFY(BITS_TESTING_LIMIT) "]");

        limit = TESTING_BITS * output_mult;
        if (file_mode) {
            const u64 amount = (limit >> 6) + !!(limit & 63);

            u64 *restrict buffer = aligned_alloc(ADAM_ALIGNMENT, sizeof(u64) * amount);
            if (buffer == NULL)
                return err("Could not allocate enough space for adam -a");

            adam_fill(data, buffer, 64, amount);
            print_ascii_bits(buffer, amount);

            free(buffer);
        } else {
            adam_stream(data, limit, file_name);
        }
    }

    fprintf(stderr,
        "\n\033[0mGenerated \033[36m%llu\033[m %s and saved to \033[36m%s\033[m\n",
        limit, (dbl_mode) ? "doubles" : "bits", file_name);

    adam_cleanup(data);

    return 0;
}

static u8 examine(adam_data data, const char *strlimit)
{
    register u64 limit = TESTING_BITS;
    if (strlimit != NULL)
        limit *= a_to_u(strlimit, 1, BITS_TESTING_LIMIT);

    printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", limit);

    adam_examine(limit, data);

    puts("\n\033[1;33mExamination Complete!\033[m\n");

    adam_cleanup(data);

    return 0;
}

int main(int argc, char **argv)
{

    adam_data data = adam_setup(NULL, NULL);

    if (data == NULL)
        return err("Could not allocate space for adam_data struct! Exiting.");

    //  Initialize the non-zero defaults
    results   = 1;
    precision = 15;
    width     = 64;

    register char opt;
    while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (opt) {
        case 'h':
            adam_cleanup(data);
            return help();
        case 'v':
            adam_cleanup(data);
            puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
            return 0;
        case 'a':
            return assess(data);
        case 'b':
            if (dbl_mode)
                streamf(data);
            else
                adam_stream(data, __UINT64_MAX__, NULL);
            adam_cleanup(data);
            return 0;
        case 'x':
            hex   = true;
            octal = false;
            continue;
        case 'o':
            octal = true;
            hex   = false;
            continue;
        case 'w':
            width = a_to_u(optarg, 8, 32);
            if (UNLIKELY(width != 8 && width != 16 && width != 32)) {
                adam_cleanup(data);
                return err("Alternate width must be either 8, 16, 32");
            }
            results = ADAM_BUF_SIZE * (64 / width);
            continue;
        case 'r':
            results = a_to_u(optarg, 1, ADAM_BUF_SIZE * (64 / width));
            if (!results) {
                adam_cleanup(data);
                return err("Invalid number of results specified for desired width");
            }
            continue;
        case 's':
            rwseed(adam_seed(data), optarg);
            continue;
        case 'n':
            rwnonce(adam_nonce(data), optarg);
            continue;
        case 'u':
            return uuid(data, optarg);
        case 'd':
            results = ADAM_BUF_SIZE * (64 / width);
            continue;
        case 'f':
            dbl_mode = true;
            continue;
        case 'p':
            dbl_mode  = true;
            precision = a_to_u(optarg, 1, 15);
            if (!precision) {
                adam_cleanup(data);
                return err("Floating point precision must be between [1, 15]");
            }
            continue;
        case 'm':
            dbl_mode = true;
            mult     = a_to_u(optarg, 1, __UINT64_MAX__);
            if (!mult) {
                adam_cleanup(data);
                return err("Floating point scaling factor must be between [1, " STRINGIFY(__UINT64_MAX__) "]");
            }
            continue;
        case 'e':
            return examine(data, optarg);
        default:
            adam_cleanup(data);
            return err("Option is invalid or missing required argument");
        }
    }

    dump_buffer(data);

    return 0;
}