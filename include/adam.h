#include "cmd.h"
#include "csprng.h"

#define ARGCOUNT 6

class ADAM : Command, CSPRNG {
  public:
    ADAM() : Command("adam", "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20", 0, 4,
    "v0.1.0"), CSPRNG() {
      PROGARGS = ARGS; 
      PROGHELP = ARGSHELP;
      PROGARGC = ARGCOUNT;
      OPTSTR = ":hvidn:p:u:b::a";
    };

    u8 exec(int argc, char** argv);
    // Precision of random integers to generate, default is unsigned 64 bit ints.
    // If a lower precision integer is requested, an entry from
    enum Precision : u8 { U64 = 64, U32 = 32, U16 = 16, U8 = 8};
        
  private:
    u8    match_option(char opt, const char* val);
    void  bit_stream();
  
    u8    results{1};
    u64   zeroes{0};
    u64   limit;
    bool  stream{false};
    const char* ARGS[ARGCOUNT] = {"i", "n", "p", "d", "b", "a"};
    const char* ARGSHELP[ARGCOUNT] = {
      "Inverts the polarity of the compression permutation",
      "Number of results to return (default 1, max 256)",
      "Desired size (8, 16, 32) of returned numbers if you need less precision than 64-bit",
      "Dump all currently generated numbers, separated by spaces.",
      "Just bits. Literally.",
      "Assess a sample of 100000000 bits (100 MB)"
    };
};
