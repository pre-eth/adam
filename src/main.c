#include "adam.h"

int main(int argc, char** argv) {
  float seed = DEFAULT_SEED;
  u8 rounds = ROUNDS;

  // TODO: GETOPT ARGPARSE LOGIC HERE

  // You can imagine this as a multidimensional 4D array of u8 with size 1024.
  u32 buffer[BUF_SIZE] ALIGN(32);
  
  u32* restrict buf_ptr = &buffer;

  u64 seed = generate(buf_ptr, seed, rounds);
  
  return 0;
}