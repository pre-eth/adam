#include "adam.h"

u8 print_binary(u64 num);

#define STREAM64 print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get())+print_binary(get()); \

u8 print_binary(u64 num) {
    u8 size = log2(num) + 1;
    char buffer[size];
    char* b = &buffer[size];
    *b = '\0';

    do
        *--b = (num & 0x01) + '0';
    while (num >>= 1);

    printf(b);

    return size;
}

u8 ADAM::matchOption(char opt, const char* val) {
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
            if (precision != 8 || precision != 16 || precision != 32)
                return puts("Invalid precision value (must be 8, 16, or 32, default is 64)");
        break;
        case 'r':
            // how many rounds to mangle buffer, at least 7
            rounds = (u8) a_to_i(val, 7, 20);
        break;
        case 'u':
            // period of the undulation wave
            cycle = (u8) a_to_i(val, 0, 2);
        break;
        case 'b':
            // stream bits
            bit_stream = true;
            results = 0;
        break;
        case 'a':
            // stream 100 samples of 1000000 bits for testing
            alimit = 100000000;
            bit_stream = true;
            results = 0;
        break;
        default:  return 1;
    }
    return 0;
}

u8 ADAM::exec(int argc, char** argv) {
    if (!Command::run(argc, argv))
        return 0;

    u64 i{0};

    if (bit_stream) 
        do {
            i += STREAM64
            i += STREAM64
            i += STREAM64
            i += STREAM64
            generate();
            i += STREAM64
            i += STREAM64
            i += STREAM64
            i += STREAM64
            generate();
        } while (i < alimit);

    do 
        printf("%llu ", get());
    while (++i < results);

    return 0;
}