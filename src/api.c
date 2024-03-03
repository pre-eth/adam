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

    //  Current index in buffer (as bytes)
    u16 buff_idx;
};

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

    data->cc = data->buff_idx = 0;

    return data;
}

static void adam(adam_data data)
{
    accumulate(data->seed, data->IV, data->chseeds, data->cc);
    diffuse(data->out, data->nonce);
    apply(data->out, data->work_buffer, data->chseeds);
    mix(data->out, data->work_buffer);
    reseed(data->seed, data->work_buffer, &data->nonce, &data->cc);
    data->buff_idx = 0;
}

u64 *adam_seed(adam_data data)
{
    return &data->seed[0];
}

u64 *adam_nonce(adam_data data)
{
    return &data->nonce;
}

const u64 *const adam_buffer(adam_data data)
{
    return &data->out[0];
}

u64 adam_int(adam_data data, u8 width, const u8 force_regen)
{
    if (data->buff_idx == ADAM_BUF_BYTES || force_regen)
        adam(data);

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    u64 num = 0;
    MEMCPY(&num, (u8 *)&data->out[data->buff_idx], width >> 3);
    data->buff_idx += width >> 3;

    return num;
}

double adam_dbl(adam_data data, const u64 scale, const u8 force_regen)
{
    if (data->buff_idx == ADAM_BUF_BYTES || force_regen)
        adam(data);

    return ((double) adam_int(data, 64, false) / (double) __UINT64_MAX__) * (double) (scale + !scale);
}

int adam_fill(adam_data data, void *buf, u8 width, const u64 amount)
{
    if (!amount || amount > 1000000000UL)
        return 1;

    if (width != 8 && width != 16 && width != 32 && width != 64)
        width = 64;

    const u16 one_run   = amount * (64 / width);
    const u32 leftovers = amount % (one_run - 1);

    register long long limit = amount >> CTZ(one_run);

    while (limit > 0) {
        adam(data);
        MEMCPY(buf, data->out, SEQ_BYTES);
        buf += SEQ_BYTES;
        --limit;
    }

    if (LIKELY(leftovers > 0)) {
        adam(data);
        MEMCPY(buf, data->out, leftovers * (width >> 3));
        // To force regeneration on next API call for fresh buffer
        data->buff_idx = ADAM_BUF_BYTES;
    }

    return 0;
}

void dbl_simd_fill(double *buf, u64 *restrict _ptr, const u16 amount)
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
        r1 = SIMD_LOAD64((reg *) &_ptr[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

void dbl_simd_fill_mult(double *buf, u64 *restrict _ptr, const u16 amount, const u64 multiplier)
{
    register u16 i = 0;

#ifdef __AARCH64_SIMD__
    const dregq range = SIMD_SETQPD(1.0 / (double) __UINT64_MAX__);
    const dregq mult  = SIMD_SETQPD((double) multiplier);
    reg64q4 r1;
    dreg4q d1;

    do {
        r1        = SIMD_LOAD64x4(&_ptr[i]);
        d1.val[0] = SIMD_MULPD(SIMD_CASTPD(r1.val[0]), range);
        d1.val[1] = SIMD_MULPD(SIMD_CASTPD(r1.val[1]), range);
        d1.val[2] = SIMD_MULPD(SIMD_CASTPD(r1.val[2]), range);
        d1.val[3] = SIMD_MULPD(SIMD_CASTPD(r1.val[3]), range);
        SIMD_MUL4QPD(d1, d1, mult);
        SIMD_STORE4PD(&buf[i], d1);
    } while ((i += 8) < amount);
#else
    const regd range = SIMD_SETPD(1.0 / (double) __UINT64_MAX__);
    const regd mult  = SIMD_SETPD((double) multiplier);

    reg r1;
    regd d1;

    do {
        r1 = SIMD_LOAD64((reg *) &_ptr[i]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i], d1);

#ifndef __AVX512F__
        r1 = SIMD_LOAD64((reg *) &_ptr[i + 4]);
        d1 = SIMD_CASTPD(r1);
        d1 = SIMD_MULPD(d1, max);
        d1 = SIMD_MULPD(d1, mult);
        SIMD_STOREPD((regd *) &buf[i + 4], d1);
#endif
    } while ((i += 8) < amount);
#endif
}

static void frun(adam_data data, double *buf, const u32 amount)
{
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        adam(data);
        tmp = amount - count;
        out = ((tmp > BUF_SIZE) << 8) | (!(tmp > BUF_SIZE) * tmp);
        dbl_simd_fill(&buf[count], data->out, out);
        count += out;
    }

    const u8 leftovers = amount & 7;

    if (LIKELY(leftovers)) {
        adam(data);
        register u8 i = 0;
        do
            buf[count] = (double) data->out[i++] / (double) __UINT64_MAX__;
        while (++count < amount);
    }
}

static void fmrun(adam_data data, double *buf, const u32 amount, const u64 multiplier)
{
    register u32 count = 0;
    register u32 out, tmp;

    while (amount - count > 8) {
        adam(data);
        tmp = amount - count;
        out = ((tmp >= BUF_SIZE) << 8) | (!(tmp >= BUF_SIZE) * tmp);
        dbl_simd_fill_mult(&buf[count], data->out, out, multiplier);
        count += out;
    }

    if (LIKELY(amount & 7)) {
        adam(data);
        const double mult = (double) multiplier;
        register u8 i     = 0;
        do
            buf[count] = ((double) data->out[i++] / (double) __UINT64_MAX__) * mult;
        while (++count < amount);
    }
}

int adam_dfill(adam_data data, double *buf, const u64 multiplier, const u32 amount)
{
    if (!amount || amount > 1000000000)
        return 1;

    if (multiplier > 1)
        fmrun(data, buf, amount, multiplier);
    else
        frun(data, buf, amount);

    data->buff_idx = ADAM_BUF_BYTES;

    return 0;
}

void *adam_choice(void *arr, const unsigned long long size)
{
    return &arr[adam_int(64, false) % size];
}
