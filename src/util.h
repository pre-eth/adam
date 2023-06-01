#ifndef UTIL_H
#define UTIL_H

  #include <sys/ioctl.h>
  #include <immintrin.h>
  #include <stdio.h>
  #include <getopt.h>

  typedef               __UINT8_TYPE__  u8;
  typedef               __UINT16_TYPE__ u16;
  typedef               __UINT32_TYPE__ u32;
  typedef               __UINT64_TYPE__ u64;

  #define ACCUMULATE(seed, i)\
    BINARY_SEQUENCE[0  + i] = 0  + seed,\
    BINARY_SEQUENCE[1  + i] = 1  + seed,\
    BINARY_SEQUENCE[2  + i] = 2  + seed,\
    BINARY_SEQUENCE[3  + i] = 3  + seed,\
    BINARY_SEQUENCE[4  + i] = 4  + seed,\
    BINARY_SEQUENCE[5  + i] = 5  + seed,\
    BINARY_SEQUENCE[6  + i] = 6  + seed,\
    BINARY_SEQUENCE[7  + i] = 7  + seed,\
    BINARY_SEQUENCE[8  + i] = 8  + seed,\
    BINARY_SEQUENCE[9  + i] = 9  + seed,\
    BINARY_SEQUENCE[10 + i] = 10 + seed,\
    BINARY_SEQUENCE[11 + i] = 11 + seed,\
    BINARY_SEQUENCE[12 + i] = 12 + seed,\
    BINARY_SEQUENCE[13 + i] = 13 + seed,\
    BINARY_SEQUENCE[14 + i] = 14 + seed,\
    BINARY_SEQUENCE[15 + i] = 15 + seed

#endif