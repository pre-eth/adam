#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#include "../include/adam.h"
#include "../include/support.h"
#include "../include/test.h"

void get_print_metrics(u16 *center, u16 *indent, u16 *swidth)
{
  struct winsize wsize;
  ioctl(0, TIOCGWINSZ, &wsize);
  *center = (wsize.ws_col >> 1);
  *indent = (wsize.ws_col >> 4);
  *swidth = wsize.ws_col;
}

FORCE_INLINE u8 err(const char *s)
{
  fprintf(stderr, "\033[1;31m%s\033[m\n", s);
  return 1;
}

u64 a_to_u(const char *s, const u64 min, const u64 max)
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

u8 rwseed(u64 *seed, const char *strseed)
{
  if (strseed != NULL)
    return load_seed(seed, strseed);
  return store_seed(seed);
}

u8 rwnonce(u64 *nonce, const char *strnonce)
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
static void print_chunks(char *restrict _bptr,
    const u64 *restrict _ptr)
{
#define PRINT_4(i, j) print_binary(_bptr + i, _ptr[(j) + 0]),       \
                      print_binary(_bptr + 64 + i, _ptr[(j) + 1]),  \
                      print_binary(_bptr + 128 + i, _ptr[(j) + 2]), \
                      print_binary(_bptr + 192 + i, _ptr[(j) + 3])

  register u8 i = 0;
  do {
    PRINT_4(0, i + 0);
    PRINT_4(256, i + 4);
    PRINT_4(512, i + 8);
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

static void print_basic_results(const u16 indent, const u32 sequences, const u64 limit, rng_test *rsl)
{
  const u64 output = sequences << 8;
  const u32 zeroes = (output << 6) - rsl->mfreq;
  const u32 diff = (zeroes > rsl->mfreq) ? zeroes - rsl->mfreq : rsl->mfreq - zeroes;

  register u64 bytes = limit;
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

  printf("\033[1;34m\033[%uC              Total Output: \033[m%llu u64 (%llu u32 | %llu u16 | %llu u8)\n", indent, output, output << 1, output << 2, output << 3);
  printf("\033[1;34m\033[%uC       Sequences Generated: \033[m%u\n", indent, sequences);
  printf("\033[1;34m\033[%uC               Sample Size: \033[m%llu BITS (%llu%s)\n", indent, limit, bytes, unit);
  printf("\033[1;34m\033[%uC         Monobit Frequency: \033[m%u ONES, %u ZEROES (+%u %s)\n", indent, rsl->mfreq, zeroes, diff, (zeroes > rsl->mfreq) ? "ZEROES" : "ONES");
  printf("\033[1;34m\033[%uC             Minimum Value: \033[m%llu\n", indent, rsl->min);
  printf("\033[1;34m\033[%uC             Maximum Value: \033[m%llu\n", indent, rsl->max);
  printf("\033[1;34m\033[%uC                     Range: \033[m%llu\n", indent, rsl->max - rsl->min);
  printf("\033[1;34m\033[%uC              Even Numbers: \033[m%llu (%u%%)\n", indent, output - rsl->odd, (u8)(((double)(output - rsl->odd) / (double)output) * 100));
  printf("\033[1;34m\033[%uC               Odd Numbers: \033[m%u (%u%%)\n", indent, rsl->odd, (u8)(((double)rsl->odd / (double)output) * 100));
  printf("\033[1;34m\033[%uC          Zeroes Generated: \033[m%u\n", indent, rsl->zeroes);
  printf("\033[1;34m\033[%uC    256-bit Seed (u64 x 4): \033[m0x%llX, 0x%llX,\n", indent, rsl->init_values[0], rsl->init_values[1]);
  printf("\033[%uC                            0x%llX, 0x%llX\n", indent, rsl->init_values[2], rsl->init_values[3]);
  printf("\033[1;34m\033[%uC              64-bit Nonce: \033[m0x%llX\n", indent, rsl->init_values[4]);
  printf("\033[1;34m\033[%uC           Initial State 1: \033[m0x%llX\n", indent, rsl->init_values[5]);
  printf("\033[1;34m\033[%uC           Initial State 2: \033[m0x%llX\n", indent, rsl->init_values[6]);
  printf("\033[1;34m\033[%uC      Total Number of Runs: \033[m%u\n", indent, rsl->up_runs + rsl->down_runs);
  printf("\033[2m\033[%uC            a.  Increasing: \033[m%u\n", indent, rsl->up_runs);
  printf("\033[2m\033[%uC            b.  Decreasing: \033[m%u\n", indent, rsl->down_runs);
  printf("\033[2m\033[%uC            c. Longest Run: \033[m%u (INCREASING)\n", indent, rsl->longest_up);
  printf("\033[2m\033[%uC            d. Longest Run: \033[m%u (DECREASING)\n", indent, rsl->longest_down);
}

static void print_ent_results(const u16 indent, ent_report *ent)
{
  char *chi_str;
  char chi_tmp[6];
  register u8 suspect_level = 32;

  if (ent->pochisq < 0.001) {
    chi_str = "<= 0.01";
    --suspect_level;
  } else if (ent->pochisq > 0.99) {
    chi_str = ">= 0.99";
    --suspect_level;
  } else {
    snprintf(&chi_tmp[0], 5, "%1.2f", ent->pochisq * 100);
    chi_str = &chi_tmp[0];
  }

  printf("\033[1;34m\033[%uC                   Entropy: \033[m%.5lf bits per byte\n", indent, ent->ent);
  printf("\033[1;34m\033[%uC                Chi-Square: \033[m\033[1;%um%1.2lf\033[m (randomly exceeded %s%% of the time) \n", indent, suspect_level, ent->chisq, chi_str);
  printf("\033[1;34m\033[%uC           Arithmetic Mean: \033[m%1.3lf (127.5 = random)\n", indent, ent->mean);
  printf("\033[1;34m\033[%uC  Monte Carlo Value for Pi: \033[m%1.9lf (error: %1.2f%%)\n", indent, ent->montepicalc, ent->monterr);
  if (ent->scc >= -99999)
    printf("\033[1;34m\033[%uC        Serial Correlation: \033[m%1.6f (totally uncorrelated = 0.0).\n", indent, ent->scc);
  else
    printf("\033[1;34m\033[%uC        Serial Correlation: \033[1;31mUNDEFINED\033[m (all values equal!).\n", indent);
}

static void print_chseed_results(const u16 indent, const u32 sequences, u64 *chseed_dist, const double avg_chseed)
{
  #define CHSEED_CRITICAL_VALUE   13.277     

  register double chi_calc = 0.0;
  const u64 expected_chseeds = (u64)((sequences * (ROUNDS << 2)) * 0.2);

  register u8 i = 0;
  for (; i < 5; ++i)
    chi_calc += pow(((double)chseed_dist[i] - (double)expected_chseeds), 2) / (double)expected_chseeds;

  register u8 suspect_level = 32 - (CHSEED_CRITICAL_VALUE <= chi_calc);

  printf("\033[1;34m\033[%uC   Chaotic Seed Chi-Square: \033[m\033[1;%um%1.2lf\n", indent, suspect_level, chi_calc);
  printf("\033[1;34m\033[%uC        Average Seed Value: \033[m%1.15lf (ideal = 0.25)\n", indent, avg_chseed / (sequences * (ROUNDS << 2)));
  printf("\033[2m\033[%uC             a. (0.0, 0.1): \033[m%llu (%llu expected)\n", indent, chseed_dist[0], expected_chseeds);
  printf("\033[2m\033[%uC             b. [0.1, 0.2): \033[m%llu (%llu expected)\n", indent, chseed_dist[1], expected_chseeds);
  printf("\033[2m\033[%uC             c. [0.2, 0.3): \033[m%llu (%llu expected)\n", indent, chseed_dist[2], expected_chseeds);
  printf("\033[2m\033[%uC             d. [0.3, 0.4): \033[m%llu (%llu expected)\n", indent, chseed_dist[3], expected_chseeds);
  printf("\033[2m\033[%uC             e. [0.4, 0.5): \033[m%llu (%llu expected)\n", indent, chseed_dist[4], expected_chseeds);
}

double examine(const char *strlimit, rng_data *data)
{
  rng_test rsl;
  ent_report ent;

  rsl.data = data;
  rsl.ent = &ent;

  register u64 limit = TESTING_BITS;
  if (strlimit != NULL)
    limit *= a_to_u(strlimit, 1, TESTING_LIMIT);

  printf("\033[1;33mExamining %llu bits of ADAM...\033[m\n", limit);

  register clock_t start = clock();
  adam_test(limit, &rsl);
  register double duration = (double)(clock() - start) / (double)CLOCKS_PER_SEC;

  printf("\033[1;33mExamination Complete! (%lfs)\033[m\n\n", duration);

  u16 center, indent, swidth;
  get_print_metrics(&center, &indent, &swidth);
  indent <<= 1;

  printf("\033[%uC[RESULTS]\n\n", center - 4);

  const u32 sequences = (limit >> 14) + !!(limit & (SEQ_SIZE - 1));

  print_basic_results(indent, sequences, limit, &rsl);
  print_ent_results(indent, &ent);
  print_chseed_results(indent, sequences, rsl.chseed_dist, rsl.avg_chseed);

  // HISTOGRAM

  return duration;
}
