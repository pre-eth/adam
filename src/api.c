#include <sys/random.h>

#include "../include/api.h"
#include "../include/defs.h"
#include "../include/rng.h"

static u64 *buffer;

void adam_setup(adam_data *data, unsigned long long *seed, unsigned long long *nonce)
{
    // data->seed[0] = 0x43CE8BAD891F0610;
    // data->seed[1] = 0xB1B2B727643446EA;
    // data->seed[2] = 0x2A5D5C83EA265024;
    // data->seed[3] = 0x07DC6BB2DF5E59DA;

    // data->nonce = 0x031CC6BD641FA81C;
    if (seed == NULL)
        getentropy(&data->seed[0], sizeof(u64) << 2);
    else
        MEMCPY(&data->seed[0], &seed[0], sizeof(u64) << 2);

    if (nonce == NULL)
        getentropy(&data->nonce, sizeof(u64));
    else
        data->nonce = *nonce;

    data->index = 0;

    // adam_data() is an internal function for accessing the raw memory used by the RNG
    adam_connect(&buffer, NULL);
}

static unsigned long long get_int(adam_data *data, const unsigned char width)
{
    const u64 mask   = (width == 64) ? __UINT64_MAX__ : ((1UL << width) - 1);
    register u64 out = buffer[data->index] & mask;
    buffer[data->index] >>= width;
    data->index += (!buffer[data->index]);
    return out;
}

static unsigned long long regen_int(adam_data *data, const unsigned char width)
{
    adam_run(data->seed, &data->nonce);
    return get_int(data, width);
}

unsigned long long adam_int(adam_data *data, unsigned char width)
{
    static unsigned long long (*int_fn[])(adam_data *, const unsigned char) = {
        &get_int,
        &regen_int
    };

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    return int_fn[!buffer[ADAM_BUF_SIZE - 1]](data, width);
}

static double get_dbl(adam_data *data, const unsigned long long scale)
{
    register double out = ((double) buffer[data->index] / (double) __UINT64_MAX__);
    buffer[data->index] = 0;
    ++data->index;
    return out * (double) (scale + !scale);
}

static double regen_dbl(adam_data *data, const unsigned long long scale)
{
    adam_run(data->seed, &data->nonce);
    return get_dbl(data, scale);
}

double adam_dbl(adam_data *data, unsigned long long scale)
{
    static double (*dbl_fn[])(adam_data *, const unsigned long long) = {
        &get_dbl,
        &regen_dbl
    };
    return dbl_fn[!buffer[BUF_SIZE - 1]](data, scale);
}

int adam_fill(adam_data *data, void *buf, unsigned char width, const unsigned int amount)
{
    if (!amount || amount > 1000000000UL)
        return 1;

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    const u16 one_run   = BUF_SIZE * (64 / width);
    const u32 leftovers = amount & (one_run - 1);

    register long limit = amount >> CTZ(one_run);

    while (limit > 0) {
        adam_run(data->seed, &data->nonce);
        MEMCPY(buf, &buffer[0], SEQ_BYTES);
        buf += SEQ_BYTES;
        --limit;
    }

    if (LIKELY(leftovers > 0)) {
        adam_run(data->seed, &data->nonce);
        MEMCPY(buf, &buffer[0], leftovers * (width >> 3));
    }

    return 0;
}

int adam_dfill(adam_data *data, double *buf, const unsigned long long multiplier, const unsigned int amount)
{
    if (!amount || amount > 1000000000UL)
        return 1;

    if (multiplier > 1)

        adam_fmrun(data->seed, &data->nonce, buf, amount, multiplier);
    else
        adam_frun(data->seed, &data->nonce, buf, amount);

    return 0;
}

void *adam_choice(adam_data *data, void *arr, const unsigned long long size)
{
    return &arr[adam_int(data, 64) % size];
}
