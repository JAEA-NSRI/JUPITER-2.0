
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "serializer_test.h"

#include <jupiter/serializer/byteswap.h>

int byteswap_test(void)
{
  int ecnt;
#ifdef UINT16_C
  uint16_t u16;
#endif
#ifdef UINT32_C
  uint32_t u32;
#endif
#ifdef UINT64_C
  uint64_t u64;
  uint64_t u64_2[2];
#endif

  ecnt = 0;

#ifdef UINT16_C
  u16 = UINT16_C(0x1234);
  u16 = msgpackx_byteswap_2(u16);
  if (test_compare(u16, UINT16_C(0x3412))) {
    ecnt++;
  }
#endif

#ifdef UINT32_C
  u32 = UINT32_C(0x12345678);
  u32 = msgpackx_byteswap_4(u32);
  if (test_compare(u32, UINT32_C(0x78563412))) {
    ecnt++;
  }
#endif

#ifdef UINT64_C
  u64 = UINT64_C(0x123456789abcdef0);
  u64 = msgpackx_byteswap_8(u64);
  if (test_compare(u64, UINT64_C(0xf0debc9a78563412))) {
    ecnt++;
  }

  u64_2[0] = UINT64_C(0x123456789abcdef0);
  u64_2[1] = UINT64_C(0x23456789abcdef01);
  msgpackx_byteswap_type(u64_2, 16);
  if (test_compare(u64_2[0], UINT64_C(0x01efcdab89674523))) {
    ecnt++;
  }
  if (test_compare(u64_2[1], UINT64_C(0xf0debc9a78563412))) {
    ecnt++;
  }
#endif

  return ecnt;
}
