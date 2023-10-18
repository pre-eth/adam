#include <sys/ioctl.h>    // for pretty help printing
#include <unistd.h>       // for sleep()

#include "adam.h"
#include "cli.h"
#include "ent.h"

FORCE_INLINE static void print_summary(const u16 swidth, const u16 indent) {
  #define SUMM_PIECES     7

  const char* pieces[SUMM_PIECES] = {
    "\033[1madam \033[m[-h|-v|-l|-b|-x|-o]",
    "[-w \033[1mwidth\033[m]",
    "[-a \033[1mmultiplier\033[m]",
    "[-r \033[3mresults?\033[m]",
    "[-s \033[3mseed?\033[m]",
    "[-n \033[3mnonce?\033[m]",
    "[-u \033[3mamount?\033[m]"
  };

  const u8 sizes[SUMM_PIECES + 1] = {24, 10, 15, 13, 10, 11, 12, 0};

  u8 i = 0, running_length = indent;

  printf("\n\033[%uC", indent);

  for (; i < SUMM_PIECES; ++i) {
    printf("%s ", pieces[i]);
    // add one for space
    running_length += sizes[i] + 1;
    if (running_length + sizes[i + 1] + 1 >= swidth) {
      // add 5 for "adam " to create hanging indent for succeeding lines
      printf("\n\033[%uC", indent + 5);
      running_length = 0;
    }
  }

  putchar('\n'), putchar('\n');
}

u8 help() {
  struct winsize wsize;
  ioctl(0, TIOCGWINSZ, &wsize);
  register u8 SWIDTH =  wsize.ws_col;
  
  const u8 CENTER = (SWIDTH >> 1) - 4;
  // subtract 1 because it is half of width for arg (ex. "-d")
  const u8 INDENT = (SWIDTH >> 4) - 1; 
  // total indent for help descriptions if they have to go to next line
  const u8 HELP_INDENT = INDENT + INDENT + 2;
  // max length for help description in COL 2 before it needs to wrap
  const u8 HELP_WIDTH = SWIDTH - HELP_INDENT;

  print_summary(SWIDTH, INDENT);

  printf("\033[%uC[OPTIONS]\n", CENTER);

  const char ARGS[ARG_COUNT] = {'h', 'v', 's', 'n', 'u', 'r', 'w', 'b', 'a', 'l', 'x', 'o'};
  const char *ARGSHELP[ARG_COUNT] = {
    "Get command summary and all available options",
    VERSION_HELP,
    "Get the seed for the generated buffer (no parameter) or provide your own. Seeds are reusable but should be kept secret.",
    "Get the nonce for the generated buffer (no parameter) or provide your own. Nonces should ALWAYS be unique and secret.",
    "Generate a universally unique identifier (UUID). Optionally specify a number of UUID's to generate (max 128)",
    "Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8). No argument dumps entire buffer",
    "Desired size (u8, u16, u32, u64) of returned numbers (default width is u64)",
    "Just bits...literally",
    "Assess a binary or ASCII sample of 1000000 bits (1 Mb) written to a filename you provide. You can choose a multiplier within [1, 8000]",
    "Live stream of continuously generated numbers",
    "Print numbers in hexadecimal format with leading prefix",
    "Print numbers in octal format with leading prefix"
  };
  const u8 lengths[ARG_COUNT] = {25, 32, 119, 117, 108, 107, 75, 21, 133, 45, 55, 49};
  
  register short len;
  for (u8 i = 0; i < ARG_COUNT; ++i) {
    printf("\033[%uC\033[1;33m-%c\033[m\033[%uC%.*s\n", INDENT, ARGS[i], INDENT, HELP_WIDTH, ARGSHELP[i]);
    len = lengths[i] - HELP_WIDTH;
    while (len > 0) {
      ARGSHELP[i] += HELP_WIDTH + (ARGSHELP[i][HELP_WIDTH] == ' ');
      printf("\033[%uC%.*s\n", HELP_INDENT, HELP_WIDTH, ARGSHELP[i]);
      len -= HELP_WIDTH;
    }
  }
  return 0;
}

u64 a_to_u(const char *s, const u64 min, const u64 max) {
  if (UNLIKELY(s[0] == '-')) return min;

  register u8  len = 0;
  register u64 val = 0;
  
  for(; s[len] != '\0'; ++len) {
    if (UNLIKELY(s[len] < '0' || s[len] > '9'))
      return 0;
  };

  switch (len) { 
    case 20:    val += 10000000000000000000LU;
    case 19:    val += (s[len-19] - '0') * 1000000000000000000LU;
    case 18:    val += (s[len-18] - '0') * 100000000000000000LU;
    case 17:    val += (s[len-17] - '0') * 10000000000000000LU;
    case 16:    val += (s[len-16] - '0') * 1000000000000000LU;
    case 15:    val += (s[len-15] - '0') * 100000000000000LU;
    case 14:    val += (s[len-14] - '0') * 10000000000000LU;
    case 13:    val += (s[len-13] - '0') * 1000000000000LU;
    case 12:    val += (s[len-12] - '0') * 100000000000LU;
    case 11:    val += (s[len-11] - '0') * 10000000000LU;
    case 10:    val += (s[len-10] - '0') * 1000000000LU;
    case  9:    val += (s[len- 9] - '0') * 100000000LU;
    case  8:    val += (s[len- 8] - '0') * 10000000LU;
    case  7:    val += (s[len- 7] - '0') * 1000000LU;
    case  6:    val += (s[len- 6] - '0') * 100000LU;
    case  5:    val += (s[len- 5] - '0') * 10000LU;
    case  4:    val += (s[len- 4] - '0') * 1000LU;
    case  3:    val += (s[len- 3] - '0') * 100LU;
    case  2:    val += (s[len- 2] - '0') * 10LU;
    case  1:    val += (s[len- 1] - '0');
      break;
  }
  return (val >= min || val < max - 1) ? val : min;
}

u8 uuid(u8 limit, rng_data *data) {
  adam(data);

  u8 buf[16];
  u128 tmp;
  u64 *restrict _ptr = &data->buffer[0];
  print_uuid:
    // Fill buf with 16 random bytes
    tmp = ((u128)_ptr[0] << 64) | _ptr[1];
    __builtin_memcpy(buf, &tmp, sizeof(u128));

    // CODE AND COMMENT PULLED FROM CRYPTOSYS (https://www.cryptosys.net/pki/Uuid.c.html)
    //
    // Adjust certain bits according to RFC 4122 section 4.4.
    // This just means do the following
    // (a) set the high nibble of the 7th byte equal to 4 and
    // (b) set the two most significant bits of the 9th byte to 10'B,
    //     so the high nibble will be one of {8,9,A,B}.
    buf[6] = 0x40 | (buf[6] & 0xf);
    buf[8] = 0x80 | (buf[8] & 0x3f);

    // Print the UUID
    printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            buf[0], buf[1], buf[2],  buf[3],  buf[4],  buf[5],  buf[6],  buf[7],
            buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

    if (--limit > 0) {
      printf(",\n");
      _ptr += 2;
      goto print_uuid;
    }

  return putchar('\n');
}

/*
  To make writes more efficient, rather than writing one
  number at a time, 16 numbers are parsed together and then
  written to stdout with 1 fwrite call.
*/ 
static char bitbuffer[BITBUF_SIZE] ALIGN(SIMD_LEN);

#ifdef __AARCH64_SIMD__
  static void print_binary(char *restrict _bptr, u64 num) {
    // Copying bit masks to high and low halves
    // 72624976668147840 == {128, 64, 32, 16, 8, 4, 2, 1}
    const uint8x16_t masks = SIMD_COMBINE8(SIMD_CREATE8(72624976668147840), SIMD_CREATE8(72624976668147840));
    const uint8x16_t zero = SIMD_SET8('0');

    // Repeat all 8 bytes 8 times each, to fill up r1 with 64 bytes total
    // Then we bitwise AND with masks to turn each byte into a 1 or 0
    reg8q4 r1;
    r1.val[0] = SIMD_COMBINE8(SIMD_MOV8((num >> 56) & 0xFF), SIMD_MOV8((num >> 48) & 0xFF));
    r1.val[1] = SIMD_COMBINE8(SIMD_MOV8((num >> 40) & 0xFF), SIMD_MOV8((num >> 32) & 0xFF));
    r1.val[2] = SIMD_COMBINE8(SIMD_MOV8((num >> 24) & 0xFF), SIMD_MOV8((num >> 16) & 0xFF));
    r1.val[3] = SIMD_COMBINE8(SIMD_MOV8((num >> 8) & 0xFF), SIMD_MOV8(num & 0xFF));

    SIMD_AND4Q8(r1, r1, masks);
    SIMD_CMP4QEQ8(r1, r1, masks);
    SIMD_AND4Q8(r1, r1, SIMD_SET8(1));

    // '0' = 48, '1' = 49. This is how we print the number
    SIMD_ADD4Q8(r1, r1, zero);

    SIMD_STORE8x4(_bptr, r1);
  }
#else
  static void print_binary(char *restrict _bptr, u64 num) {
    #define BYTE_REPEAT(n) bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n]
    #define BYTE_MASKS     128, 64, 32, 16, 8, 4, 2, 1

    u8 bytes[sizeof(u64)];
    
    MEMCPY(bytes, &num, sizeof(u64));

    const reg zeroes = SIMD_SET8('0');
    const reg masks = SIMD_SETR8(BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
                               #ifdef __AVX512F__
                               , BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
                               #endif    
                               );

    reg r1 = SIMD_SETR8(BYTE_REPEAT(7), BYTE_REPEAT(6), BYTE_REPEAT(5), BYTE_REPEAT(4)
                      #ifdef __AVX512F__
                      , BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1), BYTE_REPEAT(0)
                      #endif    
                      );

    r1 = SIMD_ANDBITS(r1, masks);
    r1 = SIMD_CMPEQ8(r1, masks);
    r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
    r1 = SIMD_ADD8(r1, zeroes);
    SIMD_STOREBITS((reg*) _bptr, r1);

    #ifndef __AVX512F__
      r1 = SIMD_SETR8(BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1), BYTE_REPEAT(0));
      r1 = SIMD_ANDBITS(r1, masks);
      r1 = SIMD_CMPEQ8(r1, masks);
      r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
      r1 = SIMD_ADD8(r1, zeroes);
      SIMD_STOREBITS((reg*) &_bptr[SIMD_LEN], r1);
    #endif
  }
#endif

// prints all bits in a buffer as chunks of 1024 bits
FORCE_INLINE static void print_chunks(FILE *fptr, char *restrict _bptr, const u64 *restrict _ptr) {  
  register u8 i = 0;

  do {
    PRINT_4(0,   i + 0),
    PRINT_4(256, i + 4),
    PRINT_4(512, i + 8),
    PRINT_4(768, i + 12);

    fwrite(_bptr, 1, BITBUF_SIZE, fptr);
  } while ((i += 16 - (i == 240)) < BUF_SIZE - 1);
}

static u8 stream_ascii(FILE *fptr, const u64 limit, rng_data *data) {
  if (UNLIKELY(fptr == NULL)) return 1;
  
  /*
    Split limit based on how many calls (if needed)
    we make to print_chunks, which prints the bits of 
    an entire buffer (aka the SEQ_SIZE)
  */ 
  register long int rate = limit >> 14;
  register short leftovers = limit & (SEQ_SIZE - 1);

  char *restrict _bptr = &bitbuffer[0];

  while (rate > 0) {
    adam(data);
    print_chunks(fptr, _bptr, &data->buffer[0]);
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
    register u16 l, limit;
    register u64 num;

    adam(data);
    print_leftovers:
      limit = (leftovers < BITBUF_SIZE) ? leftovers : BITBUF_SIZE;

      l = 0;
      do {
        num = *data->buffer++;
        print_binary(_bptr + l, num);
      } while ((l += 64) < limit);

      fwrite(_bptr, 1, limit, fptr);
      leftovers -= limit;

      if (LIKELY(leftovers > 0)) 
        goto print_leftovers;
  }

  return 0;
}

static u8 stream_bytes(FILE *fptr, const u64 limit, rng_data *data) {   
  if (UNLIKELY(fptr == NULL)) return 1;

  /*
    Split limit based on how many calls we need to make
    to write the bytes of an entire buffer directly
  */ 
  register long int rate = limit >> 14;
  register short leftovers = limit & (SEQ_SIZE - 1);

  while (LIKELY(rate > 0)) {
    adam(data);
    fwrite(data->buffer, 8, BUF_SIZE, fptr);
    --rate;
  } 

  if (LIKELY(leftovers > 0)) {
    adam(data);
    fwrite(data->buffer, 8, BUF_SIZE, fptr);
  }

  return 0;
}

u8 bits(rng_data *data) {
  return stream_bytes(stdout, __UINT64_MAX__, data);
}

u8 assess(const u16 limit, rng_data *data) {
  FILE *fptr;
  const char *file_type;
  u8 (*fn)(FILE*, const u64, rng_data*);
  char c;
  char file_name[65];

  get_file_name:
    printf("File name: \033[1;33m");
    if (!scanf("%64s", &file_name[0])) {
      fputs("\033[m\033[1;31mPlease enter a valid file name\033[m\n", stderr);
      goto get_file_name;
    }

  get_file_type:  
    printf("\033[mFile type (1 = ASCII, 2 = BINARY): \033[1;33m");
    scanf(" %c", &c);
    if (c == '1')
      file_type = "w+", fn = stream_ascii;
    else if (c == '2')
      file_type = "wb+", fn = stream_bytes;
    else {
      fputs("\033[1;31mValue must be 1 or 2\n", stderr);
      goto get_file_type;
    }

  fptr = fopen(file_name, file_type);
  fputs("\033[m", stdout);

  const u64 total = limit * ASSESS_BITS;

  if (UNLIKELY(fn(fptr, total, data)))
    return fputs("\033[1;31mError while creating file. Exiting.\033[m\n", stderr);

  fclose(fptr);

  printf("\n\033[1mWrote %llu bits to %s file \033[36m\"%s\"\033[m\n", 
                total, (c == '1') ? "ASCII" : "BINARY", file_name);

  return 0;
}

u8 examine(u64 *restrict _ptr, const u16 limit, u64 *seed, u64 *nonce) {
  double r_ent, r_chisq, r_mean, r_montepicalc, r_scc;

  u64 ones = 0;

  

  return 0;
}

u8 infinite(rng_data *data) {
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
  #define LIVE_ITER   (BUF_SIZE - 31)

  const char* ADAM_ASCII = {
    "%s\033[38;2;173;58;0m/\033[38;2;255;107;33m@@@@@@\033[38;2;173;58;0m\\\033[0m"
    "%s\033[38;2;173;58;0m/(\033[38;2;255;107;33m@@\033[38;2;173;58;0m((((((((((((((((\033[38;2;255;107;33m@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((((((((((((((((((((\033[38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;173;58;0m(((((((((((((((((((((((((((((\033[38;2;255;107;33m#@##@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((((((((((\033[38;2;255;107;33m@#####&\033[38;2;173;58;0m(((((((((((((((\033[38;2;255;107;33m@@@@\033[0m"
    "%s\033[38;2;173;58;0m((((((((((((((\033[38;2;255;107;33m#@@\033[38;2;173;58;0m((((\033[38;2;255;107;33m@@@@@\033[38;2;173;58;0m((((((((((((((\033[38;2;255;107;33m@@\033[0m"
    "%s\033[38;2;173;58;0m((((((((\033[38;2;255;107;33m@@\033[38;2;173;58;0m(((((((((\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((((((((\033[38;2;255;107;33m@&#@\033[38;2;173;58;0m(((((((((((\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;173;58;0m(\033[38;2;255;107;33m@@@\033[38;2;173;58;0m(((\033[38;2;255;107;33m##\033[38;2;173;58;0m((((((((\033[38;2;255;107;33m@@\033[38;2;173;58;0m((((((((((((((\033[38;2;255;107;33m##@##\033[38;2;173;58;0m(((((((((\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((\033[38;2;255;107;33m##\033[38;2;173;58;0m(((\033[38;2;255;107;33m@#####\033[38;2;173;58;0m(((((\033[38;2;255;107;33m######\033[38;2;173;58;0m((((\033[38;2;255;107;33m###@\033[38;2;173;58;0m(((((((((\033[38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m@&#########\033[38;2;173;58;0m((((\033[38;2;255;107;33m###@@##\033[38;2;173;58;0m(((\033[38;2;255;107;33m@###@\033[38;2;173;58;0m((((((((\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((((((\033[38;2;255;107;33m#&@@@#########@\033[38;2;173;58;0m(((((\033[38;2;255;107;33m@@\033[38;2;173;58;0m((((\033[38;2;255;107;33m#####@\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m#@@\033[0m"
    "%s\033[38;2;255;107;33m@@@@\033[38;2;173;58;0m((((\033[38;2;255;107;33m&#\033[38;2;173;58;0m(\033[38;2;255;107;33m################&@@@####@###@\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m@&&\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((\033[38;2;255;107;33m#####\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m###########@&####\033[38;2;173;58;0m((((\033[38;2;255;107;33m@##@&\033[0m"
    "%s\033[38;2;173;58;0m(\033[38;2;255;107;33m@\033[38;2;173;58;0m((((\033[38;2;255;107;33m@####@######@@###\033[38;2;173;58;0m(((\033[38;2;255;107;33m@######@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m@#\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m##############@@##\033[38;2;173;58;0m((((((((((((((((((\033[38;2;255;107;33m@\033[0m"
    "%s\033[38;2;255;107;33m@\033[0m"
    "%s\033[38;2;173;58;0m/\033[38;2;255;107;33m@\033[38;2;173;58;0m((\033[38;2;255;107;33m@###\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m###@@@@@@@@@@@@@&@##@\033[38;2;173;58;0m((\033[38;2;255;107;33m@&\033[0m"
    "%s\033[38;2;255;107;33m#@\033[0m"
    "%s\033[38;2;173;58;0m(((\033[38;2;255;107;33m################@@@#########\033[0m"
    "%s\033[38;2;173;58;0m/\033[38;2;255;107;33m#\033[38;2;173;58;0m)\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((\033[38;2;255;107;33m####\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m####&###\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m##@\033[0m"
    "%s\033[38;2;255;107;33m@&@\033[0m"
    "%s\033[38;2;255;107;33m.@\033[38;2;173;58;0m((((\033[38;2;255;107;33m##############@#@######@@@@@#####\033[0m"
    "%s\033[38;2;255;107;33m@&@\033[0m"
    "%s\033[38;2;255;107;33m@@@@\033[38;2;173;58;0m((\033[38;2;255;107;33m@####@\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m#####@@@#\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m&#@@\033[38;2;173;58;0m\\\033[0m"
    "%s\033[38;2;255;107;33m@@#@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m((\033[38;2;255;107;33m@#####@@#######@#################@\033[0m"
    "%s\033[38;2;255;107;33m@@##@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((\033[38;2;255;107;33m@#####\033[38;2;173;58;0m(((((((\033[38;2;255;107;33m######@###@@@@@\033[0m"
    "%s\033[38;2;255;107;33m@@#@##&#\033[0m"
    "%s\033[38;2;255;107;33m@@\033[38;2;173;58;0m((\033[38;2;255;107;33m@@\033[38;2;173;58;0m((((\033[38;2;255;107;33m@##########&\033[38;2;173;58;0m((((\033[38;2;255;107;33m###@\033[0m"
    "%s\033[38;2;255;107;33m@&&##@\033[38;2;173;58;0m))\033[38;2;255;107;33m@#@@\033[0m"
    "%s\033[38;2;255;107;33m@@@\033[0m"
    "%s\033[38;2;255;107;33m@\033[38;2;173;58;0m(((((\033[38;2;255;107;33m&#######\033[38;2;173;58;0m((\033[38;2;255;107;33m#@############&\033[38;2;173;58;0m)))))))\033[0m"
    "%s\033[38;2;255;107;33m@#\033[38;2;173;58;0m(((((\033[38;2;255;107;33m#@##############@\033[38;2;173;58;0m((((((\033[38;2;255;107;33m#@#@\033[0m"
    "%s\033[38;2;255;107;33m@@@@@@&@#\033[38;2;173;58;0m((((\033[38;2;255;107;33m##@@@@@#\033[38;2;173;58;0m((((((\033[38;2;255;107;33m#&#@#&\033[0m"
    "%s\033[38;2;255;107;33m####\033[0m"
    "%s\033[38;2;255;107;33m#&#\033[38;2;173;58;0m(\033[38;2;255;107;33m@@@@@\033[0m%s%s"
  };

  // set window dimensions for live stream
  printf("\033[8;29;64t");
  // no buffering
  setbuf(stdout, NULL);
  
  char lines[40][100];

  register u8 i = 0;
  adam(data);
  u64 *restrict _ptr = &data->buffer[0];
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
      snprintf(lines[37], 3,  "%llu",                     GET_1(i + 68));
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
      fwrite("\033[2J\r", 1, 5, stdout);
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
    adam(data);
    goto live_adam; 

  return 0;
}