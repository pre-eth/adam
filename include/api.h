#ifndef ADAM_API_H
#define ADAM_API_H
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

  /*    STANDARD API    */

  /*
    Configures ADAM's initial state.
    
    Call this ONCE at the start of your program, before generating any numbers.

    Params <seed> and <nonce> are optional - set to NULL if you'd like to seed
    the generator with secure random bytes from the operating system itself.
    Otherwise, the caller must ensure that <seed> points to 256-bits of data,
    and that nonce points to a u64.
  */
  void adam_setup(unsigned long long *seed, unsigned long long *nonce);

  /*
    Self-explanatory functions - return a raw pointer to the set of chaotic
    seeds, current seed, current nonce, and output vector respectively

    When working with the raw buffer, remember the ADAM_BUF_SIZE when doing
    any indexing or pointer math. adam_rng_buffer() guarantees that the
    return pointer contains at least 256 randomly generated 64-bit values.

    You can modify the buffer if you'd like, but it will have no effect, as
    the buffer is managed internally and only returned for inspection.

    NOTE: Upon receiving the output vector pointer, the internal index for
    the vector is reset, meaning the vector will be fully regenerated on ANY
    future API calls. This forced regeneration is done because once ADAM hands
    off the raw pointer to you, it assumes you use the entire buffer. And it's okay
    even if you don't - this is still a nice sanity reset as it allows us to avoid 
    tracking the internal index after the caller gets the pointer.
  */
  unsigned long long *adam_rng_seed(void);
  unsigned long long *adam_rng_nonce(void);
  unsigned long long *adam_rng_buffer(void);

  /*
    Returns a random unsigned integer of the specified <width>.
  
    Param <width> must ALWAYS be either 8, 16, 32, or 64. Any other
    value will be ignored and revert to the default width of 64.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated integer.
  */
  unsigned long long adam_int(unsigned char width, const unsigned char force_regen);

  /*
    Returns a random double after multiplying it by param <scale>.
  
    For no scaling factor, just set <scale> to 1. Also, a scale value
    of 0 is ignored and treated as 1.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated double.
  */
  double adam_dbl(unsigned long long scale, const unsigned char force_regen);

  /*
    Fills a given buffer with random integers.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(u64) bytes in length, and that the pointer is not 
    NULL. If <amount> is 0 or greater than 1 billion, this function  
    will return 1 and terminate early.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <width> must be 8, 16, 32, or 64. If you provide an invalid 
    width value, the default of 64 is used instead and the argument is 
    ignored.

    Returns 0 on success, 1 on error
  */
  int adam_fill(void *buf, unsigned char width, const unsigned int amount);

  /*
    Fills a given buffer with random doubles.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(double) bytes in length, and that the pointer is 
    not NULL. If <amount> is 0 or greater than 1 billion, this function 
    will return 1 and terminate early.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <multiplier> can be supplied to multiply all doubles by a certain 
    scaling factor so they fall within the range (0, <multiplier>). If you
    do not need a scaling factor, just pass a value of 1.

    Returns 0 on success, 1 on error
  */
  int adam_dfill(double *buf, const unsigned long long multiplier, const unsigned int amount);

  /*
    Chooses a random item from a provided collection, where param <arr> is a 
    pointer to this collection, and param <size> is to specify the total 
    range of possible indices, usually the full length of the array but you
    could pass in a smaller number than that if you want to choose from a 
    particular range or within a specific radius.

    Caller must guarantee <size> is NEVER larger than the <arr>'s capacity

    Returns a randomly picked member of <arr>
  */
  void *adam_choice(void *arr, const unsigned long long size);
#endif
