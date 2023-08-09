#ifndef UTIL_H
#define UTIL_H

  typedef __UINT8_TYPE__    u8;
  typedef __UINT16_TYPE__   u16;
  typedef __UINT32_TYPE__   u32;
  typedef __UINT64_TYPE__   u64;

  #define ALIGN(x)          __attribute__ ((aligned (x)))
  #define FORCE_INLINE	    inline __attribute__((always_inline))
  #define CLZ               __builtin_clz  
  #define MEMCPY	          __builtin_memcpy
  #define FLOOR             __builtin_floor
  #define POPCNT            __builtin_popcountl
  #define LIKELY(x, b)      __builtin_expect((x), 1)
#endif