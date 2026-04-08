
#include <stdlib.h>
#include <math.h>

#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "infomap.h"

#include "if_linear.h"

struct geom_init_func_linear_data
{
  double xf;
  double yf;
  double zf;
  double intercept;
};

static geom_variant_type
geom_init_func_linear_args_next(geom_args_builder *b,
                                geom_variant *description, int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case 0:
    geom_variant_set_string(description, "coefficient for x", 0);
    break;
  case 1:
    geom_variant_set_string(description, "coefficient for y", 0);
    break;
  case 2:
    geom_variant_set_string(description, "coefficient for z", 0);
    break;
  case 3:
    geom_variant_set_string(description, "intercept", 0);
    break;
  default:
    return GEOM_VARTYPE_NULL;
  }

  return GEOM_VARTYPE_DOUBLE;
}

static geom_error
geom_init_func_linear_args_check(void *p, geom_args_builder *b,
                                 geom_size_type index, const geom_variant *v,
                                 geom_variant *errinfo)
{
  geom_error e;
  double cval;

  e = GEOM_SUCCESS;
  switch (index) {
  case 0:
  case 1:
  case 2:
  case 3:
    cval = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (!isfinite(cval)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Value must be finite", 0);
      } else {
        geom_warn("%g: Value should be finite", cval);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  default:
    return GEOM_ERR_RANGE;
  }
}

static geom_error geom_init_func_linear_set_value(void *p, geom_size_type index,
                                                  const geom_variant *value)
{
  geom_error e;
  double d;
  struct geom_init_func_linear_data *pp;

  e = GEOM_SUCCESS;
  pp = (struct geom_init_func_linear_data *)p;
  if (index == 0) {
    d = geom_variant_get_double(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->xf = d;
    }
    return e;
  }
  if (index == 1) {
    d = geom_variant_get_double(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->yf = d;
    }
    return e;
  }
  if (index == 2) {
    d = geom_variant_get_double(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->zf = d;
    }
    return e;
  }
  if (index == 3) {
    d = geom_variant_get_double(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->intercept = d;
    }
    return e;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_init_func_linear_get_value(void *p, geom_size_type index,
                                                  geom_variant *out_variable)
{
  struct geom_init_func_linear_data *pp;

  pp = (struct geom_init_func_linear_data *)p;
  switch (index) {
  case 0:
    return geom_variant_set_double(out_variable, pp->xf);
  case 1:
    return geom_variant_set_double(out_variable, pp->yf);
  case 2:
    return geom_variant_set_double(out_variable, pp->zf);
  case 3:
    return geom_variant_set_double(out_variable, pp->intercept);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_init_func_linear_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_init_func_linear_allocator(void)
{
  struct geom_init_func_linear_data *p;
  p = (struct geom_init_func_linear_data*)
    malloc(sizeof(struct geom_init_func_linear_data));

  if (!p) return NULL; /* If could not allocate, return NULL */

  p->xf = 0.0;
  p->yf = 0.0;
  p->zf = 0.0;
  p->intercept = 0.0;

  return p;
}

static void
geom_init_func_linear_deallocator(void *p)
{
  free(p);
}

static double
geom_init_func_linear_initf(void *p, double x, double y, double z, void *a)
{
  struct geom_init_func_linear_data *pp;

  pp = (struct geom_init_func_linear_data *)p;

  return pp->xf * x + pp->yf * y + pp->zf * z + pp->intercept;
}

static geom_error
geom_init_func_linear_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_init_func_linear_data *pp;

  pp = (struct geom_init_func_linear_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_double(v, pp->xf);
  geom_info_map_append(list, v, "X coefficient", "I/L", &e);

  geom_variant_set_double(v, pp->yf);
  geom_info_map_append(list, v, "Y coefficient", "I/L", &e);

  geom_variant_set_double(v, pp->zf);
  geom_info_map_append(list, v, "Z coefficient", "I/L", &e);

  geom_variant_set_double(v, pp->intercept);
  geom_info_map_append(list, v, "Intercept", "I", &e);

  geom_variant_delete(v);

  return e;
}

static void *
geom_init_func_linear_copy(void *p)
{
  struct geom_init_func_linear_data *pp, *np;
  pp = (struct geom_init_func_linear_data*)p;
  np = (struct geom_init_func_linear_data*)geom_init_func_linear_allocator();
  if (!np) return NULL;
  *np = *pp;
  return np;
}

static
geom_init_funcs geom_init_func_linear = {
  .enum_val = GEOM_INIT_FUNC_LINEAR,
  .c = {
    .allocator = geom_init_func_linear_allocator,
    .deallocator = geom_init_func_linear_deallocator,
    .set_value = geom_init_func_linear_set_value,
    .get_value = geom_init_func_linear_get_value,
    .n_params = geom_init_func_linear_n_params,
    .args_next = geom_init_func_linear_args_next,
    .args_check = geom_init_func_linear_args_check,
    .infomap_gen = geom_init_func_linear_info_map,
    .copy = geom_init_func_linear_copy,
  },
  .func = geom_init_func_linear_initf,
};

geom_error geom_install_init_func_linear(void)
{
  return geom_install_init_func(&geom_init_func_linear);
}
