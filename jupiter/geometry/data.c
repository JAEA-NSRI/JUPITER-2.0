
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "data.h"
#include "alloc_list.h"
#include "list.h"
#include "init.h"
#include "struct_data.h"
#include "udata-priv.h"

#define geom_data_element_entry(ptr) \
  geom_list_entry(ptr, struct geom_data_element, list)

geom_data *geom_data_new(geom_error *e)
{
  geom_data *d;
  d = (geom_data *)malloc(sizeof(geom_data));
  if (!d) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  d->allocs = geom_alloc_list_new();
  if (!d->allocs) {
    free(d);
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  d->elements = NULL;
  geom_user_defined_data_init(&d->extra_data);

  if (e) *e = GEOM_SUCCESS;
  return d;
}

void geom_data_delete(geom_data *d)
{
  if (d) {
    geom_user_defined_data_free(d, &d->extra_data);

    if (d->allocs) {
      geom_alloc_free_all(d->allocs);
    }
  }
  free(d);
}

geom_error geom_data_add_pointer(geom_data *d, void *p,
                                 geom_deallocator *dealloc)
{
  GEOM_ASSERT(d);
  GEOM_ASSERT(d->allocs);

  return geom_alloc_add(d->allocs, p, dealloc);
}

geom_error geom_data_del_pointer(geom_data *d, void *p)
{
  GEOM_ASSERT(d);
  GEOM_ASSERT(d->allocs);

  return geom_alloc_free(d->allocs, p);
}

geom_error geom_data_add_element(geom_data_element *elem)
{
  geom_data_element *head;
  geom_data *parent;
  geom_error e;

  GEOM_ASSERT(elem);
  GEOM_ASSERT(elem->parent);

  parent = elem->parent;

  if (parent->elements) {
    head = parent->elements;
  } else {
    head = geom_data_element_new(parent, &e);
    if (!head) return e;
  }

  geom_list_insert_prev(&head->list, &elem->list);
  parent->elements = head;

  return GEOM_SUCCESS;
}

geom_data_element *geom_data_get_element(geom_data *data)
{
  GEOM_ASSERT(data);

  if (data->elements) {
    if (geom_list_empty(&data->elements->list)) {
      return NULL;
    } else {
      return geom_data_element_next(data->elements);
    }
  } else {
    return NULL;
  }
}

geom_data_element *geom_data_element_new(geom_data *parent, geom_error *e)
{
  geom_error er;
  geom_data_element *d;

  GEOM_ASSERT(parent);

  d = (geom_data_element *)malloc(sizeof(geom_data_element));
  if (!d) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  geom_list_init(&d->list);
  d->file = NULL;
  d->name = NULL;
  d->shape = NULL;
  d->surface_shape = NULL;
  d->parent = parent;
  d->init_data = NULL;
  geom_user_defined_data_init(&d->extra_data);

  er = geom_data_add_pointer(parent, d, free);
  if (er != GEOM_SUCCESS) {
    if (e) *e = er;
    free(d);
    return NULL;
  }

  if (e) *e = GEOM_SUCCESS;
  return d;
}

geom_data *geom_data_element_parent(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return element->parent;
}

geom_data *geom_data_element_master(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return geom_data_element_parent(element);
}

geom_file_data *geom_data_element_get_file(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return element->file;
}

geom_shape_data *geom_data_element_get_shape(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return element->shape;
}

geom_init_data *geom_data_element_get_init(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return element->init_data;
}

geom_surface_shape_data *
geom_data_element_get_surface_shape(geom_data_element *element)
{
  GEOM_ASSERT(element);

  return element->surface_shape;
}

void geom_data_element_delete(geom_data_element *element)
{
  if (!element) return;

  GEOM_ASSERT(element->parent);

  /* TODO: free sub data */
  geom_user_defined_data_free(geom_data_element_master(element),
                              &element->extra_data);
  geom_list_delete(&element->list);
  geom_data_del_pointer(element->parent, element);
}

geom_data_element *geom_data_element_next(geom_data_element *elem)
{
  struct geom_list *l;
  geom_data_element *head;

  GEOM_ASSERT(elem);
  GEOM_ASSERT(elem->parent);

  head = elem->parent->elements;
  GEOM_ASSERT(head);

  l = &elem->list;
  l = geom_list_next(l);

  if (l == &head->list) {
    return NULL;
  } else {
    return geom_data_element_entry(l);
  }
}

geom_data_element *geom_data_element_prev(geom_data_element *elem)
{
  struct geom_list *l;
  geom_data_element *head;

  GEOM_ASSERT(elem);
  GEOM_ASSERT(elem->parent);

  head = elem->parent->elements;
  GEOM_ASSERT(head);

  l = &elem->list;
  l = geom_list_prev(l);

  if (l == &head->list) {
    return NULL;
  } else {
    return geom_data_element_entry(l);
  }
}

void geom_data_element_set_name(geom_data_element *elem, const char *name)
{
  GEOM_ASSERT(elem);

  elem->name = name;
}

const char *geom_data_element_get_name(geom_data_element *elem)
{
  GEOM_ASSERT(elem);

  return elem->name;
}

geom_error geom_data_set_extra_data(geom_data *data, void *extra_data,
                                    geom_deallocator *dealloc)
{
  return geom_user_defined_data_set(data, &data->extra_data,
                                    extra_data, dealloc);
}

const geom_user_defined_data *
geom_data_get_extra_data(geom_data *data)
{
  return &data->extra_data;
}

geom_error
geom_data_element_set_extra_data(geom_data_element *el,
                                 void *extra_data, geom_deallocator *dealloc)
{
  return geom_user_defined_data_set(geom_data_element_master(el),
                                    &el->extra_data, extra_data, dealloc);
}

const geom_user_defined_data *
geom_data_element_get_extra_data(geom_data_element *el)
{
  return &el->extra_data;
}
