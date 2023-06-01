#ifndef ADAM_H
#define ADAM_H

  #include "util.h"

  #define TRNG64(ptr)   __builtin_ia32_rdrand64_step(ptr)

  /*
    The PRNG algorithm is based on the construction of three 
    chaotic maps obtained by permuting and shuffling the elements
    of an initial input vector. The permutations are performed 
    using a chaotic function to scramble the used positions. The 
    chaotic function is given by this logistic function
  */
  #define CHAOS_FN(x)   (3.9999 * x * (1 - x))
  
  #define MAGNITUDE     10
  
  // Sizes of the buffer and bit vector for the binary sequence
  #define BUF_SIZE      1U << MAGNITUDE     
  #define VEC_SIZE      BUF_SIZE * MAGNITUDE                     
  
  #define DEFAULT_SEED  0.2345678F

  // Per Bob's original implementation
  #define GOLDEN_RATIO  0x9E3779B97F4A7C13LLU

  #define ISAAC_MIX(a,b,c,d,e,f,g,h) \
    a-=e; f^=h>>9;  h+=a; c-=g; h^=b>>23; b+=c; \
    b-=f; g^=a<<9;  a+=b; d-=h; a^=c<<15; c+=d; \
    e-=a; b^=d>>14; d+=e; g-=c; d^=f>>17; f+=g; \
    f-=b; c^=e<<20; e+=f; h-=d; e^=g<<14; g+=h; \

#endif