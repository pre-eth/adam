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

  /*    MINIMAL API    */

  /*
    The minimal API lives up to its name and only has 1 generic adam() function, which runs the
    algorithm and then fills param <buffer> with 256 64-bit unsigned integers per call. Calling
    this method once is equivalent to calling the internal algorithm once.

    One disadvantage of the minimal API is that the user needs to supply a seed and nonce. Unlike
    the standard API, no initial seed or nonce is provided because the focus of the minimal API 
    is to be a simpler, lower level alternative. The standard API's initialization logic uses 
    the getentropy() function, which is only available on Unix based platforms. The minimal API 
    allows you to use other platform specific functions to read random data. It's also not too hard
    to provide a good (or bad :P) seed/nonce yourself though - there's plenty of tutorials for it
    and you can have a scheme set up in like 10 minutes. After you provide the initial <seed> and
    <nonce> parameters to ADAM, continue to pass the pointers but DO NOT modify the values again,
    unless you want to reset or create a new stream.

    Caller must guarantee that <buffer> points to an array of at least 256 * 8 = 2048 bytes.

    IMPORTANT: NEVER PASS NULL FOR params <seed> OR <nonce>!!!!
  */
  void adam(unsigned long long *restrict _ptr, unsigned long long *restrict seed, unsigned long long *restrict nonce);
#endif