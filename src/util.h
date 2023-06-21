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
  typedef               __uint128_t     u128;

  #define ALIGN(x)      __attribute__ ((aligned (x)))
  #define FORCE_INLINE	inline __attribute__((always_inline))
  #define CLZ           __builtin_clz  
  #define MEMCPY	      __builtin_memcpy
  #define MEMMOVE	      __builtin_memmove
  #define FLOOR         __builtin_floorf

  #define ACCUMULATE(seed, i)\
    _ptr[0  + i] = 0  + seed,\
    _ptr[1  + i] = 1  + seed,\
    _ptr[2  + i] = 2  + seed,\
    _ptr[3  + i] = 3  + seed,\
    _ptr[4  + i] = 4  + seed,\
    _ptr[5  + i] = 5  + seed,\
    _ptr[6  + i] = 6  + seed,\
    _ptr[7  + i] = 7  + seed,\
    _ptr[8  + i] = 8  + seed,\
    _ptr[9  + i] = 9  + seed,\
    _ptr[10 + i] = 10 + seed,\
    _ptr[11 + i] = 11 + seed,\
    _ptr[12 + i] = 12 + seed,\
    _ptr[13 + i] = 13 + seed,\
    _ptr[14 + i] = 14 + seed,\
    _ptr[15 + i] = 15 + seed

#endif