#ifndef JUPITER_PRINT_PARAM_COMP_H
#define JUPITER_PRINT_PARAM_COMP_H

/*
 * print param for simpler composite types (printable in one-line)
 *
 * @note Do not confuse with print_param_comps.h, which is for printing
 * specific type: ox_component_info and component_info.
 */

#include "common.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "strlist.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

//--- controllable_type

typedef int PP_controllable_type_ok_func(const controllable_type *value,
                                         void *arg);

struct PP_controllable_type_value_data
{
  struct pp_format_value_data data;
  controllable_type *value;
  jupiter_strlist_head lh;
  PP_controllable_type_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_controllable_type_value_format(void *a);
JUPITER_DECL
const char *PP_controllable_type_value_null_format(void *a);
JUPITER_DECL
void PP_controllable_type_value_data_delete(void *a);
JUPITER_DECL
int PP_controllable_type_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_controllable_type_value_init(struct PP_controllable_type_value_data *p,
                                controllable_type *value,
                                PP_controllable_type_ok_func *okfunc, void *arg)
{
  p->value = value;
  jupiter_strlist_head_init(&p->lh);
  p->okfunc = okfunc;
  p->arg = arg;
  return pp_format_value_init(&p->data, PP_controllable_type_value_format,
                              PP_controllable_type_value_null_format,
                              PP_controllable_type_value_custom_okfunc,
                              PP_controllable_type_value_data_delete, p);
}

static inline void PP_controllable_type_f(
  flags *f, int indent, const char *name, controllable_type *value,
  const char *unit, PP_controllable_type_ok_func *func, void *arg, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_controllable_type_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_controllable_type_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_controllable_type_precalc_ok_data
{
  int ok;
};

static inline int PP_controllable_type_precalc_ok(const controllable_type *v,
                                                  void *arg)
{
  return ((struct PP_controllable_type_precalc_ok_data *)arg)->ok;
}

static inline void PP_controllable_type(flags *f, int indent, const char *name,
                                        controllable_type *value,
                                        const char *unit, int ok, int *nogood)
{
  struct PP_controllable_type_precalc_ok_data d = {.ok = ok};
  PP_controllable_type_f(f, indent, name, value, unit,
                         PP_controllable_type_precalc_ok, &d, nogood);
}

/* Prints int pair like (a and b) */
typedef int PP_int_pair_value_ok_func(int a, int b, void *arg);

struct PP_int_pair_value_data
{
  struct pp_format_value_data data;
  int a;
  int b;
  const char *sep;
  PP_int_pair_value_ok_func *okfunc;
  void *okdata;
  char *buf;
};

JUPITER_DECL
const char *PP_int_pair_value_format(void *arg);
JUPITER_DECL
const char *PP_int_pair_value_null_format(void *arg);
JUPITER_DECL
int PP_int_pair_value_custom_okfunc(void *arg);
JUPITER_DECL
void PP_int_pair_value_delete(void *arg);

/**
 * @param a first value
 * @param b second value
 * @param sep seprator for values (including space)
 */
static inline struct pp_format_value_data *
PP_int_pair_value_init(struct PP_int_pair_value_data *p, int a, int b,
                       const char *sep, PP_int_pair_value_ok_func *okfunc,
                       void *okdata)
{
  p->a = a;
  p->b = b;
  p->sep = sep;
  p->buf = NULL;
  p->okfunc = okfunc;
  p->okdata = okdata;
  return pp_format_value_init(&p->data, PP_int_pair_value_format,
                              PP_int_pair_value_null_format,
                              PP_int_pair_value_custom_okfunc,
                              PP_int_pair_value_delete, p);
}

static inline void PP_int_pair_f(flags *f, int indent, const char *name, int a,
                                 int b, const char *sep, const char *unit,
                                 PP_int_pair_value_ok_func *okfunc,
                                 void *okdata, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_int_pair_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_int_pair_value_init(&v, a, b, sep, okfunc, okdata);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_int_pair_precalc_ok_data
{
  int ok;
};

static inline int PP_int_pair_precalc_ok(int a, int b, void *arg)
{
  return ((struct PP_int_pair_precalc_ok_data *)arg)->ok;
}

static inline void PP_int_pair(flags *f, int indent, const char *name, int a,
                               int b, const char *sep, const char *unit, int ok,
                               int *nogood)
{
  struct PP_int_pair_precalc_ok_data okd = {.ok = ok};
  PP_int_pair_f(f, indent, name, a, b, sep, unit, PP_int_pair_precalc_ok, &okd,
                nogood);
}

#ifdef __cplusplus
}
#endif

#endif
