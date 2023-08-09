#include <sys/ioctl.h>

#include "adam.h"
#include "cli.h"

/*
  To make writes more efficient, rather than writing one
  number at a time, 8 numbers are parsed together and then
  written to stdout with 1 fwrite call.
*/  
static char bitbuffer[BITBUF_SIZE] ALIGN(64);

// prints digits in reverse order to buffer
FORCE_INLINE static void print_binary(char *restrict buf, u64 num) {
  do *--buf = (num & 0x01) + '0';
  while (num >>= 1);
}

// prints minimum of 512 bits
FORCE_INLINE static u16 bit_chunk(const char *restrict _bptr, const u64 *restrict _ptr, const u64 limit) {  
  register u8 i = 0;
  register u16 ones = 0;

  do {
    ones += POPCNT(_ptr[i + 0]) + POPCNT(_ptr[i + 1]) + POPCNT(_ptr[i + 2]) + POPCNT(_ptr[i + 3]) 
          + POPCNT(_ptr[i + 4]) + POPCNT(_ptr[i + 5]) + POPCNT(_ptr[i + 6]) + POPCNT(_ptr[i + 7]);

    print_binary(_bptr + 64,  _ptr[i + 0]);
    print_binary(_bptr + 128, _ptr[i + 1]);
    print_binary(_bptr + 192, _ptr[i + 2]);
    print_binary(_bptr + 256, _ptr[i + 3]);
    print_binary(_bptr + 320, _ptr[i + 4]);
    print_binary(_bptr + 384, _ptr[i + 5]);
    print_binary(_bptr + 448, _ptr[i + 6]);
    print_binary(_bptr + 512, _ptr[i + 7]);

    fwrite(_bptr, 1, BITBUF_SIZE, stdout);
  } while ((i += 8) < BUF_SIZE);

  return ones;
}

u16 stream_bits(const u32* restrict _ptr, u32 limit) {
  // Max bits in a single buffer is 2048 * 8 = 16384
  u16 zeroes = 0;

  

  return zeroes;
}

// Only supports values up to 999,999
u32 a_to_u(const char* s, const u32 min, const u32 max) {
  register u8  len = 0;
  register u32 val = 0;
  
  while (s[len++] != '\0');

  switch (len) { 
    case  6: val += (s[len - 2]  - '0') * 100000;
    case  5: val += (s[len - 1]  - '0') * 10000;
    case  4: val += (s[len - 4]  - '0') * 1000;
    case  3: val += (s[len - 3]  - '0') * 100;
    case  2: val += (s[len - 2]  - '0') * 10;
    case  1: val += (s[len - 1]  - '0');
      break;
  }
  return (val < min + 1 || val > max + 1) ? min : val;
}

u8 help() {
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
  
  short len;
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
