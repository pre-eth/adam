#include "cmd.h"

u8 Command::print_options(const char** opts, const char** help, u8 optc) { 
    u16 SHEIGHT, SWIDTH;
    getScreenDimensions(&SHEIGHT, &SWIDTH);
    u16 center = (SWIDTH >> 1) - 4;
    printf("\e[%uC[OPTIONS]\n", center);
    // subtract 1 because it is half of width for arg (ex. "-d")
    const u16 INDENT =  (SWIDTH / 16); 
    // total indent for help descriptions if they have to go to next line
    const u16 HELP_INDENT = INDENT + INDENT + 2;
    // max length for help description in COL 2 before it needs to wrap
    const u16 HELP_WIDTH = SWIDTH - HELP_INDENT;
    printf("\e[%dC-h\e[%dCGet all available options\n", INDENT, INDENT);
    printf("\e[%dC-v\e[%dCVersion of this software\n", INDENT, INDENT);
    // reuse this val pointer and length int
    int length{0};
    // now the fun part - print the options and their help descriptions,
    for (u8 i{0}; i < optc; ++i) {
        // greater than 6 means we are in in PROGARGS territory, if itemc > 0
        printf("\e[%uC-%s\e[%uC%.*s\n", INDENT, PROGARGS[i], INDENT, HELP_WIDTH, PROGHELP[i]);
        // now subtract length of description from max help width 
        // and see if there's leftovers
        length = strlen(PROGHELP[i]) - HELP_WIDTH;
        // if there's leftovers, wrap and print till there's none
        while (length > 0) {
            PROGHELP[i] += HELP_WIDTH + (*(PROGHELP[i] + HELP_WIDTH) == ' ');
            printf("\e[%uC%.*s\n", HELP_INDENT, HELP_WIDTH, PROGHELP[i]);
            length -= HELP_WIDTH;
        }
    }
    return 0;
};

int Command::run(int argc, char** argv) {
    // reject if ARGC isn't within range
    if (argc < ARG_MIN || (ARG_MAX && argc > ARG_MAX)) 
        return printf("Invalid number of arguments. Must satisfy %d <= n <= %d", ARG_MIN, ARG_MAX);
    return match_opts(argc, argv);
}

u8 Command::match_opts(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
        switch (opt) {
            case 'h':
                return !print_options(PROGARGS, PROGHELP, PROGARGC);
            case 'v':
                return puts(VERSION);
            default:
                if (match_option(opt, optarg))
                    return 1;             
        }
    }
    return 0;
}