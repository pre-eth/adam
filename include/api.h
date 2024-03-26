#ifndef ADAM_API_H
#define ADAM_API_H
  #include <stdbool.h>

#if defined(__AARCH64_SIMD__) || defined(__AVX512F__)
  #define ADAM_ALIGNMENT    64
#else
  #define ADAM_ALIGNMENT    32
#endif

  #define ADAM_BUF_SIZE     256
  #define ADAM_BUF_BYTES    (ADAM_BUF_SIZE << 3)
  #define ADAM_BUF_BITS     (ADAM_BUF_SIZE << 6)

  #define ADAM_FILL_MAX     1000000000

  typedef struct adam_data_s *adam_data;

  /*
    Configures ADAM's initial state by allocating the adam_data
    struct and preparing it for random generation.
    
    Call this ONCE at the start of your program, before generating 
    any numbers.

    Params <seed> and <nonce> are optional - set to NULL if you'd like
    to seed the generator with secure random bytes from the operating
    system itself. Otherwise, the caller must ensure that <seed> points
    to 256-bits of data, and that nonce points to a u64.

    Returns a pointer to the adam_data. Make sure you remember to pass it
    to adam_cleanup() once you no longer need it!
  */
  adam_data adam_setup(unsigned long long *seed, unsigned long long *nonce);

  /*
    Self-explanatory functions - The first two return a raw pointer to the 
    seed/nonce respectively so you can reset them yourself at anytime you'd like.

    Since the third function adam_buffer() returns a pointer to the output
    vector, the return value of this function is a pointer to const data
    to prevent any sort of modification by the user, as technically even the 
    output vector comprises RNG state and the result of each run is dependent
    on its value. This function ALWAYS generates a fresh buffer before returning
    the pointer, as the API assumes you use the whole buffer each time.
  */
  unsigned long long *adam_seed(adam_data data);
  unsigned long long *adam_nonce(adam_data data);
  const unsigned long long *adam_buffer(const adam_data data);

  /*
    Returns a random unsigned integer of the specified <width>. Param
    <force_regen> can be used to force the generation of a new output
    vector before returning any results.
  
    Param <width> must ALWAYS be either 8, 16, 32, or 64. Any other
    value will be ignored and revert to the default width of 64.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated integer.
  */
  unsigned long long adam_int(adam_data data, unsigned char width, const bool force_regen);

  /*
    Returns a random double after multiplying it by param <scale>. Param
    <force_regen> can be used to force the generation of a new output
    vector before returning any results.
  
    For no scaling factor, just set <scale> to 1. Also, a <scale> value
    of 0 is ignored and treated as 1.

    Automatically makes internal calls when regeneration is needed to 
    ensure that you can safely expect to call this function and 
    always receive a randomly generated double.
  */
  double adam_dbl(adam_data data, const unsigned long long scale, const bool force_regen);

  /*
    Fills a given buffer with random integers.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(u64) bytes in length, and that the pointer is not 
    NULL. If <amount> is 0 or greater than 1 billion, this function  
    will return 1 and exit.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <width> must be 8, 16, 32, or 64. If you provide an invalid 
    width value, the default of 64 is used instead and the argument is 
    ignored.

    Returns 0 on success, 1 on error.
  */
  int adam_fill(adam_data data, void *buf, unsigned char width, const unsigned long long amount);

  /*
    Fills a given buffer with random doubles.

    The caller is responsible for ensuring param <buf> is of at least 
    <amount> * sizeof(double) bytes in length, and that the pointer is 
    not NULL. If <amount> is 0 or greater than 1 billion, this function 
    will return 1 and exit.

    Also, please make sure you use the ADAM_ALIGNMENT macro to align <buf> 
    before you pass it to this function.

    Param <multiplier> can be supplied to multiply all doubles by a certain 
    scaling factor so they fall within the range (0, <multiplier>). If you
    do not need a scaling factor, just pass a value of 1 or 0.

    Returns 0 on success, 1 on error.
  */
  int adam_dfill(adam_data data, double *buf, const unsigned long long multiplier, const unsigned long amount);

  /*
    Chooses a random item from a provided collection, where param <arr> is a 
    pointer to this collection, and param <size> is to specify the total 
    range of possible indices, usually the full length of the array but you
    could pass in a smaller number than that if you want to choose from a 
    particular range or within a specific radius.

    Caller must guarantee <size> is NEVER larger than the <arr>'s capacity.

    Returns a randomly picked member of <arr>.
  */
  void *adam_choice(adam_data data, void *arr, const unsigned long long size);

  /*
    Writes param <output> BITS (not bytes!) to a file descriptor of your choice.

    You can pass NULL for param <file_name> if you'd like to stream to stdout,
    rather than an actual file. If you provide a valid file name, then it will
    be created and saved with the requested amount of binary data.
  
    If param <output> is less than ADAM_BUF_BITS, this function won't do anything.

    Returns the total number of bits written out, or 0 if invalid value for <output>
  */
  unsigned long long adam_stream(adam_data data, const unsigned long long output, const char *file_name);

  /*
    Zeroizes adam_data members and frees any allocated memory.

    Call this once after you are finished using the generator.
  */
  void adam_cleanup(adam_data data);
#endif