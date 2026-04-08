#include "random.h"

#if defined(JUPITER_RANDOM_F90_EXPORT)
#if (defined(_WIN32) || defined(__CYGWIN__))
#define JUPITER_RANDOM_F90_DECL __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define JUPITER_RANDOM_F90_DECL __attribute__((visibility("default")))
#endif
#else
#define JUPITER_RANDOM_F90_DECL
#endif

#define define_random_next_f90_b(rtype, c_name, f_name, stype) \
  JUPITER_RANDOM_F90_DECL                                      \
  rtype f_name(void *seed) { return c_name((stype *)seed); }

#define DEFINE_RANDOM_NEXT_F90(rtype, c_name) \
  define_random_next_f90_b(rtype, c_name, c_name##_f90, jupiter_random_seed)

#define DEFINE_RANDOM_NEXTC_F90(rtype, c_name)          \
  define_random_next_f90_b(rtype, c_name, c_name##_f90, \
                           jupiter_random_seed_counter)

#define define_random_jump_f90_b(c_name, f_name, stype) \
  JUPITER_RANDOM_F90_DECL                               \
  void f_name(void *seed) { c_name((stype *)seed); }

#define DEFINE_RANDOM_JUMP_F90(c_name) \
  define_random_jump_f90_b(c_name, c_name##_f90, jupiter_random_seed)

#define DEFINE_RANDOM_JUMPC_F90(c_name) \
  define_random_jump_f90_b(c_name, c_name##_f90, jupiter_random_seed_counter)

DEFINE_RANDOM_NEXT_F90(int64_t, jupiter_random_nexti)
DEFINE_RANDOM_NEXT_F90(float, jupiter_random_nextf)
DEFINE_RANDOM_NEXT_F90(double, jupiter_random_nextd)
DEFINE_RANDOM_NEXT_F90(double, jupiter_random_nextde)
DEFINE_RANDOM_NEXT_F90(double, jupiter_random_nextdn)
DEFINE_RANDOM_NEXT_F90(int64_t, jupiter_random_nexti63)
DEFINE_RANDOM_NEXT_F90(int64_t, jupiter_random_nextis)
DEFINE_RANDOM_NEXT_F90(int8_t, jupiter_random_nexti8)
DEFINE_RANDOM_NEXTC_F90(int64_t, jupiter_random_nextic)
DEFINE_RANDOM_NEXTC_F90(float, jupiter_random_nextfc)
DEFINE_RANDOM_NEXTC_F90(double, jupiter_random_nextdc)

JUPITER_RANDOM_F90_DECL
int32_t jupiter_random_nextn_f90(void *seed, int32_t n)
{
  if (n < 0)
    return 0;
  return jupiter_random_nextn((jupiter_random_seed *)seed, n);
}

JUPITER_RANDOM_F90_DECL
int32_t jupiter_random_nextnc_f90(void *seed, int32_t n)
{
  if (n < 0)
    return 0;
  return jupiter_random_nextnc((jupiter_random_seed_counter *)seed, n);
}

DEFINE_RANDOM_JUMP_F90(jupiter_random_jump32)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump48)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump64)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump96)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump128)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump160)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump192)
DEFINE_RANDOM_JUMP_F90(jupiter_random_jump224)

DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump32c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump48c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump64c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump96c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump128c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump160c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump192c)
DEFINE_RANDOM_JUMPC_F90(jupiter_random_jump224c)
