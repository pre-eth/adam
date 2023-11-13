#ifndef ENT_H
#define ENT_H
  #include "defs.h"

  // General ent stuff
  #define PI              3.14159265358979323846

  #define log2of10        3.32192809488736234787
  #define LOG10           __builtin_log10
  #define LOG2(x)         (log2of10 * LOG10(x))
  #define POW(b,e)        __builtin_pow(b, e)    

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
  
  #define ex(x)           (((x) < -BIGX) ? 0.0 : __builtin_exp(x))

  typedef struct rng_data rng_data;

  typedef struct ent_report {
    rng_data *data;
    u64 limit;
    u64 freq;
    u64 ent;
    u64 chisq;
    u64 mean;
    u64 montepicalc;
    u64 scc;
  } ent_report;

  void ent_test(ent_report *rsl);
#endif