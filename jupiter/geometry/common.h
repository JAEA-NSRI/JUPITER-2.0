/**
 * @file common.h
 * @ingroup Geometry
 * @brief General function implemented in internally.
 */


#ifndef JUPITER_GEOMETRY_COMMON_H
#define JUPITER_GEOMETRY_COMMON_H

#include <stdarg.h>
#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief If supported, enables warnings about invalid printf
 *        specifier for custom function
 * @param i argument index for format string to be tested
 * @param j first argument index for embedding values
 *
 * Use -Wno-format to disable this warning
 *    (in clang, enabled by default)
 *
 * Use -Wformat to enable this warning
 *    (in gcc and icc, disabled by default)
 */
#if defined(__GNUC__)
#define GEOM_PRINTF_CHECK(i,j) __attribute__((__format__(printf,i,j)))
#else
#define GEOM_PRINTF_CHECK(i,j)
#endif

/**
 * @ingroup Geometry
 * @brief Allocates and format by C language's printf.
 * @param buf Location to store the result.
 * @param format Format string
 * @param ... arguments for format
 * @return Number of bytes to written, -1 for error
 *
 * Allocates sufficient memory to store the formatted text of
 * `format`, and write it to allocated memory.
 *
 * This function is self implementation of `asprintf()` function in
 * glibc, BSD libc or MinGW to be used under systems which does not
 * support it.
 *
 * The content of `buf` when errors occured is undefined (i.e., it
 * depends how this function is implemented; even glibc's `asprintf()`
 * function itself, which is one of implemention of this function,
 * does not define it). You **MUST NOT** use the content of buf for
 * error checking.
 *
 * If your system have `asprintf()`, man page of it may also help.
 *
 * To use `asprintf()` in your system, define `_GNU_SOURCE`.
 *
 * Here is the list of C standard library implementations or runtimes
 * which are known to `asprintf()` available:
 *   * glibc (most of Linux, or rarely some kind of other UNIX-like)
 *   * uClibc (some kind of Linux distros)
 *   * musl (some kind of Linux distros)
 *   * BSD libc (includes OS X and macOS)
 *   * newlib (includes Cygwin and MSYS2)
 *   * MinGW has implementation of `asprintf()`
 *
 * This function is publicly available, but you should have your own
 * implementation of this function in your application.
 *
 * @sa geom_vasprintf
 */
JUPITER_GEOMETRY_DECL
int geom_asprintf(char **buf, const char *format, ...) GEOM_PRINTF_CHECK(2, 3);

/**
 * @ingroup Geometry
 * @brief Allocates and format by C language's printf.
 * @param buf Location to store the result.
 * @param format Format string
 * @param ap arguments for format
 * @return Number of bytes to written, -1 for error
 *
 * Allocates sufficient memory to store the formatted text of
 * `format`, and write it to allocated memory.
 *
 * See `geom_asprintf()` for details
 *
 * @sa geom_asprintf
 */
JUPITER_GEOMETRY_DECL
int geom_vasprintf(char **buf, const char *format, va_list ap);

/**
 * @ingroup Geometry
 * @brief format double value to string
 * @param buf Location to store the result.
 * @param flags flags part of C-format
 * @param width width for formatting (negative to omit)
 * @param precision precision for formatting (negative to omit)
 * @param fmt character to format double value (a, A, e, E, f, F, g or G).
 * @return number of bytes written, -1 for error
 */
JUPITER_GEOMETRY_DECL
int geom_double_to_str(char **buf, double val, const char *flags,
                       int width, int precision, char fmt);

/**
 * @ingroup Geometry
 * @brief pretty-print double matrix to string
 * @param buf   Location to store the result.
 * @param vals  Array of values
 * @param nx    Number of columns
 * @param ny    Number of rows
 * @param sep   Separator string (typically ", ").
 * @param parst Start parenthesis (typically "(").
 * @param pared End parenthesis (typically ")").
 * @param align non-0 to align columns, 0 not to align columns
 * @param fmt   C format string for each element.
 * @param width width value for '*' width in `fmt`.
 * @param precision precision value for '*' precision in `fmt`.
 * @return Number of bytes written, -1 for error.
 *
 * `fmt` must contain single floating point value conversion.
 *
 * If a conversion is not for floating point, or multiple conversions
 * are present (`%%` is allowed to output '%'), this function fails
 * and returns -1.
 *
 * `buf` must be an array of `char*` with length of `ny` to store
 * each row of the matrix.
 *
 * Values are taken by column-major order.
 *
 * To free allocated memory, calling `free(buf[0])` is sufficient.
 * `free(buf[1])` and later will be invalid.
 */
JUPITER_GEOMETRY_DECL
int geom_matrix_to_str(char *buf[], const double *vals, int nx, int ny,
                       const char *sep, const char *parst, const char *pared,
                       int align, const char *fmt, int width, int precision);

/**
 * @macro GEOM_DOFALLTHRU
 * @brief Declares that a fallthrough on the case-statement is intentional
 *
 * GCC 7 or later (with -Wextra or -Wimplicit-fallthrough), Clang 6 or
 * later (at least, with -Wimplicit-fallthrough), C2x and C++14
 * standard supports warning on fallthrough on case-statement.
 *
 *     switch(foo) {
 *     case 0:
 *       somework(); // This emits implicit fallthrough warning
 *     case 1:
 *       // ....
 *     }
 *
 * If that fallthrough is really intentional, use this macro:
 * (note that the comment "fall through" is required for Clang 6. See
 *  https://stackoverflow.com/a/48702576 )
 *
 *     switch(foo) {
 *     case 0:
 *       somework();
 *       GEOM_DOFALLTHRU(); // fall through
 *     case 1:
 *       // ....
 *     }
 */
#if (defined(__cplusplus) && __cplusplus >= 201402L) ||                        \
  (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202001L)
#define GEOM_DOFALLTHRU() [[fallthrough]]
#elif defined(__GNUC__) && !defined(_INTEL_COMPILER) &&                        \
  (__GNUC__ >= 7 || (defined(__clang__) && __clang_major__ >= 10))
#define GEOM_DOFALLTHRU() __attribute__((fallthrough))
#else
#define GEOM_DOFALLTHRU() ((void) 0)
#endif

JUPITER_GEOMETRY_DECL_END

#endif
