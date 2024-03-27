#ifndef UTIL_H
#define UTIL_H
	#include "test.h"
	
	#define   BITBUF_SIZE     			1024
	#define   TESTING_BITS    	        8011776ULL
	#define   TESTING_DBL    		    1000ULL
	#define   BITS_TESTING_LIMIT        1000000
	#define   DBL_TESTING_LIMIT         1500000

	void     get_print_metrics(u16 *center, u16 *indent, u16 *swidth);
	u8       err(const char *s);
	u8       rwseed(u64 *seed, const char *strseed);
	u8       rwnonce(u64 *nonce, const char *strnonce);
	u64      a_to_u(const char *s, const u64 min, const u64 max);
	u8       gen_uuid(const u64 higher, const u64 lower, u8 *buf);
	u8 		 nearest_space(const char *str, u8 offset);
	double	 wh_transform(const u16 idx, const u32 test, const u8 offset);
	void 	 print_ascii_bits(u64 *_ptr, const u64 limit);
	void 	 print_basic_results(const u16 indent, const u64 limit, const basic_test *rsl);
	void	 print_mfreq_results(const u16 indent, const u64 output, const mfreq_test *rsl);
	void	 print_byte_results(const u16 indent, const basic_test *rsl);
	void	 print_range_results(const u16 indent, const u64 output, const range_test *rsl);
	void	 print_ent_results(const u16 indent, const ent_test *rsl);
	void	 print_chseed_results(const u16 indent, const basic_test *rsl);
	void	 print_fp_results(const u16 indent, const u64 output, const fp_test *fp);
	void 	 print_sp_results(const u16 indent, const rng_test *rsl, const u64 *sat_dist, const u64 *sat_range);
	void 	 print_maurer_results(const u16 indent, maurer_test *rsl);
	void	 print_tbt_results(const u16 indent, const tb_test *topo);
	void	 print_vnt_results(const u16 indent, const vn_test *von);
	void	 print_avalanche_results(const u16 indent, const basic_test *rsl, const u64 *ham_dist);
	void 	 print_wht_results(const u16 indent, const wh_test *walsh);
#endif   