#include "cmd.h"
#include "csprng.h"

#define ARGCOUNT 7

class ADAM : Command, CSPRNG {
  public:
    ADAM() : Command("adam", "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20", 0, 4,
    "v0.1.0"), CSPRNG() {
      PROGARGS = ARGS; 
      PROGHELP = ARGSHELP;
      PROGARGC = ARGCOUNT;
      OPTSTR = ":hvin:p:r:u:ba";
    };

    u8 exec(int argc, char** argv);
    // Precision of random integers to generate, default is unsigned 64 bit ints.
    // If a lower precision integer is requested, an entry from
    enum Precision : u8 { U64 = 64, U32 = 32, U16 = 16, U8 = 8};
        
  private:
    u8    match_option(char opt, const char* val);
    
    u64   alimit{UINT64_MAX};
    u8    results{1};
    bool  bit_stream{false};

    const char* ARGS[ARGCOUNT] = {"i", "n", "p", "r", "u", "b", "a"};
    const char* ARGSHELP[ARGCOUNT] = {
      "Inverts the polarity of the compression permutation",
      "Number of results to return (default 1, max 256)",
      "Desired size (8, 16, 32) of returned numbers if you need less precision than 64-bit",
      "Number of rounds to mangle buffer (default is 7). Must satisfy 7 <= ROUNDS <= 20",
      "Compression granularity, or the undulation period. Accepts 1 (DOUBLE) or 2 (ALTERNATE)",
      "Just bits. Literally.",
      "Outputs a sample of 1000000 bits"
    };
};
