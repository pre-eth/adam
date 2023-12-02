#ifndef ENT_H
#define ENT_H
  #include "defs.h"

  // General ent stuff
  #define PI              3.141592653589793238

  #define log2of10        3.32192809488736234787

  /*
    Bytes used as Monte Carlo co-ordinates.  
    This should be no more bits than the mantissa of your
    "double" floating point type.
  */
  #define MONTEN          6   

  // Ent chi square distribution calculation stuff

  #define Z_MAX           6.0                               /* maximum meaningful z value */     
  #define LOG_SQRT_PI     0.5723649429247000870717135       /* log (sqrt (pi)) */
  #define I_SQRT_PI       0.5641895835477562869480795       /* 1 / sqrt (pi) */
  #define BIGX            20.0                              /* max value to represent exp (x) */
  
  typedef struct rng_data rng_data;

  typedef struct ent_report {
    rng_data *data;
    u64 limit;
    u64 mfreq;
    u64 max;
    u64 min;
    u64 zeroes;
    u64 dupes;
    double ent;
    double chisq;
    double pochisq;
    double mean;
    double montepicalc;
    double monterr;
    double scc;
  } ent_report ALIGN(SIMD_LEN);

  void ent_test(ent_report *rsl);
#endif