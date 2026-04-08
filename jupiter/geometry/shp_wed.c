
#include <stdlib.h>

#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#ifdef JUPITER_GEOMETRY_WEDGE_USE_TETRAHEDRA
#define USE_TETRAHEDRA JUPITER_GEOMETRY_USE_TETRAHEDRA
#else
/* Comment-out if you prefer mesh version */
#define USE_TETRAHEDRA 1
#endif

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
#include "shape_tetrahedra.h"
#else
#include "shape_mesh.h"
#endif

#include "shp_wed.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for WED (Wedge) shape
 */
struct geom_shape_wed_data
{
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  struct geom_shape_tetrahedra_data *data;
#else
  struct geom_shape_mesh_data *data;
#endif
};

static const char *geom_shape_wed_descs[6] = {
  "Point 1", "Point 2", "Point 3", "Point 4", "Point 5", "Point 6",
};

static geom_variant_type
geom_shape_wed_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 5) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_wed_descs[l], 0);
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_wed_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  if (index < 0 || index > 5) {
    return GEOM_ERR_RANGE;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  return geom_shape_tetrahedra_point_check(v, errinfo);
#else
  return geom_shape_mesh_point_check(v, errinfo);
#endif
}

static geom_error geom_shape_wed_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_vec3 pnt;
  geom_error err;
  struct geom_shape_wed_data *pp;
  pp = (struct geom_shape_wed_data *)p;

  if (index < 0 || index > 5) {
    return GEOM_ERR_RANGE;
  }

  err = GEOM_SUCCESS;
  pnt = geom_variant_get_vec3(value, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  if (!geom_vec3_eql(pp->data->points[index].point, pnt)) {
    if (pp->data->refc > 1) {
      struct geom_shape_tetrahedra_data *newtd;
      newtd = geom_shape_tetrahedra_duplicate(pp->data);
      if (!newtd) {
        return GEOM_ERR_NOMEM;
      }
      geom_shape_tetrahedra_delink(pp->data);
      pp->data = newtd;
    }
    pp->data->points[index].point = pnt;
    geom_shape_tetrahedra_calc_bbox(pp->data);
  }
#else
  if (!geom_vec3_eql(pp->data->points[index].point, pnt)) {
    if (pp->data->refc > 1) {
      struct geom_shape_mesh_data *newmd;
      newmd = geom_shape_mesh_duplicate(pp->data);
      if (!newmd) {
        return GEOM_ERR_NOMEM;
      }
      geom_shape_mesh_delink(pp->data);
      pp->data = newmd;
    }
    pp->data->points[index].point = pnt;
    geom_shape_mesh_calc_bbox(pp->data);
  }
#endif
  return GEOM_SUCCESS;
}

static geom_error geom_shape_wed_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  geom_vec3 pnt;
  struct geom_shape_wed_data *pp;
  pp = (struct geom_shape_wed_data *)p;

  if (index < 0 || index > 5) {
    return GEOM_ERR_RANGE;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  pnt = pp->data->points[index].point;
#else
  pnt = pp->data->points[index].point;
#endif
  return geom_variant_set_vec3(out_variable, pnt);
}

static geom_size_type geom_shape_wed_n_params(void *p, geom_args_builder *b)
{
  return 6;
}

static geom_error
geom_shape_wed_init_set(void *p, geom_variant_list *l)
{
  geom_error e;
  struct geom_shape_wed_data *pp;
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  struct geom_shape_tetrahedra_data *mdata;
#else
  struct geom_shape_mesh_data *mdata;
#endif
  geom_variant_list *curs;
  geom_vec3 pnt;
  int i;

  pp = (struct geom_shape_wed_data *)p;
  if (pp->data) {
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
    pp->data = geom_shape_tetrahedra_delink(pp->data);
#else
    pp->data = geom_shape_mesh_delink(pp->data);
#endif
  }
  if (!pp->data) {
#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
    pp->data = geom_shape_tetrahedra_allocator(6, 3);
#else
    pp->data = geom_shape_mesh_allocator(6, 8);
#endif
    if (!pp->data) return GEOM_ERR_NOMEM;
  }
  mdata = pp->data;

  e = GEOM_SUCCESS;

  curs = l;
  for (i = 0; i < 6; ++i) {
    curs = geom_variant_list_next(curs);
    if (curs == l) return GEOM_ERR_SHORT_LIST;
    pnt = geom_variant_get_vec3(geom_variant_list_get(curs), &e);
    mdata->points[i].point = pnt;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  geom_shape_tetrahedra_calc_bbox(mdata);
#else
  geom_shape_mesh_calc_bbox(mdata);
#endif

  return GEOM_SUCCESS;
}

static void *
geom_shape_wed_allocator(void)
{
  struct geom_shape_wed_data *p;
  p = (struct geom_shape_wed_data *)
    malloc(sizeof(struct geom_shape_wed_data));
  if (!p) return NULL;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  p->data = geom_shape_tetrahedra_allocator(6, 3);
#else
  p->data = geom_shape_mesh_allocator(6, 8);
#endif
  if (!p->data) {
    free(p);
    return NULL;
  }

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  p->data->tets[0].tetrahedron.i = 1;
  p->data->tets[0].tetrahedron.j = 2;
  p->data->tets[0].tetrahedron.k = 3;
  p->data->tets[0].tetrahedron.l = 4;

  p->data->tets[1].tetrahedron.i = 2;
  p->data->tets[1].tetrahedron.j = 3;
  p->data->tets[1].tetrahedron.k = 4;
  p->data->tets[1].tetrahedron.l = 5;

  p->data->tets[2].tetrahedron.i = 3;
  p->data->tets[2].tetrahedron.j = 4;
  p->data->tets[2].tetrahedron.k = 5;
  p->data->tets[2].tetrahedron.l = 6;

#else
  p->data->faces[0].face.i = 4;
  p->data->faces[0].face.j = 1;
  p->data->faces[0].face.k = 5;

  p->data->faces[1].face.i = 5;
  p->data->faces[1].face.j = 1;
  p->data->faces[1].face.k = 2;

  p->data->faces[2].face.i = 1;
  p->data->faces[2].face.j = 3;
  p->data->faces[2].face.k = 2;

  p->data->faces[3].face.i = 2;
  p->data->faces[3].face.j = 3;
  p->data->faces[3].face.k = 5;

  p->data->faces[4].face.i = 5;
  p->data->faces[4].face.j = 3;
  p->data->faces[4].face.k = 6;

  p->data->faces[5].face.i = 5;
  p->data->faces[5].face.j = 6;
  p->data->faces[5].face.k = 4;

  p->data->faces[6].face.i = 6;
  p->data->faces[6].face.j = 3;
  p->data->faces[6].face.k = 4;

  p->data->faces[7].face.i = 4;
  p->data->faces[7].face.j = 3;
  p->data->faces[7].face.k = 1;
#endif

  return p;
}

static void
geom_shape_wed_deallocator(void *p)
{
  if (p) {
    struct geom_shape_wed_data *wp;
    wp = (struct geom_shape_wed_data *)p;
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

static void *
geom_shape_wed_copy(void *p)
{
  struct geom_shape_wed_data *pp, *copy;

  pp = (struct geom_shape_wed_data *)p;
  copy = (struct geom_shape_wed_data *)
    malloc(sizeof(struct geom_shape_wed_data));
  if (!copy) return NULL;

  copy->data = pp->data;
  if (copy->data) {
    copy->data->refc++;
  }
  return copy;
}

static void
geom_shape_wed_bboxf(void *p, geom_vec3 *start, geom_vec3 *end)
{
  struct geom_shape_wed_data *pp;

  pp = (struct geom_shape_wed_data *)p;
  if (!pp->data) return;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  geom_shape_tetrahedra_bboxf(pp->data, start, end);
#else
  geom_shape_mesh_bboxf(pp->data, start, end);
#endif
}

static int
geom_shape_wed_testf(void *p, double x, double y, double z)
{
  struct geom_shape_wed_data *pp;

  pp = (struct geom_shape_wed_data *)p;
  if (!pp->data) return 0;

#if defined(USE_TETRAHEDRA) && USE_TETRAHEDRA == 1
  return geom_shape_tetrahedra_testf(pp->data, x, y, z);
#else
  return geom_shape_mesh_testf(pp->data, x, y, z);
#endif
}

static geom_error
geom_shape_wed_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_wed_data *pp;
  int l;

  pp = (struct geom_shape_wed_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  if (!pp->data) {
    geom_variant_set_string(v, "(Memory allocation failed)", 0);
    geom_info_map_append(list, v, "Error info", "", &e);

  } else {
    for (l = 0; l < 6; ++l) {
      geom_variant_set_vec3(v, pp->data->points[l].point);
      geom_info_map_append(list, v, geom_shape_wed_descs[l], "m", &e);
    }
  }

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_wed = {
  .enum_val = GEOM_SHAPE_WED,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_wed_allocator,
    .deallocator = geom_shape_wed_deallocator,
    .set_value = geom_shape_wed_set_value,
    .get_value = geom_shape_wed_get_value,
    .n_params = geom_shape_wed_n_params,
    .args_next = geom_shape_wed_args_next,
    .args_check = geom_shape_wed_args_check,
    .infomap_gen = geom_shape_wed_info_map,
    .copy = geom_shape_wed_copy,
  },

  .body_testf = geom_shape_wed_testf,
  .body_bboxf = geom_shape_wed_bboxf,

  .transform_func = NULL,
};

geom_error geom_install_shape_wed(void)
{
  return geom_install_shape_func(&geom_shape_wed);
}
