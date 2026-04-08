
#ifndef JUPITER_GEOMETRY_SHAPE_MESH_H
#define JUPITER_GEOMETRY_SHAPE_MESH_H

#include <stddef.h>
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "util.h"
#include "variant.h"
#include "vector.h"
#include "mat33.h"
#include "list.h"

JUPITER_GEOMETRY_DECL_START

struct geom_shape_face_data
{
  struct geom_list list;
  geom_size_type i; ///< First point index
  geom_size_type j; ///< Second point index
  geom_size_type k; ///< Third point index
  geom_vec2 minp;   ///< Minimum p
  geom_vec2 maxp;   ///< Maximum p
};
#define geom_shape_face_list_entry(ptr) \
  geom_list_entry(ptr, struct geom_shape_face_data, list);

union geom_shape_mesh_element
{
  geom_vec3 point;
  struct geom_shape_face_data face;
};

/**
 * @ingroup Geometry
 * @brief Data for arbitrary triangle meshed shape
 */
struct geom_shape_mesh_data
{
  geom_size_type refc;    ///< Reference count of this data
  geom_size_type nfaces;  ///< Number of faces
  geom_size_type npoints; ///< Number of points
  geom_vec3 minp;         ///< Minimum point
  geom_vec3 maxp;         ///< Maximum point
  struct geom_shape_face_data    head;
  union geom_shape_mesh_element *faces;
  union geom_shape_mesh_element *points;
  union geom_shape_mesh_element elements[]; ///< data
};

static inline geom_error
geom_shape_mesh_point_check(const geom_variant *v, geom_variant *errinfo)
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

static inline struct geom_shape_mesh_data *
geom_shape_mesh_allocator(geom_size_type npoint, geom_size_type nface)
{
  struct geom_shape_mesh_data *p;
  geom_size_type ndata;

  if (npoint <= 0 || nface <= 0) {
    return NULL;
  }
  ndata = npoint + nface;

  p = (struct geom_shape_mesh_data *)
    malloc(sizeof(struct geom_shape_mesh_data) +
           sizeof(union geom_shape_mesh_element) * ndata);
  if (!p) return NULL;

  p->refc = 1;
  p->npoints = npoint;
  p->nfaces = nface;
  p->faces = p->elements;
  p->points = p->faces + p->nfaces;
  p->minp = geom_vec3_c( 1.0, 0.0, 0.0);
  p->maxp = geom_vec3_c(-1.0, 0.0, 0.0);
  geom_list_init(&p->head.list);

  return p;
}

static inline struct geom_shape_mesh_data *
geom_shape_mesh_duplicate(struct geom_shape_mesh_data *original)
{
  struct geom_shape_mesh_data *p;
  struct geom_list *lp;
  geom_size_type it;
  p = geom_shape_mesh_allocator(original->npoints, original->nfaces);
  if (!p) {
    return NULL;
  }

  p->minp = original->minp;
  p->maxp = original->maxp;
  for (it = 0; it < p->nfaces; ++it) {
    p->faces[it] = original->faces[it];
    geom_list_init(&p->faces[it].face.list);
  }
  for (it = 0; it < p->npoints; ++it) {
    p->points[it] = original->points[it];
  }
  geom_list_foreach (lp, &original->head.list) {
    struct geom_shape_face_data *fd;
    union geom_shape_mesh_element *el;
    fd = geom_shape_face_list_entry(lp);
    el = geom_container_of(fd, union geom_shape_mesh_element, face);
    it = el - original->faces;
    GEOM_ASSERT(it >= 0 && it < original->nfaces);
    geom_list_insert_prev(&p->head.list, &p->faces[it].face.list);
  }
  return p;
}

static inline void
geom_shape_mesh_deallocator(struct geom_shape_mesh_data *p)
{
  free(p);
}

static inline struct geom_shape_mesh_data *
geom_shape_mesh_delink(struct geom_shape_mesh_data *p)
{
  if (p->refc > 1) {
    p->refc--;
    return NULL;
  }
  p->refc = 1;
  return p;
}

static inline void
geom_shape_mesh_calc_bbox(struct geom_shape_mesh_data *p)
{
  geom_size_type i, j;
  double minx, miny, minz, maxx, maxy, maxz;
  double tmaxx, tmaxy, tminx, tminy;
  double x, y, z;
  geom_vec3 v[3];
  struct geom_shape_face_data *facep;
  struct geom_shape_face_data *fp;
  struct geom_list *lp;

  if (p->npoints <= 0) {
    p->minp = geom_vec3_c( 1.0, 0.0, 0.0);
    p->maxp = geom_vec3_c(-1.0, 0.0, 0.0);
    return;
  }

  geom_list_init(&p->head.list);

  for (i = 0; i < p->nfaces; ++i) {
    facep = &p->faces[i].face;
    v[0] = p->points[facep->i - 1].point;
    v[1] = p->points[facep->j - 1].point;
    v[2] = p->points[facep->k - 1].point;

    tmaxx = geom_vec3_x(v[0]);
    tmaxy = geom_vec3_y(v[0]);
    tminx = tmaxx;
    tminy = tmaxy;
    if (i == 0) {
      maxx = tmaxx;
      maxy = tmaxy;
      maxz = geom_vec3_z(v[0]);
      minx = maxx;
      miny = maxy;
      minz = maxz;
    }

    for (j = 1; j < 3; ++j) {
      x = geom_vec3_x(v[j]);
      y = geom_vec3_y(v[j]);
      z = geom_vec3_z(v[j]);
      if (x < tminx) tminx = x;
      if (y < tminy) tminy = y;
      if (x > tmaxx) tmaxx = x;
      if (y > tmaxy) tmaxy = y;
      if (x < minx)  minx = x;
      if (y < miny)  miny = y;
      if (z < minz)  minz = z;
      if (x > maxx)  maxx = x;
      if (y > maxy)  maxy = y;
      if (z > maxz)  maxz = z;
    }
    facep->minp = geom_vec2_c(tminx, tminy);
    facep->maxp = geom_vec2_c(tmaxx, tmaxy);

    geom_list_foreach(lp, &p->head.list) {
      fp = geom_shape_face_list_entry(lp);
      if (geom_vec2_x(fp->minp) > tminx) {
        geom_list_insert_prev(lp, &facep->list);
        break;
      }
    }
    if (lp == &p->head.list) {
      geom_list_insert_prev(lp, &facep->list);
    }
  }

  p->maxp = geom_vec3_c(maxx, maxy, maxz);
  p->minp = geom_vec3_c(minx, miny, minz);
}

static inline void
geom_shape_mesh_bboxf(struct geom_shape_mesh_data *p,
                      geom_vec3 *start, geom_vec3 *end)
{
  *start = p->minp;
  *end   = p->maxp;
}

static inline int
geom_shape_mesh_testf(struct geom_shape_mesh_data *p,
                      double x, double y, double z)
{
  struct mesh_test_work {
    struct geom_list list;
    struct geom_shape_face_data *facep;
    double f;
  };

  struct mesh_test_work wrkhead;
  struct mesh_test_work *workp, *wrkalloc, *wp;
  struct geom_shape_face_data *facep;
  geom_vec3 vi, vj, vk, n, vp, vt;
  geom_vec3 ez;
  double a, b, f;
  geom_size_type fcnt;
  struct geom_list *lp, *lq;

  if (p->nfaces <= 0) return 0;

  wrkalloc = (struct mesh_test_work *)
    malloc(sizeof(struct mesh_test_work) * p->nfaces);
  if (!wrkalloc) return 1;

  geom_list_init(&wrkhead.list);

  vp = geom_vec3_c(x, y, z);
  ez = geom_vec3_c(0.0, 0.0, 1.0);

  workp = wrkalloc;
  lp = geom_list_next(&p->head.list);
  for (; lp != &p->head.list; lp = geom_list_next(lp)) {
    facep = geom_shape_face_list_entry(lp);
    if (x < geom_vec2_x(facep->minp)) break;
    if (x > geom_vec2_x(facep->maxp)) continue;
    if (y < geom_vec2_y(facep->minp) || y > geom_vec2_y(facep->maxp)) continue;

    vi = p->points[facep->i - 1].point;
    vj = p->points[facep->j - 1].point;
    vk = p->points[facep->k - 1].point;

    vj = geom_vec3_sub(vj, vi);
    vk = geom_vec3_sub(vk, vi);
    n  = geom_vec3_cross_prod(vj, vk);
    f  = geom_vec3_inner_prod(ez, n);
    if (f != 0.0) {
      vt = geom_vec3_sub(vi, vp);
      f  = geom_vec3_inner_prod(vt, n) / f;
      vt = geom_vec3_factor(ez, f);
      vt = geom_vec3_add(vp, vt);
      vt = geom_vec3_sub(vt, vi);
      vt = geom_vec3_split(vt, vj, vk, n);
      a  = geom_vec3_x(vt);
      b  = geom_vec3_y(vt);
      if (a < 0.0 || a >= 1.0) continue;
      if (b < 0.0 || (a + b) >= 1.0) continue;
      geom_list_foreach(lq, &wrkhead.list) {
        wp = geom_list_entry(lq, struct mesh_test_work, list);
        if (wp->f >= f) break;
      }
      if (lq != &wrkhead.list && wp->f == f) {
        geom_vec3 n_other;
        vi = p->points[wp->facep->i - 1].point;
        vj = p->points[wp->facep->j - 1].point;
        vk = p->points[wp->facep->k - 1].point;
        vj = geom_vec3_sub(vj, vi);
        vk = geom_vec3_sub(vk, vi);
        n_other = geom_vec3_cross_prod(vj, vk);
        a = geom_vec3_inner_prod(ez, n);
        b = geom_vec3_inner_prod(ez, n_other);
        /* Both normals are same side of a plane whose normal is ez */
        if (a * b > 0.0) continue;
      }
      geom_list_insert_prev(lq, &workp->list);
      workp->facep = facep;
      workp->f = f;
      workp++;
    }
  }

  fcnt = 0;
  geom_list_foreach(lq, &wrkhead.list) {
    wp = geom_list_entry(lq, struct mesh_test_work, list);
    if (wp->f >= 0.0) break;
    fcnt++;
  }

  free(wrkalloc);

  if (fcnt % 2 == 1) return 1;
  return 0;
}

/**
 * @memberof geom_shape_mesh_data
 * @brief Export mesh data as STL (STereo Lithography) file.
 * @param p mesh data to export
 * @param path Output file path
 * @param name Solid name
 *
 * @retval 0 if success, non-0 if failed
 *
 * This function is for debug.
 */
JUPITER_GEOMETRY_DECL
int geom_shape_mesh_export_STL(struct geom_shape_mesh_data *p, const char *path,
                               const char *name);

JUPITER_GEOMETRY_DECL_END

#endif
