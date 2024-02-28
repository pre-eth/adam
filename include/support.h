#ifndef SUPPORT_H
#define SUPPORT_H
	#include "defs.h"

	#define   BITBUF_SIZE     			1024
	
	#define   TESTING_BITS    	        8000000ULL
	#define   TESTING_DBL    		    1000ULL
	#define   BITS_TESTING_LIMIT        100000
	#define   DBL_TESTING_LIMIT         1500000

	void     get_print_metrics(u16 *center, u16 *indent, u16 *swidth);
	u8       err(const char *s);
	u8       rwseed(u64 *seed, const char *strseed);
	u8       rwnonce(u64 *nonce, const char *strnonce);
	u64      a_to_u(const char *s, const u64 min, const u64 max);
	u8       gen_uuid(const u64 higher, const u64 lower, u8 *buf);
	double   stream_ascii(const u64 limit, u64 *seed, u64 *nonce);
	double 	 dbl_ascii(const u32 limit, u64 *seed, u64 *nonce, const u32 multiplier, const u8 precision);
	double   stream_bytes(const u64 limit, u64 *seed, u64 *nonce);
	double	 dbl_bytes(const u32 limit, u64 *seed, u64 *nonce, const u32 multiplier);
	void 	 get_seq_properties(const u64 limit, const unsigned long long *seed, const unsigned long long nonce);
#endif   