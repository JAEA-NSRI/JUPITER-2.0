#include "surface_shape.h"
#include "data.h"
#include "defs.h"
#include "geom_assert.h"
#include "func_defs.h"
#include "funcs_common.h"
#include "global.h"
#include "infomap.h"
#include "list.h"
#include "struct_data.h"
#include "surfshp_specials.h"
#include "udata-priv.h"
#include "abuilder-priv.h"

#include <stdlib.h>

#define geom_surface_shape_element_entry(ptr) \
  geom_list_entry(ptr, struct geom_surface_shape_element, list)

static void geom_surface_shape_element_init(geom_surface_shape_element *el,
  geom_surface_shape_data *parent)
{
  geom_list_init(&el->list);
  el->data = NULL;
  el->funcs = NULL;
  el->op = GEOM_SOP_INVALID;
  el->parent = parent;
  geom_user_defined_data_init(&el->extra_data);
}

geom_surface_shape_data *geom_surface_shape_data_new(geom_data_element *parent,
                                                     geom_error *e)
{
  geom_surface_shape_data *d;
  geom_error er;
  geom_data *master;
  GEOM_ASSERT(parent);

  master = geom_data_element_master(parent);
  GEOM_ASSERT(master);

  d = (geom_surface_shape_data *)malloc(sizeof(geom_surface_shape_data));
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

  parent->surface_shape = d;
  d->parant = parent;
  geom_surface_shape_element_init(&d->head, d);
  geom_user_defined_data_init(&d->extra_data);
  return d;
}

void geom_surface_shape_data_delete(geom_surface_shape_data *data)
{
  struct geom_list *n, *p;
  geom_surface_shape_element *el;
  geom_data *master;

  if (!data) return;

  master = geom_surface_shape_data_master(data);
  GEOM_ASSERT(master);

  geom_list_foreach_safe(p, n, &data->head.list) {
    el = geom_surface_shape_element_entry(p);
    geom_surface_shape_element_delete(el);
  }

  geom_user_defined_data_free(master, &data->extra_data);
  geom_data_del_pointer(master, data);
}

geom_data_element *geom_surface_shape_data_parent(geom_surface_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->parant;
}

geom_data *geom_surface_shape_data_master(geom_surface_shape_data *data)
{
  GEOM_ASSERT(data);

  return geom_data_element_master(data->parant);
}

static int geom_surface_shape_data_call_error(
  geom_error error, geom_surface_shape_data *data,
  geom_surface_shape_element *element,
  geom_surface_shape_data_error_callback *callback, void *ext_data,
  geom_error *set_err, geom_surface_shape_element **error_element)
{
  GEOM_ASSERT(data);
  if (error == GEOM_SUCCESS) return 0;
  if (set_err) *set_err = error;
  if (error_element) *error_element = element;
  if (!callback) return 0;
  return callback(error, data, element, ext_data);
}

geom_error
geom_surface_shape_data_check(geom_surface_shape_data *data,
                              geom_surface_shape_data_error_callback *callback,
                              void *ext_data,
                              geom_surface_shape_element **error_element)
{
  int first_body;
  int stack_cur;
  geom_error err;
  geom_surface_shape_element *el, *last_el;

  GEOM_ASSERT(data);

  first_body = 1;
  stack_cur = -1;
  err = GEOM_SUCCESS;

  last_el = NULL;
  el = geom_surface_shape_data_get_element(data);
  for (; el; last_el = el, el = geom_surface_shape_element_next(el)) {
    geom_shape_operator op;
    geom_surface_shape shape;
    geom_shape_type type;

    op = geom_surface_shape_element_shape_operator(el);
    shape = geom_surface_shape_element_get_shape(el);
    type = geom_surface_shape_element_get_shape_type(el);

    if (type == GEOM_SHPT_BODY) {
      if (op == GEOM_SOP_SET || op == GEOM_SOP_PUSH) {
        ++stack_cur;
        if (stack_cur >= GEOM_SHAPE_STACK_SIZE) {
          if (geom_surface_shape_data_call_error(GEOM_ERR_SHAPE_STACK_OVERFLOW,
                                                 data, el, callback, ext_data,
                                                 &err, error_element)) {
            break;
          }
        }
      } else if (first_body) {
        stack_cur = 0;
        if (geom_surface_shape_data_call_error(GEOM_ERR_SHAPE_OP_SHOULD_SET,
                                               data, el, callback, ext_data,
                                               &err, error_element)) {
          break;
        }
      }
      first_body = 0;
    } else if (shape == GEOM_SURFACE_SHAPE_COMB) {
      if (op == GEOM_SOP_SET || op == GEOM_SOP_PUSH) {
        if (geom_surface_shape_data_call_error(GEOM_ERR_INVALID_SHAPE_OP, data,
                                               el, callback, ext_data, &err,
                                               error_element)) {
          break;
        }
      }
      --stack_cur;
      if (stack_cur < 0) {
        if (geom_surface_shape_data_call_error(GEOM_ERR_SHAPE_STACK_UNDERFLOW,
                                               data, el, callback, ext_data,
                                               &err, error_element)) {
          break;
        }
      }
    }
  }

  if (err == GEOM_SUCCESS) {
    if (stack_cur > 0) {
      geom_surface_shape_data_call_error(GEOM_ERR_SHAPE_STACK_UNCLOSED, data,
                                         last_el, callback, ext_data, &err,
                                         error_element);
    }
  }
  return err;
}

static int
geom_surface_shape_data_boolean_calc(geom_shape_operator op, int a, int b)
{
  switch(op) {
  case GEOM_SOP_OR:
  case GEOM_SOP_ADD:
    if (a || b) return 1;
    return 0;

  case GEOM_SOP_AND:
  case GEOM_SOP_MUL:
    if (a && b) return 1;
    return 0;

  case GEOM_SOP_XOR:
    if ((!!a) != (!!b)) return 1;
    return 0;

  case GEOM_SOP_SUB:
    if (b) return 0;
    return a;

  case GEOM_SOP_SET:
  case GEOM_SOP_PUSH:
  case GEOM_SOP_INVALID:
    break;
  }
  GEOM_UNREACHABLE();
  return 0;
}

int geom_surface_shape_data_inout_test_at(geom_surface_shape_data *data,
                                          double x, double y)
{
  int stack[GEOM_SHAPE_STACK_SIZE];
  int stack_p = -1;
  geom_surface_shape_element *el;

  stack[0] = 0;
  el = geom_surface_shape_data_get_element(data);
  for (; el; el = geom_surface_shape_element_next(el)) {
    geom_shape_operator op;
    geom_surface_shape shape;
    geom_shape_type type;

    type = geom_surface_shape_element_get_shape_type(el);
    shape = geom_surface_shape_element_get_shape(el);
    op = geom_surface_shape_element_shape_operator(el);

    if (type == GEOM_SHPT_BODY) {
      int r;
      r = geom_surface_shape_element_inout_test_at(el, x, y);
      if (stack_p < 0 || op == GEOM_SOP_SET || op == GEOM_SOP_PUSH) {
        stack[++stack_p] = r;
      } else {
        int a = stack[stack_p];
        stack[stack_p] = geom_surface_shape_data_boolean_calc(op, a, r);
      }
    } else if (shape == GEOM_SURFACE_SHAPE_COMB) {
      int a, b;

      b = stack[stack_p];
      a = stack[--stack_p];
      stack[stack_p] = geom_surface_shape_data_boolean_calc(op, a, b);
    }
  }
  return stack[0];
}

struct geom_surface_shape_args_builder
{
  const geom_surface_shape_funcs *funcs;
  geom_args_builder *ab;
  geom_surface_shape shape;
};

geom_surface_shape_args_builder *
geom_surface_shape_args_builder_new(geom_surface_shape shape, geom_error *e)
{
  const geom_surface_shape_funcs *p;
  geom_args_builder *b;
  geom_surface_shape_args_builder *sab;

  p = geom_get_surface_shape_func(shape);
  if (!p) {
    if (e) *e = GEOM_ERR_INVALID_SURFACE_SHAPE;
    return NULL;
  }

  sab = (geom_surface_shape_args_builder *)malloc(
    sizeof(geom_surface_shape_args_builder));
  if (!sab) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  if (p->c.args_next) {
    b = geom_args_builder_new(p->c.args_next, p->c.args_check, e);
    if (!b) {
      free(sab);
      return NULL;
    }
  } else {
    b = NULL;
  }

  sab->ab = b;
  sab->shape = shape;
  sab->funcs = p;
  if (e) *e = GEOM_SUCCESS;
  return sab;
}

void geom_surface_shape_args_builder_delete(geom_surface_shape_args_builder *b)
{
  if (!b) return;

  geom_args_builder_free(b->ab);
  free(b);
}

geom_args_builder *
geom_surface_shape_args_get_builder(geom_surface_shape_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->ab;
}

void geom_surface_shape_data_add_element(geom_surface_shape_element *element)
{
  geom_surface_shape_data *shp;

  GEOM_ASSERT(element);

  shp = geom_surface_shape_element_parent(element);
  GEOM_ASSERT(shp);

  geom_list_insert_prev(&shp->head.list, &element->list);
}

geom_error
geom_surface_shape_element_set_shape(geom_surface_shape_element *e,
                                     geom_shape_operator op,
                                     geom_surface_shape_args_builder *shape)
{
  geom_error err;
  const geom_surface_shape_funcs *fun;
  geom_data *master;
  geom_args_builder *ab;
  void *data;

  GEOM_ASSERT(e);
  GEOM_ASSERT(e->parent);
  GEOM_ASSERT(shape);
  GEOM_ASSERT(!geom_surface_shape_element_is_head(e));

  master = geom_surface_shape_element_master(e);
  GEOM_ASSERT(master);

  fun = shape->funcs;
  GEOM_ASSERT(fun);

  err = GEOM_SUCCESS;
  data = geom_funcs_common_allocate(&fun->c, master, &err);
  if (err != GEOM_SUCCESS)
    return err;
  if (data) {
    ab = geom_surface_shape_args_get_builder(shape);
    err = geom_funcs_common_set_data(&fun->c, data, ab);
  }

  /* transformation is not supported yet */
  GEOM_ASSERT(fun->shape_type == GEOM_SHPT_BODY ||
              fun->shape_type == GEOM_SHPT_SPECIAL);

  if (e->data) {
    geom_data_del_pointer(master, e->data);
    geom_funcs_common_deallocate(&e->funcs->c, e->data, master);
  }

  e->funcs = fun;
  e->data = data;
  e->op = op;
  return GEOM_SUCCESS;
}


geom_surface_shape_element *
geom_surface_shape_element_new(geom_surface_shape_data *data, geom_error *e)
{
  geom_error err;
  geom_data *master;
  geom_surface_shape_element *el;

  GEOM_ASSERT(data);

  master = geom_surface_shape_data_master(data);
  GEOM_ASSERT(master);

  el = (geom_surface_shape_element *)malloc(sizeof(geom_surface_shape_element));
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

  geom_surface_shape_element_init(el, data);
  if (e) *e = GEOM_SUCCESS;
  return el;
}

void geom_surface_shape_element_delete(geom_surface_shape_element *element)
{
  geom_data *master;
  geom_error err;

  GEOM_ASSERT(element);

  master = geom_surface_shape_element_master(element);
  GEOM_ASSERT(master);

  geom_list_delete(&element->list);

  if (element->funcs) {
    geom_funcs_common_deallocate(&element->funcs->c, element->data, master);
  }
  geom_user_defined_data_free(master, &element->extra_data);

  err = geom_data_del_pointer(master, element);
  GEOM_ASSERT(err = GEOM_SUCCESS);
}

geom_surface_shape_data *
geom_surface_shape_element_parent(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  return element->parent;
}

geom_data *
geom_surface_shape_element_master(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  return geom_surface_shape_data_master(element->parent);
}

geom_surface_shape_element *
geom_surface_shape_data_get_element(geom_surface_shape_data *data)
{
  GEOM_ASSERT(data);

  return geom_surface_shape_element_next(&data->head);
}

geom_surface_shape_element *
geom_surface_shape_element_next(geom_surface_shape_element *element)
{
  geom_surface_shape_element *e;
  GEOM_ASSERT(element);

  e = geom_surface_shape_element_entry(geom_list_next(&element->list));
  if (geom_surface_shape_element_is_head(e)) {
    return NULL;
  } else {
    return e;
  }
}

geom_surface_shape_element *
geom_surface_shape_element_prev(geom_surface_shape_element *element)
{
  geom_surface_shape_element *e;
  GEOM_ASSERT(element);

  e = geom_surface_shape_element_entry(geom_list_prev(&element->list));
  if (geom_surface_shape_element_is_head(e)) {
    return NULL;
  } else {
    return e;
  }
}

int geom_surface_shape_element_is_head(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  return element == &(geom_surface_shape_element_parent(element)->head);
}

geom_shape_operator
geom_surface_shape_element_shape_operator(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  return element->op;
}

geom_surface_shape
geom_surface_shape_element_get_shape(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  if (element->funcs) {
    return element->funcs->enum_val;
  }
  return GEOM_SURFACE_SHAPE_INVALID;
}

geom_shape_type
geom_surface_shape_element_get_shape_type(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  if (element->funcs) {
    return element->funcs->shape_type;
  }
  return GEOM_SHPT_INVALID;
}

geom_shape_type geom_shape_type_for_surface_shape(geom_surface_shape shape)
{
  const geom_surface_shape_funcs *funcs;
  funcs = geom_get_surface_shape_func(shape);
  if (funcs)
    return funcs->shape_type;
  return GEOM_SHPT_INVALID;
}

geom_info_map *
geom_surface_shape_element_shape_info(geom_surface_shape_element *element)
{
  geom_info_map *m;
  geom_error err;
  const geom_surface_shape_funcs *funcs;

  GEOM_ASSERT(element);
  if (element->funcs && element->funcs->c.infomap_gen) {
    m = geom_info_map_new(&err);
    if (!m)
      return NULL;
    err = element->funcs->c.infomap_gen(element->data, m);
    if (err != GEOM_SUCCESS) {
      geom_info_map_delete_all(m);
      return NULL;
    }
    return m;
  } else {
    return NULL;
  }
}

geom_error geom_surface_shape_element_set_parameter(
  geom_surface_shape_element *element, geom_size_type index,
  const geom_variant *var, geom_variant *einfo)
{
  geom_error err;

  GEOM_ASSERT(element);
  GEOM_ASSERT(var);
  GEOM_ASSERT(element->funcs);

  if (element->funcs->c.args_check && element->funcs->c.set_value) {
    err = element->funcs->c.args_check(element->data, NULL, index, var, einfo);
    if (err != GEOM_SUCCESS) {
      return err;
    }

    err = element->funcs->c.set_value(element->data, index, var);
    return err;
  }
  return GEOM_ERR_RANGE;
}

geom_error
geom_surface_shape_element_get_parameter(geom_surface_shape_element *element,
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

const geom_user_defined_data *
geom_surface_shape_data_get_extra_data(geom_surface_shape_data *data)
{
  GEOM_ASSERT(data);

  return &data->extra_data;
}

geom_error geom_surface_shape_data_set_extra_data(geom_surface_shape_data *data,
                                                  void *extra_data,
                                                  geom_deallocator *dealloc)
{
  GEOM_ASSERT(data);

  return geom_user_defined_data_set(geom_surface_shape_data_master(data),
                                    &data->extra_data, extra_data, dealloc);
}

const geom_user_defined_data *
geom_surface_shape_element_get_extra_data(geom_surface_shape_element *element)
{
  GEOM_ASSERT(element);

  return &element->extra_data;
}

geom_error
geom_surface_shape_element_set_extra_data(geom_surface_shape_element *element,
                                          void *extra_data,
                                          geom_deallocator *dealloc)
{
  GEOM_ASSERT(element);

  return geom_user_defined_data_set(geom_surface_shape_element_master(element),
                                    &element->extra_data, extra_data, dealloc);
}

int geom_surface_shape_element_inout_test_at(
  geom_surface_shape_element *element, double x, double y)
{
  GEOM_ASSERT(element);

  if (!element->funcs || !element->funcs->body_testf)
    return 0;

  return element->funcs->body_testf(element->data, x, y);
}
