#include <stdio.h> // for output
#include <time.h> // for clock_t, clock(), CLOCKS_PER_SEC

#include "../include/adam.h"
#include "../include/ent.h"
#include "../include/support.h"

FORCE_INLINE u8 err(const char *s)
{
  fprintf(stderr, "\033[1;91m%s\033[m\n", s);
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
  while (!scanf("File name: \033[1;93m%64s", &file_name[0]))
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

