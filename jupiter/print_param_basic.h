#ifndef JUPITER_PRINT_PARAM_BASIC_H
#define JUPITER_PRINT_PARAM_BASIC_H

/* print_param implementation for basic types (string, int and double) */

#include "common.h"
#include "print_param_core.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PP_charp_name_data
{
  struct pp_format_name_data data;
  const char *message;
};

JUPITER_DECL
const char *PP_charp_name_format(void *a);

static inline struct pp_format_name_data *
PP_charp_name_init(struct PP_charp_name_data *p, const char *text)
{
  p->message = text;
  return pp_format_name_init(&p->data, PP_charp_name_format, NULL, p);
}

struct PP_charp_unit_data
{
  struct pp_format_unit_data data;
  const char *text;
};

JUPITER_DECL
const char *PP_charp_unit_format(void *a);

static inline struct pp_format_unit_data *
PP_charp_unit_init(struct PP_charp_unit_data *p, const char *text)
{
  p->text = text;
  return pp_format_unit_init(&p->data, PP_charp_unit_format, NULL, p);
}

typedef int PP_charp_ok_func(const char *value, void *arg);

struct PP_charp_value_data
{
  struct pp_format_value_data data;
  const char *value;
  const char *value_null;
  PP_charp_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_charp_value_format(void *a);
JUPITER_DECL
const char *PP_charp_value_null_format(void *a);

/* returns true if the value is not NULL (default) */
JUPITER_DECL
int PP_charp_okfunc(void *a);

/* returns true even if the value is NULL */
JUPITER_DECL
int PP_charp_null_okfunc(void *a);

/* returns true when the value is not NULL and not empty string */
JUPITER_DECL
int PP_charp_nonempty_okfunc(void *a);

static inline struct pp_format_value_data *
PP_charp_value_init_m(struct PP_charp_value_data *p, const char *value,
                      const char *value_null, pp_format_okfunc *fokfunc,
                      PP_charp_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->value_null = value_null ? value_null : PP_NA_TEXT;
  p->okfunc = okfunc;
  p->arg = arg;
  return pp_format_value_init(&p->data, PP_charp_value_format,
                              PP_charp_value_null_format, fokfunc, NULL, p);
}

static inline struct pp_format_value_data *
PP_charp_value_init(struct PP_charp_value_data *p, const char *value,
                    const char *value_null, PP_charp_ok_func *okfunc, void *arg)
{
  return PP_charp_value_init_m(p, value, value_null, PP_charp_okfunc, okfunc,
                               arg);
}

struct PP_fname_value_data
{
  struct PP_charp_value_data v;
};

/**
 * File name output
 */
static inline struct pp_format_value_data *
PP_fname_value_init(struct PP_fname_value_data *p, const char *value,
                    PP_charp_ok_func *okfunc, void *arg)
{
  return PP_charp_value_init_m(&p->v, value, "(no file)", PP_charp_null_okfunc,
                               okfunc, arg);
}

//--- int

typedef int PP_int_ok_func(int value, void *arg);

struct PP_int_value_data
{
  struct pp_format_value_data data;
  char *buf;
  int value;
  PP_int_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_int_value_format(void *a);
JUPITER_DECL
const char *PP_int_value_null_format(void *a);
JUPITER_DECL
void PP_int_value_data_delete(void *a);
JUPITER_DECL
int PP_int_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_int_value_init(struct PP_int_value_data *p, int value,
                  PP_int_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->buf = NULL;
  p->okfunc = okfunc;
  p->arg = arg;
  return pp_format_value_init(&p->data, PP_int_value_format,
                              PP_int_value_null_format,
                              PP_int_value_custom_okfunc,
                              PP_int_value_data_delete, p);
}

//--- double

typedef int PP_double_ok_func(double value, void *arg);

struct PP_double_value_data
{
  struct pp_format_value_data data;
  char *buf;
  double value;
  PP_double_ok_func *okfunc;
  void *arg; /*!< extra parameter for okfunc */
};

JUPITER_DECL
const char *PP_double_value_format(void *a);
JUPITER_DECL
const char *PP_double_ns_value_format(void *a);
JUPITER_DECL
const char *PP_double_inf_value_format(void *a);
JUPITER_DECL
const char *PP_double_value_null_format(void *a);
JUPITER_DECL
void PP_double_value_data_delete(void *a);
JUPITER_DECL
int PP_double_value_custom_okfunc(void *a);
JUPITER_DECL
int PP_double_default_value_okfunc(void *a);
JUPITER_DECL
int PP_double_ns_value_okfunc(void *a);
JUPITER_DECL
int PP_double_inf_value_okfunc(void *a);

static inline struct pp_format_value_data *
PP_double_base_value_init(struct PP_double_value_data *p, double value,
                          pp_format_func *formatter, pp_format_okfunc *fokfunc,
                          PP_double_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->buf = NULL;
  p->okfunc = okfunc;
  p->arg = arg;
  return pp_format_value_init(&p->data, formatter, PP_double_value_null_format,
                              fokfunc, PP_double_value_data_delete, p);
}

/*
 * Accepts any value, unless @p okfunc is not given
 */
static inline struct pp_format_value_data *
PP_double_b_value_init(struct PP_double_value_data *p, double value,

                       PP_double_ok_func *okfunc, void *arg)
{
  return PP_double_base_value_init(p, value, PP_double_value_format,
                                   PP_double_value_custom_okfunc, okfunc, arg);
}

/*
 * Accepts any finite value. okfunc will be called when it is finite.
 */
static inline struct pp_format_value_data *
PP_double_value_init(struct PP_double_value_data *p, double value,
                     PP_double_ok_func *okfunc, void *arg)
{
  return PP_double_base_value_init(p, value, PP_double_value_format,
                                   PP_double_default_value_okfunc, okfunc, arg);
}

/*
 * Accepts any finite value or nan (as 'not set').
 * okfunc will be called when it is finite.
 */
static inline struct pp_format_value_data *
PP_double_ns_value_init(struct PP_double_value_data *p, double value,
                        PP_double_ok_func *okfunc, void *arg)
{
  return PP_double_base_value_init(p, value, PP_double_ns_value_format,
                                   PP_double_ns_value_okfunc, okfunc, arg);
}

/*
 * Accepts any finite value, inf or nan (as 'not set').
 * okfunc will be called when it is finite.
 */
static inline struct pp_format_value_data *
PP_double_inf_value_init(struct PP_double_value_data *p, double value,
                         PP_double_ok_func *okfunc, void *arg)
{
  return PP_double_base_value_init(p, value, PP_double_inf_value_format,
                                   PP_double_inf_value_okfunc, okfunc, arg);
}

//--- compatibility functions.

static inline void PP_charp_f(flags *f, int indent, const char *name,
                              const char *value, const char *unit,
                              PP_charp_ok_func *func, void *arg, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_charp_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_charp_value_init(&v, value, "(null)", func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

static inline void PP_fname_f(flags *f, int indent, const char *name,
                              const char *value, const char *unit,
                              PP_charp_ok_func *func, void *arg, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_fname_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_fname_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.v.data, &u.data, nogood);
}

struct PP_charp_ok_precalc_data
{
  int ok;
};

static inline int PP_charp_ok_precalc(const char *p, void *arg)
{
  return ((struct PP_charp_ok_precalc_data *)arg)->ok;
}

static inline void PP_charp(flags *f, int indent, const char *name,
                            const char *value, const char *unit, int ok,
                            int *nogood)
{
  struct PP_charp_ok_precalc_data d = {.ok = ok};
  PP_charp_f(f, indent, name, value, unit, PP_charp_ok_precalc, &d, nogood);
}

static void PP_fname(flags *f, int indent, const char *name, const char *value,
                     const char *unit, int ok, int *nogood)
{
  struct PP_charp_ok_precalc_data d = {.ok = ok};
  PP_fname_f(f, indent, name, value, unit, PP_charp_ok_precalc, &d, nogood);
}

//---- int

static inline void PP_int_f(flags *f, int indent, const char *name, int value,
                            const char *unit, PP_int_ok_func *okfunc, void *arg,
                            int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_int_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_int_value_init(&v, value, okfunc, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_int_ok_precalc_data
{
  int ok;
};

static inline int PP_int_ok_precalc(int value, void *arg)
{
  return ((struct PP_int_ok_precalc_data *)arg)->ok;
}

static inline void PP_int(flags *f, int indent, const char *name, int value,
                          const char *unit, int ok, int *nogood)
{
  struct PP_int_ok_precalc_data d = {.ok = ok};
  PP_int_f(f, indent, name, value, unit, PP_int_ok_precalc, &d, nogood);
}

//---- double

static inline void PP_double_f(flags *f, int indent, const char *name,
                               double value, const char *unit,
                               PP_double_ok_func *okfunc, void *arg,
                               int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_double_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_double_value_init(&v, value, okfunc, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

static inline void PP_double_ns_f(flags *f, int indent, const char *name,
                                  double value, const char *unit,
                                  PP_double_ok_func *okfunc, void *arg,
                                  int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_double_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_double_ns_value_init(&v, value, okfunc, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

static inline void PP_double_inf_f(flags *f, int indent, const char *name,
                                   double value, const char *unit,
                                   PP_double_ok_func *okfunc, void *arg,
                                   int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_double_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_double_inf_value_init(&v, value, okfunc, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_double_ok_precalc_data
{
  int ok;
};

static inline int PP_double_ok_precalc(double value, void *arg)
{
  return ((struct PP_double_ok_precalc_data *)arg)->ok;
}

static inline void PP_double(flags *f, int indent, const char *name,
                             double value, const char *unit, int ok,
                             int *nogood)
{
  struct PP_double_ok_precalc_data d = {.ok = ok};
  PP_double_f(f, indent, name, value, unit, PP_double_ok_precalc, &d, nogood);
}

static inline void PP_double_ns(flags *f, int indent, const char *name,
                                double value, const char *unit, int ok,
                                int *nogood)
{
  struct PP_double_ok_precalc_data d = {.ok = ok};
  PP_double_ns_f(f, indent, name, value, unit, PP_double_ok_precalc, &d,
                 nogood);
}

static inline void PP_double_inf(flags *f, int indent, const char *name,
                                 double value, const char *unit, int ok,
                                 int *nogood)
{
  struct PP_double_ok_precalc_data d = {.ok = ok};
  PP_double_inf_f(f, indent, name, value, unit, PP_double_ok_precalc, &d,
                  nogood);
}

#ifdef __cplusplus
}
#endif

#endif
