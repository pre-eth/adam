#include "cmd.h"
#include "csprng.h"

#define ARGCOUNT 8

class ADAM : Command, CSPRNG {
  public:
    ADAM() : Command("adam", "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20", 0, 4,
    "v0.1.0"), CSPRNG() {
      PROGARGS = ARGS; 
      PROGHELP = ARGSHELP;
      PROGARGC = ARGCOUNT;
      OPTSTR = ":hvdln:p:u:a:b::s::";
    };

    u8 exec(int argc, char** argv);
    // Precision of random integers to generate, default is unsigned 64 bit ints.
    // If a lower precision integer is requested, an entry from
    enum Precision : u8 { U64 = 64, U32 = 32, U16 = 16, U8 = 8};
        
  private:
    u8    match_option(char opt, const char* val);
    void  live_stream();
    void  bit_stream();

    // Add one for real number of results, since we can't store 256 in u8
    u8    results{0}; 
    u64   zeroes{0};
    u64   limit;
    FILE* fp{stdout};
    const char* ARGS[ARGCOUNT] = {"u", "n", "p", "d", "b", "a", "s", "l"};
    const char* ARGSHELP[ARGCOUNT] = {
      "Set the uniform distributor's bitwise right shift depth (default 3, max 8)",
      "Number of results to return (default 1, max 256)",
      "Desired size (8, 16, 32) of returned numbers if you need less precision than 64-bit",
      "Dump all currently generated numbers, separated by spaces",
      "Just bits. Literally. Optionally, you can provide a certain limit of N bits",
      "Assess a sample of 100000000 bits (100 MB) written to a filename you provide",
      "Set the seed (u64). If no argument is provided, returns seed for current buffer",
      "Live stream of continuously generated numbers"
    };
};
