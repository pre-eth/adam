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
FORCE_INLINE static u16 print_chunks(FILE *fptr, char *restrict _bptr, const u64 *restrict _ptr) {  
  register u8 i = 0;
  register u16 ones = 0;

  do {
    ones += POPCNT_4(i + 0) + POPCNT_4(i + 4) 
          + POPCNT_4(i + 8) + POPCNT_4(i + 12);

    PRINT_4(0,   i + 0),
    PRINT_4(256, i + 4),
    PRINT_4(512, i + 8),
    PRINT_4(768, i + 12);    

    fwrite(_bptr, 1, BITBUF_SIZE, fptr);
  } while ((i += 16 - (i == 240)) < BUF_SIZE - 1);

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
    "Assess a binary or ASCII sample of 1000000 bits (1 MB) written to a filename you provide. You can choose a multiplier within [1,1000]",
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

u64 stream_bits(FILE *fptr, u64 *restrict _ptr, const u64 limit) {
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
    ones += print_chunks(fptr, _bptr, _ptr);
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

      fwrite(_bptr, 1, limit, fptr);
      leftovers -= limit;

      if (LIKELY(leftovers > 0)) 
        goto print_leftovers;
  }

  return ones;
}

u8 stream_live(u64 *restrict ptr) {
  /*
    There are 256 numbers per buffer. But we only need 75 to print one
    iteration. So 75 * 3 = 225. 256 - 225 = 31. Thus, for each buffer 31
    numbers will be left over. To avoid waste, these numbers ARE used, but
    only when the index upon exiting the loop is 225, meaning the index 
    started from 0 for this iteration.

    If the 31 numbers are leftover and printed, the index is appropriately
    changed to 31 before the goto statement trigger, starting the loop with
    i = 31. Meaning when we exit the loop this time, our index value is at 
    255! (31 + 75 * 3) - 1. We subtract one to avoid overflow. Since the 
    index is NOT 225 in this case, the statement to print leftovers won't 
    happen, and the next iteration will start clean.

    This pattern is cyclical as you may be able to tell - one round there will
    be leftovers, next round there won't. On and on until the program exits.
  */ 
  const u8 LIVE_ITER = BUF_SIZE - 31;
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

  u8 i = 0;
  adam(ptr);
  live_adam:
    do {
      snprintf(lines[0],  96, "%llu%llu%llu%llu%llu%llu", GET_3(i + 0), GET_3(i + 3));
      snprintf(lines[1],  50, "%llu%llu%llu",             GET_3(i + 6));
      snprintf(lines[2],  39, "%llu%llu%llu",             GET_3(i + 9));
      snprintf(lines[3],  32, "%llu%llu%llu",             GET_3(i + 12));
      snprintf(lines[4],  29, "%llu%llu",                 GET_2(i + 15));
      snprintf(lines[5],  26, "%llu%llu",                 GET_2(i + 17));
      snprintf(lines[6],  16, "%llu%llu",                 GET_2(i + 19));
      snprintf(lines[7],  15, "%llu",                     GET_1(i + 21));
      snprintf(lines[8],  17, "%llu%llu",                 GET_2(i + 22));
      snprintf(lines[9],  17, "%llu%llu",                 GET_2(i + 24));
      snprintf(lines[10], 11, "%llu%llu",                 GET_2(i + 26));
      snprintf(lines[11], 14, "%llu",                     GET_1(i + 28));
      snprintf(lines[12], 18, "%llu%llu",                 GET_2(i + 29));
      snprintf(lines[13], 20, "%llu%llu",                 GET_2(i + 31));
      snprintf(lines[14], 21, "%llu%llu",                 GET_2(i + 33));
      snprintf(lines[15], 7,  "%llu",                     GET_1(i + 35));
      snprintf(lines[16], 18, "%llu%llu",                 GET_2(i + 36));
      snprintf(lines[17], 8,  "%llu",                     GET_1(i + 38));
      snprintf(lines[18], 18, "%llu%llu",                 GET_2(i + 39));
      snprintf(lines[19], 15, "%llu",                     GET_1(i + 41));
      snprintf(lines[20], 17, "%llu%llu",                 GET_2(i + 42));
      snprintf(lines[21], 13, "%llu",                     GET_1(i + 44));
      snprintf(lines[22], 15, "%llu",                     GET_1(i + 45));
      snprintf(lines[23], 8,  "%llu",                     GET_1(i + 46));
      snprintf(lines[24], 16, "%llu",                     GET_1(i + 47));
      snprintf(lines[25], 5,  "%llu",                     GET_1(i + 48));
      snprintf(lines[26], 21, "%llu%llu",                 GET_2(i + 49));
      snprintf(lines[27], 3,  "%llu",                     GET_1(i + 51));
      snprintf(lines[28], 22, "%llu%llu",                 GET_2(i + 52));
      snprintf(lines[29], 5,  "%llu",                     GET_1(i + 54));
      snprintf(lines[30], 19, "%llu%llu",                 GET_2(i + 55));
      snprintf(lines[31], 6,  "%llu",                     GET_1(i + 57));
      snprintf(lines[32], 16, "%llu%llu",                 GET_2(i + 58));
      snprintf(lines[33], 4,  "%llu",                     GET_1(i + 60));
      snprintf(lines[34], 29, "%llu%llu",                 GET_2(i + 61));
      snprintf(lines[35], 31, "%llu%llu",                 GET_2(i + 63));
      snprintf(lines[36], 38, "%llu%llu%llu",             GET_3(i + 65));
      snprintf(lines[37], 3,  "%llu%llu",                 GET_1(i + 68));
      snprintf(lines[38], 26, "%llu%llu",                 GET_2(i + 69));
      snprintf(lines[39], 65, "%llu%llu%llu%llu",         GET_3(i + 71), GET_1(i + 74));
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
    } while ((i += 75 - (i == 181)) < LIVE_ITER);

    const u8 leftovers = (i == 225);

    if (leftovers) {
      snprintf(lines[0],  96, "%llu%llu%llu%llu%llu%llu", GET_3(i + 0), GET_3(i + 3));
      snprintf(lines[1],  50, "%llu%llu%llu",             GET_3(i + 6));
      snprintf(lines[2],  39, "%llu%llu%llu",             GET_3(i + 9));
      snprintf(lines[3],  32, "%llu%llu%llu",             GET_3(i + 12));
      snprintf(lines[4],  29, "%llu%llu",                 GET_2(i + 15));
      snprintf(lines[5],  26, "%llu%llu",                 GET_2(i + 17));
      snprintf(lines[6],  16, "%llu%llu",                 GET_2(i + 19));
      snprintf(lines[7],  15, "%llu",                     GET_1(i + 21));
      snprintf(lines[8],  17, "%llu%llu",                 GET_2(i + 22));
      snprintf(lines[9],  17, "%llu%llu",                 GET_2(i + 24));
      snprintf(lines[10], 11, "%llu%llu",                 GET_2(i + 26));
      snprintf(lines[11], 14, "%llu",                     GET_1(i + 28));
      snprintf(lines[12], 18, "%llu%llu",                 GET_2(i + 29)); 
    }
    i = ((leftovers) << 5) - (leftovers);
    adam(ptr);
    goto live_adam; 

  return 0;
}