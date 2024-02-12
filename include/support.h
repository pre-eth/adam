#ifndef SUPPORT_H
#define SUPPORT_H
	#include "defs.h"

	#define BITBUF_SIZE     1024
						
	#define TESTING_BITS    1000000ULL
	#define TESTING_LIMIT   8000

	typedef struct rng_test rng_test;

	void     get_print_metrics(u16 *center, u16 *indent, u16 *swidth);
	u8       err(const char *s);
	u8       rwseed(u64 *seed, const char *strseed);
	u8       rwnonce(u64 *nonce, const char *strnonce);
	u64      a_to_u(const char *s, const u64 min, const u64 max);
	u8       gen_uuid(const u64 higher, const u64 lower, u8 *buf);
	double   stream_ascii(const u64 limit, u64 *seed, u64 *nonce);
	double   stream_bytes(const u64 limit, u64 *seed, u64 *nonce);
	double 	 get_seq_properties(const u64 limit, rng_test *rsl);
#endif