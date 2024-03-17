#ifndef SUPPORT_H
#define SUPPORT_H
	#include "defs.h"
	
	#define   BITBUF_SIZE     			1024
	#define   TESTING_BITS    	        8011776ULL
	#define   TESTING_DBL    		    1000ULL
	#define   BITS_TESTING_LIMIT        1000000
	#define   DBL_TESTING_LIMIT         1500000

	// Forward declarations - see test.h and ent.h
	typedef struct rng_test rng_test;
	typedef struct ent_test ent_test;

	void     get_print_metrics(u16 *center, u16 *indent, u16 *swidth);
	u8       err(const char *s);
	u8       rwseed(u64 *seed, const char *strseed);
	u8       rwnonce(u64 *nonce, const char *strnonce);
	u64      a_to_u(const char *s, const u64 min, const u64 max);
	u8       gen_uuid(const u64 higher, const u64 lower, u8 *buf);
	u8 		 nearest_space(const char *str, u8 offset);
	double	 wh_transform(const u16 idx, const u32 test);
	void 	 print_ascii_bits(u64 *_ptr, const u64 limit);
	void 	 print_basic_results(const u16 indent, const rng_test *rsl, const u64 limit);
	void	 print_mfreq_results(const u16 indent, const rng_test *rsl);
	void	 print_byte_results(const u16 indent, const rng_test *rsl, const u8 *restrict mcb, const u8 *restrict lcb);
	void	 print_range_results(const u16 indent, const rng_test *rsl, const u64 *restrict range_dist);
	void	 print_ent_results(const u16 indent, const ent_test *ent);
	void	 print_chseed_results(const u16 indent, const u64 expected, const u64 *chseed_dist, double avg_chseed);
	void	 print_fp_results(const u16 indent, const rng_test *rsl, const u64 *restrict fpf_dist, const u64 *restrict fpf_quad, const u64 *fp_perm_dist, const u64 *fp_max_dist);
	void 	 print_sp_results(const u16 indent, const rng_test *rsl, const u64 *sat_dist, const u64 *sat_range);
	void 	 print_maurer_results(const u16 indent, rng_test *rsl, const u64 sequences);
	void	 print_tbt_results(const u16 indent, const u64 sequences, const u64 tbt_prop, const u64 tbt_pass);
	void 	 print_wht_results(const u16 indent, const double fisher_value, const u64 seq_pass, const u64 num_pass, const u64 seq, const u64 *pdist);
	void	 print_avalanche_results(const u16 indent, const rng_test *rsl, const u64 *ham_dist);
#endif   