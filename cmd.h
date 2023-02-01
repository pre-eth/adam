#ifndef CMD_H
#define CMD_H 
  #include <getopt.h>
  #include "util.h"

  class Command {
    public:
      Command(const char* cmd, const char* n, unsigned short min, 
      unsigned short max, const char* v)
      : ARG_MIN(min), ARG_MAX(max), COMM(cmd), NAME(n), VERSION(v) {};

    protected:
      u8 PROGARGC{0};
      const char** PROGARGS{nullptr};
      const char** PROGHELP{nullptr};
      const char* OPTSTR;
      
      int run(int argc, char** argv);
      virtual u8 exec(int argc, char** argv) = 0;
      virtual u8 matchOption(char opt, const char* val) = 0;
            
      u8 matchOpts(int argc, char** argv);
      u8 print_options(const char** opts, const char** help, u8 optc);
      
    private:
      const char* COMM;
      const char* NAME;
      const char* VERSION;
      u8 ARG_MIN{0};
      u8 ARG_MAX{1};
  };
#endif