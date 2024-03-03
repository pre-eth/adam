#ifndef RNG_H
#define RNG_H
  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u64; 256] with size 256 * 8 = 2048 bytes

    Need to left shift for SEQ_SIZE because there's 64 bits per index
  */
  #define MAGNITUDE           8  
  #define BUF_SIZE            (1U << MAGNITUDE)   
  #define SEQ_BYTES           (BUF_SIZE << 3)    
  #define SEQ_SIZE            (BUF_SIZE << 6)  
  
  /* 
    ROUNDS must satisfy k = T / 3 where T % 3 = 0, where k is 
    the iterations per each chaotic map. 

    To avoid brute force attacks, ROUNDS must also satisfy the 
    condition: pow([5 x pow(10, c - 1) - 1], ROUNDS) > pow(2, 256),
    where c is the number of digits in the seed.

    Note: The paper says that "In any cryptosystem, for poor keys 
    or limited key space K, the cryptosystem can be easily broken. 
    
    Indeed, given today’s computer speed, it is generally accepted 
    that a key space of size smaller than 2^128."

    However, the paper was written in 2014, and computing power has
    increased even more since then, so instead of 128, 256 is used 
    above as well as the following formula used to calculate K:
      floor(256 / log2(5 x pow(10, c - 1) - 1)) + 1

    The rounds value is supposed to be generated from taking this 
    function 3 times and computing the max, in this case set to 8
    because of the compressed nature of the implementation compared
    to the original implementation
  */
  #define ROUNDS                6

  /*
    The CSPRNG algorithm is based on the construction of three 
    chaotic maps obtained by supplying a set of chaotic seeds and
    partitioning the elements of an initial input vector, with the 
    mixing of all elements in the vector performed between chaotic 
    iterations to produce the final output sequence in place. The 
    chaotic function is given as:

    3.9999 * X * (1 - X)
  */
  #define COEFFICIENT           3.9999      

  /* 
    BETA is derived from the number of digits we'd like to pull from the mantissa
    before converting a chaotic seed output to an integer value
  */
  #define BETA                  10E18

  /*  The main primitives for using ADAM's algorithm  */

  void accumulate(unsigned long long *restrict seed, unsigned long long *restrict IV, double *restrict chseeds, const unsigned long long cc);
  void diffuse(unsigned long long *restrict _ptr, const unsigned long long nonce);
  void apply(unsigned long long *restrict _ptr, unsigned long long *restrict work_buffer, double *restrict chseeds);
  void mix(unsigned long long *restrict _ptr, const unsigned long long *restrict work_buffer);
  void reseed(unsigned long long *restrict seed, unsigned long long *restrict work_buffer, unsigned long long *restrict nonce, unsigned long long *restrict cc);
#endif