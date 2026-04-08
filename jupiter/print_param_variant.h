#ifndef JUPITER_PRINT_PARAM_VARIANT_H
#define JUPITER_PRINT_PARAM_VARIANT_H

/* print_param for geom_variant */

#include "geometry/defs.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "print_param_keywords.h"
#include "print_param_vecmat.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @formatter
 * @param type Type number set in geom_variant
 * @param data Data stored in geom_variant (pointer to int for enum values)
 * @param arg  Pointer to extra argument passed in PP_geom_variant_init()
 *             function
 * @return Formatter data
 */
typedef struct pp_format_value_data *
PP_geom_variant_extenumd_format(int type, void *data, void *arg);

typedef void PP_geom_variant_extenumd_delete(void *arg);

typedef int PP_geom_variant_ok_func(const geom_variant *var, void *arg);

union PP_geom_variant_value_union
{
  struct PP_double_value_data d;
  struct PP_int_value_data i;
  struct PP_charp_value_data str;
  struct PP_geom_vec2_value_data v2;
  struct PP_geom_vec3_value_data v3;
  struct PP_geom_svec3_value_data sv3;
  struct PP_geom_mat43_value_data m43;
  struct PP_geom_op_value_data op;
  struct PP_geom_sop_value_data sop;
  struct PP_vphase_value_data vph;
  struct pp_format_value_data *extp;
};

struct PP_geom_variant_value_data
{
  struct pp_format_value_data data;
  int type;
  union PP_geom_variant_value_union uni;
  PP_geom_variant_extenumd_format *extenum_format;
  PP_geom_variant_extenumd_delete *extenum_delete;
  void *extenum_arg;
  PP_geom_variant_ok_func *okfunc;
  void *okarg;
  const char *tmp;
  const geom_variant *value;
};

JUPITER_DECL
void PP_geom_variant_value_format(int *argc, const char ***argv, void *a);
JUPITER_DECL
const char *PP_geom_variant_value_null_format(void *a);
JUPITER_DECL
void PP_geom_variant_value_delete(void *a);
JUPITER_DECL
int PP_geom_variant_value_custom_okfunc(void *a);

static inline struct pp_format_value_data *PP_geom_variant_value_init(
  struct PP_geom_variant_value_data *p, const geom_variant *value,
  PP_geom_variant_ok_func *okfunc, void *ok_arg,
  PP_geom_variant_extenumd_format *extenum_format,
  PP_geom_variant_extenumd_delete *extenum_delete, void *extenum_arg)
{
  p->type = GEOM_VARTYPE_NULL; /* Sets to real value when it is required */
  memset(&p->uni, 0, sizeof(union PP_geom_variant_value_union));
  p->value = value;
  p->extenum_format = extenum_format;
  p->extenum_delete = extenum_delete;
  p->extenum_arg = extenum_arg;
  p->okfunc = okfunc;
  p->okarg = p->okarg;
  p->tmp = NULL;
  return pp_format_value_init_vec(&p->data, PP_geom_variant_value_format,
                                  PP_geom_variant_value_null_format,
                                  PP_geom_variant_value_custom_okfunc,
                                  PP_geom_variant_value_delete, p);
}

#ifdef __cplusplus
}
#endif

#endif
