#ifndef JUPITER_PRINT_PARAM_CORE_H
#define JUPITER_PRINT_PARAM_CORE_H

#include "common.h"
#include "struct.h"

#include <stdarg.h>

#define PP_HEADER_LEN 70
#define PP_BASELEN 52
#define PP_INVALID_MARK "!!"
#define PP_NA_TEXT "(N/A)"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Base message writer to @p flg->list_fp or @p flg->list_fp_mpi.
 *
 * This function is MPI collective (it's not true if flg->list_fp_mpi is
 * MPI_FILE_NULL, but do not assume).
 */
JUPITER_DECL
void print_param_supportn(flags *flg, const char *text);
JUPITER_DECL
void print_param_supportv(flags *flg, const char *format, va_list ap);
JUPITER_DECL
void print_param_support(flags *flg, const char *format, ...);

/**
 * This function collects share @p cond if flg->list_fp_mpi is not
 * MPI_FILE_NULL, Otherwise just returns cond.
 *
 * This means that the flow is synchronized only if writing to same file for all
 * ranks
 */
JUPITER_DECL
int pp_for_all_mpi(flags *flg, int cond);
JUPITER_DECL
int pp_for_any_mpi(flags *flg, int cond);

/**
 * Writes header line message
 */
JUPITER_DECL
void print_param_header(flags *flg, char border_char, int indent, int pos,
                        int len, const char *format, ...);

/* These functions are for implementing type-base printer */

/* When you allocate data, @p a should handle it. */
typedef const char *pp_format_func(void *a);
typedef void pp_formatv_func(int *argc, const char ***argv, void *a);
typedef void pp_format_clean(void *a);
typedef int pp_format_okfunc(void *a);

struct pp_format_name_data
{
  pp_format_func *name; /*!< formatter of name. */
  void *data;
  pp_format_clean *name_delete;
};

static inline struct pp_format_name_data *
pp_format_name_init(struct pp_format_name_data *p, pp_format_func *formatter,
                    pp_format_clean *deleter, void *format_data)
{
  p->name = formatter;
  p->name_delete = deleter;
  p->data = format_data;
  return p;
}

/**
 * Format name for extending.
 *
 * @note This function is not synchronized (as flags is not provided).
 */
JUPITER_DECL
const char *pp_format_name(struct pp_format_name_data *n);
JUPITER_DECL
void pp_format_name_clean(struct pp_format_name_data *n);

struct pp_format_unit_data
{
  pp_format_func *unit; /*!< formatter for value unit */
  void *data;
  pp_format_clean *unit_delete;
};

static inline struct pp_format_unit_data *
pp_format_unit_init(struct pp_format_unit_data *p, pp_format_func *formatter,
                    pp_format_clean *deleter, void *format_data)
{
  p->unit = formatter;
  p->unit_delete = deleter;
  p->data = format_data;
  return p;
}

/**
 * Format unit for extending.
 *
 * @note This function is not synchronized (as flags is not provided).
 */
JUPITER_DECL
const char *pp_format_unit(struct pp_format_unit_data *u);
JUPITER_DECL
void pp_format_unit_clean(struct pp_format_unit_data *u);

struct pp_format_value_data
{
  pp_format_func *value;        /*!< formatter for singleline data */
  pp_formatv_func *mline_value; /*!< formatter for multiline data */
  pp_format_func *value_null;   /*!< formatter for NULL value */
  pp_format_func *value_except; /*!< formatter when exception thrown */
  pp_format_okfunc *okfunc;     /*!< value check function */
  pp_format_okfunc *okexcept;   /*!< check function when exception */
  void *data;
  pp_format_clean *value_delete;
};

static inline struct pp_format_value_data *
pp_format_value_init(struct pp_format_value_data *p, pp_format_func *formatter,
                     pp_format_func *null_formatter, pp_format_okfunc *okfunc,
                     pp_format_clean *deleter, void *data)
{
  p->value = formatter;
  p->mline_value = NULL;
  p->value_null = null_formatter;
  p->value_delete = deleter;
  p->value_except = NULL;
  p->okfunc = okfunc;
  p->okexcept = NULL;
  p->data = data;
  p->value_delete = deleter;
  return p;
}

static inline struct pp_format_value_data *pp_format_value_init_vec(
  struct pp_format_value_data *p, pp_formatv_func *formeter,
  pp_format_func *null_formatter, pp_format_okfunc *okfunc,
  pp_format_clean *deleter, void *data)
{
  p->value = NULL;
  p->mline_value = formeter;
  p->value_null = null_formatter;
  p->value_delete = deleter;
  p->value_except = NULL;
  p->okfunc = okfunc;
  p->okexcept = NULL;
  p->data = data;
  p->value_delete = deleter;
  return p;
}

/**
 * Format value for extending.
 *
 * @note This function is not synchronized (as flags is not provided) across
 * MPI ranks.
 */
JUPITER_DECL
int pp_format_value(struct pp_format_value_data *v, const char **retp,
                    int *argc, const char ***argv);
JUPITER_DECL
const char *pp_format_value_null(struct pp_format_value_data *v);
JUPITER_DECL
void pp_format_value_clean(struct pp_format_value_data *v);

struct pp_m_line_g_data
{
  flags *f;
  int indent;
  int baselen;
  struct pp_format_name_data *name;
  struct pp_format_value_data *value;
  struct pp_format_unit_data *unit;
  int *nogood;
};

JUPITER_DECL
void pp_m_line_g(struct pp_m_line_g_data *data);

static inline void pp_m_line_f(flags *f, int indent, int baselen,
                               struct pp_format_name_data *name,
                               struct pp_format_value_data *value,
                               struct pp_format_unit_data *unit, int *nogood)
{
  struct pp_m_line_g_data d = {
    .f = f,
    .indent = indent,
    .baselen = baselen,
    .name = name,
    .value = value,
    .unit = unit,
    .nogood = nogood,
  };
  pp_m_line_g(&d);
}

#define PPHead1I(ind, ...) \
  print_param_header(flg, '=', ind, 3, PP_HEADER_LEN - ind, __VA_ARGS__)
#define PPHead1(...) PPHead1I(0, __VA_ARGS__)
#define PPHead2I(ind, ...) \
  print_param_header(flg, '-', ind, 3, PP_HEADER_LEN - ind, __VA_ARGS__)
#define PPHead2(...) PPHead2I(0, __VA_ARGS__)
#define PPHead3I(ind, ...) print_param_header(flg, '.', ind, 3, 3, __VA_ARGS__)
#define PPHead3(...) PPHead3I(0, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
