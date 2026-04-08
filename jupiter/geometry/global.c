
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "defs.h"
#include "func_defs.h"
#include "func_data.h"
#include "global.h"
#include "geom_assert.h"
#include "rbtree.h"
#include "common.h"

#include "if_none.h"
#include "if_const.h"
#include "if_linear.h"
#include "if_poly.h"
#include "if_poly_n.h"
#include "if_exp_poly.h"

#include "shp_specials.h"
#include "shp_box.h"
#include "shp_pla.h"
#include "shp_app.h"
#include "shp_wed.h"
#include "shp_arb.h"
#include "shp_rpr.h"
#include "shp_trp.h"
#include "shp_sph.h"
#include "shp_rcc.h"
#include "shp_trc.h"
#include "shp_rec.h"
#include "shp_tec.h"
#include "shp_ell.h"
#include "shp_tor.h"
#include "shp_eto.h"

#include "shp_pl1.h"
#include "shp_pl2.h"
#include "shp_pl3.h"
#include "shp_pl4.h"
#include "shp_pln.h"

#include "shp_tra.h"
#include "shp_rot.h"
#include "shp_sca.h"
#include "shp_mat.h"

#include "surfshp_specials.h"
#include "surfshp_parallelogram.h"

static
void geom_default_warn_func(void *p, const char *file, long int line,
                            const char *func, const char *text)
{
  fprintf(stderr, "%s(%ld): %s: warning: %s\n", file, line, func, text);
}

static geom_warning_func *geom_wf_global = geom_default_warn_func;
static void *geom_wf_data = NULL;

/* To make global variable to thread private, OpenMP 3.0 is required */
#ifdef JUPITER_GEOMETRY_USE_OPENMP
#pragma omp threadprivate(geom_wf_global)
#pragma omp threadprivate(geom_wf_data)
#endif

void geom_set_warning_function(geom_warning_func *w, void *p)
{
  if (w) {
    geom_wf_global = w;
    geom_wf_data = p;
  } else {
    geom_wf_global = geom_default_warn_func;
    geom_wf_data = NULL;
  }
}

geom_warning_func *geom_get_warning_function(void)
{
  return geom_wf_global;
}

void geom_warn_x(const char *file, long int line, const char *func,
                 const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  geom_vwarn_x(file, line, func, format, ap);
  va_end(ap);
}

void geom_vwarn_x(const char *file, long int line, const char *func,
                  const char *format, va_list ap)
{
  char *buf;
  int r;

  r = geom_vasprintf(&buf, format, ap);
  if (r < 0) {
    geom_wf_global(geom_wf_data, file, line, func,
                   "(memory allocation error occurred"
                   " while printing warning)");
  } else {
    geom_wf_global(geom_wf_data, file, line, func, buf);
    free(buf);
  }
}

static struct geom_rbtree *geom_init_funcs_registry = NULL;
static struct geom_rbtree *geom_shape_funcs_registry = NULL;
static struct geom_rbtree *geom_surface_shape_funcs_registry = NULL;

static int
geom_install_init_funcs_compare(struct geom_rbtree *a, struct geom_rbtree *b)
{
  geom_init_funcs *aa;
  geom_init_funcs *bb;

  aa = geom_init_funcs_tree_entry(a);
  bb = geom_init_funcs_tree_entry(b);

  if (aa->enum_val <  bb->enum_val) return -1;
  if (aa->enum_val == bb->enum_val) return  0;
  return 1;
}

geom_error geom_install_init_func(geom_init_funcs *funcs)
{
  struct geom_rbtree *tmx;
  geom_init_funcs *tfn;

  GEOM_ASSERT(funcs);

  if (funcs->enum_val == GEOM_INIT_FUNC_USER) {
    tmx = NULL;
    tfn = NULL;
    if (geom_init_funcs_registry) {
      tmx = geom_rbtree_maximum(geom_init_funcs_registry);
    }
    if (tmx) {
      tfn = geom_init_funcs_tree_entry(tmx);
    }
    if (!tfn || tfn->enum_val <= GEOM_INIT_FUNC_USER) {
      funcs->enum_val = GEOM_INIT_FUNC_USER + 1;
    } else {
      funcs->enum_val = tfn->enum_val + 1;
    }
  }

  if (geom_init_funcs_registry) {
    struct geom_rbtree *t;
    t = geom_rbtree_insert(geom_init_funcs_registry,
                           &funcs->c.tree, geom_install_init_funcs_compare,
                           NULL);
    if (!t) return GEOM_ERR_ALREADY_REGISTERED_INIT_FUNC;

    geom_init_funcs_registry = t;
  } else {
    geom_rbtree_init(&funcs->c.tree);
    geom_init_funcs_registry = &funcs->c.tree;
  }
  return GEOM_SUCCESS;
}

const geom_init_funcs *geom_get_init_func(geom_init_func f)
{
  struct geom_rbtree *t;
  geom_init_funcs search;
  search.enum_val = f;

  if (!geom_init_funcs_registry) return NULL;

  t = geom_rbtree_find(geom_init_funcs_registry,
                       &search.c.tree, geom_install_init_funcs_compare);
  if (t) {
    return geom_init_funcs_tree_entry(t);
  } else {
    return NULL;
  }
}

static int
geom_install_shape_funcs_compare(struct geom_rbtree *a, struct geom_rbtree *b)
{
  geom_shape_funcs *aa;
  geom_shape_funcs *bb;

  aa = geom_shape_funcs_tree_entry(a);
  bb = geom_shape_funcs_tree_entry(b);

  if (aa->enum_val <  bb->enum_val) return -1;
  if (aa->enum_val == bb->enum_val) return  0;
  return 1;
}

geom_error geom_install_shape_func(geom_shape_funcs *funcs)
{
  struct geom_rbtree *tmx;
  geom_shape_funcs *tfn;

  GEOM_ASSERT(funcs);

  if (funcs->enum_val == GEOM_SHAPE_USER) {
    tmx = NULL;
    tfn = NULL;
    if (geom_shape_funcs_registry) {
      tmx = geom_rbtree_maximum(geom_shape_funcs_registry);
    }
    if (tmx) {
      tfn = geom_shape_funcs_tree_entry(tmx);
    }
    if (!tfn || tfn->enum_val <= GEOM_SHAPE_USER) {
      funcs->enum_val = GEOM_SHAPE_USER + 1;
    } else {
      funcs->enum_val = tfn->enum_val + 1;
    }
  }

  if (geom_shape_funcs_registry) {
    struct geom_rbtree *t;
    t = geom_rbtree_insert(geom_shape_funcs_registry,
                           &funcs->c.tree, geom_install_shape_funcs_compare,
                           NULL);
    if (!t) return GEOM_ERR_ALREADY_REGISTERED_SHAPE;

    geom_shape_funcs_registry = t;
  } else {
    geom_rbtree_init(&funcs->c.tree);
    geom_shape_funcs_registry = &funcs->c.tree;
  }
  return GEOM_SUCCESS;
}

const geom_shape_funcs *geom_get_shape_func(geom_shape shape)
{
  struct geom_rbtree *t;
  geom_shape_funcs search;
  search.enum_val = shape;

  if (!geom_shape_funcs_registry) return NULL;

  t = geom_rbtree_find(geom_shape_funcs_registry,
                       &search.c.tree, geom_install_shape_funcs_compare);
  if (t) {
    return geom_shape_funcs_tree_entry(t);
  } else {
    return NULL;
  }
}

static int geom_install_surface_shape_funcs_compare(struct geom_rbtree *a,
                                                    struct geom_rbtree *b)
{
  geom_surface_shape_funcs *aa;
  geom_surface_shape_funcs *bb;

  aa = geom_surface_shape_funcs_tree_entry(a);
  bb = geom_surface_shape_funcs_tree_entry(b);

  if (aa->enum_val < bb->enum_val)
    return -1;
  if (aa->enum_val == bb->enum_val)
    return 0;
  return 1;
}

geom_error geom_install_surface_shape_func(geom_surface_shape_funcs *funcs)
{
  struct geom_rbtree *tmx;
  geom_surface_shape_funcs *tfn;

  GEOM_ASSERT(funcs);

  if (funcs->enum_val == GEOM_SURFACE_SHAPE_USER) {
    tmx = NULL;
    tfn = NULL;
    if (geom_surface_shape_funcs_registry) {
      tmx = geom_rbtree_maximum(geom_surface_shape_funcs_registry);
    }
    if (tmx) {
      tfn = geom_surface_shape_funcs_tree_entry(tmx);
    }
    if (!tfn || tfn->enum_val <= GEOM_SURFACE_SHAPE_USER) {
      funcs->enum_val = GEOM_SURFACE_SHAPE_USER + 1;
    } else {
      funcs->enum_val = tfn->enum_val + 1;
    }
  }

  if (geom_surface_shape_funcs_registry) {
    struct geom_rbtree *t;
    t = geom_rbtree_insert(geom_surface_shape_funcs_registry, &funcs->c.tree,
                           geom_install_surface_shape_funcs_compare, NULL);
    if (!t)
      return GEOM_ERR_ALREADY_REGISTERED_SURFACE_SHAPE;

    geom_surface_shape_funcs_registry = t;
  } else {
    geom_rbtree_init(&funcs->c.tree);
    geom_surface_shape_funcs_registry = &funcs->c.tree;
  }
  return GEOM_SUCCESS;
}

const geom_surface_shape_funcs *geom_get_surface_shape_func(geom_surface_shape shape)
{
  struct geom_rbtree *t;
  geom_surface_shape_funcs search;
  search.enum_val = shape;

  if (!geom_surface_shape_funcs_registry)
    return NULL;

  t = geom_rbtree_find(geom_surface_shape_funcs_registry, &search.c.tree,
                       geom_install_surface_shape_funcs_compare);

  if (t) {
    return geom_surface_shape_funcs_tree_entry(t);
  } else {
    return NULL;
  }
}

static int geom_initialized_val = 0;

void geom_initialize(void)
{
  geom_install_init_func_none();
  geom_install_init_func_const();
  geom_install_init_func_linear();
  geom_install_init_func_poly();
  geom_install_init_func_poly_n();
  geom_install_init_func_exp_poly();

  geom_install_shape_specials();
  geom_install_shape_box();
  geom_install_shape_pla();
  geom_install_shape_wed();
  geom_install_shape_arb();
  geom_install_shape_app();
  geom_install_shape_rpr();
  geom_install_shape_trp();
  geom_install_shape_sph();
  geom_install_shape_rcc();
  geom_install_shape_trc();
  geom_install_shape_rec();
  geom_install_shape_tec();
  geom_install_shape_ell();
  geom_install_shape_tor();
  geom_install_shape_eto();

  geom_install_shape_pl1();
  geom_install_shape_pl2();
  geom_install_shape_pl3();
  geom_install_shape_pl4();
  geom_install_shape_pln();

  geom_install_shape_tra();
  geom_install_shape_rot();
  geom_install_shape_sca();
  geom_install_shape_mat();

  geom_install_surface_shape_specials();
  geom_install_surface_shape_parallelogram();

  geom_initialized_val = 1;
}

int geom_initialized(void)
{
  return geom_initialized_val;
}
