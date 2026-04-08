#ifndef CSVTMPL_FORMAT_H
#define CSVTMPL_FORMAT_H

#include "common.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief format integers to fmt
 * @param retp pointer to be store formatted string
 * @param fmt format string
 * @param format_keys charaters to be formatted with variadic arguments
 * @param ... number or string to be formatted.
 *
 * This function allocates enough memory to store the result.
 * You must free with free after use them.
 *
 * Format is C-compliant, but:
 *  * Only supports decimal (as 'd'), string (as 's'), and '%' (literal).
 *  * No format options (including width and precision) can be used for
 *    string.
 *  * The altername form flag '#' will be ignored. (It's undefined in C)
 *  * '*' cannot be used for width and precision.
 *
 * This function is safe for user inputted fmt. If multiple %..d (if
 * format_keys is "d") used, format intgers each time.  So, `a%db%04d`
 * with 4, output will be `a4b0004`.
 *
 * The format of format_keys is list of characters like "abc" and end
 * with NUL '\0'. For example, the format is "abc", 3 integer variadic
 * arguments with formatting `%a`, `%b`, `%c` respectively. It is
 * case-sensitive, so total 52 arguments (a to z and A to Z) can be
 * usable.
 *
 * To switch type in format_keys, use "[d]" (to change decimal) and
 * "[s]" (to change string). Spaces in format_keys string will be
 * ignored. Default type is decimal. So, for example, the format_keys
 * "z [s]aT [d]cy" indicates the the order of the varidic argument
 * part and each contents are:
 *
 *  1. A decimal value (int), value for `%z` in fmt.
 *  2. A NUL-terminated string (const char *), text for `%a` in fmt.
 *  3. A NUL-terminated string (const char *), text for `%T` in fmt.
 *  4. A decimal value (int), value for `%c` in fmt.
 *  5. A decimal value (int), value for `%y` in fmt.
 *
 * If NULL is given for retp, returns required size.
 *
 * The feature of this function is likely to `strftime()` function,
 * but for general purpose.
 */
JUPITER_DECL
int format_integers(char **retp, const char *fmt,
                    const char *format_keys, ...);

/**
 * Same as format_integers, just pass stdarg va_list.
 */
JUPITER_DECL
int format_integersv(char **retp, const char *fmt,
                     const char *format_keys, va_list ap);

/**
 * @brief format integers to fmt
 * @param retp pointer to be store formatted string
 * @param fmt format string
 * @return number of bytes written
 *
 * Make a glob pattern for @p fmt.
 */
JUPITER_DECL
int make_glob_pattern(char **retp, const char *fmt);

struct format_integers_match_data
{
  char key; ///< Key charator [in]
  enum format_integers_keytype {
    FORMAT_MATCH_INT,
    FORMAT_MATCH_STR,
  } type;

  int matched; ///< Whether matched or not. [out]
  int value;   ///< Integer matched value. [out]
  char **string; ///< String matched value (if NULL is set, skip setting) [out].
  int err; ///< Errno value if occured [out]
};
typedef struct format_integers_match_data format_integers_match_data;

/**
 * @brief format integers to fmt
 * @param retp pointer to be store formatted string
 * @param fmt format string
 * @param ndata Number of data
 * @param data Array of match data to give or get information.
 * @retval 0 did not match.
 * @retval 1 matches to pattern.
 * @retval -1 allocation failed (may occurs when caller needs matched string)
 *
 * Format is C-compliant, but:
 *  * Only supports decimal (as 'd'), string (as 's'), and '%' (literal).
 *  * No format options (including width and precision) can be used for
 *    string.
 *  * The altername form flag '#' will be ignored. (It's undefined in C)
 *  * '*' cannot be used for width and precision.
 *
 * The format of format_keys is list of characters like "abc" and end
 * with NUL '\0'. For example, the format is "abc", 3 integer variadic
 * arguments with formatting `%a`, `%b`, `%c` respectively. It is
 * case-sensitive, so total 52 arguments (a to z and A to Z) can be
 * usable.
 *
 * @todo Currently, only a string which starts with an alphabet and
 *       follows alphanumrics will match for strings.
 *       (maybe regex?, charactor class specifier?, ...)
 */
JUPITER_DECL
int format_integers_match(const char *scan, const char *fmt, int ndata,
                          format_integers_match_data *data);

#ifdef __cplusplus
}
#endif

#endif
