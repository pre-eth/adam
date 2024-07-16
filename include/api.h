#ifndef ADAM_API_H
#define ADAM_API_H
    #include "defs.h"

#if defined(__AARCH64_SIMD__) || defined(__AVX512F__)
    #define ADAM_ALIGNMENT          64
#else
    #define ADAM_ALIGNMENT          32
#endif

    #define ADAM_WORD_BITS          64
    #define ADAM_WORD_SIZE          (ADAM_WORD_BITS >> 3)
    #define ADAM_SEED_SIZE          (ADAM_WORD_SIZE * 4)
    #define ADAM_NONCE_SIZE         (sizeof(u32) * 3)
    #define ADAM_FILL_MAX           1000000000

    typedef struct adam_data_s  *adam_data;

    typedef enum {
        UINT8  = 1,
        UINT16 = 2,
        UINT32 = 4,
        UINT64 = 8
    } NumWidth;

    /*
        Configures ADAM's initial state by allocating the adam_data
        struct and preparing it for random generation.

        Call this ONCE at the start of your program, before generating 
        any numbers.

        Params <seed> and <nonce> are optional - set to NULL if you'd like
        to seed the generator with secure random bytes from the operating
        system itself. Otherwise, the caller must ensure that <seed> points
        to 256 bits of data [u64; 4], and that nonce points to 96 bits [u32; 3].

        Returns an opaque pointer to the internal adam_data_s struct. Make
        sure you remember to adam_cleanup() once you no longer need it!
    */
    adam_data adam_setup(u64 *seed, u32 *nonce);

    /*
        Resets and reconstructs ADAM's internal state based on a new
        <seed> and <nonce>. 

        If provided, the caller must ensure that <seed> points to 256 bits
        of data [u64; 4], and that nonce points to 96 bits [u32; 3]. If they
        do not, then the program is ill-formed.

        Returns 0 on success.
    */
    int adam_reset(adam_data data, u64 *seed, u32 *nonce);

    /*
        Save the current seed and nonce, if you didn't set it yourself and
        would like to record the parameters.

        The caller must ensure that <seed> points to 256 bits of data 
        [u64; 4], and that nonce points to 96 bits [u32; 3]. If they do 
        not, then the program is ill-formed.

        Returns 0 on success.
    */
    int adam_record(adam_data data, u64 *seed, u32 *nonce);

    /*
        Returns a random unsigned integer of the specified <width>.

        Param <width> must ALWAYS be a member of the NumWidth enum. Other
        arbitrary values will probably result in undefined behavior or a
        seg fault. Your program is ill-formed if other values are passed!
    */
    u64 adam_int(adam_data data, const NumWidth width);

    /*
        Returns a random double after multiplying it by param <scale>.

        For no scaling factor, just set <scale> to 1. Also, a <scale> value
        of 0 is ignored and treated as 1.
    */
    double adam_dbl(adam_data data, const u64 scale);

    /*
        Fills a given buffer with random integers.

        The caller is responsible for ensuring param <buf> is of at least 
        <amount> * sizeof(u64) bytes in length, and that the pointer is not 
        NULL. If <amount> is 0 or greater than 1 billion, this function  
        will return 1 and exit.

        Param <width> must ALWAYS be a member of the NumWidth enum. Other
        arbitrary values will probably result in undefined behavior or a
        seg fault. Your program is ill-formed if other values are passed!

        Returns 0 on success, 1 on error.
    */
    int adam_fill(adam_data data, void *buf, const NumWidth width, const size_t amount);

    /*
        Fills a given buffer with random doubles.

        The caller is responsible for ensuring param <buf> is of at least 
        <amount> * sizeof(double) bytes in length, and that the pointer is 
        not NULL. If <amount> is 0 or greater than 1 billion, this function 
        will return 1 and exit.

        Param <multiplier> can be supplied to multiply all doubles by a certain 
        scaling factor so they fall within the range (0, <multiplier>). If you
        do not need a scaling factor, just pass a value of 1 or 0.

        Returns 0 on success, 1 on error.
    */
    int adam_dfill(adam_data data, double *buf, const u64 multiplier, const size_t amount);

    /*
        Chooses a random item from a provided collection, where param <arr> is a 
        pointer to this collection, and param <size> is to specify the total 
        range of possible indices, usually the full length of the array but you
        could pass in a smaller number than that if you want to choose from a 
        particular range or within a specific radius.

        Caller must guarantee <size> is NEVER larger than the <arr>'s capacity.

        Returns a randomly picked member of <arr>.
    */
    void *adam_choice(adam_data data, void *arr, const size_t size);

    /*
        Writes param <amount> BITS (not bytes!) to a file descriptor of your choice.

        You can pass NULL for param <file_name> if you'd like to stream to stdout,
        rather than an actual file. If you provide a valid file name, then it will
        be created and saved with the requested amount of binary data.

        If param <amount> is less than 64, this function won't do anything.

        Returns the total number of bits written out, or 0 if invalid value for <amount>.
    */
    size_t adam_stream(adam_data data, const size_t amount, const char *file_name);

    /*
        Zeroizes adam_data members and frees any allocated memory.

        Call this ONCE after you are finished using the generator.
    */
    void adam_cleanup(adam_data data);
#endif
