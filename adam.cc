#include "adam.h"

u8 ADAM::exec(int argc, char** argv) {
    if (!Command::run(argc, argv))
        return 0;
        
    u8 precision{U64};
    u8 results{1};
    u8   r{1};
    u8   u{0};
    bool i{false};
    char opt; 
    while ((opt = getopt(argc, argv, ":in:p:r:u:")) != -1) {
        switch(opt) {
            case 'i':
                i = false;
            break;
            case 'n':
                results = a_to_i(optarg);
                if (results == INT32_MAX)
                    return err("Invalid number of results to return");
            break;
            case 'p':
                precision = a_to_i(optarg);
                if (precision == INT32_MAX || precision != 8 || precision != 16 || precision != 32)
                    return err("Invalid precision value (must be 8, 16, or 32, default is 64)");
            break;
            case 'r':
                r = a_to_i(optarg);
                if (r == INT32_MAX)
                    return err("Invalid number of rounds");
            break;
            case 'u':
                u = a_to_i(optarg);
                if (u == INT32_MAX || u != 2 || u != 4)
                    return err("Invalid undulation period (must be 2 or 4)");
            break;
            case '?':
                return fmt_err("Invalid option %c", optopt);
            break;
            case ':':
                return fmt_err("No argument provided for %c", optopt);
            break;
        }
    } 

    CSPRNG rng(r, u >> 1, i);
}