
#include <stdlib.h>

#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#ifdef JUPITER_GEOMETRY_ARB8_USE_TETRAHEDRA
#define USE_TETRAHEDRA JUPITER_GEOMETRY_ARB8_USE_TETRAHEDRA
#else
/* Comment-out if you prefer mesh version */
#define USE_TETRAHEDRA 1
#endif

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
#include "shape_tetrahedra.h"
#else
#include "shape_mesh.h"
#endif

#include "shp_arb.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for ARB (Arbitrary 8-point shape) shape
 */
struct geom_shape_arb_data
{
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  struct geom_shape_tetrahedra_data *data;
#else
  struct geom_shape_mesh_data *data;
#endif
};

static const char *geom_shape_arb_descs[8] = {"Point 1", "Point 2", "Point 3",
                                              "Point 4", "Point 5", "Point 6",
                                              "Point 7", "Point 8"};

static geom_variant_type geom_shape_arb_args_next(geom_args_builder *b,
                                                  geom_variant *description,
                                                  int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 7) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_arb_descs[l], 0);
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error geom_shape_arb_args_check(void *p, geom_args_builder *b,
                                            geom_size_type index,
                                            const geom_variant *v,
                                            geom_variant *errinfo)
{
  if (index < 0 || index > 7) {
    return GEOM_ERR_RANGE;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  return geom_shape_tetrahedra_point_check(v, errinfo);
#else
  return geom_shape_mesh_point_check(v, errinfo);
#endif
}

static geom_error geom_shape_arb_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_vec3 pnt;
  geom_error e;
  struct geom_shape_arb_data *pp;

  e = GEOM_SUCCESS;
  pp = (struct geom_shape_arb_data *)p;

  if (index < 0 || index > 7) {
    return GEOM_ERR_RANGE;
  }

  pnt = geom_variant_get_vec3(value, &e);
  if (e == GEOM_SUCCESS) {
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
    pp->data->points[index].point = pnt;
    geom_shape_tetrahedra_calc_bbox(pp->data);
#else
    pp->data->points[index].point = pnt;
    geom_shape_mesh_calc_bbox(pp->data);
#endif
  }
  return e;
}

static geom_error geom_shape_arb_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_arb_data *pp;

  pp = (struct geom_shape_arb_data *)p;
  if (index < 0 || index > 7) {
    return GEOM_ERR_RANGE;
  }

  /* These are currently same expression, however it is coincidence. */
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  return geom_variant_set_vec3(out_variable, pp->data->points[index].point);
#else
  return geom_variant_set_vec3(out_variable, pp->data->points[index].point);
#endif
}

static geom_size_type geom_shape_arb_n_params(void *p, geom_args_builder *b)
{
  return 8;
}

static void *geom_shape_arb_allocator(void)
{
  struct geom_shape_arb_data *p;
  p = (struct geom_shape_arb_data *)malloc(sizeof(struct geom_shape_arb_data));
  if (!p)
    return NULL;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  p->data = geom_shape_tetrahedra_allocator(8, 5);
#else
  p->data = geom_shape_mesh_allocator(8, 12);
#endif
  if (!p->data) {
    free(p);
    return NULL;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  p->data->tets[0].tetrahedron.i = 2;
  p->data->tets[0].tetrahedron.j = 1;
  p->data->tets[0].tetrahedron.k = 3;
  p->data->tets[0].tetrahedron.l = 6;

  p->data->tets[1].tetrahedron.i = 4;
  p->data->tets[1].tetrahedron.j = 1;
  p->data->tets[1].tetrahedron.k = 3;
  p->data->tets[1].tetrahedron.l = 8;

  p->data->tets[2].tetrahedron.i = 1;
  p->data->tets[2].tetrahedron.j = 3;
  p->data->tets[2].tetrahedron.k = 6;
  p->data->tets[2].tetrahedron.l = 8;

  p->data->tets[3].tetrahedron.i = 5;
  p->data->tets[3].tetrahedron.j = 1;
  p->data->tets[3].tetrahedron.k = 6;
  p->data->tets[3].tetrahedron.l = 8;

  p->data->tets[4].tetrahedron.i = 7;
  p->data->tets[4].tetrahedron.j = 3;
  p->data->tets[4].tetrahedron.k = 6;
  p->data->tets[4].tetrahedron.l = 8;

#else
  p->data->faces[0].face.i = 1;
  p->data->faces[0].face.j = 2;
  p->data->faces[0].face.k = 3;

  p->data->faces[1].face.i = 1;
  p->data->faces[1].face.j = 4;
  p->data->faces[1].face.k = 3;

  p->data->faces[2].face.i = 5;
  p->data->faces[2].face.j = 6;
  p->data->faces[2].face.k = 7;

  p->data->faces[3].face.i = 5;
  p->data->faces[3].face.j = 7;
  p->data->faces[3].face.k = 8;

  p->data->faces[4].face.i = 1;
  p->data->faces[4].face.j = 2;
  p->data->faces[4].face.k = 5;

  p->data->faces[5].face.i = 2;
  p->data->faces[5].face.j = 6;
  p->data->faces[5].face.k = 5;

  p->data->faces[6].face.i = 3;
  p->data->faces[6].face.j = 7;
  p->data->faces[6].face.k = 2;

  p->data->faces[7].face.i = 2;
  p->data->faces[7].face.j = 7;
  p->data->faces[7].face.k = 6;

  p->data->faces[8].face.i = 3;
  p->data->faces[8].face.j = 4;
  p->data->faces[8].face.k = 8;

  p->data->faces[9].face.i = 3;
  p->data->faces[9].face.j = 8;
  p->data->faces[9].face.k = 7;

  p->data->faces[10].face.i = 4;
  p->data->faces[10].face.j = 1;
  p->data->faces[10].face.k = 5;

  p->data->faces[11].face.i = 4;
  p->data->faces[11].face.j = 5;
  p->data->faces[11].face.k = 8;
#endif

  return p;
}

static void geom_shape_arb_deallocator(void *p)
{
  if (p) {
    struct geom_shape_arb_data *wp;
    wp = (struct geom_shape_arb_data *)p;
    if (wp->data) {
      if (wp->data->refc <= 1) {
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
        geom_shape_tetrahedra_deallocator(wp->data);
#else
        geom_shape_mesh_deallocator(wp->data);
#endif
      } else {
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
        geom_shape_tetrahedra_delink(wp->data);
#else
        geom_shape_mesh_delink(wp->data);
#endif
      }
    }
  }

  free(p);
}

static void *geom_shape_arb_copy(void *p)
{
  struct geom_shape_arb_data *pp, *copy;

  pp = (struct geom_shape_arb_data *)p;
  copy =
    (struct geom_shape_arb_data *)malloc(sizeof(struct geom_shape_arb_data));
  if (!copy)
    return NULL;

  copy->data = pp->data;
  if (copy->data) {
    copy->data->refc++;
  }
  return copy;
}

static void geom_shape_arb_bboxf(void *p, geom_vec3 *start, geom_vec3 *end)
{
  struct geom_shape_arb_data *pp;

  pp = (struct geom_shape_arb_data *)p;
  if (!pp->data)
    return;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  geom_shape_tetrahedra_bboxf(pp->data, start, end);
#else
  geom_shape_mesh_bboxf(pp->data, start, end);
#endif
}

static int geom_shape_arb_testf(void *p, double x, double y, double z)
{
  struct geom_shape_arb_data *pp;

  pp = (struct geom_shape_arb_data *)p;
  if (!pp->data)
    return 0;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  return geom_shape_tetrahedra_testf(pp->data, x, y, z);
#else
  return geom_shape_mesh_testf(pp->data, x, y, z);
#endif
}

static geom_error geom_shape_arb_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_arb_data *pp;
  int l;

  pp = (struct geom_shape_arb_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v)
    return e;

  if (!pp->data) {
    geom_variant_set_string(v, "(Memory allocation failed)", 0);
    geom_info_map_append(list, v, "Error info", "", &e);

  } else {
    for (l = 0; l < 8; ++l) {
      geom_variant_set_vec3(v, pp->data->points[l].point);
      geom_info_map_append(list, v, geom_shape_arb_descs[l], "m", &e);
    }
  }

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static geom_shape_funcs geom_shape_arb = {
  .enum_val = GEOM_SHAPE_ARB,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_arb_allocator,
    .deallocator = geom_shape_arb_deallocator,
    .set_value = geom_shape_arb_set_value,
    .get_value = geom_shape_arb_get_value,
    .n_params = geom_shape_arb_n_params,
    .args_next = geom_shape_arb_args_next,
    .args_check = geom_shape_arb_args_check,
    .infomap_gen = geom_shape_arb_info_map,
    .copy = geom_shape_arb_copy,
  },

  .body_testf = geom_shape_arb_testf,
  .body_bboxf = geom_shape_arb_bboxf,

  .transform_func = NULL,
};

geom_error geom_install_shape_arb(void)
{
  return geom_install_shape_func(&geom_shape_arb);
}
