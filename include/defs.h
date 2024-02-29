#ifndef DEFS_H
#define DEFS_H  
  #include <stdbool.h>

  typedef __UINT8_TYPE__    u8;
  typedef __UINT16_TYPE__   u16;
  typedef __UINT32_TYPE__   u32;
  typedef __UINT64_TYPE__   u64;
  typedef __uint128_t       u128;

  #define ALIGN(x)          __attribute__ ((aligned (x)))
  #define CTZ               __builtin_ctz 
  #define MEMCPY            __builtin_memcpy
  #define MEMSET            __builtin_memset
  #define POPCNT            __builtin_popcountll
  #define LIKELY(x)         __builtin_expect((x), 1)
  #define UNLIKELY(x)       __builtin_expect((x), 0)
#endif
