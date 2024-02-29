#include <sys/random.h>

#include "../include/api.h"
#include "../include/defs.h"
#include "../include/rng.h"

static u64 buffer[ADAM_BUF_SIZE] ALIGN(ADAM_ALIGNMENT); //  Our output vector of 256 64-bit integers
static u8 buff_idx;                                     //  Current index in buffer
static u64 adam_seed[4];                                //  256-bit seed
static u64 adam_nonce;                                  //  64-bit nonce

void adam_setup(unsigned long long *seed, unsigned long long *nonce)
{
    // data->seed[0] = 0x43CE8BAD891F0610;
    // data->seed[1] = 0xB1B2B727643446EA;
    // data->seed[2] = 0x2A5D5C83EA265024;
    // data->seed[3] = 0x07DC6BB2DF5E59DA;

    // data->nonce = 0x031CC6BD641FA81C;
    if (seed == NULL)
        getentropy(&adam_seed[0], sizeof(u64) << 2);
    else
        MEMCPY(&adam_seed[0], &seed[0], sizeof(u64) << 2);

    if (nonce == NULL)
        getentropy(&adam_nonce, sizeof(u64));
    else
        adam_nonce = *nonce;
}

unsigned long long *adam_rng_seed()
{
    return &adam_seed[0];
}

unsigned long long *adam_rng_nonce()
{
    return &adam_nonce;
}

unsigned long long *adam_rng_buffer()
{
    buff_idx = 0;
    return &buffer[0];
}

static unsigned long long get_int(const unsigned char width)
{
    const u64 mask   = (width == 64) ? __UINT64_MAX__ : ((1UL << width) - 1);
    register u64 out = buffer[buff_idx] & mask;
    buffer[buff_idx] >>= width;
    buff_idx += (!buffer[buff_idx]);
    return out;
}

static unsigned long long regen_int(const unsigned char width)
{
    adam(&buffer[0], adam_seed, &adam_nonce);
    return get_int(width);
}

unsigned long long adam_int(unsigned char width, const unsigned char force_regen)
{
    static unsigned long long (*int_fn[])(const unsigned char) = {
        &get_int,
        &regen_int
    };

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    return int_fn[!buff_idx || force_regen](width);
}

static double get_dbl(const unsigned long long scale)
{
    register double out = ((double) buffer[buff_idx] / (double) __UINT64_MAX__);
    buffer[buff_idx]    = 0;
    ++buff_idx;
    return out * (double) (scale + !scale);
}

static double regen_dbl(const unsigned long long scale)
{
    adam(&buffer[0], adam_seed, &adam_nonce);
    return get_dbl(scale);
}

double adam_dbl(unsigned long long scale, const unsigned char force_regen)
{
    static double (*dbl_fn[])(const unsigned long long) = {
        &get_dbl,
        &regen_dbl
    };
    return dbl_fn[!buff_idx || force_regen](scale);
}

int adam_fill(void *buf, unsigned char width, const unsigned int amount)
{
    if (!amount || amount > 1000000000UL)
        return 1;

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    const u16 one_run   = BUF_SIZE * (64 / width);
    const u32 leftovers = amount & (one_run - 1);

    register long limit = amount >> CTZ(one_run);

    while (limit > 0) {
        adam(&buffer[0], adam_seed, &adam_nonce);
        MEMCPY(buf, &buffer[0], SEQ_BYTES);
        buf += SEQ_BYTES;
        --limit;
    }

    if (LIKELY(leftovers > 0)) {
        adam(&buffer[0], adam_seed, &adam_nonce);
        MEMCPY(buf, &buffer[0], leftovers * (width >> 3));
    }

    return 0;
}

int adam_dfill(double *buf, const unsigned long long multiplier, const unsigned int amount)
{
    if (!amount || amount > 1000000000UL)
        return 1;

    if (multiplier > 1)
        adam_fmrun(buf, &buffer[0], adam_seed, &adam_nonce, amount, multiplier);
    else
        adam_frun(buf, &buffer[0], adam_seed, &adam_nonce, amount);

    return 0;
}

void *adam_choice(void *arr, const unsigned long long size)
{
    return &arr[adam_int(64, false) % size];
}
