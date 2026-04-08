
#include <stdlib.h>

#include "defs.h"
#include "func_defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "shp_sph.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for SPH (sphere) shape
 */
struct geom_shape_sph_data
{
  geom_vec3 basep;   ///< Base point
  double radius;     ///< Radius of sphere.
};


static geom_variant_type
geom_shape_sph_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case 0:
    geom_variant_set_string(description, "Center point", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 1:
    geom_variant_set_string(description, "Sphere radius", 0);
    return GEOM_VARTYPE_DOUBLE;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error
geom_shape_sph_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  if (index == 0) {
    e = GEOM_SUCCESS;
    vec = geom_variant_get_vec3(v, &e);
    if (e != GEOM_SUCCESS) return e;

    if (!geom_vec3_isfinite(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "All elements must be finite", 0);
      } else {
        geom_warn("(%g, %g, %g): Value should be finite",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 1) {
    x = geom_variant_get_double(v, &e);
    if (!isfinite(x)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Radius must be finite", 0);
      } else {
        geom_warn("%g: Value should be finite", x);
      }
      return GEOM_ERR_RANGE;
    }
    if (x <= 0.0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Radius must be positive", 0);
      } else {
        geom_warn("%g: Radius must be positive");
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error
geom_shape_sph_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  geom_error e;
  struct geom_shape_sph_data *pp;

  pp = (struct geom_shape_sph_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0) {
    geom_vec3 pnt;
    pnt = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->basep = pnt;
    return GEOM_SUCCESS;
  }
  if (index == 1) {
    double r;
    r = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->radius = r;
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error
geom_shape_sph_get_value(void *p, geom_size_type index,
                         geom_variant *out_variable)
{
  struct geom_shape_sph_data *pp;
  pp = (struct geom_shape_sph_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 1:
    return geom_variant_set_double(out_variable, pp->radius);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_shape_sph_n_params(void *p, geom_args_builder *b)
{
  return 2;
}

static geom_error
geom_shape_sph_init_set(void *p, geom_variant_list *l)
{
  geom_error e;
  struct geom_shape_sph_data *pp;
  geom_variant_list *curs;

  pp = (struct geom_shape_sph_data *)p;

  e = GEOM_SUCCESS;

  curs = l;

  curs = geom_variant_list_next(curs);
  if (curs == l) return GEOM_ERR_SHORT_LIST;
  pp->basep = geom_variant_get_vec3(geom_variant_list_get(curs), &e);

  curs = geom_variant_list_next(curs);
  if (curs == l) return GEOM_ERR_SHORT_LIST;
  pp->radius = geom_variant_get_double(geom_variant_list_get(curs), &e);

  return GEOM_SUCCESS;
}

static void *
geom_shape_sph_allocator(void)
{
  struct geom_shape_sph_data *p;
  p = (struct geom_shape_sph_data *)
    malloc(sizeof(struct geom_shape_sph_data));
  if (!p) return NULL;

  p->basep  = geom_vec3_c(0.0, 0.0, 0.0);
  p->radius = 0.0;

  return p;
}

static void
geom_shape_sph_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_sph_copy(void *p)
{
  struct geom_shape_sph_data *pp, *copy;

  pp = (struct geom_shape_sph_data *)p;
  copy = (struct geom_shape_sph_data *)geom_shape_sph_allocator();
  if (!copy) return NULL;

  copy->basep = pp->basep;
  copy->radius = pp->radius;
  return copy;
}

static int
geom_shape_sph_testf(void *p, double x, double y, double z)
{
  struct geom_shape_sph_data *pp;
  geom_vec3 vp;

  pp = (struct geom_shape_sph_data *)p;

  vp = geom_vec3_c(x, y, z);
  vp = geom_vec3_sub(vp, pp->basep);
  if (geom_vec3_length(vp) <= pp->radius) return 1;
  return 0;
}

static geom_error
geom_shape_sph_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_sph_data *pp;

  pp = (struct geom_shape_sph_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, "Center point", "L", &e);

  geom_variant_set_double(v, pp->radius);
  geom_info_map_append(list, v, "Radius", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_sph = {
  .enum_val = GEOM_SHAPE_SPH,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_sph_allocator,
    .deallocator = geom_shape_sph_deallocator,
    .set_value = geom_shape_sph_set_value,
    .get_value = geom_shape_sph_get_value,
    .n_params = geom_shape_sph_n_params,
    .args_next = geom_shape_sph_args_next,
    .args_check = geom_shape_sph_args_check,
    .infomap_gen = geom_shape_sph_info_map,
    .copy = geom_shape_sph_copy,
  },

  .body_testf = geom_shape_sph_testf,

  /*
   * Boundary box not defined yet.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_sph(void)
{
  return geom_install_shape_func(&geom_shape_sph);
}
