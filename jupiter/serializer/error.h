
#ifndef JUPITER_SERIALIZER_ERROR_H
#define JUPITER_SERIALIZER_ERROR_H

#include "defs.h"

JUPITER_SERIALIZER_DECL_START

#ifdef __GNUC__
#define MSGPACKX_EXPECT(cond)                   \
  __builtin_expect(!!(cond), 1)
#else
#define MSGPACKX_EXPECT(cond)                   \
  ((!!(cond)) == 1)
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

JUPITER_SERIALIZER_DECL
void msgpackx_assert_impl(const char *file, long line, const char *func,
                          const char *cond, const char *format, ...) NORETURN;

#ifndef NDEBUG
#define MSGPACKX_ASSERT_X(cond, ...)                                    \
  do {                                                                  \
    if (!MSGPACKX_EXPECT(cond)) {                                       \
      msgpackx_assert_impl(__FILE__, __LINE__, __func__, #cond,         \
                           __VA_ARGS__);                                \
    }                                                                   \
  } while(0)

#define MSGPACKX_UNREACHABLE()                              \
  msgpackx_assert_impl(__FILE__, __LINE__, __func__, NULL,  \
                       "Unreachable reached");

#else
#define MSGPACKX_ASSERT_X(cond, ...) ((void)0)

#ifdef __GNUC__
#define MSGPACKX_UNREACHABLE() __builtin_unreachable()
#else
#define MSGPACKX_UNREACHABLE()
#endif
#endif

#define MSGPACKX_ASSERT(cond)  MSGPACKX_ASSERT_X(cond, NULL)

JUPITER_SERIALIZER_DECL
const char *msgpackx_strerror(enum msgpackx_error eval);

JUPITER_SERIALIZER_DECL_END

#endif
