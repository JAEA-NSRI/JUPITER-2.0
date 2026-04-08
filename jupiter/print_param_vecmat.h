#ifndef JUPITER_PRINT_PARAM_VECMAT_H
#define JUPITER_PRINT_PARAM_VECMAT_H

#include "geometry/defs.h"
#include "geometry/svector.h"
#include "geometry/mat43.h"
#include "print_param_basic.h"
#include "print_param_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* param printer for constant-size vectors, matrices */

//--- geom_vec2

typedef int PP_geom_vec2_ok_func(geom_vec2 value, void *arg);

struct PP_geom_vec2_value_data
{
  struct pp_format_value_data data;
  geom_vec2 value;
  char *buf;
  PP_geom_vec2_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_geom_vec2_value_format(void *a);
JUPITER_DECL
const char *PP_geom_vec2_value_null_format(void *a);
JUPITER_DECL
void PP_geom_vec2_value_data_delete(void *a);
JUPITER_DECL
int PP_geom_vec2_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_geom_vec2_value_init(struct PP_geom_vec2_value_data *p, geom_vec2 value,
                        PP_geom_vec2_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->okfunc = okfunc;
  p->arg = arg;
  p->buf = NULL;
  return pp_format_value_init(&p->data, PP_geom_vec2_value_format,
                              PP_geom_vec2_value_null_format,
                              PP_geom_vec2_value_custom_okfunc,
                              PP_geom_vec2_value_data_delete, p);
}

//--- geom_vec3

typedef int PP_geom_vec3_ok_func(geom_vec3 value, void *arg);

struct PP_geom_vec3_value_data
{
  struct pp_format_value_data data;
  geom_vec3 value;
  char *buf;
  PP_geom_vec3_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_geom_vec3_value_format(void *a);
JUPITER_DECL
const char *PP_geom_vec3_value_null_format(void *a);
JUPITER_DECL
void PP_geom_vec3_value_data_delete(void *a);
JUPITER_DECL
int PP_geom_vec3_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_geom_vec3_value_init(struct PP_geom_vec3_value_data *p, geom_vec3 value,
                        PP_geom_vec3_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->okfunc = okfunc;
  p->arg = arg;
  p->buf = NULL;
  return pp_format_value_init(&p->data, PP_geom_vec3_value_format,
                              PP_geom_vec3_value_null_format,
                              PP_geom_vec3_value_custom_okfunc,
                              PP_geom_vec3_value_data_delete, p);
}

//--- geom_svec3

typedef int PP_geom_svec3_ok_func(geom_svec3 value, void *arg);

struct PP_geom_svec3_value_data
{
  struct pp_format_value_data data;
  geom_svec3 value;
  char *buf;
  PP_geom_svec3_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
const char *PP_geom_svec3_value_format(void *a);
JUPITER_DECL
const char *PP_geom_svec3_value_null_format(void *a);
JUPITER_DECL
void PP_geom_svec3_value_data_delete(void *a);
JUPITER_DECL
int PP_geom_svec3_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_geom_svec3_value_init(struct PP_geom_svec3_value_data *p, geom_svec3 value,
                         PP_geom_svec3_ok_func *okfunc, void *arg)
{
  p->value = value;
  p->okfunc = okfunc;
  p->arg = arg;
  p->buf = NULL;
  return pp_format_value_init(&p->data, PP_geom_svec3_value_format,
                              PP_geom_svec3_value_null_format,
                              PP_geom_svec3_value_custom_okfunc,
                              PP_geom_svec3_value_data_delete, p);
}

//--- geom_mat43

typedef int PP_geom_mat43_ok_func(const geom_mat43 *value, void *arg);

struct PP_geom_mat43_value_data
{
  struct pp_format_value_data data;
  geom_mat43 value;
  char *buf[4];
  const char *cbuf[4];
  PP_geom_mat43_ok_func *okfunc;
  void *arg;
};

JUPITER_DECL
void PP_geom_mat43_value_format(int *argc, const char ***argv, void *a);
JUPITER_DECL
const char *PP_geom_mat43_value_null_format(void *a);
JUPITER_DECL
void PP_geom_mat43_value_data_delete(void *a);
JUPITER_DECL
int PP_geom_mat43_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *
PP_geom_mat43_value_init(struct PP_geom_mat43_value_data *p, geom_mat43 value,
                         PP_geom_mat43_ok_func *okfunc, void *arg)
{
  p->value = value;
  for (int i = 0; i < 4; ++i) {
    p->buf[i] = NULL;
    p->cbuf[i] = NULL;
  }
  p->okfunc = okfunc;
  p->arg = arg;
  return pp_format_value_init_vec(&p->data, PP_geom_mat43_value_format,
                                  PP_geom_mat43_value_null_format,
                                  PP_geom_mat43_value_custom_okfunc,
                                  PP_geom_mat43_value_data_delete, p);
}

//---- Compatibility functions

//-- geom_vec2

static inline void PP_geom_vec2_f(flags *f, int indent, const char *name,
                                  geom_vec2 value, const char *unit,
                                  PP_geom_vec2_ok_func *func, void *arg,
                                  int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_geom_vec2_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_geom_vec2_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_geom_vec2_precalc_ok_data
{
  int ok;
};

static inline int PP_geom_vec2_precalc_ok(geom_vec2 v, void *arg)
{
  return ((struct PP_geom_vec2_precalc_ok_data *)arg)->ok;
}

static inline void PP_geom_vec2(flags *f, int indent, const char *name,
                                geom_vec2 value, const char *unit, int ok,
                                int *nogood)
{
  struct PP_geom_vec2_precalc_ok_data d = {.ok = ok};
  PP_geom_vec2_f(f, indent, name, value, unit, PP_geom_vec2_precalc_ok, &d,
                 nogood);
}

//-- geom_vec3

static inline void PP_geom_vec3_f(flags *f, int indent, const char *name,
                                  geom_vec3 value, const char *unit,
                                  PP_geom_vec3_ok_func *func, void *arg,
                                  int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_geom_vec3_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_geom_vec3_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_geom_vec3_precalc_ok_data
{
  int ok;
};

static inline int PP_geom_vec3_precalc_ok(geom_vec3 v, void *arg)
{
  return ((struct PP_geom_vec3_precalc_ok_data *)arg)->ok;
}

static inline void PP_geom_vec3(flags *f, int indent, const char *name,
                                geom_vec3 value, const char *unit, int ok,
                                int *nogood)
{
  struct PP_geom_vec3_precalc_ok_data d = {.ok = ok};
  PP_geom_vec3_f(f, indent, name, value, unit, PP_geom_vec3_precalc_ok, &d,
                 nogood);
}

//-- geom_svec3

static inline void PP_geom_svec3_f(flags *f, int indent, const char *name,
                                   geom_svec3 value, const char *unit,
                                   PP_geom_svec3_ok_func *func, void *arg,
                                   int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_geom_svec3_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_geom_svec3_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_geom_svec3_precalc_ok_data
{
  int ok;
};

static inline int PP_geom_svec3_precalc_ok(geom_svec3 v, void *arg)
{
  return ((struct PP_geom_svec3_precalc_ok_data *)arg)->ok;
}

static inline void PP_geom_svec3(flags *f, int indent, const char *name,
                                 geom_svec3 value, const char *unit, int ok,
                                 int *nogood)
{
  struct PP_geom_svec3_precalc_ok_data d = {.ok = ok};
  PP_geom_svec3_f(f, indent, name, value, unit, PP_geom_svec3_precalc_ok, &d,
                  nogood);
}

//-- geom_mat43

static inline void PP_geom_mat43_f(flags *f, int indent, const char *name,
                                   geom_mat43 value, const char *unit,
                                   PP_geom_mat43_ok_func *func, void *arg,
                                   int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_geom_mat43_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_geom_mat43_value_init(&v, value, func, arg);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_geom_mat43_precalc_ok_data
{
  int ok;
};

static inline int PP_geom_mat43_precalc_ok(const geom_mat43 *v, void *arg)
{
  return ((struct PP_geom_mat43_precalc_ok_data *)arg)->ok;
}

static inline void PP_geom_mat43(flags *f, int indent, const char *name,
                                 geom_mat43 value, const char *unit, int ok,
                                 int *nogood)
{
  struct PP_geom_mat43_precalc_ok_data d = {.ok = ok};
  PP_geom_mat43_f(f, indent, name, value, unit, PP_geom_mat43_precalc_ok, &d,
                  nogood);
}

#ifdef __cplusplus
}
#endif

#endif
