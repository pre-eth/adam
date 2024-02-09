#include <sys/random.h>
#include <time.h>

#include "../include/adam.h"
#include "../include/defs.h"
#include "../include/rng.h"

static u64 *buffer;

void adam_init(rng_data *data, bool generate_dbls)
{
  getentropy(&data->seed[0], sizeof(u64) << 2);
  getentropy(&data->nonce, sizeof(u64));

  // flag to trigger initial generation
  data->index = __UINT8_MAX__;
  data->dbl_mode = generate_dbls;
  data->aa = (data->nonce & 0xFFFFFFFF00000000) | (~data->seed[data->nonce & 3] & 0xFFFFFFFF);
  data->bb = (data->seed[data->aa & 3] & 0xFFFFFFFF00000000) | (~data->nonce & 0xFFFFFFFF);

  // adam_data() is an internal function for accessing the raw memory used by the RNG
  adam_data(&buffer, NULL);
}

int adam_get(void *output, rng_data *data, const unsigned char width, double *duration)
{
  if (width != 8 && width != 16 && width != 32 && width != 64)
    return 1;

  register u64 mask = (width == 64) ? __UINT64_MAX__ : ((1UL << width) - 1);

  if (data->index == __UINT8_MAX__) {
    register clock_t start = clock();
    adam_run(data->seed, &data->nonce, &data->aa, &data->bb);
    if (duration != NULL)
      *duration += (double)(clock() - start) / (double)CLOCKS_PER_SEC;
  }

  if (!data->dbl_mode) {
    u64 out = buffer[data->index] & mask;
    MEMCPY(output, &out, width >> 3);
    buffer[data->index] >>= width;
    data->index += (!buffer[data->index]);
  } else {
    double out = ((double)buffer[data->index] / (double)__UINT64_MAX__);
    MEMCPY(output, &out, sizeof(double));
    ++data->index;
  }

  return 0;
}

int adam_fill(void *buf, rng_data *data, const unsigned int amount, double *duration)
{
  if (amount > 1000000000)
    return 1;

  void *_ptr = buf;
  register clock_t start;
  double _duration = 0.0;

  if (!data->dbl_mode) {
    register long long limit = amount / BUF_SIZE;
    register u64 leftovers = amount & (BUF_SIZE - 1);

    while (limit > 0) {
      start = clock();
      adam_run(data->seed, &data->nonce, &data->aa, &data->bb);
      _duration += (double)(clock() - start) / (double)CLOCKS_PER_SEC;
      MEMCPY(_ptr, &buffer[0], SEQ_BYTES);
      _ptr += SEQ_BYTES;
      --limit;
    }

    if (leftovers > 0) {
      start = clock();
      adam_run(data->seed, &data->nonce, &data->aa, &data->bb);
      _duration += (double)(clock() - start) / (double)CLOCKS_PER_SEC;
      MEMCPY(_ptr, &buffer[0], leftovers * sizeof(u64));
    }
  } else {
    register u32 out = 0;
    while (out < amount) {
      adam_get(_ptr, data, 64, &_duration);
      _ptr += sizeof(double);
      ++out;
    }
  }

  if (duration != NULL)
    *duration += _duration;

  return 0;
}
