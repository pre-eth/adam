#include <getopt.h>

#include "adam.h"

int main(int argc, char** argv) {
  u64 BINARY_SEQUENCE[BUF_SIZE];

  u8 res;
  u64 init;
  while (!(res = TRNG64(&init)));
  init -= BUF_SIZE;

  ACCUMULATE256(init, 0),
  ACCUMULATE256(init, 256),    
  ACCUMULATE256(init, 512),
  ACCUMULATE256(init, 768);

  float x = DEFAULT_SEED;

  
}