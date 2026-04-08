
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "struct_data.h"
#include "func_defs.h"
#include "func_data.h"
#include "shape_transform.h"
#include "funcs_common.h"
#include "variant.h"
#include "data.h"
#include "infomap.h"

/* Data for GEOM_SHPT_TRANS */
/**
 * @ingroup Geometry
 * @brief Common data for transformations
 */
struct geom_shape_transform
{
  int num_copy; ///< Number of copies to generate
  void *data;   ///< Transformation specific data
  const geom_shape_funcs *data_func; ///< Data delegations
  geom_data *master; ///< data master (to used deallocate `data`)
};

static
void geom_shape_transform_data_dealloc(geom_shape_transform *m)
{
  GEOM_ASSERT(m);
  if (!m->data || !m->data_func) return;
  geom_funcs_common_deallocate(&m->data_func->c, m->data, m->master);
}

void geom_shape_transform_set_data(geom_shape_transform *m,
                                   geom_data *master, void *p,
                                   const geom_shape_funcs *deleg)
{
  GEOM_ASSERT(m);
  if (m->data == p) return;

  geom_shape_transform_data_dealloc(m);

  GEOM_ASSERT(deleg);
  GEOM_ASSERT(master);
  m->master = master;
  m->data = p;
  m->data_func = deleg;
}

void *geom_shape_transform_get_data(geom_shape_transform *m)
{
  GEOM_ASSERT(m);
  return m->data;
}

void geom_shape_transform_set_copy_num(geom_shape_transform *m, int n)
{
  GEOM_ASSERT(m);
  m->num_copy = n;
}

int geom_shape_transform_get_copy_num(geom_shape_transform *m)
{
  GEOM_ASSERT(m);
  return m->num_copy;
}

geom_mat43 geom_shape_transform_get_matrix(geom_shape_transform *m)
{
  if (m->data && m->data_func) {
    return m->data_func->transform_func(m->data);
  }
  return geom_mat43_E();
}

geom_shape geom_shape_transform_get_shape(geom_shape_transform *m)
{
  if (m->data_func) {
    return m->data_func->enum_val;
  }
  return GEOM_SHAPE_INVALID;
}

static
void *geom_shape_transform_allocator(void)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)malloc(sizeof(geom_shape_transform));
  if (!m) return NULL;
  m->data = NULL;
  m->data_func = NULL;
  m->num_copy = 0;
  return m;
}

static
void geom_shape_transform_deallocator(void *p)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;

  geom_shape_transform_data_dealloc(m);
  free(m);
}

static geom_error geom_shape_transform_args_check(void *p, geom_args_builder *b,
                                                  geom_size_type index,
                                                  const geom_variant *v,
                                                  geom_variant *errinfo)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;

  if (m->data_func->c.args_check) {
    return m->data_func->c.args_check(m->data, b, index, v, errinfo);
  }
  return GEOM_ERR_RANGE;
}

static geom_error
geom_shape_transform_get_value(void *p, geom_size_type index,
                               geom_variant *out_variable)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;

  GEOM_ASSERT(m->data);
  GEOM_ASSERT(m->data_func);
  GEOM_ASSERT(m->data_func->c.get_value);

  return m->data_func->c.get_value(m->data, index, out_variable);
}

static geom_error
geom_shape_transform_set_value(void *p, geom_size_type index,
                               const geom_variant *value)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;

  GEOM_ASSERT(m->data);
  GEOM_ASSERT(m->data_func);
  GEOM_ASSERT(m->data_func->c.set_value);

  return m->data_func->c.set_value(m->data, index, value);
}

static geom_size_type
geom_shape_transform_n_params(void *p, geom_args_builder *b)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;

  GEOM_ASSERT(m->data);
  GEOM_ASSERT(m->data_func);
  GEOM_ASSERT(m->data_func->c.n_params);

  return m->data_func->c.n_params(p, b);
}

static geom_error
geom_shape_transform_info_map(void *p, geom_info_map *list)
{
  geom_error e;
  geom_shape_transform *m;
  geom_variant *v;

  v = geom_variant_new(&e);
  if (!v) return GEOM_ERR_NOMEM;

  m = (geom_shape_transform *)p;

  if (m->data && m->data_func) {
    e = m->data_func->c.infomap_gen(m->data, list);
    if (e != GEOM_SUCCESS) goto error;
  }

error:
  geom_variant_delete(v);
  return e;
}

static void *
geom_shape_transform_copy(void *p)
{
  geom_shape_transform *np;
  geom_shape_transform *pp;

  np = (geom_shape_transform *)geom_shape_transform_allocator();
  if (!np) return NULL;

  pp = (geom_shape_transform *)p;

  /* Perform deep copy */
  np->data = NULL;
  np->master = pp->master;
  np->data_func = pp->data_func;
  np->num_copy = pp->num_copy;
  if (pp->data && pp->data_func) {
    if (pp->master) {
      np->data = geom_funcs_common_copy_data(&pp->data_func->c, pp->data,
                                             pp->master, NULL);
    }
    if (!np->data) {
      geom_shape_transform_deallocator(np);
      return NULL;
    }
  }
  return np;
}

static
geom_mat43 geom_shape_transform_delegate(void *p)
{
  geom_shape_transform *m;
  m = (geom_shape_transform *)p;
  return geom_shape_transform_get_matrix(m);
}

static
geom_shape_funcs geom_shape_transform_funcs = {
  .enum_val = GEOM_SHAPE_INVALID,
  .shape_type = GEOM_SHPT_TRANS,
  .c = {
    .allocator = geom_shape_transform_allocator,
    .deallocator = geom_shape_transform_deallocator,
    .n_params = geom_shape_transform_n_params,
    .set_value = geom_shape_transform_set_value,
    .get_value = geom_shape_transform_get_value,
    .args_next = NULL,
    .args_check = geom_shape_transform_args_check,
    .infomap_gen = geom_shape_transform_info_map,
    .copy = geom_shape_transform_copy,
  },
  .body_testf = NULL,
  .body_bboxf = NULL,
  .transform_func = geom_shape_transform_delegate,
};

const geom_shape_funcs *geom_shape_transform_func(void)
{
  return &geom_shape_transform_funcs;
}
