
#ifndef JUPITER_SERIALIZER_BYTESWAP_H
#define JUPITER_SERIALIZER_BYTESWAP_H

#include <stdint.h>
#include <inttypes.h>

#include "defs.h"

JUPITER_SERIALIZER_DECL_START

#ifdef UINT16_C
inline static uint16_t
msgpackx_byteswap_2(uint16_t u16)
{
  return (u16 >> 8) | (u16 << 8);
}
#endif

#ifdef UINT32_C
inline static uint32_t
msgpackx_byteswap_4(uint32_t u32)
{
  u32 = ((u32 << 8) & UINT32_C(0xff00ff00)) |
        ((u32 >> 8) & UINT32_C(0x00ff00ff));
  return (u32 << 16) | (u32 >> 16);
}
#endif

#ifdef UINT64_C
inline static uint64_t
msgpackx_byteswap_8(uint64_t u64)
{
  u64 = ((u64 << 8) & UINT64_C(0xff00ff00ff00ff00)) |
        ((u64 >> 8) & UINT64_C(0x00ff00ff00ff00ff));
  u64 = ((u64 << 16) & UINT64_C(0xffff0000ffff0000)) |
        ((u64 >> 16) & UINT64_C(0x0000ffff0000ffff));
  return (u64 << 32) | (u64 >> 32);
}
#endif

/**
 * @group Serializer
 * @brief swap endian on memory
 * @param v pointer to memory to swap endian.
 * @param size size of data (must be 2, 4, 8 or 16 [bytes])
 *
 * In this function, @p v must be aligned at @p sz bytes. If
 * it cannot be assumed, use memcpy() to uint16_t, uint32_t or
 * uint64_t and use msgpackx_node_byteswap_2(),
 * msgpackx_node_byteswap_4() or msgpackx_node_byteswap_8().
 *
 * The optimization may optimize out this problem (this may run on
 * register-to-register opration, but the source context is not). So
 * please be aware.
 *
 * Some architectures cannot read/write memory on unaligned data at
 * all (raises SIGBUS error on Linux).
 *
 * You can check alignment on x86(_64), which does not cause hardware
 * error, with GCC or Clang option `-fsanitize=alignment`. To trap
 * (abort execution) on detection, use `-fsanitize-trap=alignment`
 * instead.
 *
 * You can not use this for the type `long double`. This type is very
 * architecture dependent, and it's known to have 10, 12 or 16
 * bytes. If the architecture support `_Float128` (this type is
 * defined by ISO/IEC TS 18661-3:2015), you can promote long double
 * value to it, and it can be exchangable type.
 */
inline static void
msgpackx_byteswap_type(void *v, size_t sz)
{
  switch(sz) {
  case 2:
    *(uint16_t *)v = msgpackx_byteswap_2(*(uint16_t *)v);
    return;
  case 4:
    *(uint32_t *)v = msgpackx_byteswap_4(*(uint32_t *)v);
    return;
  case 8:
    *(uint64_t *)v = msgpackx_byteswap_8(*(uint64_t *)v);
    return;
  case 16:
    {
      uint64_t p;
      p = msgpackx_byteswap_8(*(uint64_t *)v);
      *(uint64_t *)v = msgpackx_byteswap_8(*((uint64_t *)v + 1));
      *((uint64_t *)v + 1) = p;
    }
    return;
  default: return;
  }
}

JUPITER_SERIALIZER_DECL_END

#endif
