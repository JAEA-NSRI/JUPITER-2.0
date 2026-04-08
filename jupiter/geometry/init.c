
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "init.h"
#include "defs.h"
#include "geom_assert.h"
#include "list.h"
#include "data.h"
#include "abuilder.h"
#include "abuilder-priv.h"
#include "global.h"
#include "struct_data.h"
#include "func_defs.h"
#include "func_data.h"
#include "variant.h"
#include "infomap.h"
#include "udata-priv.h"
#include "funcs_common.h"

#define geom_init_element_entry(ptr) \
  geom_list_entry(ptr, struct geom_init_element, list)

struct geom_init_args_builder
{
  const geom_init_funcs *funcs;
  geom_args_builder *ab;
  geom_init_func f;
};

static void
geom_init_element_init(geom_init_element *el, geom_init_data *parent)
{
  GEOM_ASSERT(el);
  GEOM_ASSERT(parent);

  el->data = NULL;
  geom_user_defined_data_init(&el->component_data);
  el->component_id = 0;
  el->component_name = NULL;
  el->op = GEOM_OP_INVALID;
  el->threshold = HUGE_VAL;
  el->parent = parent;
  el->funcs = NULL;
  geom_list_init(&el->list);
  geom_user_defined_data_init(&el->extra_data);
}

geom_init_data *geom_init_data_new(geom_data_element *parent, geom_error *e)
{
  geom_error er;
  geom_data *master;
  geom_init_data *d;

  GEOM_ASSERT(parent);

  master = geom_data_element_parent(parent);
  GEOM_ASSERT(master);

  d = (geom_init_data *)malloc(sizeof(geom_init_data));
  if (!d) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  er = geom_data_add_pointer(master, d, free);
  if (e) *e = er;
  if (er != GEOM_SUCCESS) {
    free(d);
    return NULL;
  }

  if (parent->init_data) {
    geom_init_data_delete(parent->init_data);
  }

  parent->init_data = d;
  d->parent = parent;
  geom_init_element_init(&d->element, d);

  return d;
}

void geom_init_data_delete(geom_init_data *data)
{
  geom_data *master;
  geom_data_element *par;
  geom_init_element *el;
  struct geom_list *n, *p;

  if (!data) return;

  master = geom_init_data_master(data);
  GEOM_ASSERT(master);

  geom_user_defined_data_free(master, &data->extra_data);

  geom_list_foreach_safe(p, n, &data->element.list) {
    el = geom_init_element_entry(p);
    geom_init_element_delete(el);
  }

  par = geom_init_data_parent(data);
  par->init_data = NULL;

  geom_data_del_pointer(master, data);
}

geom_data_element *geom_init_data_parent(geom_init_data *data)
{
  GEOM_ASSERT(data);

  return data->parent;
}

geom_data *geom_init_data_master(geom_init_data *data)
{
  geom_data_element *d;

  GEOM_ASSERT(data);

  d = geom_init_data_parent(data);

  return geom_data_element_master(d);
}

void geom_init_data_add_element(geom_init_element *el)
{
  geom_init_data *ip;

  GEOM_ASSERT(el);

  ip = el->parent;
  GEOM_ASSERT(ip);

  geom_list_insert_prev(&ip->element.list, &el->list);
}

geom_init_element *geom_init_data_get_element(geom_init_data *data)
{
  struct geom_list *l;

  GEOM_ASSERT(data);

  l = geom_list_next(&data->element.list);
  if (l == &data->element.list) return NULL;

  return geom_init_element_entry(l);
}

geom_init_element *geom_init_element_new(geom_init_data *data, geom_error *e)
{
  geom_error err;
  geom_init_element *el;
  geom_data *master;

  GEOM_ASSERT(data);

  master = geom_init_data_master(data);
  GEOM_ASSERT(master);

  el = (geom_init_element *)malloc(sizeof(geom_init_element));
  if (!el) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  err = geom_data_add_pointer(master, el, free);
  if (err != GEOM_SUCCESS) {
    free(el);
    if (e) *e = err;
    return NULL;
  }

  if (e) *e = GEOM_SUCCESS;
  geom_init_element_init(el, data);

  return el;
}

void geom_init_element_delete(geom_init_element *e)
{
  geom_data *master;
  geom_error err;

  GEOM_ASSERT(e);

  master = geom_init_element_master(e);
  GEOM_ASSERT(master);

  if (e->funcs) {
    geom_funcs_common_deallocate(&e->funcs->c, e->data, master);
  }
  geom_user_defined_data_free(master, &e->component_data);
  geom_list_delete(&e->list);
  err = geom_data_del_pointer(master, e);
  GEOM_ASSERT(err == GEOM_SUCCESS);
}

geom_init_data *geom_init_element_parent(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return e->parent;
}

geom_data *geom_init_element_master(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return geom_init_data_master(geom_init_element_parent(e));
}

geom_error
geom_init_element_set_component(geom_init_element *e,
                                int comp_id, void *comp_data,
                                geom_deallocator *comp_data_dealloc,
                                const char *comp_name)
{
  geom_error err;
  GEOM_ASSERT(e);
  GEOM_ASSERT(!geom_init_element_is_head(e));

  err = geom_user_defined_data_set(geom_init_element_master(e),
                                   &e->component_data, comp_data,
                                   comp_data_dealloc);
  if (err != GEOM_SUCCESS) {
    return err;
  }

  e->component_id = comp_id;
  e->component_name = comp_name;
  return err;
}

void geom_init_element_set_threshold(geom_init_element *e,
                                     double threshold)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(!geom_init_element_is_head(e));

  e->threshold = threshold;
}

void geom_init_element_set_operator(geom_init_element *e,
                                    geom_data_operator op)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(!geom_init_element_is_head(e));

  e->op = op;
}

const geom_user_defined_data *
geom_init_element_get_comp_data(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return &e->component_data;
}

const char *geom_init_element_get_comp_name(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return e->component_name;
}

int geom_init_element_get_comp_id(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return e->component_id;
}

double geom_init_element_get_threshold(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return e->threshold;
}

geom_data_operator geom_init_element_get_operator(geom_init_element *e)
{
  GEOM_ASSERT(e);

  return e->op;
}

geom_init_func geom_init_element_get_func(geom_init_element *e)
{
  GEOM_ASSERT(e);

  if (e->funcs) {
    return e->funcs->enum_val;
  } else {
    return GEOM_INIT_FUNC_INVALID;
  }
}

geom_info_map *geom_init_element_func_info(geom_init_element *e)
{
  geom_info_map *m;
  geom_error err;

  GEOM_ASSERT(e);

  if (e->funcs && e->funcs->c.infomap_gen) {
    m = geom_info_map_new(&err);
    if (!m) return NULL;
    err = e->funcs->c.infomap_gen(e->data, m);
    if (err != GEOM_SUCCESS) {
      geom_info_map_delete_all(m);
      return NULL;
    }
    return m;
  } else {
    return NULL;
  }
}

geom_error geom_init_element_set_func(geom_init_element *e,
                                      geom_init_args_builder *init_func)
{
  geom_error err;
  const geom_init_funcs *fun;
  geom_data *master;
  geom_args_builder *ab;
  void *data;

  GEOM_ASSERT(e);
  GEOM_ASSERT(init_func);
  GEOM_ASSERT(!geom_init_element_is_head(e));

  master = geom_init_element_master(e);
  GEOM_ASSERT(master);

  fun = init_func->funcs;
  GEOM_ASSERT(fun);

  err = GEOM_SUCCESS;
  data = geom_funcs_common_allocate(&fun->c, master, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }
  if (data) {
    ab = geom_init_args_get_builder(init_func);
    err = geom_funcs_common_set_data(&fun->c, data, ab);
  }

  e->funcs = fun;
  e->data = data;
  return GEOM_SUCCESS;
}

geom_error geom_init_element_set_func_vargs(geom_init_element *elem,
                                            geom_variant *einfo,
                                            geom_init_func f, ...)
{
  geom_error e;
  geom_args_builder *bb;
  geom_init_args_builder *ab;
  va_list ap;

  GEOM_ASSERT(elem);
  GEOM_ASSERT(!geom_init_element_is_head(elem));

  e = GEOM_SUCCESS;

  ab = geom_init_args_builder_new(f, &e);
  if (!ab) return e;

  bb = geom_init_args_get_builder(ab);
  va_start(ap, f);
  e = geom_args_builder_vargs(bb, einfo, ap);
  va_end(ap);

  if (e == GEOM_SUCCESS) {
    geom_init_element_set_func(elem, ab);
  }

  geom_init_args_builder_delete(ab);

  return e;
}

geom_error geom_init_element_set_func_vlist(geom_init_element *elem,
                                            geom_variant *einfo,
                                            geom_init_func f,
                                            geom_variant_list *vl,
                                            geom_variant_list **retcur)
{
  geom_error e;
  geom_args_builder *bb;
  geom_init_args_builder *ab;

  GEOM_ASSERT(elem);
  GEOM_ASSERT(vl);
  GEOM_ASSERT(!geom_init_element_is_head(elem));

  e = GEOM_SUCCESS;

  ab = geom_init_args_builder_new(f, &e);
  if (!ab) return e;

  bb = geom_init_args_get_builder(ab);
  e = geom_args_builder_vlist(bb, vl, retcur, einfo);

  if (e == GEOM_SUCCESS) {
    geom_init_element_set_func(elem, ab);
  }

  geom_init_args_builder_delete(ab);

  return e;
}

geom_init_element *geom_init_element_next(geom_init_element *e)
{
  geom_init_element *el;
  GEOM_ASSERT(e);

  el = geom_init_element_entry(geom_list_next(&e->list));
  if (geom_init_element_is_head(el)) {
    return NULL;
  } else {
    return el;
  }
}

geom_init_element *geom_init_element_prev(geom_init_element *e)
{
  geom_init_element *el;
  GEOM_ASSERT(e);

  el = geom_init_element_entry(geom_list_prev(&e->list));
  if (geom_init_element_is_head(el)) {
    return NULL;
  } else {
    return el;
  }
}

int geom_init_element_is_head(geom_init_element *e)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(e->parent);

  if (e == &e->parent->element) return 1;
  return 0;
}

double geom_init_element_calc_func_at(geom_init_element *element,
                                      double x, double y, double z,
                                      void *arg, geom_error *e)
{
  GEOM_ASSERT(element);
  GEOM_ASSERT(!geom_init_element_is_head(element));

  if (!element->funcs) {
    if (e) *e = GEOM_ERR_INVALID_INIT_FUNC;
    return HUGE_VAL;
  }

  if (!element->funcs->func) {
    if (e) *e = GEOM_ERR_INVALID_INIT_FUNC;
    return HUGE_VAL;
  }

  return element->funcs->func(element->data, x, y, z, arg);
}

static
double geom_init_element_calc_op(double old_val, double fac,
                                 double new_val, geom_data_operator op)
{
  switch(op) {
  case GEOM_OP_SET:
    return fac * new_val + (1.0 - fac) * old_val;
  case GEOM_OP_ADD:
    return old_val + fac * new_val;
  case GEOM_OP_SUB:
    return old_val - fac * new_val;
  case GEOM_OP_MUL:
    return fac * new_val * old_val + (1.0 - fac) * old_val;
  case GEOM_OP_NONE:
  default:
    return old_val;
  }
}

double geom_init_element_calc_at(geom_init_element *element,
                                 double old_val, double vof_val,
                                 double x, double y, double z,
                                 void *arg, geom_error *e)
{
  double nv;
  int do_calc = 0;

  GEOM_ASSERT(element);
  GEOM_ASSERT(!geom_init_element_is_head(element));

  if (!element->funcs) {
    if (e) *e = GEOM_ERR_INVALID_INIT_FUNC;
    return old_val;
  }

  if (!element->funcs->func) {
    if (e) *e = GEOM_ERR_INVALID_INIT_FUNC;
    return old_val;
  }

  nv = old_val;
  if (element->threshold < -1.0) {
    do_calc = 1;
  } else if (element->threshold < 0.0) {
    if (vof_val < fabs(element->threshold)) {
      do_calc = 1;
      vof_val = 1.0;
    }
  } else if (element->threshold == 0.0) {
    if (vof_val > 0.0) {
      do_calc = 1;
      vof_val = 1.0;
    }
  } else if (element->threshold <= 1.0) {
    if (vof_val >= element->threshold) {
      do_calc = 1;
      vof_val = 1.0;
    }
  } else {
    do_calc = 1;
  }
  if (do_calc) {
    double v;
    v = element->funcs->func(element->data, x, y, z, arg);
    nv = geom_init_element_calc_op(old_val, vof_val, v, element->op);
  }
  return nv;
}

geom_error geom_init_element_set_parameter(geom_init_element *element,
                                           geom_size_type index,
                                           const geom_variant *value,
                                           geom_variant *errinfo)
{
  geom_error err;

  GEOM_ASSERT(element);
  GEOM_ASSERT(element->funcs);
  GEOM_ASSERT(value);

  if (element->funcs->c.args_check && element->funcs->c.set_value) {
    err = element->funcs->c.args_check(element->data, NULL, index, value,
                                       errinfo);
    if (err != GEOM_SUCCESS) {
      return err;
    }

    err = element->funcs->c.set_value(element->data, index, value);
    return err;
  }
  return GEOM_ERR_RANGE;
}

geom_error geom_init_element_get_parameter(geom_init_element *element,
                                           geom_size_type index,
                                           geom_variant *out_variable)
{
  GEOM_ASSERT(element);
  GEOM_ASSERT(out_variable);
  GEOM_ASSERT(element->funcs);

  if (element->funcs->c.get_value) {
    return element->funcs->c.get_value(element->data, index, out_variable);
  }
  return GEOM_ERR_RANGE;
}

void *
geom_init_element_get_func_data(geom_init_element *element, geom_init_func f)
{
  GEOM_ASSERT(element);

  if (!element->funcs) return NULL;
  if (element->funcs->enum_val != f) return NULL;
  return element->data;
}

geom_init_args_builder *
geom_init_args_builder_new(geom_init_func f, geom_error *e)
{
  const geom_init_funcs *p;
  geom_args_builder *b;
  geom_init_args_builder *iab;

  p = geom_get_init_func(f);
  if (!p) {
    if (e) *e = GEOM_ERR_INVALID_INIT_FUNC;
    return NULL;
  }

  iab = (geom_init_args_builder *)malloc(sizeof(geom_init_args_builder));
  if (!iab) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  if (p->c.args_next) {
    b = geom_args_builder_new(p->c.args_next, p->c.args_check, e);
    if (!b) {
      free(iab);
      return NULL;
    }
  } else {
    b = NULL;
  }

  iab->ab = b;
  iab->f = f;
  iab->funcs = p;
  if (e) *e = GEOM_SUCCESS;
  return iab;
}

void geom_init_args_builder_delete(geom_init_args_builder *b)
{
  if (!b) return;

  geom_args_builder_free(b->ab);
  free(b);
}

geom_args_builder *geom_init_args_get_builder(geom_init_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->ab;
}

geom_init_func geom_init_args_get_func(geom_init_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->f;
}

geom_error geom_init_element_set_extra_data(geom_init_element *e, void *data,
                                            geom_deallocator *dealloc)
{
  GEOM_ASSERT(e);

  return geom_user_defined_data_set(geom_init_element_master(e),
                                    &e->extra_data, data, dealloc);
}

const geom_user_defined_data *
geom_init_element_get_extra_data(geom_init_element *element)
{
  GEOM_ASSERT(element);

  return &element->extra_data;
}
