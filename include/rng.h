#ifndef RNG_H
#define RNG_H
    #include "defs.h"

    /* 
        Size of the internal chaotic buffer used to generate numbers
        The buffer is of type [u64; 256] with size 256 * 8 = 2048 bytes
    */
    #define MAGNITUDE               8
    #define BUF_SIZE                (1U << MAGNITUDE)   
    #define WORD_BITS               64
    #define WORD_SIZE               (WORD_BITS >> 3)

    // Number of rounds we perform at a time
    #define ROUNDS                  8

    /*
        The CSPRNG prepares itself for number generation using the function:

            3.9999 * X * (1 - X)

        CHFUNCTION is used for updating internal state using the output of the
        chaotic function

        CHMANTISSA is for extracting the lower 32 bits of the chaotic value's
        binary representation. 

        Must take the mod 7 of x because it will be the data->buff_idx member
    */
    #define COEFFICIENT             3.9999
    #define CHFUNCTION(x, ch)       (COEFFICIENT * ch[x & 7] * (1.0 - ch[x & 7]))
    #define CHMANT32(x, ch)         (*((u64 *) &ch[x & 7]) & 0xFFFFFFFF) // 4503599627370495
    

    // To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random int casted to double D
    #define RANGE_LIMIT             2.7105054E-20

    /*  The main primitives for using ADAM's algorithm  */

    void initialize(const u64 *restrict seed, const u64 nonce, u64 *restrict out, u64 *restrict mix);
    void accumulate(u64 *restrict out, u64 *restrict mix, double *restrict chseeds);
    void diffuse(u64 *restrict out, u64 *restrict mix, const u64 nonce);
    void apply(u64 *restrict out, double *restrict chseeds);
    void mix(u64 *restrict out, const u64 *restrict mix);
    u64 generate(u64 *restrict out, u8 *restrict idx, double *restrict chseeds);
#endif
