#include "adam.h"

u8 ADAM::matchOption(char opt, const char* val) {
    switch(opt) {
        case 'i':
            invert = true;
        break;
        case 'n':
            results = a_to_i(val, 1, 256);
            if (results == INT32_MAX)
                return err("Invalid number of results to return (0-256)");
        break;
        case 'p':
            precision = a_to_i(val, 8, 32);
            if (precision == INT32_MAX || precision != 8 || precision != 16 || precision != 32)
                return err("Invalid precision value (must be 8, 16, or 32, default is 64)");
        break;
        case 'r':
            rounds = a_to_i(val);
            if (rounds == INT32_MAX)
                return err("Invalid number of rounds");
        break;
        case 'u':
            undulation_cycle = a_to_i(val, 2, 4);
            if (undulation_cycle == INT32_MAX || undulation_cycle != 2 || undulation_cycle != 4)
                return err("Invalid undulation period (must be 2 or 4)");
        break;
        default:  return fmt_err("Invalid option \"%c\"", opt);
    }
    return 0;
}

u8 ADAM::exec(int argc, char** argv) {
    if (Command::run(argc, argv))
        return 1;

    puts("RUNNING ADAM...");
    CSPRNG rng(rounds, undulation_cycle >> 1, invert);
}