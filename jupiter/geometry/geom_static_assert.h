#ifndef JUPITER_GEOMETRY_STATIC_ASSERT_H
#define JUPITER_GEOMETRY_STATIC_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Static assertion
 * @param cond Condition of constant expression
 * @param message Message
 *
 * In C99 mode, message will not be used.
 *
 * In C++98 or C++03, this macro cannot be used (disabled)
 */
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define GEOM_STATIC_ASSERT(cond, message) static_assert(!!(cond), message)
#endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define GEOM_STATIC_ASSERT(cond, message) _Static_assert(cond, message)
#else
#if ((defined(__GNUC__) &&                                                     \
      (__GNUC__ >= 5 ||                                                        \
       (__GNUC__ == 4 && defined(__GNUC_MINOR__) && __GNUC_MINOR__ >= 3))) ||  \
     defined(_MSC_VER)) && defined(__COUNTER__)
#define GEOM_STATIC_ASSERT_I __COUNTER__
#else
#define GEOM_STATIC_ASSERT_I __LINE__
#endif
#define GEOM_STATIC_ASSERT_IDENT_X(x) GEOM_STATIC_ASSERT_##x
#define GEOM_STATIC_ASSERT_IDENT_E(x) GEOM_STATIC_ASSERT_IDENT_X(x)
#define GEOM_STATIC_ASSERT_IDENT() \
  GEOM_STATIC_ASSERT_IDENT_E(GEOM_STATIC_ASSERT_I)
#define GEOM_STATIC_ASSERT(cond, message)                                      \
  typedef char GEOM_STATIC_ASSERT_IDENT()[(cond) ? (int)sizeof(message) : -1]
#endif

#ifdef __cplusplus
}
#endif

#endif
