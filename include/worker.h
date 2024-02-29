#ifndef ADAM_THREADS_H
#define ADAM_THREADS_H
    #include "defs.h"

    typedef struct adam_worker {
        u64 *restrict _ptr;
        u64 *restrict _buffer;
        u64 IV[8];
        u64 seed[4];
        u64 nonce;
        double *restrict _chseeds;
        u8 cc;
    } adam_worker;

    void adam_worker_setup(adam_worker *worker, unsigned long long *seed, unsigned long long nonce);
    void adam_worker_cleanup(adam_worker *worker);
#endif