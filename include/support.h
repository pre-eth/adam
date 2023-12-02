#ifndef SUPPORT_H
#define SUPPORT_H
    #include "defs.h"

    #define BITBUF_SIZE     1024

    #define PRINT_4(i, j)   print_binary(_bptr + i,       _ptr[(j) + 0]), \
                            print_binary(_bptr + 64 + i,  _ptr[(j) + 1]), \
                            print_binary(_bptr + 128 + i, _ptr[(j) + 2]), \
                            print_binary(_bptr + 192 + i, _ptr[(j) + 3])  

    #define TESTING_BITS    1000000ULL
    #define TESTING_LIMIT   8000UL

    typedef struct rng_data rng_data;

    u8      get_print_metrics(u16 *center, u16 *indent, u16 *swidth);
    u8      err(const char *s);
    u8      rwseed(u64 *seed, const char *strseed);
    u8      rwnonce(u64 *nonce, const char *strnonce);
    u64     a_to_u(const char *s, const u64 min, const u64 max);
    u8      gen_uuid(u64 *_ptr, u8 *buf);
    double  stream_ascii(const u64 limit, rng_data *data);
    double  stream_bytes(const u64 limit, rng_data *data);
    double  examine(const char *strlimit, rng_data *data);
#endif