#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>

#include "../include/rng.h"
#include "../include/api.h"

struct adam_data_s {
    // 256-bit seed
    u64 seed[4];

    // 64-bit nonce
    u64 nonce;

    // Output vector (work buffer) - 256 64-bit integers = 2048 bytes
    u64 out[BUF_SIZE] ALIGN(ADAM_ALIGNMENT);

    // Chaotic state maps - sizeof(u64) * 512 = 4096 bytes
    u64 state_maps[BUF_SIZE << 1] ALIGN(ADAM_ALIGNMENT);

    // The seeds supplied to each iteration of the chaotic function
    double chseeds[ROUNDS << 2] ALIGN(ADAM_ALIGNMENT);

    // Work array to store intermediate mix results
    u64 work_rsl[8] ALIGN(ADAM_ALIGNMENT);

    // Current index in buffer (as bytes)
    u16 buff_idx;
};

static void adam(adam_data data)
{
    apply(data->out, data->state_maps, data->chseeds, data->work_rsl);
    mix(data->out, data->state_maps);
    data->buff_idx = 0;
}

adam_data adam_setup(u64 *seed, u64 *nonce)
{
    // Allocate the struct
    adam_data data = aligned_alloc(ADAM_ALIGNMENT, sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    
    // Get nonce from secure system RNG, or use user provided nonce
    if (nonce == NULL) {
        getentropy(&data->nonce, sizeof(u64));
    } else {
        data->nonce = *nonce; 
    }

    /*
        The algorithm requres 3 u64 buffers of size BUF_SIZE - 2 internal maps used for generating
        the chaotic function output, and an output vector. 
        
        The final output vector isn't stored contiguously so the pointer can be returned to the 
        user without any fears of them indexing into the internal state maps.

        aligned_alloc + memset is used rather than calloc to use the appropriate SIMD alignment.

        We only need to memset the first 8 bytes of the array containing the state maps, because
        the ISAAC_MIX diffuses and assigns the previous 512 bits to the current section of 512 bits
    */
    MEMSET(data->state_maps, 0, sizeof(u64) * 8);

    /*
        8 64-bit IV's that correspond to the verse:
        "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
    */
    data->out[0] = 0x4265206672756974;
    data->out[1] = 0x66756C20616E6420;
    data->out[2] = 0x6D756C7469706C79;
    data->out[3] = 0x2C20616E64207265;
    data->out[4] = 0x706C656E69736820;
    data->out[5] = 0x7468652065617274;
    data->out[6] = 0x68202847656E6573;
    data->out[7] = 0x697320313A323829;

    // Get seed bytes from secure system RNG, or use user provided seed
    if (seed == NULL) {
        getentropy(&data->seed[0], sizeof(u64) * 4);
    } else {
        data->seed[0] = seed[0];
        data->seed[1] = seed[1];
        data->seed[2] = seed[2];
        data->seed[3] = seed[3];
    }
    
    // Mix IV's with different configurations of seed values
    data->out[0] ^= data->seed[0];
    data->out[1] ^= (~data->seed[1] << (data->nonce & 63)) | (~data->seed[3] >> (data->nonce & 63));
    data->out[2] ^= data->seed[1];
    data->out[3] ^= (~data->seed[0] << 32) | (~data->seed[2] >> 32);
    data->out[4] ^= data->seed[2];
    data->out[5] ^= (~data->seed[2] << (data->nonce & 63)) | (~data->seed[0] >> (data->nonce & 63));
    data->out[6] ^= data->seed[3];
    data->out[7] ^= (~data->seed[3] << 32) | (~data->seed[1] >> 32);

    // Initialize chaotic seeds, work buffer, and chaotic maps
    accumulate(data->out, data->work_rsl, data->chseeds);

    data->work_rsl[0] ^= data->nonce;
    data->work_rsl[1] ^= data->nonce;
    data->work_rsl[2] ^= data->nonce;
    data->work_rsl[3] ^= data->nonce;
    data->work_rsl[4] ^= data->nonce;
    data->work_rsl[5] ^= data->nonce;
    data->work_rsl[6] ^= data->nonce;
    data->work_rsl[7] ^= data->nonce;

    diffuse(data->out, 256, data->work_rsl);
    diffuse(data->state_maps, 512, data->work_rsl);

    // Get first batch of results
    adam(data);

    return data;
}

u64 *adam_seed(const adam_data data)
{
    return &data->seed[0];
}

u64 *adam_nonce(const adam_data data)
{
    return &data->nonce;
}

const u64 *adam_buffer(const adam_data data)
{
    adam(data);
    return &data->out[0];
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
