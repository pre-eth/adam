#ifndef RNG_H
#define RNG_H
  /* 
    Sizes of the buffer and bit vector for the binary sequence
    The buffer is of type [u64; 256] with size 256 * 8 = 2048 bytes

    Need to left shift for SEQ_SIZE because there's 64 bits per index
  */
  #define MAGNITUDE           8  
  #define BUF_SIZE            (1U << MAGNITUDE)     
  #define SEQ_SIZE            (BUF_SIZE << 6)  
  #define SEQ_BYTES           (BUF_SIZE << 3)  

  /*
    The PRNG algorithm is based on the construction of three 
    chaotic maps obtained by permuting and shuffling the elements
    of an initial input vector. The permutations are performed 
    using a chaotic function to scramble the used positions. The 
    chaotic function is given by this logistic function:

    3.9999 * X * (1 - X)
  */
  #define COEFFICIENT           3.9999      
  
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
    function 3 times and computing the max, in this case 6
  */
  #define ROUNDS                8
  #define ITER                  (ROUNDS / 3)

  /* 
    BETA is derived from the length of the significant digits
    ADAM uses the max double accuracy length of 15
  */
  #define BETA                  10E15

  void adam_run(unsigned long long *seed, unsigned long long *nonce, unsigned long long *aa, unsigned long long *bb);
  void adam_data(unsigned long long **_ptr, double **_chptr);
#endif
