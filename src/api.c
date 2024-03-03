#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>

#include "../include/defs.h"
#include "../include/simd.h"
#include "../include/rng.h"
#include "../include/api.h"

struct adam_data_s {
    // 256-bit seed
    u64 seed[4];

    // 64-bit nonce
    u64 nonce;

    // 8 64-bit initialization vectors part of internal state
    u64 IV[8];

    // Output vector - 256 64-bit integers
    u64 *out;

    // Work maps - sizeof(u64) * 512
    u64 *work_buffer;

    // The seeds supplied to each iteration of the chaotic function
    double chseeds[ROUNDS << 2];

    // Counter
    u64 cc;
};

//  Current index in buffer (as bytes)
static u16 buff_idx;

adam_data adam_setup(const u64 *seed, const u64 *nonce)
{
    // Allocate the struct
    adam_data data = aligned_alloc(ADAM_ALIGNMENT, sizeof(struct adam_data_s));

    // Get seed and nonce bytes from secure system RNG, or use user provided one(s)
    if (seed == NULL)
        getentropy(&data->seed[0], sizeof(u64) << 2);
    else
        MEMCPY(&data->seed[0], &seed[0], sizeof(u64) << 2);

    if (nonce == NULL)
        getentropy(&data->nonce, sizeof(u64));
    else
        data->nonce = *nonce;

    /*
        The algorithm requres 3 u64 buffers of size BUF_SIZE - 2 internal maps used for generating
        the chaotic function output, and an output vector. 
        
        The final output vector isn't stored contiguously so the pointer can be returned to the 
        user without any fears of them indexing into the internal state maps.

        aligned_alloc + memset is used rather than calloc to use the appropriate SIMD alignment.
    */
    data->out = aligned_alloc(ADAM_ALIGNMENT, ADAM_BUF_BYTES);
    MEMSET(&data->out[0], 0, sizeof(u64) * BUF_SIZE);
    data->work_buffer = aligned_alloc(ADAM_ALIGNMENT, ADAM_BUF_BYTES * 2);
    MEMSET(&data->work_buffer[0], 0, sizeof(u64) * (BUF_SIZE << 1));

    /*
        8 64-bit IV's that correspond to the verse:
        "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
    */
    data->IV[0] = 0x4265206672756974;
    data->IV[1] = 0x66756C20616E6420;
    data->IV[2] = 0x6D756C7469706C79;
    data->IV[3] = 0x2C20616E64207265;
    data->IV[4] = 0x706C656E69736820;
    data->IV[5] = 0x7468652065617274;
    data->IV[6] = 0x68202847656E6573;
    data->IV[7] = 0x697320313A323829;

    data->cc = 0;

    return data;
}

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
