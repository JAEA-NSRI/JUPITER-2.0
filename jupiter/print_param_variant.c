#include "print_param_variant.h"
#include "geometry/defs.h"
#include "geometry/mat43.h"
#include "geometry/svector.h"
#include "geometry/variant.h"
#include "geometry/vector.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "print_param_keywords.h"
#include "print_param_vecmat.h"

#include <stdlib.h>

static struct pp_format_value_data *
PP_geom_variant_init_formatter(struct PP_geom_variant_value_data *p)
{
  struct pp_format_value_data *d;
  geom_error e;
  int t;

  if (!p->value) {
    p->type = GEOM_VARTYPE_NULL;
    return NULL;
  }

  d = NULL;
  t = geom_variant_get_type(p->value);
  e = GEOM_SUCCESS;

  switch (t) {
  case GEOM_VARTYPE_NULL:
    break;
  case GEOM_VARTYPE_INT:
    d = PP_int_value_init(&p->uni.i, geom_variant_get_int(p->value, &e), NULL,
                          NULL);
    break;
  case GEOM_VARTYPE_LONG_INT:
    t = GEOM_VARTYPE_INT;
    d = PP_int_value_init(&p->uni.i, geom_variant_get_long_int(p->value, &e),
                          NULL, NULL);
    break;
  case GEOM_VARTYPE_SIZE:
    t = GEOM_VARTYPE_INT;
    d = PP_int_value_init(&p->uni.i, geom_variant_get_size_value(p->value, &e),
                          NULL, NULL);
    break;
  case GEOM_VARTYPE_DOUBLE:
    d =
      PP_double_ns_value_init(&p->uni.d, geom_variant_get_double(p->value, &e),
                              NULL, NULL);
    break;
  case GEOM_VARTYPE_STRING:
  case GEOM_VARTYPE_STRING_SHORT:
    t = GEOM_VARTYPE_STRING;
    d = PP_charp_value_init(&p->uni.str, geom_variant_get_string(p->value, &e),
                            "(null)", NULL, NULL);
    break;
  case GEOM_VARTYPE_VECTOR2:
    d = PP_geom_vec2_value_init(&p->uni.v2, geom_variant_get_vec2(p->value, &e),
                                NULL, NULL);
    break;
  case GEOM_VARTYPE_VECTOR3:
    d = PP_geom_vec3_value_init(&p->uni.v3, geom_variant_get_vec3(p->value, &e),
                                NULL, NULL);
    break;
  case GEOM_VARTYPE_MATRIX43:
    d = PP_geom_mat43_value_init(&p->uni.m43,
                                 geom_variant_get_mat43(p->value, &e), NULL,
                                 NULL);
    break;
  case GEOM_VARTYPE_SIZE_VECTOR3:
    d = PP_geom_svec3_value_init(&p->uni.sv3,
                                 geom_variant_get_svec3(p->value, &e), NULL,
                                 NULL);
    break;
  case GEOM_VARTYPE_DATA_OPERATOR:
    d =
      PP_geom_op_value_init(&p->uni.op, geom_variant_get_data_op(p->value, &e),
                            NULL, NULL);
    break;
  case GEOM_VARTYPE_SHAPE_OPERATOR:
    d = PP_geom_sop_value_init(&p->uni.sop,
                               geom_variant_get_shape_op(p->value, &e), NULL,
                               NULL);
    break;
  case GEOM_VARTYPE_PHASE:
    d = PP_vphase_value_init(&p->uni.vph, geom_variant_get_phase(p->value, &e),
                             NULL, NULL);
    break;

  case GEOM_VARTYPE_CHAR: /* not supported, unused as data */
  case GEOM_VARTYPE_UCHAR:
  case GEOM_VARTYPE_INFO_MAP:
  case GEOM_VARTYPE_MATRIX22:
  case GEOM_VARTYPE_MATRIX33:
  case GEOM_VARTYPE_VECTOR4:
  case GEOM_VARTYPE_QUATERNION:
  case GEOM_VARTYPE_ERROR:
  case GEOM_VARTYPE_INIT_FUNC:
  case GEOM_VARTYPE_SHAPE:
  case GEOM_VARTYPE_SURFACE_SHAPE:
  case GEOM_VARTYPE_INT_OR_SVEC3:
  case GEOM_VARTYPE_LIST_HEAD:
  case GEOM_VARTYPE_STORABLE_MAX:
    break;

  case GEOM_VARTYPE_ENUMTYPE_MIN: /* handled later */
  case GEOM_VARTYPE_ENUMTYPE_MAX:
  case GEOM_VARTYPE_EXTTYPE_MIN:
  case GEOM_VARTYPE_EXTTYPE_MAX:
    break;
  }

  if (!d) {
    if (p->extenum_format) {
      if (geom_variant_is_enumtype(t)) {
        int i = geom_variant_get_enum(p->value, t, 0, &e);
        if (e == GEOM_SUCCESS)
          d = p->uni.extp = p->extenum_format(t, &i, p->extenum_arg);
      } else if (geom_variant_is_exttype(t)) {
        void *d = geom_variant_get_ext(p->value, t, &e);
        if (e == GEOM_SUCCESS)
          d = p->uni.extp = p->extenum_format(t, d, p->extenum_arg);
      }
    } else {
      e = GEOM_ERR_VARIANT_TYPE;
      p->uni.extp = NULL;
    }
  }

  if (e != GEOM_SUCCESS) {
    if (d)
      pp_format_value_clean(d);

    p->type = GEOM_VARTYPE_NULL;
    return NULL;
  } else {
    p->type = t;
  }

  return d;
}

static struct pp_format_value_data *
PP_geom_variant_get_formatter(struct PP_geom_variant_value_data *p)
{
  switch (p->type) {
  case GEOM_VARTYPE_NULL:
    return NULL;
  case GEOM_VARTYPE_INT:
  case GEOM_VARTYPE_LONG_INT:
  case GEOM_VARTYPE_SIZE:
    return &p->uni.i.data;
  case GEOM_VARTYPE_DOUBLE:
    return &p->uni.d.data;
  case GEOM_VARTYPE_STRING:
  case GEOM_VARTYPE_STRING_SHORT:
    return &p->uni.str.data;
  case GEOM_VARTYPE_VECTOR2:
    return &p->uni.v2.data;
  case GEOM_VARTYPE_VECTOR3:
    return &p->uni.v3.data;
  case GEOM_VARTYPE_MATRIX43:
    return &p->uni.m43.data;
  case GEOM_VARTYPE_SIZE_VECTOR3:
    return &p->uni.sv3.data;
  case GEOM_VARTYPE_DATA_OPERATOR:
    return &p->uni.op.data;
  case GEOM_VARTYPE_SHAPE_OPERATOR:
    return &p->uni.sop.data;
  case GEOM_VARTYPE_PHASE:
    return &p->uni.vph.data;

  case GEOM_VARTYPE_CHAR: /* not supported, unused as data */
  case GEOM_VARTYPE_UCHAR:
  case GEOM_VARTYPE_INFO_MAP:
  case GEOM_VARTYPE_MATRIX22:
  case GEOM_VARTYPE_MATRIX33:
  case GEOM_VARTYPE_VECTOR4:
  case GEOM_VARTYPE_QUATERNION:
  case GEOM_VARTYPE_ERROR:
  case GEOM_VARTYPE_INIT_FUNC:
  case GEOM_VARTYPE_SHAPE:
  case GEOM_VARTYPE_SURFACE_SHAPE:
  case GEOM_VARTYPE_INT_OR_SVEC3:
  case GEOM_VARTYPE_LIST_HEAD:
  case GEOM_VARTYPE_STORABLE_MAX:
    return NULL;

  case GEOM_VARTYPE_ENUMTYPE_MIN: /* handled later */
  case GEOM_VARTYPE_ENUMTYPE_MAX:
  case GEOM_VARTYPE_EXTTYPE_MIN:
  case GEOM_VARTYPE_EXTTYPE_MAX:
    break;
  }
  if (geom_variant_is_enumtype(p->type) || geom_variant_is_exttype(p->type))
    return p->uni.extp;
  return NULL;
}

void PP_geom_variant_value_format(int *argc, const char ***argv, void *a)
{
  struct pp_format_value_data *d;
  struct PP_geom_variant_value_data *p;
  p = (struct PP_geom_variant_value_data *)a;

  if (p->type == GEOM_VARTYPE_NULL) {
    d = PP_geom_variant_init_formatter(p);
  } else {
    d = PP_geom_variant_get_formatter(p);
  }
  if (!d) {
    *argc = 0;
    *argv = NULL;
  } else {
    const char *r = NULL;
    pp_format_value(d, &r, argc, argv);
    if (r) {
      p->tmp = r;
      *argc = 1;
      *argv = &p->tmp;
    }
  }
}

const char *PP_geom_variant_value_null_format(void *a) { return "(error)"; }

void PP_geom_variant_value_delete(void *a)
{
  struct pp_format_value_data *d;
  struct PP_geom_variant_value_data *p;
  p = (struct PP_geom_variant_value_data *)a;

  d = PP_geom_variant_get_formatter(p);
  if (d)
    pp_format_value_clean(d);

  if (p->extenum_delete)
    p->extenum_delete(p->extenum_arg);

  p->type = GEOM_VARTYPE_NULL;
  p->tmp = NULL;
}

int PP_geom_variant_value_custom_okfunc(void *a)
{
  struct PP_geom_variant_value_data *p;
  p = (struct PP_geom_variant_value_data *)a;

  if (p->okfunc)
    return p->okfunc(p->value, p->okarg);
  return 1;
}
