#ifndef ADAM_H
#define ADAM_H
	#include <stdbool.h>

  #define ADAM_BUF_SIZE     256

  // Data for RNG process
  typedef struct rng_data {
    //  Output type (0 = INT, 1 = DOUBLE)
    bool dbl_mode; 

    //  Current index in output vector
    unsigned char index;       

    //  256-bit seed/key
    unsigned long long seed[4];          

    //  64-bit nonce      
    unsigned long long nonce;              

    //  State variable 1    
    unsigned long long aa;      

    //  State variable 2               
    unsigned long long bb;              
  } rng_data;

  /* LIBRARY API */

  /*
    Initializes the rng_data struct and configures its initial state.
    
    Call this ONCE at the start of your program, before you generate any 
    numbers. Set param "gen_dbls" to true to get double precision numbers.
  */
  void  adam_init(rng_data *data, bool generate_dbls);

  /*
    Automatically makes internal calls to the adam_run function when 
    regeneration is needed to ensure that you can safely expect to call 
    this function and always receive a randomly generated number.

    Param <width> must ALWAYS be either 8, 16, 32, or 64. Any other
    value will make this function return 1 to let you know there's an 
    error with the width parameter. When floating point output mode is
    enabled, this field is ignored.

    Caller must guarantee that param <output> is of at least (<width> / 8)
    bytes in size, unless dbl_mode is set to true, in which case <output>
    size is assumed to be of sizeof(double).

    Optional param <duration> can be used to accumulate the total amount 
    of time taken by the number generation process. Set this to NULL
    if you don't need this.

    Returns 0 on success, 1 on error
  */
  int   adam_get(void *output, rng_data *data, const unsigned char width, double *duration);

  /*
    Fills a given buffer with random integers or doubles.

    Caller is responsible for ensuring param <buffer> is of at least
    param <amount> * sizeof(u64 || double) bytes in length, and that 
    the pointer is not NULL. If <amount> is greater than 1 billion, 
    this function will return 1 and terminate early.

    Optional param <duration> can be used to accumulate the total
    amount of time taken by the number generation process. Set this 
    to NULL if you don't need this.

    Returns 0 on success, 1 on error
  */
  int   adam_fill(void *buf, rng_data *data, const unsigned int amount, double *duration);
#endif