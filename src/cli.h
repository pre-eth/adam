#ifndef CLI_H
#define CLI_H
  #define MAJOR       1
  #define MINOR       0
  #define PATCH       0

  #define OPTSTR      ":hvlbdn:r:p:a::s::u::"
  #define ARG_MAX     5

  // prints digits in reverse order to buffer
  // and returns the number of zeroes
  FORCE_INLINE static u8 print_binary(u64 num) {
    u8 zeroes = 0;
    char c;
    do {
      c = (num & 0x01) + '0';
      zeroes += (49 - c); 
      fputc(c, stdout);
    } while (num >>= 1);
    return zeroes;
  }

  FORCE_INLINE static u8 a_to_u(const char* s, u8 min, u8 max) {
    u8 val, len = 0;
    while (s[len++] != '\0');

    switch (len) { 
      case  3:  val += (s[len- 3] - '0') * 100;
      case  2:  val += (s[len- 2] - '0') * 10;
      case  1:  val += (s[len- 1] - '0');
        break;
    }
    return (val < min + 1 || val > max + 1) ? min : val;
  }

#endif