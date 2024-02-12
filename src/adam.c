#include <sys/random.h>

#include "../include/adam.h"
#include "../include/defs.h"
#include "../include/rng.h"

static u64 *buffer;

void adam_init(rng_data *data, bool generate_dbls)
{
  getentropy(&data->seed[0], sizeof(u64) << 2);
  getentropy(&data->nonce, sizeof(u64));

  data->index = 0;
  data->dbl_mode = generate_dbls;
  data->aa = (data->nonce & 0xFFFFFFFF00000000) | (~data->seed[data->nonce & 3] & 0xFFFFFFFF);
  data->bb = (data->seed[data->aa & 3] & 0xFFFFFFFF00000000) | (~data->nonce & 0xFFFFFFFF);

  // adam_data() is an internal function for accessing the raw memory used by the RNG
  adam_data(&buffer, NULL);
}

int adam_get(rng_data *data, void *output, const unsigned char width)
{
  if (width != 8 && width != 16 && width != 32 && width != 64)
    return 1;

  if (buffer[BUF_SIZE - 1] == 0)
    adam_run(data->seed, &data->nonce, &data->aa, &data->bb);

  if (!data->dbl_mode) {
    const u64 mask = (width == 64) ? __UINT64_MAX__ : ((1UL << width) - 1);
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

int adam_fill(rng_data *data, void *buf, const unsigned char width, const unsigned int amount)
{
  if ((width != 8 && width != 16 && width != 32 && width != 64) || amount > 1000000)
    return 1;

  const u16 one_run = BUF_SIZE * (64 / width);
  const u32 leftovers = amount & (one_run - 1);

  register long limit = amount >> CTZ(one_run);

  if (!data->dbl_mode) {
    while (limit > 0) {
      adam_run(data->seed, &data->nonce, &data->aa, &data->bb);
      MEMCPY(buf, &buffer[0], SEQ_BYTES);
      buf += SEQ_BYTES;
      --limit;
    }

    if (leftovers > 0) {
      adam_run(data->seed, &data->nonce, &data->aa, &data->bb);
      MEMCPY(buf, &buffer[0], leftovers * (width >> 3));
    }
  } else {
    register u32 out = 0;
    while (out < amount) {
      adam_get(buf, data, 64);
      buf += sizeof(double);
      ++out;
    }
  }

  return 0;
}
