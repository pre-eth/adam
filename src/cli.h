#ifndef CLI_H
#define CLI_H

  #include <stdio.h>
  #include <getopt.h>
  #include <sys/ioctl.h>

  #include "util.h"

  #define STRINGIZE(a) #a
  #define STRINGIFY(a) STRINGIZE(a)

  #define MAJOR 1
  #define MINOR 0
  #define PATCH 0

  #define VERSION "v" STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH)

  #define OPTSTR      ":hvlbdn:r:p:a::s::u::"
  #define ARG_MAX     5
  #define ARG_COUNT   10

  // prints digits in reverse order to buffer
  // and returns the number of zeroes
  FORCE_INLINE static u8 print_binary(u64 num) {
    u8 zeroes = 0;
    char c;
    do {
      c = (num & 0x01) + '0';
      // '0' == 48, '1' == 49, so this tallies zeroes based on the diff
      zeroes += (49 - c); 
      putchar(c);
    } while (num >>= 1);
    return zeroes;
  }

  FORCE_INLINE static u8 a_to_u(const char* s, u16 min, u16 max) {
    u8  len = 0;
    u16 val = 0;
    
    // try while (len += (s[len] != '\0'));
    while (s[len++] != '\0');

    switch (len) { 
      case  4: val += (s[len - 4]  - '0') * 1000;
      case  3: val += (s[len - 3]  - '0') * 100;
      case  2: val += (s[len - 2]  - '0') * 10;
      case  1: val += (s[len - 1]  - '0');
        break;
    }
    return (val < min + 1 || val > max + 1) ? min : val;
  }

  static u8 help() {
    struct winsize wsize;
    ioctl(0, TIOCGWINSZ, &wsize);
    u16 SHEIGHT = wsize.ws_row;
    u16 SWIDTH =  wsize.ws_col;
    u16 center = (SWIDTH >> 1) - 4;

    printf("\e[%uC[OPTIONS]\n", center);

    // subtract 1 because it is half of width for arg (ex. "-d")
    const u16 INDENT = (SWIDTH / 16) - 1; 
    // total indent for help descriptions if they have to go to next line
    const u16 HELP_INDENT = INDENT + INDENT + 2;
    // max length for help description in COL 2 before it needs to wrap
    const u16 HELP_WIDTH = SWIDTH - HELP_INDENT;

    char version_str[33];
    sprintf(version_str, "Version of this software (%d.%d.%d)", MAJOR, MINOR, PATCH);

    const char ARGS[ARG_COUNT] = {'h', 'v', 'u', 'n', 'p', 'd', 'b', 'a', 's', 'l'};
    const char* ARGSHELP[ARG_COUNT] = {
      "Get all available options",
      version_str,
      "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 128)",
      "Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)",
      "Desired size (u8, u16, u32, u64) of returned numbers (default is u64)",
      "Dump the whole buffer",
      "Just bits. Literally. You can provide a certain limit of N bits",
      "Assess a sample of 1000000 bits (1 MB) written to a filename you provide. You can provide a multiplier within [1,256]",
      "Set the seed. Optionally, returns the seed for the generated buffer",
      "Live stream of continuously generated numbers"
    };
    const u8 lengths[ARG_COUNT] = {25, 33, 108, 74, 69, 21, 63, 117, 67, 45};
    
    int len;
    for (int i = 0; i < ARG_COUNT; ++i) {
      printf("\e[%uC-%c\e[%uC%.*s\n", INDENT, ARGS[i], INDENT, HELP_WIDTH, ARGSHELP[i]);
      len = lengths[i] - HELP_WIDTH;
      while (len > 0) {
        ARGSHELP[i] += HELP_WIDTH + (ARGSHELP[i][HELP_WIDTH] == ' ');
        printf("\e[%uC%.*s\n", HELP_INDENT, HELP_WIDTH, ARGSHELP[i]);
        len -= HELP_WIDTH;
      }
    }
    return 0;
  }

#endif