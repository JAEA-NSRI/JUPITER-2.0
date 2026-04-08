#ifndef JUPITER_OS_ASPRINTF_H
#define JUPITER_OS_ASPRINTF_H

#include <stdlib.h>
#include <stdarg.h>

#include "defs.h"

JUPITER_OS_DECL_START

/**
 * @brief Generalized implementation of glibc's vasprintf.
 * @param t Address to store the result.
 * @param fmt C's printf format string
 * @param ap Arguments for fmt
 * @return the formatted text length, -1 if failed.
 *
 * Allocates a required space to t, and format a text by fmt.
 *
 * @note After use, you must free t by free().
 * @note If failed, the contents of t is undefined
 * @note errno may change regardless of this function's status.
 */
JUPITER_OS_DECL
int jupiter_vasprintf(char **t, const char *fmt, va_list ap);

/**
 * @brief Generalized implementation of glibc's asprintf.
 * @param t Address to store the result.
 * @param fmt C's printf format string
 * @param ... Arguments for fmt
 * @return the formatted text length, -1 if failed.
 *
 * Allocates a required space to t, and format a text by fmt.
 *
 * @note After use, you must free t by free().
 * @note If failed, the contents of t is undefined
 * @note errno may change regardless of this function's status.
 */
JUPITER_OS_DECL
int jupiter_asprintf(char **t, const char *fmt, ...);

/**
 * @brief Duplicates the string `s`
 * @param s NUL-terminated string to copy.
 * @return duplicated string, or NULL if allocation failed.
 *
 * This function is reimplementation of strdup in POSIX for
 * non-POSIX-compliant systems (such as Windows)
 */
JUPITER_OS_DECL
char *jupiter_strdup(const char *s);

/**
 * @brief Duplicates the string `s`
 * @param s string to copy
 * @return dupicated string, or NULL if allocation failed.
 *
 * This function is reimplementation of strndup in POSIX for
 * non-POSIX-compliant systems (such as Windows)
 */
JUPITER_OS_DECL
char *jupiter_strndup(const char *s, size_t n);

JUPITER_OS_DECL_END

#endif
