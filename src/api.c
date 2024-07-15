#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>

#include "../include/api.h"
#include "../include/rng.h"

struct adam_data_s {
    // 256-bit seed
    u64 seed[4];

    // 64-bit nonce
    u32 nonce[3];

    // Chaotic work buffer - 256 64-bit integers = 2048 bytes
    u64 out[BUF_SIZE] ALIGN(ADAM_ALIGNMENT);

    // The seeds supplied to each iteration of the chaotic function
    double chseeds[ROUNDS] ALIGN(ADAM_ALIGNMENT);

    // Current index in buffer
    u8 buff_idx;
};

adam_data adam_setup(u64 *seed, u32 *nonce)
{
    // Allocate the struct
    adam_data data = aligned_alloc(ADAM_ALIGNMENT, sizeof(*data));
    if (data == NULL) {
        return NULL;
    }

    adam_reset(data, seed, nonce);

    return data;    
}

int adam_reset(adam_data data, u64 *seed, u32 *nonce)
{
    // Get nonce from secure system RNG, or use user provided nonce
    if (nonce == NULL) {
        getentropy(data->nonce, ADAM_NONCE_SIZE);
    } else {
        data->nonce[0] = nonce[0];
        data->nonce[1] = nonce[1];
        data->nonce[2] = nonce[2];
    }

    // Get seed bytes from secure system RNG, or use user provided seed
    if (seed == NULL) {
        getentropy(data->seed, ADAM_SEED_SIZE);
    } else {
        data->seed[0] = seed[0];
        data->seed[1] = seed[1];
        data->seed[2] = seed[2];
        data->seed[3] = seed[3];
    }

    // Intermediate chaotic mix - WORD_SIZE * 8 = 64 bytes
    u64 mix_arr[ROUNDS] ALIGN(ADAM_ALIGNMENT);

    // Create IV's and initialize chaotic mix
    initialize(data->seed, *((u64 *)&data->nonce[0]), data->out, mix_arr);

    // Accumulate set of chaotic seeds
    accumulate(data->out, mix_arr, data->chseeds);

    // Diffuse our internal work buffer
    diffuse(data->out, mix_arr, *((u64 *)&data->nonce[1]));

    // Apply the chaotic function to the buffer
    apply(data->out, data->chseeds);

    // Set index to random starting point
    // We do this BEFORE mix() so that the byte used here is replaced
    data->buff_idx = data->out[BUF_SIZE - 1] & (BUF_SIZE - 1);

    // Mix buffer one last time before it's ready for use!
    mix(data->out, mix_arr);

    return 0;
}

int adam_record(adam_data data, u64 *seed, u32 *nonce)
{
    MEMCPY(seed, data->seed, ADAM_SEED_SIZE);
    MEMCPY(nonce, data->nonce, ADAM_NONCE_SIZE);
    return 0;
}

u64 adam_int(adam_data data, const NumWidth width)
{
    #define WIDTH_MASK(w)   ((((1ULL << ((w << 3) - 1)) - 1) << 1) | 1)

    return generate(data->out, &data->buff_idx, data->chseeds) & WIDTH_MASK(width);
}

double adam_dbl(adam_data data, const u64 scale)
{
    return (double) (scale + !scale) * ((double) adam_int(data, UINT64) / (double) __UINT64_MAX__);
}

int adam_fill(adam_data data, void *buf, const NumWidth width, const size_t amount)
{
    if (!amount || amount > ADAM_FILL_MAX) {
        return 1;
    }

    const size_t total = amount * width;
    
    register size_t written = 0;

    u8 *_ptr = (u8 *) buf;
    u64 tmp;

    while (total - written >= ADAM_WORD_SIZE) {
        tmp = adam_int(data, UINT64);
        MEMCPY(&_ptr[written], &tmp, ADAM_WORD_SIZE);
        written += ADAM_WORD_SIZE;
    }

    if (total - written > 0) {
        tmp = adam_int(data, UINT64);
        MEMCPY(&_ptr[written], &tmp, total - written);
    }

    return 0;
}

int adam_dfill(adam_data data, double *buf, const u64 multiplier, const size_t amount)
{
    if (!amount || amount > ADAM_FILL_MAX) {
        return 1;
    }

    const double mult = multiplier + !multiplier;

    register size_t count = 0;

    do {
        buf[count] = mult * ((double) adam_int(data, UINT64) / (double) __UINT64_MAX__);
    } while (++count < amount);

    return 0;
}

void *adam_choice(adam_data data, void *arr, const size_t size)
{
    return &arr[adam_int(data, UINT64) % size];
}

size_t adam_stream(adam_data data, const size_t amount, const char *file_name)
{
    if (amount < ADAM_WORD_BITS) {
        return 0;
    }

    if (file_name != NULL) {
        freopen(file_name, "wb+", stdout);
    }

    register size_t written = 0;
    u64 tmp;
    while (amount - written >= ADAM_WORD_BITS) {
        tmp = adam_int(data, UINT64);
        fwrite(&tmp, ADAM_WORD_SIZE, 1, stdout);
        written += ADAM_WORD_BITS;
    }

    return written;
}

void adam_cleanup(adam_data data)
{
    MEMSET(data, 0, sizeof(*data));
    free(data);
}
