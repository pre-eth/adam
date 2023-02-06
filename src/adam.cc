#include "adam.h"

static inline u8 print_binary(u64 num, u64* ctr) {
    static char buffer[64];
    char* b = &buffer[64];
    *b = '\0';

    u8 size = log2(num) + 1;
    while (num) {
        *ctr += !(num & 0x01);
        *--b = (num & 0x01) + '0';
        num >>= 1;
    }

    fwrite(b, 1, size, stdout);
    return size;
}

void ADAM::bit_stream() {
    i64 total{limit};
    u8 i{0};    
    do {
        generate();
        while (i < FRUIT_SIZE && total > 0)
            total -= print_binary(get(i++), &zeroes);
    } while (total > 0);    
}

u8 ADAM::match_option(char opt, const char* val) {
    switch(opt) {
        case 'i':
            // for inverting polarity of undulation
           flip = !flip;
        break;
        case 'n':
            // number of results to print on screen after generating
            results = a_to_i(val, 1, 256);
        break;
        case 'p':
            // precision of values to return 
            precision = (u8) a_to_i(val, 8, 32);
        break;
        case 'd':
            results = 256;
        break;
        case 'b':
            // stream bits
            limit = val != NULL ? a_to_i(val, 64) : INT64_MAX - 1;
            bit_stream();
            printf("\n\nDumped %llu bits (%llu ZEROES, %llu ONES)\n", limit, zeroes, limit - zeroes);
        return 1;
        case 'a':
            // stream 100 samples of 1000000 bits for testing
            limit = 100000000;
            bit_stream();
        return 1;
        default:  
            puts("Invalid option!");
        return 1;
    }
    return 0;
}

u8 ADAM::exec(int argc, char** argv) {
    if (Command::run(argc, argv))
        return 0;

    generate();

    do {
        u64 num = get();
        printf("%llu ", num);
    } while (--results);
    
    putchar('\n');
    
    return 0;
}
