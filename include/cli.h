#ifndef CLI_H
#define CLI_H
  #include "defs.h"

  #define STRINGIZE(a)    #a
  #define STRINGIFY(a)    STRINGIZE(a)

  #define MAJOR           1
  #define MINOR           4
  #define PATCH           0

  #define OPTSTR          ":hvbxow:a:e:r::u::s::n::"
  #define ARG_COUNT       12

  // Forward declaration - see adam.h for details
  typedef struct rng_data rng_data;

  typedef struct rng_cli {
    rng_data *data;             //  Pointer to RNG buffer and state
    u8 width;                   //  Number of bits in results (8, 16, 32, 64)
    // u8 precision;
    const char *fmt;            //  Format string for displaying results
    u16 results;                //  Number of results to return to user (varies based on width, max 2048 u8)
  } rng_cli;
  
  u8 cli_runner(int argc, char **argv);
  u8 help(void);
  u8 set_width(rng_cli *cli, const char *strwidth);
  u8 print_buffer(rng_cli *cli);
  u8 uuid(const char *strlimit, rng_data *data);
  u8 assess(const char *strlimit, rng_cli *cli);
#endif
