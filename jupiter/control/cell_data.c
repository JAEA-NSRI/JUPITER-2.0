#include "cell_data.h"
#include "data_array.h"
#include "error.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "defs.h"

#include "jupiter/geometry/list.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#define jcntrl_cell_data_entry_entry(ptr) \
  geom_list_entry(ptr, struct jcntrl_cell_data_entry, list)

static void jcntrl_cell_data_entry_init(struct jcntrl_cell_data_entry *p)
{
  geom_list_init(&p->list);
  p->array = NULL;
}

static struct jcntrl_cell_data_entry *jcntrl_cell_data_entry_new(void)
{
  struct jcntrl_cell_data_entry *p;
  p = (struct jcntrl_cell_data_entry *)
    calloc(sizeof(struct jcntrl_cell_data_entry), 1);
  if (!p) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return 0;
  }

  jcntrl_cell_data_entry_init(p);
  return p;
}

static void jcntrl_cell_data_entry_delete(struct jcntrl_cell_data_entry *e)
{
  JCNTRL_ASSERT(e->array);
  geom_list_delete(&e->list);
  jcntrl_data_array_delete(e->array);
  free(e);
}

static void jcntrl_data_array_name_n(const char **str, jcntrl_size_type *len,
                                     jcntrl_data_array *a)
{
  if (a) {
    *str = jcntrl_data_array_name(a, len);
  } else {
    *str = "";
    *len = 0;
  }
}

static int jcntrl_data_array_namecmp_cc(const char *astr, jcntrl_size_type la,
                                        const char *bstr, jcntrl_size_type lb)
{
  for (; la > 0 && lb > 0 && *astr == *bstr; ++astr, ++bstr, --la, --lb)
    /* nop */;
  if (la > 0 && lb > 0)
    return *bstr - *astr;
  if (la > 0)
    return -1;
  if (lb > 0)
    return 1;
  return 0;
}

static int jcntrl_data_array_namecmp_aa(jcntrl_data_array *a,
                                        jcntrl_data_array *b)
{
  jcntrl_size_type la, lb;
  const char *astr, *bstr;

  jcntrl_data_array_name_n(&astr, &la, a);
  jcntrl_data_array_name_n(&bstr, &lb, b);
  return jcntrl_data_array_namecmp_cc(astr, la, bstr, lb);
}

static int jcntrl_data_array_namecmp_ac(jcntrl_data_array *a, const char *bstr,
                                        jcntrl_size_type lb)
{
  jcntrl_size_type la;
  const char *astr;

  jcntrl_data_array_name_n(&astr, &la, a);
  return jcntrl_data_array_namecmp_cc(astr, la, bstr, lb);
}

static struct jcntrl_cell_data_entry *
jcntrl_cell_data_get_entry(jcntrl_cell_data *data, jcntrl_size_type index)
{
  struct jcntrl_cell_data_entry *e;
  struct geom_list *lp, *lh;

  JCNTRL_ASSERT(data);
  lh = &data->head.list;
  if (index >= 0) {
    lp = geom_list_next(lh);
    while (index-- > 0 && lp != lh)
      lp = geom_list_next(lp);
  } else {
    lp = geom_list_prev(lh);
    while (index++ < -1 && lp != lh)
      lp = geom_list_prev(lp);
  }
  if (lp == lh)
    return NULL;

  return jcntrl_cell_data_entry_entry(lp);
}

static struct jcntrl_cell_data_entry *
jcntrl_cell_data_get_entry_by_name(jcntrl_cell_data *data, const char *name,
                                   jcntrl_size_type namelen)
{
  struct geom_list *lp, *lh;

  if (namelen < 0)
    namelen = strlen(name);

  lh = &data->head.list;
  geom_list_foreach (lp, lh) {
    struct jcntrl_cell_data_entry *e;
    e = jcntrl_cell_data_entry_entry(lp);
    if (jcntrl_data_array_namecmp_ac(e->array, name, namelen) == 0)
      return e;
  }
  return NULL;
}

void jcntrl_cell_data_remove_all_arrays(jcntrl_cell_data *cdata)
{
  struct geom_list *lp, *ln, *lh;
  lh = &cdata->head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    struct jcntrl_cell_data_entry *e;
    e = jcntrl_cell_data_entry_entry(lp);
    jcntrl_cell_data_entry_delete(e);
  }
}

static jcntrl_cell_data *jcntrl_cell_data_downcast_impl(jcntrl_shared_object *p)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_cell_data, p);
}

static void *jcntrl_cell_data_downcast_v(jcntrl_shared_object *p)
{
  return jcntrl_cell_data_downcast_impl(p);
}

static jcntrl_shared_object *jcntrl_cell_data_allocator(void)
{
  jcntrl_cell_data *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_cell_data);
  return p ? jcntrl_cell_data_object(p) : NULL;
}

static void jcntrl_cell_data_deleter(jcntrl_shared_object *p)
{
  jcntrl_shared_object_default_deleter(p);
}

static int jcntrl_cell_data_initializer(jcntrl_shared_object *obj)
{
  jcntrl_cell_data *p;
  p = jcntrl_cell_data_downcast_impl(obj);
  jcntrl_cell_data_entry_init(&p->head);
  return 1;
}

static void jcntrl_cell_data_destructor(jcntrl_shared_object *obj)
{
  jcntrl_cell_data *p;
  p = jcntrl_cell_data_downcast_impl(obj);
  jcntrl_cell_data_remove_all_arrays(p);
}

static void jcntrl_cell_data_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jcntrl_cell_data_initializer;
  p->destructor = jcntrl_cell_data_destructor;
  p->allocator = jcntrl_cell_data_allocator;
  p->deleter = jcntrl_cell_data_deleter;
  p->downcast = jcntrl_cell_data_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_cell_data, jcntrl_cell_data_init_func)

jcntrl_cell_data *jcntrl_cell_data_new(void)
{
  return jcntrl_shared_object_new(jcntrl_cell_data);
}

void jcntrl_cell_data_delete(jcntrl_cell_data *data)
{
  JCNTRL_ASSERT(data);
  jcntrl_shared_object_delete(jcntrl_cell_data_object(data));
}

jcntrl_shared_object *jcntrl_cell_data_object(jcntrl_cell_data *data)
{
  return &data->object;
}

jcntrl_cell_data *jcntrl_cell_data_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_cell_data, object);
}

int jcntrl_cell_data_add_array(jcntrl_cell_data *data, jcntrl_data_array *array)
{
  const char *name;
  jcntrl_size_type namelen;
  struct geom_list *lp, *lh;
  struct jcntrl_cell_data_entry *e, *en;

  JCNTRL_ASSERT(data);

  e = NULL;
  name = jcntrl_data_array_name(array, &namelen);
  if (name && namelen > 0)
    e = jcntrl_cell_data_get_entry_by_name(data, name, namelen);

  en = jcntrl_cell_data_entry_new();
  if (!en)
    return 0;

  en->array = jcntrl_data_array_take_ownership(array);
  JCNTRL_ASSERT(en->array);
  if (e) {
    geom_list_insert_prev(&e->list, &en->list);
    jcntrl_cell_data_entry_delete(e);
  } else {
    geom_list_insert_prev(&data->head.list, &en->list);
  }
  return 1;
}

jcntrl_size_type jcntrl_cell_data_get_number_of_arrays(jcntrl_cell_data *data)
{
  struct geom_list *lp, *lh;
  jcntrl_size_type n;

  JCNTRL_ASSERT(data);

  n = 0;
  lh = &data->head.list;
  geom_list_foreach (lp, lh)
    ++n;
  return n;
}

jcntrl_data_array *jcntrl_cell_data_get_array(jcntrl_cell_data *data,
                                              jcntrl_size_type index)
{
  struct jcntrl_cell_data_entry *e;

  e = jcntrl_cell_data_get_entry(data, index);
  return e ? e->array : NULL;
}

jcntrl_data_array *jcntrl_cell_data_get_array_by_name(jcntrl_cell_data *data,
                                                      const char *name,
                                                      jcntrl_size_type namelen)
{
  struct jcntrl_cell_data_entry *e;
  e = jcntrl_cell_data_get_entry_by_name(data, name, namelen);
  return e ? e->array : NULL;
}

static int jcntrl_cell_data_remove_entry(struct jcntrl_cell_data_entry *e)
{
  if (!e)
    return 0;

  jcntrl_cell_data_entry_delete(e);
  return 1;
}

int jcntrl_cell_data_remove_array(jcntrl_cell_data *data,
                                  jcntrl_size_type index)
{
  struct jcntrl_cell_data_entry *e;
  e = jcntrl_cell_data_get_entry(data, index);
  return jcntrl_cell_data_remove_entry(e);
}

int jcntrl_cell_data_remove_array_by_name(jcntrl_cell_data *data,
                                          const char *name,
                                          jcntrl_size_type namelen)
{
  struct jcntrl_cell_data_entry *e;
  e = jcntrl_cell_data_get_entry_by_name(data, name, namelen);
  return jcntrl_cell_data_remove_entry(e);
}

int jcntrl_cell_data_shallow_copy(jcntrl_cell_data *dest,
                                  jcntrl_cell_data *from)
{
  jcntrl_data_array *aryp;
  jcntrl_cell_data_remove_all_arrays(dest);
  jcntrl_cell_data_array_foreach (from, aryp, iter) {
    if (!jcntrl_cell_data_add_array(dest, aryp))
      return 0;
  }
  return 1;
}

int jcntrl_cell_data_deep_copy(jcntrl_cell_data *dest, jcntrl_cell_data *from)
{
  jcntrl_data_array *aryp;
  jcntrl_cell_data_remove_all_arrays(dest);
  jcntrl_cell_data_array_foreach(from, aryp, iter) {
    jcntrl_data_array *nary;
    nary = jcntrl_data_array_dup(aryp);
    if (!nary)
      return 0;

    if (!jcntrl_cell_data_add_array(dest, nary)) {
      jcntrl_data_array_delete(nary);
      return 0;
    }

    jcntrl_shared_object_release_ownership(jcntrl_data_array_object(nary));
  }
  return 1;
}

static void
jcntrl_cell_data_iterator_move(jcntrl_cell_data_iterator *iter,
                               struct geom_list *mover(struct geom_list *p))
{
  struct geom_list *lp, *lh;

  JCNTRL_ASSERT(iter);
  JCNTRL_ASSERT(iter->celldata);

  if (jcntrl_cell_data_iterator_is_end(iter))
    return;

  lh = &iter->celldata->head.list;
  lp = iter->cursor ? &iter->cursor->list : lh;
  lp = mover(lp);
  *(jcntrl_cell_data_entry **)&iter->cursor = jcntrl_cell_data_entry_entry(lp);
}

static void jcntrl_cell_data_iterator_rewind(jcntrl_cell_data_iterator *iter)
{
  *(jcntrl_cell_data_entry **)&iter->cursor = NULL;
}

void jcntrl_cell_data_iterator_goto_first(jcntrl_cell_data_iterator *iter)
{
  jcntrl_cell_data_iterator_rewind(iter);
  jcntrl_cell_data_iterator_next(iter);
}

void jcntrl_cell_data_iterator_goto_last(jcntrl_cell_data_iterator *iter)
{
  jcntrl_cell_data_iterator_rewind(iter);
  jcntrl_cell_data_iterator_prev(iter);
}

int jcntrl_cell_data_iterator_is_end(jcntrl_cell_data_iterator *iter)
{
  if (!iter->cursor)
    return 0;
  return iter->cursor == &iter->celldata->head;
}

jcntrl_data_array *
jcntrl_cell_data_iterator_get(jcntrl_cell_data_iterator *iter)
{
  if (!iter->cursor)
    return NULL;

  return iter->cursor->array;
}

void jcntrl_cell_data_iterator_next(jcntrl_cell_data_iterator *iter)
{
  jcntrl_cell_data_iterator_move(iter, geom_list_next);
}

void jcntrl_cell_data_iterator_prev(jcntrl_cell_data_iterator *iter)
{
  jcntrl_cell_data_iterator_move(iter, geom_list_prev);
}

void jcntrl_cell_data_iterator_remove(jcntrl_cell_data_iterator *iter)
{
  jcntrl_cell_data_entry *e = iter->cursor;
  if (!e)
    return;

  jcntrl_cell_data_iterator_next(iter);
  jcntrl_cell_data_entry_delete(e);
}
