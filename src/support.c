#include <stdio.h> // for output
#include <sys/ioctl.h> // for pretty printing
#include <time.h> // for clock_t, clock(), CLOCKS_PER_SEC

#include "../include/adam.h"
#include "../include/ent.h"
#include "../include/support.h"

FORCE_INLINE u8 get_print_metrics(u16 *center, u16 *indent, u16 *swidth)
{
  struct winsize wsize;
  ioctl(0, TIOCGWINSZ, &wsize);
  *center = (wsize.ws_col >> 1);
  *indent = (wsize.ws_col >> 4);
  *swidth = wsize.ws_col;
  return 0;
}

FORCE_INLINE u8 err(const char *s)
{
  fprintf(stderr, "\033[1;31m%s\033[m\n", s);
  return 1;
}

FORCE_INLINE u64 a_to_u(const char *s, const u64 min, const u64 max)
{
  if (UNLIKELY(s == NULL || s[0] == '-'))
    return min;

  register u8 len = 0;
  register u64 val = 0;

  for (; s[len] != '\0'; ++len) {
    if (UNLIKELY(s[len] < '0' || s[len] > '9'))
      return 0;
  };

  switch (len) {
  case 20:
    val += 10000000000000000000LU;
  case 19:
    val += (s[len - 19] - '0') * 1000000000000000000LU;
  case 18:
    val += (s[len - 18] - '0') * 100000000000000000LU;
  case 17:
    val += (s[len - 17] - '0') * 10000000000000000LU;
  case 16:
    val += (s[len - 16] - '0') * 1000000000000000LU;
  case 15:
    val += (s[len - 15] - '0') * 100000000000000LU;
  case 14:
    val += (s[len - 14] - '0') * 10000000000000LU;
  case 13:
    val += (s[len - 13] - '0') * 1000000000000LU;
  case 12:
    val += (s[len - 12] - '0') * 100000000000LU;
  case 11:
    val += (s[len - 11] - '0') * 10000000000LU;
  case 10:
    val += (s[len - 10] - '0') * 1000000000LU;
  case 9:
    val += (s[len - 9] - '0') * 100000000LU;
  case 8:
    val += (s[len - 8] - '0') * 10000000LU;
  case 7:
    val += (s[len - 7] - '0') * 1000000LU;
  case 6:
    val += (s[len - 6] - '0') * 100000LU;
  case 5:
    val += (s[len - 5] - '0') * 10000LU;
  case 4:
    val += (s[len - 4] - '0') * 1000LU;
  case 3:
    val += (s[len - 3] - '0') * 100LU;
  case 2:
    val += (s[len - 2] - '0') * 10LU;
  case 1:
    val += (s[len - 1] - '0');
    break;
  }
  return (val >= min || val < max - 1) ? val : min;
}

static u8 load_seed(u64 *seed, const char *strseed)
{
  FILE *seed_file = fopen(strseed, "rb");
  if (!seed_file)
    return err("Couldn't read seed file");
  fread(seed, sizeof(u64), 4, seed_file);
  fclose(seed_file);
  return 0;
}

static u8 store_seed(u64 *seed)
{
  char file_name[65];
  printf("File name: \033[1;93m");
  while (!scanf(" %64s", &file_name[0]))
    err("Please enter a valid file name");
  FILE *seed_file = fopen(file_name, "rb");
  if (!seed_file)
    return err("Couldn't read seed file");
  fread(seed, sizeof(u64), 4, seed_file);
  fclose(seed_file);
  return 0;
}

FORCE_INLINE u8 rwseed(u64 *seed, const char *strseed)
{
  if (strseed != NULL)
    return load_seed(seed, strseed);
  return store_seed(seed);
}

FORCE_INLINE u8 rwnonce(u64 *nonce, const char *strnonce)
{
  if (strnonce != NULL)
    *nonce = a_to_u(strnonce, 0, __UINT64_MAX__);
  else
    fprintf(stderr, "\033[1;96mNONCE:\033[m %llu", *nonce);
  return 0;
}

/*
  To make writes more efficient, rather than writing one
  number at a time, 16 numbers are parsed together and then
  written to stdout with 1 fwrite call.
*/
static char bitbuffer[BITBUF_SIZE] ALIGN(SIMD_LEN);

#ifdef __AARCH64_SIMD__
static void print_binary(char *restrict _bptr, u64 num)
{
  // Copying bit masks to high and low halves
  // 72624976668147840 == {128, 64, 32, 16, 8, 4, 2, 1}
  const reg8q masks = SIMD_COMBINE8(SIMD_CREATE8(72624976668147840),
      SIMD_CREATE8(72624976668147840));
  const reg8q zero = SIMD_SET8('0');

  // Repeat all 8 bytes 8 times each, to fill up r1 with 64 bytes total
  // Then we bitwise AND with masks to turn each byte into a 1 or 0
  reg8q4 r1;
  r1.val[0] = SIMD_COMBINE8(SIMD_MOV8((num >> 56) & 0xFF),
      SIMD_MOV8((num >> 48) & 0xFF));
  r1.val[1] = SIMD_COMBINE8(SIMD_MOV8((num >> 40) & 0xFF),
      SIMD_MOV8((num >> 32) & 0xFF));
  r1.val[2] = SIMD_COMBINE8(SIMD_MOV8((num >> 24) & 0xFF),
      SIMD_MOV8((num >> 16) & 0xFF));
  r1.val[3] = SIMD_COMBINE8(SIMD_MOV8((num >> 8) & 0xFF), SIMD_MOV8(num & 0xFF));

  SIMD_AND4Q8(r1, r1, masks);
  SIMD_CMP4QEQ8(r1, r1, masks);
  SIMD_AND4Q8(r1, r1, SIMD_SET8(1));

  // '0' = 48, '1' = 49. This is how we print the number
  SIMD_ADD4Q8(r1, r1, zero);

  SIMD_STORE8x4(_bptr, r1);
}
#else
static void print_binary(char *restrict _bptr, u64 num)
{
#define BYTE_REPEAT(n) \
  bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n], bytes[n]
#define BYTE_MASKS 128, 64, 32, 16, 8, 4, 2, 1

  u8 bytes[sizeof(u64)];

  MEMCPY(bytes, &num, sizeof(u64));

  const reg zeroes = SIMD_SET8('0');
  const reg masks = SIMD_SETR8(BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
#ifdef __AVX512F__
      ,
      BYTE_MASKS, BYTE_MASKS, BYTE_MASKS, BYTE_MASKS
#endif
  );

  reg r1 = SIMD_SETR8(BYTE_REPEAT(7), BYTE_REPEAT(6), BYTE_REPEAT(5), BYTE_REPEAT(4)
#ifdef __AVX512F__
                                                                          ,
      BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1), BYTE_REPEAT(0)
#endif
  );

  r1 = SIMD_ANDBITS(r1, masks);
  r1 = SIMD_CMPEQ8(r1, masks);
  r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
  r1 = SIMD_ADD8(r1, zeroes);
  SIMD_STOREBITS((reg *)_bptr, r1);

#ifndef __AVX512F__
  r1 = SIMD_SETR8(BYTE_REPEAT(3), BYTE_REPEAT(2), BYTE_REPEAT(1),
      BYTE_REPEAT(0));
  r1 = SIMD_ANDBITS(r1, masks);
  r1 = SIMD_CMPEQ8(r1, masks);
  r1 = SIMD_ANDBITS(r1, SIMD_SET8(1));
  r1 = SIMD_ADD8(r1, zeroes);
  SIMD_STOREBITS((reg *)&_bptr[SIMD_LEN], r1);
#endif
}
#endif

// prints all bits in a buffer as chunks of 1024 bits
FORCE_INLINE static void print_chunks(char *restrict _bptr,
    const u64 *restrict _ptr)
{
  register u8 i = 0;

  do {
    PRINT_4(0, i + 0), PRINT_4(256, i + 4), PRINT_4(512, i + 8),
        PRINT_4(768, i + 12);

    fwrite(_bptr, 1, BITBUF_SIZE, stdout);
  } while ((i += 16 - (i == 240)) < BUF_SIZE - 1);
}

u8 gen_uuid(u64 *_ptr, u8 *buf)
{
  u128 tmp;

  // Fill buf with 16 random bytes
  tmp = ((u128)_ptr[0] << 64) | _ptr[1];
  MEMCPY(buf, &tmp, sizeof(u128));

  /*
    CODE AND COMMENT PULLED FROM CRYPTOSYS
    (https://www.cryptosys.net/pki/Uuid.c.html)

    Adjust certain bits according to RFC 4122 section 4.4.
    This just means do the following
    (a) set the high nibble of the 7th byte equal to 4 and
    (b) set the two most significant bits of the 9th byte to 10'B,
        so the high nibble will be one of {8,9,A,B}.
  */
  buf[6] = 0x40 | (buf[6] & 0xf);
  buf[8] = 0x80 | (buf[8] & 0x3f);
  return 0;
}

double stream_ascii(const u64 limit, rng_data *data)
{
  /*
    Split limit based on how many calls (if needed)
    we make to print_chunks, which prints the bits of
    an entire buffer (aka the SEQ_SIZE)
  */
  register long int rate = limit >> 14;
  register short leftovers = limit & (SEQ_SIZE - 1);
  register clock_t start;
  register double duration = 0.0;

  char *restrict _bptr = &bitbuffer[0];

  while (rate > 0) {
    start = clock();
    adam(data);
    duration += (double)(clock() - start) / CLOCKS_PER_SEC;
    print_chunks(_bptr, &data->buffer[0]);
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

    start = clock();
    adam(data);
    duration += (double)(clock() - start) / CLOCKS_PER_SEC;

  print_leftovers:
    limit = (leftovers < BITBUF_SIZE) ? leftovers : BITBUF_SIZE;

    l = 0;
    do {
      num = *data->buffer++;
      print_binary(_bptr + l, num);
    } while ((l += 64) < limit);

    fwrite(_bptr, 1, limit, stdout);
    leftovers -= limit;

    if (LIKELY(leftovers > 0))
      goto print_leftovers;
  }

  return duration;
}

double stream_bytes(const u64 limit, rng_data *data)
{
  /*
    Split limit based on how many calls we need to make
    to write the bytes of an entire buffer directly
  */
  const short leftovers = limit & (SEQ_SIZE - 1);
  register long int rate = limit >> 14;
  register clock_t start;
  register double duration = 0.0;

  while (LIKELY(rate > 0)) {
    start = clock();
    adam(data);
    duration += (double)(clock() - start) / CLOCKS_PER_SEC;
    fwrite(&data->buffer[0], sizeof(u64), BUF_SIZE, stdout);
    --rate;
  }

  if (LIKELY(leftovers > 0)) {
    start = clock();
    adam(data);
    duration += (double)(clock() - start) / CLOCKS_PER_SEC;
    fwrite(&data->buffer[0], sizeof(u64), BUF_SIZE, stdout);
  }

  return duration;
}

double examine(const char *strlimit, rng_data *data)
{
  ent_report rsl;
  rsl.data = data;
  rsl.limit = TESTING_BITS;
  if (strlimit != NULL)
    rsl.limit *= a_to_u(strlimit, 1, TESTING_LIMIT);

  u64 init_values[7];
  MEMCPY(&init_values[0], &data->seed, sizeof(u64) << 2);
  init_values[4] = data->nonce;
  init_values[5] = data->aa;
  init_values[6] = data->bb;

  printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", rsl.limit);
  register clock_t start = clock();
  ent_test(&rsl);
  double duration = (double)(clock() - start) / (double)CLOCKS_PER_SEC;
  printf("\033[1;33mExamination Complete! (%lfs)\033[m\n\n", duration);

  u16 center, indent, swidth;
  get_print_metrics(&center, &indent, &swidth);

  u64 bytes = rsl.limit;
  const char *unit = " BYTES\0KB\0MB\0GB\0TB";

  if (bytes > 8000000000000UL) {
    bytes /= 8000000000000;
    unit += 16;
  } else if (bytes > 8000000000UL) {
    bytes /= 8000000000;
    unit += 13;
  } else if (bytes > 8000000) {
    bytes /= 8000000;
    unit += 10;
  } else if (bytes > 8000) {
    bytes /= 8000;
    unit += 7;
  }

  printf("\033[%uC[RESULTS]\n\n", center - 4);
  const u64 sequences = (rsl.limit >> 14) + !!(rsl.limit & (SEQ_SIZE - 1));
  const u64 output = sequences << 8;
  printf("\033[1;34m            Total Output: \033[m%llu u64 (%llu u32, %llu u16, %llu u8)\n", output, output << 1, output << 2, output << 3);
  printf("\033[1;34m     Sequences Generated: \033[m%llu \n", sequences);
  printf("\033[1;34m             Sample Size: \033[m%llu BITS (%llu%s)\n", rsl.limit, bytes, unit);
  const u64 zeroes = rsl.limit - rsl.mfreq;
  const u64 diff = (zeroes > rsl.mfreq) ? zeroes - rsl.mfreq : rsl.mfreq - zeroes;
  printf("\033[1;34m       Monobit Frequency: \033[m%llu ONES, %llu ZEROES (+%llu %s)\n", rsl.mfreq, zeroes, diff, (zeroes > rsl.mfreq) ? "ZEROES" : "ONES");
  printf("\033[1;34m           Minimum Value: \033[m%llu\n", rsl.min);
  printf("\033[1;34m           Maximum Value: \033[m%llu\n", rsl.max);
  printf("\033[1;34m                   Range: \033[m%llu\n", rsl.max - rsl.min);
  printf("\033[1;34m        Zeroes in Buffer: \033[m%llu\n", rsl.zeroes);
  printf("\033[1;34m            256-bit Seed: \033[m0x%llX,\n", init_values[0]);
  printf("                          0x%llX,\n", init_values[1]);
  printf("                          0x%llX,\n", init_values[2]);
  printf("                          0x%llX\n", init_values[3]);
  printf("\033[1;34m            64-bit Nonce: \033[m0x%llX\n", init_values[4]);
  printf("\033[1;34m         Initial State 1: \033[m0x%llX\n", init_values[5]);
  printf("\033[1;34m         Initial State 2: \033[m0x%llX\n", init_values[6]);
  printf("\033[1;34m                 Entropy: \033[m%.5lf bits per byte\n", rsl.ent);
  char *chi_str;
  char chi_tmp[6];
  register u8 suspect_level = 32;
  if (rsl.pochisq < 0.0001) {
    chi_str = "<= 0.01";
    --suspect_level;
  } else if (rsl.pochisq > 0.9999) {
    chi_str = ">= 99.99";
    --suspect_level;
  } else {
    const double pochisq = rsl.pochisq * 100;
    snprintf(&chi_tmp[0], 5, "%1.2f", pochisq);
    if (pochisq < 5.0 || pochisq > 95.0)
      ++suspect_level;
    else if (pochisq < 10.0 || pochisq > 90.0)
      suspect_level += 61;
    chi_str = &chi_tmp[0];
  }
  printf("\033[1;34m              Chi square: \033[m%1.2lf (randomly exceeded \033[1;%um%s%%\033[m of the time) \n", rsl.chisq, suspect_level, chi_str);
  printf("\033[1;34m         Arithmetic Mean: \033[m%1.4lf (127.5 = random)\n", rsl.mean);
  printf("\033[1;34mMonte Carlo Value for Pi: \033[m%1.9lf (error: %1.2f%%)\n", rsl.montepicalc, rsl.monterr);
  if (rsl.scc >= -99999)
    printf("\033[1;34m      Serial correlation: \033[m%1.6f (totally uncorrelated = 0.0).\n", rsl.scc);
  else
    printf("\033[1;34m      Serial correlation: \033[1;31mUNDEFINED\033[m (all values equal!).\n");
  return duration;
}
