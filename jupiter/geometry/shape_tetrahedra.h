
#ifndef JUPITER_GEOMETRY_SHAPE_TETRAHEDRA_H
#define JUPITER_GEOMETRY_SHAPE_TETRAHEDRA_H

#include <stdlib.h>

#include "defs.h"
#include "global.h"
#include "variant.h"
#include "vector.h"
#include "mat33.h"

JUPITER_GEOMETRY_DECL_START

struct geom_shape_tetrahedron_data
{
  geom_size_type i; ///< First point index
  geom_size_type j; ///< Second point index
  geom_size_type k; ///< Third point index
  geom_size_type l; ///< Fourth point index
};

union geom_shape_tetrahedron_element
{
  geom_vec3 point;
  struct geom_shape_tetrahedron_data tetrahedron;
};

/**
 * @ingroup Geometry
 * @brief Data for arbitrary Tetrahedron constructed convex shape
 */
struct geom_shape_tetrahedra_data
{
  geom_size_type refc;    ///< Reference count of this data
  geom_size_type ntets;   ///< Number of tetrahedra
  geom_size_type npoints; ///< Number of points
  geom_vec3 minp;         ///< Minimum point
  geom_vec3 maxp;         ///< Maximum point
  union geom_shape_tetrahedron_element *tets;
  union geom_shape_tetrahedron_element *points;
  union geom_shape_tetrahedron_element elements[]; ///< data
};

static inline geom_error
geom_shape_tetrahedra_point_check(const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

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

static inline struct geom_shape_tetrahedra_data *
geom_shape_tetrahedra_allocator(geom_size_type npoint, geom_size_type ntets)
{
  struct geom_shape_tetrahedra_data *p;
  geom_size_type ndata;

  if (npoint <= 0 || ntets <= 0) {
    return NULL;
  }
  ndata = npoint + ntets;

  p = (struct geom_shape_tetrahedra_data *)
    malloc(sizeof(struct geom_shape_tetrahedra_data) +
           sizeof(union geom_shape_tetrahedron_element) * ndata);
  if (!p) return NULL;

  p->refc = 1;
  p->npoints = npoint;
  p->ntets = ntets;
  p->tets = p->elements;
  p->points = p->tets + p->ntets;
  p->minp = geom_vec3_c( 1.0, 0.0, 0.0);
  p->maxp = geom_vec3_c(-1.0, 0.0, 0.0);

  return p;
}

static inline struct geom_shape_tetrahedra_data *
geom_shape_tetrahedra_duplicate(struct geom_shape_tetrahedra_data *original)
{
  geom_size_type it;
  struct geom_shape_tetrahedra_data *p;
  p = geom_shape_tetrahedra_allocator(original->npoints, original->ntets);
  if (!p) {
    return NULL;
  }

  p->minp = original->minp;
  p->maxp = original->maxp;
  for (it = 0; it < p->ntets; ++it) {
    p->tets[it] = original->tets[it];
  }
  for (it = 0; it < p->npoints; ++it) {
    p->points[it] = original->points[it];
  }

  return p;
}

static inline void
geom_shape_tetrahedra_deallocator(struct geom_shape_tetrahedra_data *p)
{
  free(p);
}

static inline struct geom_shape_tetrahedra_data *
geom_shape_tetrahedra_delink(struct geom_shape_tetrahedra_data *p)
{
  if (p->refc > 1) {
    p->refc--;
    return NULL;
  }
  p->refc = 1;
  return p;
}

static inline void
geom_shape_tetrahedra_calc_bbox(struct geom_shape_tetrahedra_data *p)
{
  geom_size_type i;
  double minx, miny, minz, maxx, maxy, maxz;
  double x, y, z;
  geom_vec3 v;

  if (p->npoints <= 0) {
    p->minp = geom_vec3_c( 1.0, 0.0, 0.0);
    p->maxp = geom_vec3_c(-1.0, 0.0, 0.0);
    return;
  }

  v = p->points[0].point;
  minx = geom_vec3_x(v);
  miny = geom_vec3_y(v);
  minz = geom_vec3_z(v);
  maxx = minx;
  maxy = miny;
  maxz = minz;

  for (i = 1; i < p->npoints; ++i) {
    v = p->points[i].point;
    x = geom_vec3_x(v);
    y = geom_vec3_y(v);
    z = geom_vec3_z(v);

    if (x < minx)  minx = x;
    if (y < miny)  miny = y;
    if (z < minz)  minz = z;
    if (x > maxx)  maxx = x;
    if (y > maxy)  maxy = y;
    if (z > maxz)  maxz = z;
  }

  p->maxp = geom_vec3_c(maxx, maxy, maxz);
  p->minp = geom_vec3_c(minx, miny, minz);
}

static inline void
geom_shape_tetrahedra_bboxf(struct geom_shape_tetrahedra_data *p,
                            geom_vec3 *start, geom_vec3 *end)
{
  *start = p->minp;
  *end   = p->maxp;
}

static inline int
geom_shape_tetrahedra_testf(struct geom_shape_tetrahedra_data *p,
                            double x, double y, double z)
{
  geom_vec3 vi, vj, vk, vp, vt;
  geom_size_type i;
  double a, b, c;

  if (p->ntets <= 0) return 0;

  vp = geom_vec3_c(x, y, z);

  for (i = 0; i < p->ntets; ++i) {
    vt = p->points[p->tets[i].tetrahedron.i - 1].point;
    vi = p->points[p->tets[i].tetrahedron.j - 1].point;
    vj = p->points[p->tets[i].tetrahedron.k - 1].point;
    vk = p->points[p->tets[i].tetrahedron.l - 1].point;
    vi = geom_vec3_sub(vi, vt);
    vj = geom_vec3_sub(vj, vt);
    vk = geom_vec3_sub(vk, vt);
    vt = geom_vec3_sub(vp, vt);
    vt = geom_vec3_split(vt, vi, vj, vk);
    a = geom_vec3_x(vt);
    b = geom_vec3_y(vt);
    c = geom_vec3_z(vt);
    if (a >= 0.0 && b >= 0.0 && c >= 0.0 && (a + b + c) <= 1.0) return 1;
  }

  return 0;
}

JUPITER_GEOMETRY_DECL_END

#endif
