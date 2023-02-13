#include "adam.h"

static inline void print_binary(char* b, u64 num, u64* ctr, u8 sz) {
    // increments counter by whatever binary digit is next
    // prints digits in reverse order to buffer
    do {
        *ctr += (49 - (*--b = (num & 0x01) + '0')); 
        num >>= 1;
    } while (--sz);
}

void ADAM::live_stream() {
    const char* ADAM_ASCII = {
        "%s\e[38;2;173;58;0m/\e[38;2;255;107;33m@@@@@@\e[38;2;173;58;0m\\\e[0m"
        "%s\e[38;2;173;58;0m/(\e[38;2;255;107;33m@@\e[38;2;173;58;0m((((((((((((((((\e[38;2;255;107;33m@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m((((((((((((((((((((((((\e[38;2;255;107;33m@@@\e[0m"
        "%s\e[38;2;173;58;0m(((((((((((((((((((((((((((((\e[38;2;255;107;33m#@##@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((((((((((((\e[38;2;255;107;33m@#####&\e[38;2;173;58;0m(((((((((((((((\e[38;2;255;107;33m@@@@\e[0m"
        "%s\e[38;2;173;58;0m((((((((((((((\e[38;2;255;107;33m#@@\e[38;2;173;58;0m((((\e[38;2;255;107;33m@@@@@\e[38;2;173;58;0m((((((((((((((\e[38;2;255;107;33m@@\e[0m"
        "%s\e[38;2;173;58;0m((((((((\e[38;2;255;107;33m@@\e[38;2;173;58;0m(((((((((\e[38;2;255;107;33m@\e[38;2;173;58;0m((((((((((((\e[38;2;255;107;33m@&#@\e[38;2;173;58;0m(((((((((((\e[38;2;255;107;33m#@@\e[0m"
        "%s\e[38;2;173;58;0m(\e[38;2;255;107;33m@@@\e[38;2;173;58;0m(((\e[38;2;255;107;33m##\e[38;2;173;58;0m((((((((\e[38;2;255;107;33m@@\e[38;2;173;58;0m((((((((((((((\e[38;2;255;107;33m##@##\e[38;2;173;58;0m(((((((((\e[38;2;255;107;33m#@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((((\e[38;2;255;107;33m##\e[38;2;173;58;0m(((\e[38;2;255;107;33m@#####\e[38;2;173;58;0m(((((\e[38;2;255;107;33m######\e[38;2;173;58;0m((((\e[38;2;255;107;33m###@\e[38;2;173;58;0m(((((((((\e[38;2;255;107;33m@@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m((((((\e[38;2;255;107;33m@&#########\e[38;2;173;58;0m((((\e[38;2;255;107;33m###@@##\e[38;2;173;58;0m(((\e[38;2;255;107;33m@###@\e[38;2;173;58;0m((((((((\e[38;2;255;107;33m#@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((((((((\e[38;2;255;107;33m#&@@@#########@\e[38;2;173;58;0m(((((\e[38;2;255;107;33m@@\e[38;2;173;58;0m((((\e[38;2;255;107;33m#####@\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m#@@\e[0m"
        "%s\e[38;2;255;107;33m@@@@\e[38;2;173;58;0m((((\e[38;2;255;107;33m&#\e[38;2;173;58;0m(\e[38;2;255;107;33m################&@@@####@###@\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m@&&\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m((((\e[38;2;255;107;33m#####\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m###########@&####\e[38;2;173;58;0m((((\e[38;2;255;107;33m@##@&\e[0m"
        "%s\e[38;2;173;58;0m(\e[38;2;255;107;33m@\e[38;2;173;58;0m((((\e[38;2;255;107;33m@####@######@@###\e[38;2;173;58;0m(((\e[38;2;255;107;33m@######@\e[38;2;173;58;0m((((((\e[38;2;255;107;33m@#\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m((((((\e[38;2;255;107;33m##############@@##\e[38;2;173;58;0m((((((((((((((((((\e[38;2;255;107;33m@\e[0m"
        "%s\e[38;2;255;107;33m@\e[0m"
        "%s\e[38;2;173;58;0m/\e[38;2;255;107;33m@\e[38;2;173;58;0m((\e[38;2;255;107;33m@###\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m###@@@@@@@@@@@@@&@##@\e[38;2;173;58;0m((\e[38;2;255;107;33m@&\e[0m"
        "%s\e[38;2;255;107;33m#@\e[0m"
        "%s\e[38;2;173;58;0m(((\e[38;2;255;107;33m################@@@#########\e[0m"
        "%s\e[38;2;173;58;0m/\e[38;2;255;107;33m#\e[38;2;173;58;0m)\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((\e[38;2;255;107;33m####\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m####&###\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m##@\e[0m"
        "%s\e[38;2;255;107;33m@&@\e[0m"
        "%s\e[38;2;255;107;33m.@\e[38;2;173;58;0m((((\e[38;2;255;107;33m##############@#@######@@@@@#####\e[0m"
        "%s\e[38;2;255;107;33m@&@\e[0m"
        "%s\e[38;2;255;107;33m@@@@\e[38;2;173;58;0m((\e[38;2;255;107;33m@####@\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m#####@@@#\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m&#@@\e[38;2;173;58;0m\\\e[0m"
        "%s\e[38;2;255;107;33m@@#@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m((\e[38;2;255;107;33m@#####@@#######@#################@\e[0m"
        "%s\e[38;2;255;107;33m@@##@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((\e[38;2;255;107;33m@#####\e[38;2;173;58;0m(((((((\e[38;2;255;107;33m######@###@@@@@\e[0m"
        "%s\e[38;2;255;107;33m@@#@##&#\e[0m"
        "%s\e[38;2;255;107;33m@@\e[38;2;173;58;0m((\e[38;2;255;107;33m@@\e[38;2;173;58;0m((((\e[38;2;255;107;33m@##########&\e[38;2;173;58;0m((((\e[38;2;255;107;33m###@\e[0m"
        "%s\e[38;2;255;107;33m@&&##@\e[38;2;173;58;0m))\e[38;2;255;107;33m@#@@\e[0m"
        "%s\e[38;2;255;107;33m@@@\e[0m"
        "%s\e[38;2;255;107;33m@\e[38;2;173;58;0m(((((\e[38;2;255;107;33m&#######\e[38;2;173;58;0m((\e[38;2;255;107;33m#@############&\e[38;2;173;58;0m)))))))\e[0m"
        "%s\e[38;2;255;107;33m@#\e[38;2;173;58;0m(((((\e[38;2;255;107;33m#@##############@\e[38;2;173;58;0m((((((\e[38;2;255;107;33m#@#@\e[0m"
        "%s\e[38;2;255;107;33m@@@@@@&@#\e[38;2;173;58;0m((((\e[38;2;255;107;33m##@@@@@#\e[38;2;173;58;0m((((((\e[38;2;255;107;33m#&#@#&\e[0m"
        "%s\e[38;2;255;107;33m####\e[0m"
        "%s\e[38;2;255;107;33m#&#\e[38;2;173;58;0m(\e[38;2;255;107;33m@@@@@\e[0m%s%s"
    };

    // set window dimensions for live stream
    printf("\e[8;29;64t");
    // no buffering
    setbuf(stdout, NULL);
    
    char lines[40][100];

    do
    {
        generate();
        size = FRUIT_SIZE;
        seed = trng64();
        snprintf(lines[0],  96, "%llu%llu%llu%llu%llu%llu", get(), get(), get(), get(), get(), get());
        snprintf(lines[1],  50, "%llu%llu%llu",             get(), get(), get());
        snprintf(lines[2],  39, "%llu%llu%llu",             get(), get(), get());
        snprintf(lines[3],  32, "%llu%llu%llu",             get(), get(), get());
        snprintf(lines[4],  29, "%llu%llu",                 get(), get());
        snprintf(lines[5],  26, "%llu%llu",                 get(), get());
        snprintf(lines[6],  16, "%llu%llu",                 get(), get());
        snprintf(lines[7],  15, "%llu",                     get());
        snprintf(lines[8],  17, "%llu%llu",                 get(), get());
        snprintf(lines[9],  17, "%llu%llu",                 get(), get());
        snprintf(lines[10], 11, "%llu%llu",                 get(), get());
        snprintf(lines[11], 14, "%llu",                     get());
        snprintf(lines[12], 18, "%llu%llu",                 get(), get());
        snprintf(lines[13], 20, "%llu%llu",                 get(), get());
        snprintf(lines[14], 21, "%llu%llu",                 get(), get());
        snprintf(lines[15], 7,  "%llu",                     get());
        snprintf(lines[16], 18, "%llu%llu",                 get(), get());
        snprintf(lines[17], 8,  "%llu",                     get());
        snprintf(lines[18], 18, "%llu%llu",                 get(), get());
        snprintf(lines[19], 15, "%llu",                     get());
        snprintf(lines[20], 17, "%llu%llu",                 get(), get());
        snprintf(lines[21], 13, "%llu",                     get());
        snprintf(lines[22], 15, "%llu",                     get());
        snprintf(lines[23], 8,  "%llu",                     get());
        snprintf(lines[24], 16, "%llu",                     get());
        snprintf(lines[25], 5,  "%llu",                     get());
        snprintf(lines[26], 21, "%llu%llu",                 get(), get());
        snprintf(lines[27], 3,  "%llu",                     get());
        snprintf(lines[28], 22, "%llu%llu",                 get(), get());
        snprintf(lines[29], 5,  "%llu",                     get());
        snprintf(lines[30], 19, "%llu%llu",                 get(), get());
        snprintf(lines[31], 6,  "%llu",                     get());
        snprintf(lines[32], 16, "%llu%llu",                 get(), get());
        snprintf(lines[33], 4,  "%llu",                     get());
        snprintf(lines[34], 29, "%llu%llu",                 get(), get());
        snprintf(lines[35], 31, "%llu%llu",                 get(), get());
        snprintf(lines[36], 38, "%llu%llu%llu",             get(), get(), get());
        snprintf(lines[37], 3,  "%llu%llu",                 get());
        snprintf(lines[38], 26, "%llu%llu",                 get(), get());
        snprintf(lines[39], 65, "%llu%llu%llu%llu",         get(), get(), get(), get());
        printf(ADAM_ASCII,
               lines[0],  lines[1],  lines[2],  lines[3],
               lines[4],  lines[5],  lines[6],  lines[7],
               lines[8],  lines[9],  lines[10], lines[11],
               lines[12], lines[13], lines[14], lines[15],
               lines[16], lines[17], lines[18], lines[19],
               lines[20], lines[21], lines[22], lines[23],
               lines[24], lines[25], lines[26], lines[27],
               lines[28], lines[29], lines[30], lines[31],
               lines[32], lines[33], lines[34], lines[35],
               lines[36], lines[37], lines[38], lines[39]
        );
        sleep(1);
        fwrite("\e[2J\r", 1, 5, stdout);
    } while (1);
}

void ADAM::bit_stream() {
    u64 total{limit};
    char buffer[64];
    buffer[64] = '\0';
    u8 buf_size;
    u64 num;

    do {
        // generate new buffer of 256 64-bit ints
        generate();
        // reset size
        size = FRUIT_SIZE;
        // get a new seed for next iteration
        seed = trng64();
        do {
            num = get();
            // get a number's length in binary form
            buf_size = log2(num) + 1;
            // set the buf_size to the total if it's less than buf_size
            // this is to avoid buffer overruns
            // then subtract this number from the running total
            total -= buf_size -= (buf_size - total) * (total < 65);
            print_binary(&buffer[64], num, &zeroes, buf_size);
            // do 64 - buf_size to get real starting index for this number
            fwrite(&buffer[64-buf_size], 1, buf_size, fp);
            // size here refers to the size of the rng buffer, which
            // gets decremented for every call to get()
        } while (total && size); 
    } while (total);
}

u8 ADAM::match_option(char opt, const char *val) {
    switch (opt)
    {
        case 's':
            if (val != NULL)
                seed = a_to_u(val, 1);
            else
                printf("SEED: %llu\n\n", seed);
        break;
        case 'u':
            // for inverting polarity of undulation
            undulation = (u8) a_to_u(val, 3, 8);
        break;
        case 'n':
            // number of results to print on screen after generating
            results = (u8) (a_to_u(val, 1, 256) - 1);
        break;
        case 'p':
            // precision of values to return
            precision = (u8) a_to_u(val, 8, 32);
            if (precision != 8 && precision != 16 && precision != 32) 
                return puts("ERROR! Precision must be 8, 16, or 32");
        break;
        case 'd':
            results = 255;
        break;
        case 'b':
            // stream bits
            limit = (val != NULL) ? a_to_u(val, 64) : UINT64_MAX - 1;
            bit_stream();
            printf("\n\nDumped %llu bits (%llu ZEROES, %llu ONES)\n", limit, zeroes, limit - zeroes);
        return 1;
        case 'a':
            // stream 100 samples of 1000000 bits for testing
            fp = fopen(val, "w+");
            limit = 100000000;
            bit_stream();
            fclose(fp);
        return 1;
        case 'l':
            live_stream();
        return 1;
        default:
            puts("Option is invalid or missing required argument.");
        return 1;
    }
    return 0;
}

u8 ADAM::exec(int argc, char **argv) {
    if (Command::run(argc, argv))
        return 0;

    generate();

    do {
        u64 num = get();
        printf("%llu ", num);
    } while (results-- > 0);

    putchar('\n');

    return 0;
}
