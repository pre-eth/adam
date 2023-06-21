#include "adam.h"

int main(int argc, char** argv) {
  float seed = DEFAULT_SEED;

  // TODO: GETOPT ARGPARSE LOGIC HERE

  // You can imagine this as a multidimensional 8 arrays of u8 with size 1024.
  // This is to support the maximum ROUNDS count of 24, as each u64
  // stores values from a previous iteration of the algorithm  
  u64 buffer[BUF_SIZE] ALIGN(32);

  generate(&buffer, seed);
  
  return 0;
}