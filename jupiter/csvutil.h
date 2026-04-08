/**
 * @file   csvutil.h
 * @ingroup JUPITER
 * @brief  CSV and other input-file support functions.
 */

#ifndef CSVUTIL_H
#define CSVUTIL_H

#include "common.h"
#include "component_info_defs.h"
#include "control/defs.h"
#include "control/fv_table.h"
#include "control/write_fv_csv.h"
#include "geometry/bitarray.h"
#include "struct.h"

#include <stdarg.h>
#include <stdint.h>

#include "csv.h"
#include "geometry/defs.h"
#include "tmcalc_defs.h"
#include "dccalc_defs.h"
#include "tempdep_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * defined in strlist.h
 */
struct jupiter_strlist_head;

struct jupiter_print_levels
{
  geom_bitarray_n(levels, CSV_EL_MAX);
};
typedef struct jupiter_print_levels jupiter_print_levels;

JUPITER_DECL
void set_jupiter_print_levels(jupiter_print_levels lvls);
JUPITER_DECL
jupiter_print_levels get_jupiter_print_levels(void);

/* The arguemnt is whether myrank should print */
JUPITER_DECL
void set_jupiter_print_rank(int flag);
JUPITER_DECL
int get_jupiter_print_rank(void);

/**
 * @brief Prints error text with line l and column c.
 * @param fname file name
 * @param l line number
 * @param c column number
 * @param errlevel Error level indicate
 * @param value content which cause an error
 * @param csv_errcode CSV error code (errors from csv.h/csvutil.h fuctions)
 * @param syscall_errcode system call error code (errors from errno)
 * @param ext_errcode external library error code (MPI or geometry)
 * @param additional_info additional info text some errors.
 *
 * This function will never fail, but line and column info will not be
 * displayed if memory allocation was failed or you are running on
 * some architecture, compiler or OS which is not supported.
 *
 * If `csv_errcode` is 0 (`CSV_ERR_SUCC`), this function prints "No
 * error".
 *
 * If `csv_errcode` is `CSV_ERR_SYS`, the result of
 * `strerror(syscall_errcode)` will be printed (OS dependent).
 *
 * If csv_errcode is `CSV_ERR_MPI` and ext_errcode is `MPI_SUCCESS`, or
 * csv_errcode is `CSV_ERR_GEOMETRY` and ext_errcode is `GEOM_SUCCESS`,
 * this function will print no error.
 *
 * @todo Type of syscall_errcode should be `error_t`. It is recent
 *       standard (C11), and should be replaced later.  (Using error_t
 *       is just declarating that we needs the value of errno, and
 *       there is no other effect. Because error_t is typedef'd name
 *       to int.)
 */
JUPITER_DECL
void csvperror(const char *fname, long l, long c, csv_error_level errlevel,
               const char *value, csv_error csv_errcode, int syscall_errcode,
               int ext_errcode, const char *additional_info);

/**
 * @brief Prints error text with line l and column c.
 * @param fname file name
 * @param l line number
 * @param c column number
 * @param errlevel Error level indicate
 * @param cell content which cause an error
 * @param fmt Error text format.
 * @param ... printf arguments.
 *
 * This function will never fail, but line and column info will
 * not be displayed if memory allocation was failed or you are
 * running on some architecture, compiler or OS which is not
 * supported.
 */
JUPITER_DECL
void csvperrorf(const char *fname, long l, long c, csv_error_level errlevel,
                const char *value, const char *fmt, ...);

/**
 * @brief Prints error text with line l and column c.
 * @param fname file name
 * @param l line number
 * @param c column number
 * @param errlevel Error level indicate
 * @param value cell content which cause an error
 * @param fmt Error text format.
 * @param ap printf arguments.
 *
 * Same as csvperrorf, just take argument of va_list for variadic argument.
 */
JUPITER_DECL
void csvperrorv(const char *fname, long ln, long cl, csv_error_level errlevel,
                const char *value, const char *format, va_list ap);

/**
 * @brief Prints error text with line l and column c.
 * @param fname file name
 * @param l line number
 * @param c column number
 * @param errlevel Error level indicate
 * @param value cell content which cause an error
 * @param message_head message(s) to print
 *
 * Same as csvperrorf, just prints (concatenated) message from @p message_head.
 *
 * @note This function will modify the content of @p message_head. So each node
 * pointers in @p message_head will be invalid.
 */
JUPITER_DECL
void csvperrorl(const char *fname, long ln, long cl, csv_error_level errlevel,
                const char *value, struct jupiter_strlist_head *message_head);

/**
 * @brief Prints error text with CSV column
 * @param fname file name
 * @param col CSV's column
 * @param errlevel Error level indicate
 * @param fmt Error text format.
 * @param ap printf arguments.
 *
 * Equivalent to csvperrorv using line, column and value information
 * from CSV `col`.
 */
JUPITER_DECL
void csvperrorv_col(const char *fname, csv_column *col,
                    csv_error_level errlevel, const char *fmt, va_list ap);

/**
 * @brief Prints error text with CSV column
 * @param fname file name
 * @param col CSV's column
 * @param errlevel Error level indicate
 * @param fmt Error text format.
 * @param ... printf arguments.
 *
 * Equivalent to csvperrorf using line, column and value information
 * from CSV `col`.
 */
JUPITER_DECL
void csvperrorf_col(const char *fname, csv_column *col,
                    csv_error_level errlevel, const char *fmt, ...);

/**
 * @brief Prints error text with CSV column
 * @param fname file name
 * @param col CSV's column
 * @param errlevel Error level indicate
 * @param csv_errcode CSV error code (errors from csv.h/csvutil.h fuctions)
 * @param syscall_errcode system call error code (errors from errno)
 * @param ext_errcode external library error code (MPI or geometry)
 * @param additional_info additional info text some errors.
 *
 * Equivalent to csvperror using line, column and value information
 * from CSV `col`.
 */
JUPITER_DECL
void csvperror_col(const char *fname, csv_column *col, csv_error_level errlevel,
                   csv_error csv_errcode, int syscall_errcode, int ext_errcode,
                   const char *additional_info);

/**
 * @brief Prints error text with CSV row
 * @param fname file name
 * @param row CSV's row
 * @param index index for column
 * @param errlevel Error level indicate
 * @param fmt Error text format.
 * @param ap printf arguments.
 *
 * Equivalent to csvperrorv_col using `getColumnOfCSV(row, index)`.
 */
JUPITER_DECL
void csvperrorv_row(const char *fname, csv_row *row, int index,
                    csv_error_level errlevel, const char *fmt, va_list ap);

/**
 * @brief Prints error text with CSV row
 * @param fname file name
 * @param row CSV's row
 * @param index index for column
 * @param errlevel Error level indicate
 * @param fmt Error text format.
 * @param ... printf arguments.
 *
 * Equivalent to csvperrorf_col using `getColumnOfCSV(row, index)`.
 */
JUPITER_DECL
void csvperrorf_row(const char *fname, csv_row *row, int index,
                    csv_error_level errlevel, const char *fmt, ...);

/**
 * @brief Prints error text with CSV row
 * @param fname file name
 * @param row CSV's row
 * @param index index of column
 * @param errlevel Error level indicate
 * @param csv_errcode CSV error code (errors from csv.h/csvutil.h fuctions)
 * @param syscall_errcode system call error code (errors from errno)
 * @param ext_errcode external library error code (MPI or geometry)
 * @param additional_info additional info text some errors.
 *
 * Equivalent to csvperror_col using `getColumnOfCSV(row, index)`.
 */
JUPITER_DECL
void csvperror_row(const char *fname, csv_row *row, int index,
                   csv_error_level errlevel, csv_error csv_errcode,
                   int syscall_errcode, int ext_errcode,
                   const char *additional_info);

/**
 * @brief Print errors or warnings to use with SET_P* macros
 * @param fname file name of CSV
 * @param found_col column data which is read from.
 * @param pelvl Error level to be used when print error.
 * @param format Format of message
 * @param ... Argument to format
 *
 * See csvperrorf for format info.
 *
 * Prints specified message only if found_col is not NULL.
 * The line, column and info is taken from found_col.
 */
JUPITER_DECL
void set_p_perror_internal(const char *fname, csv_column *found_col,
                           csv_error_level pelvl, const char *format, ...);

/**
 * @brief Test the value is range of min and max, and if not acceptable,
 *        print error.
 * @param fname file name of CSV
 * @param type_name stringified type name
 * @param found_col column data which is read from.
 * @param val value to be tested.
 * @param min minimum value
 * @param max maximum value
 * @param min_incl if ON, include min value to be accepted
 * @param max_incl if ON, include max value to be accepted
 * @param pelvl Error level to be used when print error
 * @param format Format of message
 * @param ... Argument to format
 * @return 0 if accepted, otherwise rejected
 */
JUPITER_DECL
int set_p_perror_range_internal(const char *fname, csv_column *found_col,
                                double val, double min, double max,
                                int min_incl, int max_incl,
                                csv_error_level pelvl, const char *format, ...);

/**
 * @brief Setter function prototype
 * @param dest pointer to store the result.
 * @param value_on_fail Pointer to value should be set on failure.
 * @param fname Source file name of @p c
 * @praam c Reading column (NULL if not found)
 * @param errlevel Error level should be used for printing error
 * @param perror 1 if caller wants to print error
 * @return 0 if success, non-0 if error.
 */
typedef int set_p_setter_func_type(void *dest, const void *value_on_fail,
                                   const char *fname, csv_column *c,
                                   csv_error_level errlevel, int perror);

JUPITER_DECL
int set_p_internal_base(void *dest, const void *value_on_fail,
                        set_p_setter_func_type *setter, const char *type_name,
                        const char *keystr, int index, int perror_not_found,
                        int perror_invalid, csv_error_level errlevel_not_found,
                        csv_error_level errlevel_invalid, const char *fname,
                        csv_data *csv, csv_row **found_row,
                        csv_column **found_col, int *stat);

/**
 * @brief template for defineing set_p_base_(type)
 * @param type type name to implement
 *
 * Use macro SET_P_DESTTYPE_(type) to get actual required type.
 * Use macro SET_P_ERRVALTYPE_(type) to get actual error value type.
 */
#define SET_P_DEFINE_SET_P_BASE_F(type)                                        \
  static inline int set_p_base_##type(SET_P_DESTTYPE_##type(dest),             \
                                      const char *keystr, int index,           \
                                      SET_P_ERRVALTYPE_##type(value_on_fail),  \
                                      int perror_not_found,                    \
                                      int perror_invalid,                      \
                                      csv_error_level errlevel_not_found,      \
                                      csv_error_level errlevel_invalid,        \
                                      const char *fname, csv_data *csv,        \
                                      csv_row **found_row,                     \
                                      csv_column **found_col, int *stat)       \
  {                                                                            \
    return set_p_internal_base(dest, &value_on_fail, csv_to_##type, #type,     \
                               keystr, index, perror_not_found,                \
                               perror_invalid, errlevel_not_found,             \
                               errlevel_invalid, fname, csv, found_row,        \
                               found_col, stat);                               \
  }

struct set_p_source_data {
  csv_data *csv;
  const char *fname;
  csv_row **found_row;
  csv_column **found_col;
};

static inline struct set_p_source_data
_set_p_source_set(csv_data *csv, const char *fname, csv_row **found_row,
                  csv_column **found_col);

#define SET_P_SOURCE_VARNAME _set_p_source
#define SET_P_SOURCE_DECL()  struct set_p_source_data SET_P_SOURCE_VARNAME

/**
 * Obtain currently bound source CSV data
 *
 * You cannot assign via this macro. Use `SET_P_FROM()` to change.
 */
#define SET_P_SOURCE_CSV() ((csv_data *const)SET_P_SOURCE_VARNAME.csv)

/**
 * Obtain currently bound source CSV file name.
 *
 * You cannot assign via this macro. Use `SET_P_FROM()` to change.
 */
#define SET_P_SOURCE_FILENAME() ((const char *const)SET_P_SOURCE_VARNAME.fname)

/**
 * Obtain or set current row in source CSV.
 *
 * This macro allows to assign as the referenced variable is also
 * allowed to do this.
 */
#define SET_P_SOURCE_ROW() (*(SET_P_SOURCE_VARNAME.found_row))

/**
 * Obtain or set current column in source CSV.
 *
 * This macro allows to assign as the referenced variable is also
 * allowed to do this.
 */
#define SET_P_SOURCE_COL() (*(SET_P_SOURCE_VARNAME.found_col))


/**
 * @brief Initialize the source variables for SET_P*() macros.
 * @param csv CSV data to read from (used in SET_P)
 * @param fname The file name of CSV source come from
 * @param found_row Row pointer to store the current row position
 * @param found_col Column pointer to store the current column position
 *
 * This macro uses `_set_p_source` as the variable name.
 *
 * @warning You can change the source for separate scope, but you have
 * to assign different row and column pointer variable. Otherwise, it
 * may cause unexpected behavior.
 *
 * ```c
 * SET_P_INIT(...); // source 1
 *
 * SET_P(...);  // (a)
 *
 * if (...) {
 *   SET_P_NEXT(...); // (b)
 * }
 *
 * {
 *   csv_row *r;
 *   csv_column *c;
 *   SET_P_INIT(..., &r, &c); // source 2
 *   SET_P(...); // (c)
 * }
 *
 * SET_P_NEXT(...); // (d)
 * ```
 *
 * - (a), (b) and (d) reads from source 1. (d) will be read from the next
 *   of (b) if the `if` condition was true and (a) otherwise.
 * - (c) reads from source 2.
 *
 * `SET_P_FROM` and `SET_P_RESET` allows you to change the source
 * without changing scope.
 */
#define SET_P_INIT(csv, fname, found_row, found_col)                           \
  SET_P_SOURCE_DECL() = _set_p_source_set(csv, fname, found_row, found_col)

/**
 * @brief Initialize the source variables for SET_P*() macros, without
 *        location variables
 * @param csv CSV data to read from (used in SET_P)
 * @param fname The file name of CSV source come from
 *
 * This macro declares a new pointer for storing the location. They
 * are accessible via `SET_P_SOURCE_ROW()` and `SET_P_SOURCE_COL()`.
 *
 * If you use this macro, you should start setting from `SET_P()`.
 * Otherwise, it may cause an unexpected behavior.
 */
#define SET_P_INIT_NOLOC(csv, fname) \
  SET_P_INIT(csv, fname, &(csv_row *){0}, &(csv_column *){0})

/**
 * @brief Change the source of SET_P*() macros.
 * @param csv CSV data to read from (used in SET_P)
 * @param fname The file name of CSV source come from
 * @param found_row Row pointer to store the current row position
 * @param found_col Column pointer to store the current column position
 *
 * @note To change the location (for SET_P_CURRENT() etc.), you don't need
 * to call this macro:
 *
 * ```c
 * csv_data *csv1, *csv2;
 * const char *fname1, *fname2;
 * csv_row *row1;
 * csv_column *col1;
 *
 * SET_P_INIT(csv1, fname1, &row1, &col1);
 *
 * row1 = ...; // You can change the location just assigning to there.
 * col1 = ...;
 * SET_P_CURRENT(...);
 *
 * // You have to use this macro to change csv (and file name) data.
 * SET_P_RESET(csv2, fname2, &row1, &col1);
 * SET_P(...);
 * ```
 */
#define SET_P_RESET(csv, fname, found_row, found_col)                          \
  (SET_P_SOURCE_VARNAME = _set_p_source_set(csv, fname, found_row, found_col))

/**
 * @brief Change the source of SET_P*() macros.
 * @param csv CSV data to read from (used in SET_P)
 * @param fname The file name of CSV source come from
 *
 * Similar to `SET_P_RESET()` but just changes source CSV data and
 * file name only.
 *
 * You should start setting from `SET_P()`. Otherwise, it may cause
 * unexpected behavior.
 */
#define SET_P_FROM(csv, fname)                                                 \
  SET_P_RESET(csv, fname, SET_P_SOURCE_VARNAME.found_row,                      \
              SET_P_SOURCE_VARNAME.found_col)

/**
 * @brief Common macro for SET_P* macros
 * @param var Variable name to store result.
 * @param type Type name (function name of csv_to_(*))
 * @param keystr Keyname to find or NULL for relative find
 * @param index Column index to refer, offset if keystr is NULL
 * @param value_on_fail The value to be stored when errors occured.
 * @param perror_not_found If non-0 value, print errors that the
 *                         specified column is not found.
 * @param perror_invalid If non-0 value, print errors that the spcified column
 *                       text format is invalid and cannot be representable
 *                       in the result type
 * @param errlevel_not_found Error level to be uesd for not found error
 * @param errlevel_invalid Error level to be used for invalid format error
 * @param stat If a pointer to int is given, set to ON when any errors occured
 * @return 0 if success, 1 if not found, or other non-0 value for type
 *    specific errors.
 *
 * If you need some special case, you can use this macro. For example,
 *
 *     SET_P_BASE(&ret, int, NULL, -1, 0, 1, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
 *
 * can get the value from the previous column of found_col. And,
 *
 *     SET_P_BASE(&ret, int, NULL, 2, 0, 1, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
 *
 * for second next column of found_col.
 *
 * To use this macro, you need to call `SET_P_INIT()` macro to set the
 * source of data, before this macro.
 */
#define SET_P_BASE(var, type, keystr, index, value_on_fail, perror_not_found,  \
                   perror_invalid, errlevel_not_found, errlevel_invalid, stat) \
  set_p_base_##type(var, keystr, index, value_on_fail, perror_not_found,       \
                    perror_invalid, errlevel_not_found, errlevel_invalid,      \
                    SET_P_SOURCE_VARNAME.fname, SET_P_SOURCE_VARNAME.csv,      \
                    SET_P_SOURCE_VARNAME.found_row,                            \
                    SET_P_SOURCE_VARNAME.found_col, stat)

/**
 * @brief Set var to the value at row of keystr and column of index.
 *
 * value_on_fail set value on fail (in other words, it will be default)
 *
 * To use this macro, you need to call `SET_P_INIT()` macro to set the
 * source of data, before this macro.
 */
#define SET_P(var, type, keystr, index, value_on_fail)                         \
  SET_P_BASE(var, type, keystr, (index), (value_on_fail), 1, 1, CSV_EL_WARN,   \
             CSV_EL_ERROR, NULL)

/**
 * Same as SET_P, but do not print not found error.
 */
#define SET_P_PASS_NOTFOUND(var, type, keystr, index, value_on_fail)           \
  SET_P_BASE(var, type, keystr, (index), (value_on_fail), 0, 1, CSV_EL_WARN,   \
             CSV_EL_ERROR, NULL)

/**
 * Same as SET_P, but do not print invalid format error.
 */
#define SET_P_PASS_INVAL(var, type, keystr, index, value_on_fail, stat)        \
  SET_P_BASE(var, type, keystr, (index), (value_on_fail), 1, 0, CSV_EL_WARN,   \
             CSV_EL_ERROR, stat)

/**
 * Same as SET_P, but make error when not found rather than warning
 */
#define SET_P_REQUIRED(var, type, keystr, index, value_on_fail, stat)          \
  SET_P_BASE(var, type, keystr, (index), (value_on_fail), 1, 1, CSV_EL_ERROR,  \
             CSV_EL_ERROR, stat)

/**
 * Increment column and set.
 *
 * `csv` and `found_row` is not mandatory, but they should be set if
 * possible. Otherwise, errors will not be printed, because we
 * considers errors were printed via SET_P call (which sets found_row)
 * when found_row is NULL.
 */
#define SET_P_NEXT(var, type, value_on_fail)                                   \
  SET_P_BASE(var, type, NULL, 1, (value_on_fail), 1, 1, CSV_EL_WARN,           \
             CSV_EL_ERROR, NULL)

/**
 * Same as SET_P_NEXT, do not print error messages on not found error.
 */
#define SET_P_NEXT_PASS_NOTFOUND(var, type, value_on_fail)                     \
  SET_P_BASE(var, type, NULL, 1, (value_on_fail), 0, 1, CSV_EL_WARN,           \
             CSV_EL_ERROR, NULL)

/**
 * Same as SET_P_NEXT, do not print error messages on invalid format error
 */
#define SET_P_NEXT_PASS_INVAL(var, type, value_on_fail, stat)                  \
  SET_P_BASE(var, type, NULL, 1, (value_on_fail), 1, 0, CSV_EL_WARN,           \
             CSV_EL_ERROR, stat)

/**
 * Same as SET_P_NEXT, but make error when not fonud rather than warning
 */
#define SET_P_NEXT_REQUIRED(var, type, value_on_fail, stat)                    \
  SET_P_BASE(var, type, NULL, 1, (value_on_fail), 1, 1, CSV_EL_ERROR,          \
             CSV_EL_ERROR, stat)

/**
 * SET_P for found_row and found_col is already set.
 *
 * No error message will be shown for found_col is NULL
 */
#define SET_P_CURRENT(var, type, value_on_fail)                                \
  SET_P_BASE(var, type, NULL, 0, (value_on_fail), 0, 1, CSV_EL_WARN,           \
             CSV_EL_ERROR, NULL)

/**
 * SET_P for found_row and found_col is already set.
 *
 * No error message will be shown for both found_col is NULL and
 * invalid format value
 */
#define SET_P_CURRENT_PASS_INVAL(var, type, value_on_fail, stat)               \
  SET_P_BASE(var, type, NULL, 0, (value_on_fail), 0, 0, CSV_EL_WARN,           \
             CSV_EL_ERROR, NULL)

/**
 * @brief shortcut macro for print errors on last SET_P.
 *
 * 1st argument: error level, follower of CSV_EL_. (ex. ERROR)
 * 2nd argument: format text
 * 3rd argument and so far: parameters for format text.
 *
 * This error message will shown on no error occured in SET_P(_NEXT).
 *
 * This macro does not use `csv` and `found_row`.
 */
#define SET_P_PERROR(el, ...)                                                  \
  set_p_perror_internal(SET_P_SOURCE_VARNAME.fname,                            \
                        *(SET_P_SOURCE_VARNAME.found_col), CSV_EL_##el,        \
                        __VA_ARGS__)

/**
 * @brief shortcut macro for printing errors on last SET_P.
 * @param val value to be checked
 * @param min acceptable minimum value
 * @param max acceptable maximum value
 * @param min_incl if ON, accept when val is equal to min, otherwise reject
 * @paarm max_incl if ON, accept when val is equal to max, otherwise reject
 * @param el error level, follower of CSV_EL. (ex. ERROR)
 * @param ... Format text (for first argument of `...`)
 * @param ... parameters for format text (remaining part of `...`)
 * @return 0 if val is not accepted, otherwise accepted.
 *
 * If a numerical value `val` is not range from min to max, prints
 * error message.
 *
 * The value is evaluated in type of `double` (nor `int` or `type`),
 * which means you can safely pass an `int` value, if the bit width
 * of fractional part of double type is greater than `int` type.
 *
 * Hint: IEEE754 compliant double has 53 bits for fractional part.
 *
 * Testing partial range of floating-point values is little complicated
 * because the user can input NaN/Inf value. For example, to print errors
 * that which accept 0 to 1 value:
 *
 *     if (val < 0.0 || val > 1.0)
 *        SET_P_PERROR(...);
 *
 * is incomplete (Comparing with NAN is always *false*) and must be
 *
 *     if (isnan(val) || val < 0.0 || val > 1.0)
 *        SET_P_PERROR(...);
 *
 *
 * One more example, the error condition that positive value (but finite) is:
 *
 *     if (!isfinite(val) || val <= 0.0)
 *        SET_P_PERROR(...);
 *
 * This macro supports such comparisons easily:
 *
 *     SET_P_PERROR_RANGE(val, 0.0, 1.0, ON, ON, ERROR, "must be 0<=val<=1");
 *
 * and
 *
 *     SET_P_PERROR_RANGE(val, 0.0, INFINITY, OFF, OFF, ERROR, "must be positive
 * finite value");
 *
 * This macro does not use `csv` and `found_row`.
 */
#define SET_P_PERROR_RANGE(val, min, max, min_incl, max_incl, el, ...)         \
  set_p_perror_range_internal(SET_P_SOURCE_VARNAME.fname,                      \
                              *(SET_P_SOURCE_VARNAME.found_col), (val), (min), \
                              (max), (min_incl), (max_incl), CSV_EL_##el,      \
                              __VA_ARGS__)

/**
 * @brief shortcut macro for testing values on SET_P* macros.
 * @param val value to be checked
 * @param el error level
 * @param ... format text for first, parameter for format text remaining)
 * @return see SET_P_PERROR_RANGE.
 *
 * Accepts any finite value (not NAN or INIFINITY).
 *
 * @note any `int` value `val` will be accepted even if double coversion is
 *       lossy (because there are no infinite value in `int`).
 */
#define SET_P_PERROR_FINITE(val, el, ...)                                      \
  SET_P_PERROR_RANGE((val), -INFINITY, INFINITY, OFF, OFF, el, __VA_ARGS__)

/**
 * @brief shortcut macro for testing values on SET_P* macros.
 * @param val value to be checked
 * @param min minimum value
 * @param incl whether accept min value
 * @param incl_inf whether accept INFINITY
 * @param el error level
 * @param ... format text for first, parameter for format text remaining)
 * @return see SET_P_PERROR_RANGE.
 *
 * Accepts any finite value (or INFINITY if incl_inf is ON) when
 * val is greater (or equal to) than min.
 */
#define SET_P_PERROR_GREATER(val, min, incl, incl_inf, el, ...)                \
  SET_P_PERROR_RANGE((val), (min), INFINITY, (incl), (incl_inf), el,           \
                     __VA_ARGS__)

/**
 * @brief shortcut macro for testing values on SET_P* macros.
 * @param val value to be checked
 * @param max maximum value
 * @param incl whether accept min value
 * @param incl_inf whether accept -INFINITY
 * @param el error level
 * @param ... format text for first, parameter for format text remaining)
 * @return see SET_P_PERROR_RANGE.
 *
 * Accepts any finite value (or -INFINITY if incl_inf is ON) when
 * val is less than (or equal to) max.
 */
#define SET_P_PERROR_LESS(val, max, incl, incl_inf, el, ...)                   \
  SET_P_PERROR_RANGE((val), -INFINITY, (max), (incl_inf), (incl), el,          \
                     __VA_ARGS__)

/**
 * If c in not valid or contains empty string, set *endptr to NULL.
 * If c is valid, but invalid integer value, endptr points non-'\0'.
 * On error, returns 0.
 *
 * For base, see man strtol(3) (0 for auto with prefix, or 2-36).
 *
 * The pointee of endptr is available until freeCSV is called.
 */
JUPITER_DECL
int csv_to_int_base(csv_column *c, const char **endptr, int base);

/**
 * Convert from decimal integer text to integer
 */
JUPITER_DECL
int csv_to_int(void *dest, const void *value_on_fail, const char *fname,
               csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_int(x) int *x
#define SET_P_ERRVALTYPE_int(x) int x
SET_P_DEFINE_SET_P_BASE_F(int)

/**
 * Convert from floating point value text to JUPITER type
 *
 * There is no way to specify base in strtod.
 * (i.e. if you start with 0x, it will be treated as hexadecimal value)
 *
 * Info: The name of function is 'double' for historical reason. Please use
 *       'exact_double' for 'double' variable.
 */
JUPITER_DECL
int csv_to_double(void *dest, const void *value_on_fail, const char *fname,
                  csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_double(x) type *x
#define SET_P_ERRVALTYPE_double(x) type x
SET_P_DEFINE_SET_P_BASE_F(double)

/**
 * Convert from floating point value text to double precision floating
 * point value.
 *
 * There is no way to specify base in strtod.
 * (i.e. if you start with 0x, it will be treated as hexadecimal value)
 */
JUPITER_DECL
int csv_to_exact_double(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_exact_double(x) double *x
#define SET_P_ERRVALTYPE_exact_double(x) double x
SET_P_DEFINE_SET_P_BASE_F(exact_double)

/**
 * Just read the column data
 *
 * Returning pointer is part of CSV data @p c.
 */
JUPITER_DECL
int csv_to_const_charp(void *dest, const void *value_on_fail, const char *fname,
                       csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_const_charp(x) const char **x
#define SET_P_ERRVALTYPE_const_charp(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(const_charp)

/**
 * Just read the column data.
 *
 * Returning pointer is copied.
 */
JUPITER_DECL
int csv_to_charp(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_charp(x) char **x
#define SET_P_ERRVALTYPE_charp(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(charp)

struct csv_to_FILEn_data
{
  char **filename;
  int *has_r;
};

/**
 * Filename, may embedded with rank numbers.
 *
 * Returns NULL if the name is "-".
 *
 * You must free returned pointer.
 *
 * `has_r` member holds whether given filename include `%r` or
 * not. See `format_has_mpi_rank()` for more info.
 */
JUPITER_DECL
int csv_to_FILEn(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_FILEn(x) struct csv_to_FILEn_data *x
#define SET_P_ERRVALTYPE_FILEn(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(FILEn)

/**
 * Filename, **with** embedded rank numbers.
 *
 * If MPI rank number embed has not been performed, this function will fail.
 *
 * @note This function silently returns NULL if the given name is "-",
 * which does not include `%r` notation.
 *
 * You must free returned pointer.
 */
JUPITER_DECL
int csv_to_FILEn_withMPI(void *dest, const void *value_on_fail,
                         const char *fname, csv_column *c,
                         csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_FILEn_withMPI(x) char **x
#define SET_P_ERRVALTYPE_FILEn_withMPI(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(FILEn_withMPI)

/**
 * Filename, **without** embedded rank numbers.
 *
 * If MPI rank number embed is performed, this function will fail.
 *
 * Returns NULL if the name is "-".
 *
 * You must free returned pointer.
 */
JUPITER_DECL
int csv_to_FILEn_noMPI(void *dest, const void *value_on_fail, const char *fname,
                       csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_FILEn_noMPI(x) char **x
#define SET_P_ERRVALTYPE_FILEn_noMPI(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(FILEn_noMPI)

/**
 * Directory name, with embedded rank numbers.
 *
 * You must free returned pointer.
 */
JUPITER_DECL
int csv_to_DIRn(void *dest, const void *value_on_fail, const char *fname,
                csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_DIRn(x) char **x
#define SET_P_ERRVALTYPE_DIRn(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(DIRn)

/**
 * Convert ON/OFF text
 */
JUPITER_DECL
int csv_to_bool(void *dest, const void *value_on_fail, const char *fname,
                csv_column *c, csv_error_level errlevel, int perror);

#if defined(__bool_true_false_are_defined) && !defined(__cplusplus)
#undef bool
#endif
#define SET_P_DESTTYPE_bool(x) int *x
#define SET_P_ERRVALTYPE_bool(x) int x
SET_P_DEFINE_SET_P_BASE_F(bool)
#if defined(__bool_true_false_are_defined) && !defined(__cplusplus)
#define bool _Bool
#endif

/**
 * Convert PLIC / THINC_AWLIC / THINC / THINC_WLIC text
 */
JUPITER_DECL
int csv_to_interface_capturing_scheme(void *dest, const void *value_on_fail, const char *fname,
                    csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_interface_capturing_scheme(x) int *x
#define SET_P_ERRVALTYPE_interface_capturing_scheme(x) int x
SET_P_DEFINE_SET_P_BASE_F(interface_capturing_scheme)


/**
 * Convert WALL/SLIP/... text
 */
JUPITER_DECL
int csv_to_boundary(void *dest, const void *value_on_fail, const char *fname,
                    csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_boundary(x) int *x
#define SET_P_ERRVALTYPE_boundary(x) int x
SET_P_DEFINE_SET_P_BASE_F(boundary)

/**
 * Convert INSULATION/... text.
 */
JUPITER_DECL
int csv_to_tboundary(void *dest, const void *value_on_fail, const char *fname,
                     csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_tboundary(x) int *x
#define SET_P_ERRVALTYPE_tboundary(x) int x
SET_P_DEFINE_SET_P_BASE_F(tboundary)

/**
 * Convert NEUMANN/CONST text.
 */
int csv_to_out_p_cond(void *dest, const void *value_on_fail, const char *fname,
                      csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_out_p_cond(x) out_p_cond *x
#define SET_P_ERRVALTYPE_out_p_cond(x) out_p_cond x
SET_P_DEFINE_SET_P_BASE_F(out_p_cond)

/**
 * Convert VOF phase definition like SOLID, LIQUID, GAS
 */
JUPITER_DECL
int csv_to_vof_phase(void *dest, const void *value_on_fail, const char *fname,
                     csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_vof_phase(x) geom_vof_phase *x
#define SET_P_ERRVALTYPE_vof_phase(x) geom_vof_phase x
SET_P_DEFINE_SET_P_BASE_F(vof_phase)

/**
 * Convert data operation action like SET, ADD, SUB, MUL, NONE
 */
JUPITER_DECL
int csv_to_geom_data_op(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_geom_data_op(x) geom_data_operator *x
#define SET_P_ERRVALTYPE_geom_data_op(x) geom_data_operator x
SET_P_DEFINE_SET_P_BASE_F(geom_data_op)

/**
 * Convert shape operation action like SET, PUSH, OR, ADD, MUL, ...
 */
JUPITER_DECL
int csv_to_geom_shape_op(void *dest, const void *value_on_fail,
                         const char *fname, csv_column *c,
                         csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_geom_shape_op(x) geom_shape_operator *x
#define SET_P_ERRVALTYPE_geom_shape_op(x) geom_shape_operator x
SET_P_DEFINE_SET_P_BASE_F(geom_shape_op)

/**
 * Convert geometry_shape like BOX, ...
 */
JUPITER_DECL
int csv_to_geom_shape(void *dest, const void *value_on_fail, const char *fname,
                      csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_geom_shape(x) geom_shape *x
#define SET_P_ERRVALTYPE_geom_shape(x) geom_shape x
SET_P_DEFINE_SET_P_BASE_F(geom_shape)

/**
 * Convert geometry_surface_shape like PARALLELOGRAM, ...
 */
JUPITER_DECL
int csv_to_geom_surface_shape(void *dest, const void *value_on_fail,
                              const char *fname, csv_column *c,
                              csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_geom_surface_shape(x) geom_surface_shape *x
#define SET_P_ERRVALTYPE_geom_surface_shape(x) geom_surface_shape x
SET_P_DEFINE_SET_P_BASE_F(geom_surface_shape)

/**
 * Convert initializer function like CONST, LINEAR
 */
JUPITER_DECL
int csv_to_init_func(void *dest, const void *value_on_fail, const char *fname,
                     csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_init_func(x) geom_init_func *x
#define SET_P_ERRVALTYPE_init_func(x) geom_init_func x
SET_P_DEFINE_SET_P_BASE_F(init_func)

/**
 * Convert trip control type, CONST
 */
JUPITER_DECL
int csv_to_trip_control(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_trip_control(x) trip_control *x
#define SET_P_ERRVALTYPE_trip_control(x) trip_control x
SET_P_DEFINE_SET_P_BASE_F(trip_control)

/**
 * Convert boundary direction, WEST, EAST, ....
 */
JUPITER_DECL
int csv_to_boundary_dir(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_boundary_dir(x) boundary_direction *x
#define SET_P_ERRVALTYPE_boundary_dir(x) boundary_direction x
SET_P_DEFINE_SET_P_BASE_F(boundary_dir)

/**
 * Convert inlet boundary direction, NORMAL, ...
 */
JUPITER_DECL
int csv_to_inlet_dir(void *dest, const void *value_on_fail, const char *fname,
                     csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_inlet_dir(x) surface_inlet_dir *x
#define SET_P_ERRVALTYPE_inlet_dir(x) surface_inlet_dir x
SET_P_DEFINE_SET_P_BASE_F(inlet_dir)

/**
 * Convert tm_func2_model, LIQUIDUS_FE_ZR etc.
 */
JUPITER_DECL
int csv_to_tm_func2_model(void *dest, const void *value_on_fail,
                          const char *fname, csv_column *c,
                          csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_tm_func2_model(x) tm_func2_model *x
#define SET_P_ERRVALTYPE_tm_func2_model(x) tm_func2_model x
SET_P_DEFINE_SET_P_BASE_F(tm_func2_model)

/**
 * Convert dc_func2_model, SUS304_B4C etc.
 */
JUPITER_DECL
int csv_to_dc_func2_model(void *dest, const void *value_on_fail,
                          const char *fname, csv_column *c,
                          csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_dc_func2_model(x) dc_func2_model *x
#define SET_P_ERRVALTYPE_dc_func2_model(x) dc_func2_model x
SET_P_DEFINE_SET_P_BASE_F(dc_func2_model)

/**
 * Convert solid_form, IBM or POROUS
 */
JUPITER_DECL
int csv_to_solid_form(void *dest, const void *value_on_fail, const char *fname,
                      csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_solid_form(x) enum solid_form *x
#define SET_P_ERRVALTYPE_solid_form(x) enum solid_form x
SET_P_DEFINE_SET_P_BASE_F(solid_form)

/**
 * Convert binary_output_mode, BYPROCESS, UNIFY_MPI, UNIFY_GATHER.
 */
JUPITER_DECL
int csv_to_binary_output_mode(void *dest, const void *value_on_fail,
                              const char *fname, csv_column *c,
                              csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_binary_output_mode(x) binary_output_mode *x
#define SET_P_ERRVALTYPE_binary_output_mode(x) binary_output_mode x
SET_P_DEFINE_SET_P_BASE_F(binary_output_mode)

/**
 * Convert tempdep_property_type, CONST, TABLE, POLY, POLY_L, ARRHENIUS
 */
JUPITER_DECL
int csv_to_tempdep_property_type(void *dest, const void *value_on_fail,
                                 const char *fname, csv_column *c,
                                 csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_tempdep_property_type(x) tempdep_property_type *x
#define SET_P_ERRVALTYPE_tempdep_property_type(x) tempdep_property_type x
SET_P_DEFINE_SET_P_BASE_F(tempdep_property_type)

/**
 * Convert ox_reaction_rate_model, UH, BJ, CP, LS, PC, ...
 */
JUPITER_DECL
int csv_to_ox_kp_model(void *dest, const void *value_on_fail, const char *fname,
                       csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_ox_kp_model(x) enum ox_reaction_rate_model *x
#define SET_P_ERRVALTYPE_ox_kp_model(x) enum ox_reaction_rate_model x
SET_P_DEFINE_SET_P_BASE_F(ox_kp_model)

/**
 * Convert component_phase_name, SOLID, LIQUID, GAS
 */
JUPITER_DECL
int csv_to_component_phase(void *dest, const void *value_on_fail,
                           const char *fname, csv_column *c,
                           csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_component_phase(x) component_phase_name *x
#define SET_P_ERRVALTYPE_component_phase(x) component_phase_name x
SET_P_DEFINE_SET_P_BASE_F(component_phase)

/**
 * Convert LPT time-integration scheme model, ADAMS_BASHFORTH_2, ...
 *
 * This method is still available even if LPT is not enabled. And
 * for such case, this function always returns -1.
 */
JUPITER_DECL
int csv_to_LPTts(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_LPTts(x) int *x
#define SET_P_ERRVALTYPE_LPTts(x) int x
SET_P_DEFINE_SET_P_BASE_F(LPTts)

/**
 * Convert LPT heat exchange scheme model, RANZ_MARSHALL, ...
 *
 * This method is still available even if LPT is not enabled or using LPT. And
 * for such case, this function always returns -1.
 */
JUPITER_DECL
int csv_to_LPTht(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_LPTht(x) int *x
#define SET_P_ERRVALTYPE_LPTht(x) int x
SET_P_DEFINE_SET_P_BASE_F(LPTht)

/**
 * Convert postprocess type, SUM, VORTICITY, ...
 */
JUPITER_DECL
int csv_to_postp(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_postp(x) int *x
#define SET_P_ERRVALTYPE_postp(x) int x
SET_P_DEFINE_SET_P_BASE_F(postp)

/**
 * Convert mask and mask function type, GEOMETRY, GRID, EXTENT, ...
 */
JUPITER_DECL
int csv_to_maskp(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_maskp(x) int *x
#define SET_P_ERRVALTYPE_maskp(x) int x
SET_P_DEFINE_SET_P_BASE_F(maskp)

/**
 * Convert field variable and field function type, CALCULRATOR, TABLE, ...
 */
JUPITER_DECL
int csv_to_fieldp(void *dest, const void *value_on_fail, const char *fname,
                  csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_fieldp(x) int *x
#define SET_P_ERRVALTYPE_fieldp(x) int x
SET_P_DEFINE_SET_P_BASE_F(fieldp)

/**
 * Convert extend mode for TABLE field variable, EXTRAPOLATE, CIRCULAR, ...
 */
JUPITER_DECL
int csv_to_fv_tabbnd(void *dest, const void *value_on_fail, const char *fname,
                     csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_fv_tabbnd(x) jcntrl_fv_table_extend_mode *x
#define SET_P_ERRVALTYPE_fv_tabbnd(x) jcntrl_fv_table_extend_mode x
SET_P_DEFINE_SET_P_BASE_F(fv_tabbnd)

/**
 * Convert logical operator in control library, ADD, OR, ...
 */
JUPITER_DECL
int csv_to_jcntrl_lop(void *dest, const void *value_on_fail, const char *fname,
                      csv_column *c, csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_jcntrl_lop(x) jcntrl_logical_operator *x
#define SET_P_ERRVALTYPE_jcntrl_lop(x) jcntrl_logical_operator x
SET_P_DEFINE_SET_P_BASE_F(jcntrl_lop)

/**
 * Convert comparator in control library, LESS, LESS_EQ, ...
 */
JUPITER_DECL
int csv_to_jcntrl_compp(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_jcntrl_compp(x) jcntrl_comparator *x
#define SET_P_ERRVALTYPE_jcntrl_compp(x) jcntrl_comparator x
SET_P_DEFINE_SET_P_BASE_F(jcntrl_compp)

/**
 * Convert time step mode for output field variable, NSTEP, TIME
 */
JUPITER_DECL
int csv_to_write_fv_csv_mode(void *dest, const void *value_on_fail,
                             const char *fname, csv_column *c,
                             csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_write_fv_csv_mode(x) jcntrl_write_fv_csv_step_mode *x
#define SET_P_ERRVALTYPE_write_fv_csv_mode(x) jcntrl_write_fv_csv_step_mode x
SET_P_DEFINE_SET_P_BASE_F(write_fv_csv_mode)

/**
 * Convert time step mode for output field variable, NSTEP, TIME
 */
JUPITER_DECL
int csv_to_write_fv_form(void *dest, const void *value_on_fail,
                         const char *fname, csv_column *c,
                         csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_write_fv_form(x) enum write_field_variables_format *x
#define SET_P_ERRVALTYPE_write_fv_form(x) enum write_field_variables_format x
SET_P_DEFINE_SET_P_BASE_F(write_fv_form)

struct csv_to_component_info_data_data
{
  struct component_info_data *dest;
  component_data *comp_data_head;
};

/**
 * Convert component_info_data type (setting by JUPITER ID)
 *
 * The dest type is struct csv_component_info_data_data; this means
 * you must given data of destination pointer and list (head) of
 * comopnent_data. So `SET_P` macro would be like:
 *
 *    struct csv_to_component_info_data_data p = {
 *      .comp_data_head = &prm->comp_data_head
 *    };
 *
 *    p.dest = &cdo->[value1];
 *    SET_P(&p, component_info_data, "keyname1", 0, JUPITER_ID_INVALID);
 *
 * Recommend to use JUPITER_ID_INVALID for invalid default.
 */
JUPITER_DECL
int csv_to_component_info_data(void *dest, const void *value_on_fail,
                               const char *fname, csv_column *c,
                               csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_component_info_data(x) \
  struct csv_to_component_info_data_data *x
#define SET_P_ERRVALTYPE_component_info_data(x) int x
SET_P_DEFINE_SET_P_BASE_F(component_info_data)

/**
 * Convert variable delta function, CONST, CONST_RATIO_INC, ...
 */
JUPITER_DECL
int csv_to_non_uniform_grid_func(void *dest, const void *value_on_fail,
                                 const char *fname, csv_column *c,
                                 csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_non_uniform_grid_func(x) non_uniform_grid_function *x
#define SET_P_ERRVALTYPE_non_uniform_grid_func(x) non_uniform_grid_function x
SET_P_DEFINE_SET_P_BASE_F(non_uniform_grid_func)

struct csv_to_controllable_type_data
{
  controllable_type *dest;
  jcntrl_executive_manager *manager;
  controllable_type *head;
};

/**
 * Convert controllable double type
 *
 * The dest type is struct csv_to_controllable_type_data; this means
 * you must given data of destination pointer, executive manager of
 * database to get from, and, link head is required. So `SET_P` macro
 * would looks like:
 *
 *     struct csv_to_controllable_type_data p = {
 *       .manager = prm->controls,
 *       .head = &prm->control_head,
 *     };
 *
 *     p.dest = &cdo->[value1];
 *     SET_P(&p, controllable_type, "keyname1", 0, 1.0);
 *
 *     p.dest = &cdo->[value2];
 *     SET_P(&p, controllable_type, "keyname2", 0, 1.0);
 *
 * For error value, only single double is allowed.
 *
 * You can use compound literals, but you must enclose variable struct with
 * parentheses. This is because braces are just a charactor on macro
 * expansion. Using compound literals here is not recommended.
 *
 *     SET_P(&((csv_to_controllable_type_data) { ... }), ...);
 */
JUPITER_DECL
int csv_to_controllable_type(void *dest, const void *value_on_fail,
                             const char *fname, csv_column *c,
                             csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_controllable_type(x)                                    \
  struct csv_to_controllable_type_data *x
#define SET_P_ERRVALTYPE_controllable_type(x) double x
SET_P_DEFINE_SET_P_BASE_F(controllable_type)

struct csv_to_control_executive_data
{
  jcntrl_executive_manager_entry **dest;
  jcntrl_executive_manager *manager;
};

/**
 * Converts to any type of control executive
 */
JUPITER_DECL
int csv_to_control_executive(void *dest, const void *value_on_fail,
                             const char *fname, csv_column *c,
                             csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_executive(x) \
  struct csv_to_control_executive_data *x
#define SET_P_ERRVALTYPE_control_executive(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_executive)

struct csv_to_control_grid_data
{
  jcntrl_executive_manager_entry **dest;
  jcntrl_executive_manager *manager;
};

/**
 * Converts to control executive which generates grids
 */
JUPITER_DECL
int csv_to_control_grid(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_grid(x) struct csv_to_control_grid_data *x
#define SET_P_ERRVALTYPE_control_grid(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_grid)

struct csv_to_control_mask_data
{
  jcntrl_executive_manager_entry **dest;
  jcntrl_executive_manager *manager;
};

/**
 * Converts to control executive which generates mask or mask function
 */
JUPITER_DECL
int csv_to_control_mask(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_mask(x) struct csv_to_control_mask_data *x
#define SET_P_ERRVALTYPE_control_mask(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_mask)

struct csv_to_control_geom_data
{
  jcntrl_executive_manager_entry **dest;
  jcntrl_executive_manager *manager;
};

/**
 * Converts to control executive which generates geometry
 */
JUPITER_DECL
int csv_to_control_geom(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_geom(x) struct csv_to_control_geom_data *x
#define SET_P_ERRVALTYPE_control_geom(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_geom)

struct csv_to_control_fvar_data
{
  jcntrl_executive_manager_entry **dest;
  jcntrl_executive_manager *manager;
};

/**
 * Converts to control executive which generates field variable or field
 * function
 */
JUPITER_DECL
int csv_to_control_fvar(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_fvar(x) struct csv_to_control_fvar_data *x
#define SET_P_ERRVALTYPE_control_fvar(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_fvar)

/**
 * Converts to string with variable name to specify variables in a grid
 *
 * Returning pointer is part of CSV data.
 */
JUPITER_DECL
int csv_to_control_varname(void *dest, const void *value_on_fail,
                           const char *fname, csv_column *c,
                           csv_error_level errlevel, int perror);

#define SET_P_DESTTYPE_control_varname(x) const char **x
#define SET_P_ERRVALTYPE_control_varname(x) const char *x
SET_P_DEFINE_SET_P_BASE_F(control_varname)

/**
 * @brief Format MPI rank number into the text.
 * @param buf Buffer to store result (allocates and set)
 * @param text Text to format.
 * @retval -1 Invalid format (whether `%r` is included or not).
 * @retval 0  `%r` is not included.
 * @retval 1  `%r` is included.
 */
JUPITER_DECL
int format_has_mpi_rank(const char *text);

/**
 * @brief Format MPI rank number into the text.
 * @param buf Buffer to store result (allocates and set)
 * @param text Text to format.
 * @return Number of bytes, or -1 if failed.
 *
 * This function does not fail even if @p text does not contain
 * rank-number embedding notation `%r`. Just copies it.
 *
 * To detect rank-number embedding notation is included or not,
 * you can use format_without_mpi_rank() or format_has_mpi_rank() to
 * more appropriate check.
 */
JUPITER_DECL
int format_mpi_rank(char **buf, const char *text);

/**
 * @brief Format text without MPI rank number
 * @param buf Buffer to store result (allocates and set)
 * @param text Text to format.
 * @return Number of bytes, or -1 if failed.
 *
 * This function fails if @p text cotains rank-number embedding notation.
 *
 * @sa format_mpi_rank()
 */
JUPITER_DECL
int format_without_mpi_rank(char **buf, const char *text);

/**
 * @brief Obtain the error value from last SET_P*() macros.
 * @return 0 success, non-0 if errors.
 *
 * The returning value is same as the value that the last SET_P*()
 * macro call returned.
 *
 * This function is used for implementing SET_P_PERROR*() macros.
 *
 * @note There are no agreement for individual returning value, but
 *       negative values tend to represent fatal errors (e.g. memory
 *       allocation failure, logic error, etc), and positive values
 *       are value errors (e.g., not found, cannot represent input
 *       value in the destination type, invalid format, etc).
 *
 * @warning The value is thread-local only if OpenMP is enabled, and
 *          for OpenMP threeads. For other thread parallel mechanisms,
 *          the variable may be shared (i.e., it's not supported, or
 *          it's not guaranteed to be thread local if OpenMP is
 *          enabled).
 */
JUPITER_DECL
int set_p_last_error_value(void);

/**
 * @brief Assign the last error value for SET_P*() macros.
 * @param val The value to set
 *
 * The purpose of this fnuctioin is just to grant write access for the
 * last error value. SET_P*() sets the status unconditionally and no
 * need to use this function manually.
 */
JUPITER_DECL
void set_p_set_last_error_value(int val);

JUPITER_DECL
void csvassert_x_impl(const char *file, const char *func, long line,
                      const char *cond_text, const char *message);
JUPITER_DECL
void csvunreachable_impl(const char *file, const char *func, long line);

#if !defined(NDEBUG)
#ifdef __GNUC__
#define CSVEXPECT(condition) __builtin_expect(!!(intmax_t)(condition), 1)
#else
#define CSVEXPECT(condition) (!!(intmax_t)(condition) == 1)
#endif
#define CSVASSERT_X(condition, message)                                        \
  do {                                                                         \
    if (!CSVEXPECT(condition)) {                                               \
      csvassert_x_impl(__FILE__, __func__, __LINE__, #condition, message);     \
    }                                                                          \
  } while (0)

#define CSVASSERT(condition)                                                   \
  do {                                                                         \
    if (!CSVEXPECT(condition)) {                                               \
      csvassert_x_impl(__FILE__, __func__, __LINE__, #condition, NULL);        \
    }                                                                          \
  } while (0)

#define CSVUNREACHABLE() csvunreachable_impl(__FILE__, __func__, __LINE__);
#else
/* Not debug. */
#define CSVASSERT_X(condition, message)                                        \
  do {                                                                         \
  } while (0)
#define CSVASSERT(condition)                                                   \
  do {                                                                         \
  } while (0)

#ifdef __GNUC__
#define CSVUNREACHABLE() (__builtin_unreachable())
#else
#define CSVUNREACHABLE() ((void)0)
#endif
#endif

/**
 * @macro CSV_DOFALLTHRU
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
 *       CSV_DOFALLTHRU(); // fall through
 *     case 1:
 *       // ....
 *     }
 */
#if (defined(__cplusplus) && __cplusplus >= 201402L) ||                        \
  (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202001L)
#define CSV_DOFALLTHRU() [[fallthrough]]
#elif !defined(__cplusplus)
#if defined(__GNUC__)
#if defined(_INTEL_COMPILER)
#define CSV_DOFALLTHRU() ((void)0)
#elif __GNUC__ >= 7 || (defined(__clang__) && __clang_major__ >= 10)
#define CSV_DOFALLTHRU() __attribute__((fallthrough))
#else
#define CSV_DOFALLTHRU() ((void)0)
#endif
#else
#define CSV_DOFALLTHRU() ((void)0)
#endif
#else
#define CSV_DOFALLTHRU() ((void)0)
#endif

/**
 * @brief CSV_UNUSED
 * @param x Variable that declare to unused.
 *
 * Declares given variable is not used in the block or function.
 */
#define CSV_UNUSED(x) ((void)x)

static inline struct set_p_source_data _set_p_source_set(csv_data *csv,
                                                         const char *fname,
                                                         csv_row **found_row,
                                                         csv_column **found_col)
{
  struct set_p_source_data data;
  data.csv = csv;
  data.fname = fname;
  data.found_row = found_row;
  data.found_col = found_col;
  CSVASSERT(found_row);
  CSVASSERT(found_col);
  return data;
}

#ifdef __cplusplus
}
#endif

#endif // CSVUTIL_H
