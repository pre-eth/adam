#ifndef ADAM_H
#define ADAM_H
	#include <stdbool.h>

#ifdef __AARCH64_SIMD__
  #define ADAM_ALIGNMENT    64
#else
#ifdef __AVX512F__
  #define ADAM_ALIGNMENT    64
#else
  #define ADAM_ALIGNMENT    32
#endif
#endif

  #define ADAM_BUF_SIZE     256

  // Data for RNG process
  typedef struct adam_data {
    //  Output type (0 = INT, 1 = DOUBLE)
    bool dbl_mode; 

    //  Current index in output vector
    unsigned char index;       

    //  256-bit seed/key
    unsigned long long seed[4];          

    //  64-bit nonce      
    unsigned long long nonce;                        
  } adam_data;

  /*
    Initializes the adam_data struct and configures its initial state.
    
    Call this ONCE at the start of your program, before you generate any 
    numbers. Set param "gen_dbls" to true to get double precision numbers.

    Params <seed> and <nonce> are optional - set to NULL if you'd like to seed
    the generator with secure random bytes from the operating system itself.
    Otherwise, the caller must ensure that <seed> points to 256-bits of data,
    and that nonce points to a u64.
  */
  void adam_setup(adam_data *data, bool generate_dbls, unsigned long long *seed, unsigned long long *nonce);

  /*
    Returns a random unsigned integer of the specified <width>.
  
    Param <width> must ALWAYS be either 8, 16, 32, or 64. Any other
    value will be ignored and revert to the default width of 64.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated integer.
  */
  unsigned long long adam_int(adam_data *data, unsigned char width);

  /*
    Returns a random double after multiplying it by param <scale>.
  
    For no scaling factor, just set <scale> to 1. Also, a scale value
    of 0 is ignored and treated as 1.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated double.
  */
  double adam_dbl(adam_data *data, unsigned long long scale);

  /*
    Fills a given buffer with random integers.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(u64) bytes in length, and that the pointer is not 
    NULL. If <amount> is 0 or greater than 125 million, this function  
    will return 1 and terminate early.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <width> must be 8, 16, 32, or 64. If you provide an invalid 
    width value, the default of 64 is used instead and the argument is 
    ignored.

    Returns 0 on success, 1 on error
  */
  int adam_fill(adam_data *data, void *buf, unsigned char width, const unsigned int amount);

  /*
    Fills a given buffer with random doubles.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(double) bytes in length, and that the pointer is 
    not NULL. If <amount> is 0 or greater than 125 million, this function 
    will return 1 and terminate early.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <multiplier> can be supplied to multiply all doubles by a certain 
    scaling factor so they fall within the range (0, <multiplier>). If you
    do not need a scaling factor, just pass a value of 1.

    Returns 0 on success, 1 on error
  */
  int adam_dfill(adam_data *data, double *buf, const unsigned long long multiplier, const unsigned int amount);

#endif