/**
 * @addtogroup doxygen_filter
 * @{
 * @brief Utilities to filter sources for fitting with doxygen input.
 *
 * @file doxygen-filter.h
 * @brief Common header file for creating doxygen filters.
 */

#ifndef CMAKE_DOXYGEN_FILTER_H
#define CMAKE_DOXYGEN_FILTER_H

#include <stdarg.h>

/**
 * @brief default program name.
 *
 * If given by externally (ex. command line argument, another header,
 * ...), we use it as DEFAULT_PROG_NAME.
 */
#ifndef DEFAULT_PROG_NAME
#define DEFAULT_PROG_NAME "doxygen-filter"
#endif

/**
 * @brief set program name to be used while printing errors.
 * @param argv0 name to be set.
 *
 * If argv0 contains directory, its basename is used as program name.
 *
 * The pointer must not be freed until program is going to exit.
 *
 * @note Technically, you can include '/' into `argv[0]` intentionally
 *       in UNIX-like OSs (see man page of `exec(3)`), unrelated to
 *       actual filename of the executable. So it does not always mean
 *       directory.
 */
void set_prog_name(const char *argv0);

/**
 * @brief Prints errors
 *
 */
void print_error(const char *fmt, ...);
int doxfilter_vasprintf(char **t, const char *fmt, va_list ap);
int doxfilter_asprintf(char **t, const char *fmt, ...);

#define print_error print_error

/*! @} */
#endif
