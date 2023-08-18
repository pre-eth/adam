#include <sys/ioctl.h>

#include "adam.h"
#include "cli.h"

/*
  To make writes more efficient, rather than writing one
  number at a time, 16 numbers are parsed together and then
  written to stdout with 1 fwrite call.
*/  
static char bitbuffer[BITBUF_SIZE] ALIGN(64);

// prints digits in reverse order to buffer
FORCE_INLINE static void print_binary(char *restrict buf, u64 num) {
  char *bend = buf + 64;
  *bend = '\0';
  do *--bend = (num & 0x01) + '0';
  while (num >>= 1);

  // If we reach the end of a number before printing 64 digits,
  // fill in the rest with zeroes
  while (bend - buf > 0)
    *--bend = '0';
}

// prints all bits in a buffer as chunks of 1024 bits
FORCE_INLINE static u16 print_chunks(char *restrict _bptr, const u64 *restrict _ptr) {  
  register u8 i = 0;
  register u16 ones = 0;

  do {
    ones += POPCNT_4(i + 0) + POPCNT_4(i + 4) 
          + POPCNT_4(i + 8) + POPCNT_4(i + 12);

    PRINT_4(0,   i + 0),
    PRINT_4(256, i + 4),
    PRINT_4(512, i + 8),
    PRINT_4(768, i + 12);    

    fwrite(_bptr, 1, BITBUF_SIZE, stdout);
  } while ((i += 16 - (i == 240)) < BUF_SIZE - 1);

  return ones;
}

u64 stream_bits(u64 *restrict _ptr, const u64 limit) {
  register u64 ones = 0;

  /*
    Split limit based on how many calls (if needed)
    we make to print_chunks, which prints the bits of 
    an entire buffer (aka the SEQ_SIZE)
  */ 
  register short rate = limit >> 14;
  register short leftovers = limit & (SEQ_SIZE - 1);

  char *restrict _bptr = &bitbuffer[0];

  while (rate > 0) {
    adam(_ptr);
    ones += print_chunks(_bptr, _ptr);
    --rate;
  } 

  /*
    Since there are SEQ_SIZE (16384) bits in every 
    buffer, adam_bits is designed to print up to SEQ_SIZE
    bits per call, so any leftovers must be processed
    independently. 
    
    Most users probably won't enter powers of 2, especially 
    if assessing bits, so this branch has been marked as LIKELY.
  */
  if (LIKELY(leftovers > 0)) {
    register short l;
    register u16 limit;
    register u64 num;

    print_leftovers:
      limit = (leftovers < BITBUF_SIZE) ? leftovers : BITBUF_SIZE;

      l = 0;
      adam(_ptr);
      do {
        num = *(_ptr + (l >> 6));
        ones += POPCNT(num);
        print_binary(_bptr + l, num);
      } while ((l += 64) < limit);

      fwrite(_bptr, 1, limit, stdout);
      leftovers -= limit;

      if (LIKELY(leftovers > 0)) 
        goto print_leftovers;
  }

  return ones;
}

// Only supports values up to 4 digits
u16 a_to_u(const char *s, const u16 min, const u16 max) {
  register u8  len = 0;
  register u16 val = 0;
  
  for(; s[len] != '\0'; ++len);

  switch (len) { 
    case 4: val += (s[len - 4]  - '0') * 1000;
    case 3: val += (s[len - 3]  - '0') * 100;
    case 2: val += (s[len - 2]  - '0') * 10;
    case 1: val += (s[len - 1]  - '0');
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

  const char ARGS[ARG_COUNT] = {'h', 'v', 'u', 'n', 'p', 'd', 'b', 'a', 'l'};
  const char *ARGSHELP[ARG_COUNT] = {
    "Get all available options",
    VERSION_HELP,
    "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 128)",
    "Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)",
    "Desired size (u8, u16, u32, u64) of returned numbers (default is u64)",
    "Dump the whole buffer",
    "Just bits. Literally",
    "Assess a binary or ASCII sample of 1000000 bits (1 MB) written to a filename you provide. You can choose a multiplier within [1,256]",
    "Live stream of continuously generated numbers"
  };
  const u8 lengths[ARG_COUNT] = {25, 33, 108, 74, 69, 21, 20, 132, 45};
  
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
