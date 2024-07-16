#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/api.h"
#include "../include/util.h"

#define STRINGIZE(a) #a
#define STRINGIFY(a) STRINGIZE(a)

#define MAJOR        0
#define MINOR        11
#define PATCH        0

#define OPTSTR       ":hvfbxoap:m:w:e:r:u::i::"
#define ARG_COUNT    14

// Number of bits in results (8, 16, 32, 64)
static NumWidth width;

// Print hex?
static bool hex;

// Print octal?
static bool octal;

// Number of results to return to user (max 1000)
static u16 results;

// Flag for floating point output
static bool dbl_mode;

// Number of decimal places for floating point output (default 15)
static u8 precision;

// Multiplier to scale floating point results, if the user wants
// This returns doubles within range (0, mult)
static u64 mult;

static void print_summary(const u16 swidth, const u16 indent)
{
#define SUMM_PIECES 9

    const char *pieces[SUMM_PIECES] = {
        "\033[1madam\033[m [-h|-v|-a|-b]", "[-i[\033[3mfilename?\033[m]]", "[-xof]",
        "[-w \033[1mwidth\033[m]", "[-m \033[1mmultiplier\033[m]", "[-p \033[1mprecision\033[m]",
        "[-e \033[1mmultiplier\033[m]", "[-r \033[1mresults\033[m]", "[-u[\033[3mamount?\033[m]]"
    };

    const u8 sizes[SUMM_PIECES] = { 18, 15, 6, 10, 15, 14, 15, 12, 13 };

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

    printf("%s\n\n", pieces[SUMM_PIECES - 1]);
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
        'i',
        'u',
        'r',
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
        "Get the input parameters for the last run, or provide your own using a filename.",
        "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 1000)",
        "The amount of numbers to generate and return, written to stdout. Must be within [1, 1000]",
        "Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64",
        "Just bits... literally. Pass the -f flag beforehand to stream random doubles instead of integers",
        "Write an ASCII or binary sample of bits/doubles to file for external assessment. You can choose a multiplier to output up to 100GB of bits, or 1 billion doubles (with optional scaling factor), at a time",
        "Examine a sample of 1MB with the ENT framework and some other statistical tests to reveal properties of the output sequence. You can choose a multiplier within [1, " STRINGIFY(BITS_TESTING_LIMIT) "] to examine up to 100GB at a time",
        "Print numbers in hexadecimal format with leading prefix",
        "Print numbers in octal format with leading prefix",
        "Enable floating point mode to generate doubles in (0.0, 1.0) instead of integers",
        "How many decimal places to print when printing doubles. Must be within [1, 15]. Default is 15",
        "Multiplier for randomly generated doubles, such that they fall in the range (0, MULTIPLIER)"
    };

    const u8 lengths[ARG_COUNT] = { 45, 33, 80, 109, 89, 81, 96, 202, 204, 55, 49, 80, 93, 91 };

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
    const u64 num = adam_int(data, width);
    if (hex) {
        printf("0x%llx", num);
    } else if (UNLIKELY(octal)) {
        printf("0o%llo", num);
    } else {
        printf("%llu", num);
    }
}

static void print_dbl(adam_data data)
{
    const double d = adam_dbl(data, mult);
    printf("%.*lf", precision, d);
}

static u8 dump_buffer(adam_data data)
{
    void (*write_fn)(adam_data) = (!dbl_mode) ? &print_int : &print_dbl;
    
    do {
        write_fn(data);
        putchar('\n');
    } while (--results > 0);

    adam_cleanup(data);

    return 0;
}

static u8 uuid(adam_data data, const char *strlimit)
{
    register u16 limit = a_to_u(optarg, 1, 1000);
    if (!limit) {
        return err("Invalid amount specified. Value must be within range [1, 1000]");
    }

    u8 buf[sizeof(u128)];

    register u16 i = 0;

    register u64 lower, upper;

    do {
        lower = adam_int(data, UINT64);
        upper = adam_int(data, UINT64);
        gen_uuid(upper, lower, &buf[0]);

        // Print the UUID
        printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%"
               "02x%02x%02x%02x",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
            buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

        putchar('\n');
    } while (++i < limit);

    adam_cleanup(data);

    return 0;
}

static u8 assessf(adam_data data, const u64 limit, bool ascii_mode)
{
    double *_buf = aligned_alloc(ADAM_ALIGNMENT, limit * sizeof(double));
    if (_buf == NULL) {
        return 1;
    }

    // We allow higher assess fill values for testing than supported by the API
    // so for the larger user provided values we need to split fills
    if (UNLIKELY(limit > ADAM_FILL_MAX)) {
        adam_dfill(data, _buf, mult, limit - ADAM_FILL_MAX);
        adam_dfill(data, &_buf[limit - ADAM_FILL_MAX], mult, ADAM_FILL_MAX);
    } else {
        adam_dfill(data, _buf, mult, limit);
    }

    if (ascii_mode) {
        register u64 i = 0;
        while (limit - i >= 8) {
            fprintf(stdout, "%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n%.*lf\n",
                precision, _buf[i], precision, _buf[i + 1], precision, _buf[i + 2], precision, _buf[i + 3],
                precision, _buf[i + 4], precision, _buf[i + 5], precision, _buf[i + 6], precision, _buf[i + 7]);
            i += 8;
        }

        while (i < limit) {
            fprintf(stdout, "%.*lf\n", precision, _buf[i++]);
        }
    } else {
        fwrite(_buf, sizeof(double), limit, stdout);
    }

    free(_buf);
    return 0;
}

static u8 stream(adam_data data)
{
    if (dbl_mode) {
        register u64 written = 0;
        double d;

        do {
            d = adam_dbl(data, 1);
            fwrite(&d, sizeof(double), 1, stdout);
            written += sizeof(double) * 8;
        } while (written < __UINT64_MAX__);

        adam_cleanup(data);
    } else {
        adam_stream(data, __UINT64_MAX__, NULL);
    }

    return 0;
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
    ASSESS_PROMPT(file_name, "File name:", " %64s", false, "Please enter a valid file name");

    char c = '0';
    ASSESS_PROMPT(&c, "Output type (1 = ASCII, 2 = BINARY):", " %c", (c != '1' && c != '2'), "Value must be 1 or 2");

    const bool file_mode = (c == '1');
    freopen(file_name, file_mode ? "w+" : "wb+", stdout);

    u32 output_mult;
    register u64 limit;

    if (dbl_mode) {
        ASSESS_PROMPT(&output_mult, "Sequence Size (x 1000):", " %u", (output_mult < 1 || output_mult > DBL_TESTING_LIMIT), "Output multiplier must be between [1, " STRINGIFY(DBL_TESTING_LIMIT) "]");
        limit = TESTING_DBL * output_mult;
        if (assessf(data, limit, file_mode)) {
            return err("Could not allocate enough space for adam -a");
        }
    } else {
        ASSESS_PROMPT(&output_mult, "Sequence Size (x 1MB):", " %u", (output_mult < 1 || output_mult > BITS_TESTING_LIMIT), "Output multiplier must be between [1, " STRINGIFY(BITS_TESTING_LIMIT) "]");

        limit = TESTING_BITS * output_mult;
        if (file_mode) {
            const u64 amount = (limit >> 6) + !!(limit & 63);

            u64 *restrict buffer = aligned_alloc(ADAM_ALIGNMENT, sizeof(u64) * amount);
            if (buffer == NULL) {
                return err("Could not allocate enough space for adam -a");
            }

            adam_fill(data, buffer, UINT64, amount);
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
    if (strlimit != NULL) {
        limit *= a_to_u(strlimit, 1, BITS_TESTING_LIMIT);
    }

    printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n\n", limit);

    adam_examine(limit, data);

    puts("\n\033[1;33mExamination Complete!\033[m\n");

    adam_cleanup(data);

    return 0;
}

static bool rwparams(u64 *seed, u32 *nonce, const char *file_name)
{
    const bool op = (file_name != NULL);

    FILE *file;
    
    if (op) {
        file = fopen(file_name, "rb");
        if (!file) {
            return err("Couldn't read input file!");
        }

        fread(seed, sizeof(u8), ADAM_SEED_SIZE, file);
        fread(nonce, sizeof(u8), ADAM_NONCE_SIZE, file);
    } else {
        char out_name[65];
        while (true) {
            fprintf(stderr, "Output file name (max length 64): \033[1;93m");
            if (!scanf(" %64s", &out_name[0])) {
                err("Please enter a valid file name");
                continue;
            }
            break;
        }

        file = fopen((const char *) out_name, "wb+");
        if (!file) {
            return err("Couldn't create output file!");
        }

        fwrite(seed, sizeof(u8), ADAM_SEED_SIZE, file);
        fwrite(nonce, sizeof(u8), ADAM_NONCE_SIZE, file);

        printf("\033[mSuccessfully wrote input parameters to \033[1;36m%s\033[m\n", out_name);
    }

    fclose(file);

    return op;
}

int main(int argc, char **argv)
{
    // Initialize the non-zero defaults
    results   = 1;
    precision = 15;
    width     = UINT64;

    u64 seed[ADAM_SEED_SIZE / sizeof(u64)] = {0, 0, 0, 0};
    u64 *seed_ptr  = NULL; // &seed[0];

    u32 nonce[ADAM_NONCE_SIZE / sizeof(u32)] = {0, 0, 0};
    u32 *nonce_ptr = NULL; // &nonce;

    adam_data data = adam_setup(seed_ptr, nonce_ptr);
    if (data == NULL) {
        return err("Could not allocate space for adam_data struct! Exiting.");
    }

    register char opt;
    while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (opt) {
        case 'h':
            return help();
        case 'v':
            return !puts("v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH));
        case 'a':
            return assess(data);
        case 'b':
            return stream(data);
        case 'x':
            hex   = true;
            octal = false;
            continue;
        case 'o':
            hex   = false;
            octal = true;
            continue;
        case 'w':
            width = a_to_u(optarg, 8, 32);
            if (UNLIKELY((width & (width - 1)) & !!width)) {
                return err("Alternate width must be either 8, 16, 32");
            }
            width >>= 3;
            continue;
        case 'r':
            results = a_to_u(optarg, 1, 1000);
            if (!results) {
                return err("Invalid number of results specified");
            }
            continue;
        case 'i':
            seed_ptr = &seed[0];
            nonce_ptr = &nonce[0];

            // If we load user provided input params, then
            // we need to call adam_reset() to rebuild state
            if (rwparams(seed_ptr, nonce_ptr, optarg)) {
                adam_reset(data, seed_ptr, nonce_ptr);
            }

            continue;
        case 'u':
            return uuid(data, optarg);
        case 'f':
            dbl_mode = true;
            continue;
        case 'p':
            dbl_mode  = true;
            precision = a_to_u(optarg, 1, 15);
            if (!precision) {
                return err("Floating point precision must be between [1, 15]");
            }
            continue;
        case 'm':
            dbl_mode = true;
            mult     = a_to_u(optarg, 1, __UINT64_MAX__);
            if (UNLIKELY(!mult)) {
                return err("Floating point scaling factor must be between [1, " STRINGIFY(__UINT64_MAX__) "]");
            }
            continue;
        case 'e':
            return examine(data, optarg);
        default:
            return err("Option is invalid or missing required argument");
        }
    }

    dump_buffer(data);

    return 0;
}
