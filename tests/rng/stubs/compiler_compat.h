#if defined(_MSC_VER)
#define __attribute__(...)
#include <intrin.h>
static __inline int aurora_test_clz(unsigned value) {
  unsigned long bit;
  return _BitScanReverse(&bit, value) ? 31 - (int)bit : 32;
}
#define __builtin_clz aurora_test_clz
#endif
