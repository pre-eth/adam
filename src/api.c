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

u64 adam_int(adam_data data, u8 width, const bool force_regen)
{
    if (width != 8 && width != 16 && width != 32 && width != 64) {
        width = 64;
    }

    if (data->buff_idx + (width >>= 3) >= ADAM_BUF_BYTES || force_regen) {
        adam(data);
    }

    u64 num = 0;
    MEMCPY(&num, (((u8 *) data->out) + data->buff_idx), width);
    data->buff_idx += width;

    return num;
}

double adam_dbl(adam_data data, const u64 scale, const bool force_regen)
{
    return (double) (scale + !scale) * ((double) adam_int(data, 64, force_regen) / (double) __UINT64_MAX__);
}

int adam_fill(adam_data data, void *buf, u8 width, const u64 amount)
{
    if (!amount || amount > ADAM_FILL_MAX) {
        return 1;
    }

    if (width != 8 && width != 16 && width != 32 && width != 64) {
        width = 64;
    }

    // Determine the divisor based on width and bit shifting
    const u16 one_run   = (7 + (64 / width));
    const u32 leftovers = amount & ((1UL << one_run) - 1);

    register long long runs = amount >> one_run;

    while (runs > 0) {
        adam(data);
        MEMCPY(buf, data->out, ADAM_BUF_BYTES);
        buf += ADAM_BUF_BYTES;
        --runs;
    }

    if (LIKELY(leftovers > 0)) {
        adam(data);
        MEMCPY(buf, data->out, leftovers);
    }

    // To force regeneration on next API call for fresh buffer
    data->buff_idx = ADAM_BUF_BYTES;

    return 0;
}

static void dbl_simd_fill(double *buf, u64 *restrict _ptr, const u16 amount)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);

    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&_ptr[i]);
        d1.val[0] = SIMD_MULPD(SIMD_CASTPD(r1.val[0]), range);
        d1.val[1] = SIMD_MULPD(SIMD_CASTPD(r1.val[1]), range);
        d1.val[2] = SIMD_MULPD(SIMD_CASTPD(r1.val[2]), range);
        d1.val[3] = SIMD_MULPD(SIMD_CASTPD(r1.val[3]), range);
        SIMD_STORE4PD(&buf[i], d1);
    } while ((i += 8) < amount);
#else
    const regd range = SIMD_SETPD(1.0 / (double) __UINT64_MAX__);

    reg r1;
    regd d1;

    do {
#ifdef __AVX512F__
        r1 = SIMD_LOADBITS((reg *) &_ptr[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&buf[i], d1);
#else
        r1 = SIMD_LOADBITS((reg *) &_ptr[i]);
        d1 = SIMD_CVTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&buf[i], d1);

        r1 = SIMD_LOADBITS((reg *) &_ptr[i + 4]);
        d1 = SIMD_CVTPD(r1);
        d1 = SIMD_MULPD(d1, range);
        SIMD_STOREPD(&buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

static void dbl_simd_mult(double *buf, const u16 amount, const double multiplier)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq mult = SIMD_SETQPD(multiplier);
    dreg4q d1;

    do {
        d1 = SIMD_LOAD4PD(&buf[i]);
        SIMD_MUL4QPD(d1, d1, mult);
        SIMD_STORE4PD(&buf[i], d1);
    } while ((i += 8) < amount);
#else
    const regd mult = SIMD_SETPD(multiplier);
    regd d1;

    do {
        d1 = SIMD_LOADPD(&buf[i]);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD(&buf[i], d1);

#ifndef __AVX512F__
        d1 = SIMD_LOADPD(&buf[i + 4]);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD(&buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

int adam_dfill(adam_data data, double *buf, const u64 multiplier, const u64 amount)
{
    if (!amount || amount > ADAM_FILL_MAX) {
        return 1;
    }

    register u32 count = 0;
    register u32 out;
    register u32 tmp = amount - count;

    while (tmp >= 8) {
        adam(data);
        out = ((tmp > BUF_SIZE) << 8) | (!(tmp > BUF_SIZE) * tmp);
        dbl_simd_fill(&buf[count], data->out, out);
        count += out;
        tmp = amount - count;
    }

    const u8 leftovers = amount & 7;

    if (LIKELY(leftovers)) {
        adam(data);
        register u8 i = 0;
        do {
            buf[count] = (double) data->out[i++] / (double) __UINT64_MAX__;
        } while (++count < amount);
    }

    if (multiplier > 1) {
        const double mult = multiplier;
        if (amount >= 8) {
            dbl_simd_mult(buf, amount, mult);
        }

        if (LIKELY(leftovers)) {
            count = amount - leftovers;
            do {
                buf[count] *= mult;
            } while (++count < amount);
        }
    }

    data->buff_idx = ADAM_BUF_BYTES;

    return 0;
}

void *adam_choice(adam_data data, void *arr, const u64 size)
{
    return &arr[adam_int(data, 64, false) % size];
}

u64 adam_stream(adam_data data, const u64 output, const char *file_name)
{
    if (output < ADAM_BUF_BITS) {
        return 0;
    }

    if (file_name != NULL) {
        freopen(file_name, "wb+", stdout);
    }

    const u16 leftovers = output & (ADAM_BUF_BITS - 1);

    register u64 written = 0;
    while (output - written >= ADAM_BUF_BITS) {
        adam(data);
        fwrite(data->out, sizeof(u64), BUF_SIZE, stdout);
        written += ADAM_BUF_BITS;
    }

    if (LIKELY(leftovers)) {
        adam(data);
        fwrite(data->out, sizeof(u64), leftovers >> 6, stdout);
        written += leftovers;
    }

    // To force regeneration on next API call for fresh buffer
    data->buff_idx = ADAM_BUF_BYTES;

    return written;
}

void adam_cleanup(adam_data data)
{
    MEMSET(data, 0, sizeof(*data));
    free(data);
}
