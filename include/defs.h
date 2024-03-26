#ifndef DEFS_H
#define DEFS_H  
  #include <stdbool.h>
  #include <stdint.h>

  typedef uint8_t       u8;
  typedef uint16_t      u16;
  typedef uint32_t      u32;
  typedef uint64_t      u64;
  typedef __uint128_t   u128;

  #define ALIGN(x)            __attribute__ ((aligned (x)))
  #define CTZ                 __builtin_ctz 
  #define MEMCPY              __builtin_memcpy
  #define MEMSET              __builtin_memset
  #define POPCNT              __builtin_popcountll
  #define LIKELY(x)           __builtin_expect((x), 1)
  #define UNLIKELY(x)         __builtin_expect((x), 0)
#endif
