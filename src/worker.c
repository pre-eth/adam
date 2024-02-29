#include <pthread.h>
#include <stdlib.h>

#include "../include/rng.h"
#include "../include/simd.h"
#include "../include/worker.h"

void adam_worker_setup(adam_worker *worker, unsigned long long *seed, unsigned long long nonce)
{
    worker->seed[0] = seed[0];
    worker->seed[1] = seed[1];
    worker->seed[2] = seed[2];
    worker->seed[3] = seed[3];
    worker->nonce   = nonce;

    // Output vector
    worker->_ptr = (u64 *restrict) aligned_alloc(SIMD_LEN, BUF_SIZE * sizeof(u64));
    MEMSET(&worker->_ptr[0], 0, sizeof(u64) * BUF_SIZE);

    // Work buffers
    worker->_buffer = (u64 *restrict) aligned_alloc(SIMD_LEN, (BUF_SIZE << 1) * sizeof(u64));
    MEMSET(&worker->_buffer[0], 0, sizeof(u64) * (BUF_SIZE << 1));

    // Chaotic seeds
    worker->_chseeds = (double *restrict) aligned_alloc(SIMD_LEN, 24 * sizeof(double));

    // IV's
    worker->IV[0] = 0x4265206672756974;
    worker->IV[1] = 0x66756C20616E6420;
    worker->IV[2] = 0x6D756C7469706C79;
    worker->IV[3] = 0x2C20616E64207265;
    worker->IV[4] = 0x706C656E69736820;
    worker->IV[5] = 0x7468652065617274;
    worker->IV[6] = 0x68202847656E6573;
    worker->IV[7] = 0x697320313A323829;

    // Counter
    worker->cc = 0;
}

void adam_worker_cleanup(adam_worker *worker)
{
    worker->cc = worker->nonce = 0;
    MEMSET(worker->IV, 0, sizeof(u64) * 8);
    MEMSET(worker->seed, 0, sizeof(u64) * 4);
    free(worker->_ptr);
    free(worker->_buffer);
    free(worker->_chseeds);
}