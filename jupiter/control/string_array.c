#include "string_array.h"
#include "abstract_array.h"
#include "data_array.h"
#include "error.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"
#include "defs.h"

#include <stdlib.h>

struct jcntrl_string_array
{
  jcntrl_abstract_array abstract;
  jcntrl_size_type narray;
  jcntrl_char_array **array;
};
#define jcntrl_string_array__ancestor jcntrl_abstract_array
#define jcntrl_string_array__dnmem abstract.jcntrl_abstract_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_string_array);

static int jcntrl_string_array_resize_base(jcntrl_string_array *ary,
                                           jcntrl_size_type ntuple)
{
  jcntrl_size_type bytesize;
  jcntrl_char_array **np;

  if (ary->narray == ntuple)
    return 1;

  bytesize = sizeof(jcntrl_char_array *);
  if (jcntrl_s_mul_overflow(bytesize, ntuple, &bytesize))
    return 0;

  if (bytesize < 0) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  if (ary->narray < ntuple) {
    np = realloc(ary->array, bytesize);
    if (!np) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      return 0;
    }

    ary->array = np;
    for (jcntrl_size_type n = ary->narray; n < ntuple; ++n)
      ary->array[n] = NULL;
  } else {
    for (jcntrl_size_type n = ntuple; n < ary->narray; ++n) {
      if (ary->array[n])
        jcntrl_char_array_delete(ary->array[n]);
    }

    np = realloc(ary->array, bytesize);
    if (ntuple > 0 && !np) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      return 0;
    }

    ary->array = np;
  }

  ary->narray = ntuple;
  return 1;
}

static jcntrl_string_array *
jcntrl_string_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_string_array, obj);
}

static void *jcntrl_string_array_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_string_array_downcast_impl(obj);
}

static int jcntrl_string_array_initializer(jcntrl_shared_object *obj)
{
  jcntrl_string_array *ary = jcntrl_string_array_downcast_impl(obj);
  ary->array = NULL;
  ary->narray = 0;
  return 1;
}

static void jcntrl_string_array_desctructor(jcntrl_shared_object *obj)
{
  jcntrl_string_array *ary = jcntrl_string_array_downcast_impl(obj);
  jcntrl_string_array_resize_base(ary, 0);
  JCNTRL_ASSERT(!ary->array);
}

static jcntrl_shared_object *jcntrl_string_array_allocaltor(void)
{
  jcntrl_string_array *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_string_array);
  return p ? jcntrl_string_array_object(p) : NULL;
}

static void jcntrl_string_array_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static jcntrl_size_type
jcntrl_string_array_get_ntuple_impl(jcntrl_shared_object *obj)
{
  jcntrl_string_array *ary = jcntrl_string_array_downcast_impl(obj);
  return ary->narray;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_string_array, jcntrl_abstract_array, get_ntuple)

static int jcntrl_string_array_resize_impl(jcntrl_shared_object *obj,
                                           jcntrl_size_type ntuple)
{
  jcntrl_string_array *ary = jcntrl_string_array_downcast_impl(obj);
  return jcntrl_string_array_resize_base(ary, ntuple);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_string_array, jcntrl_abstract_array, resize)

static void jcntrl_string_array_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_string_array_downcast_v;
  p->initializer = jcntrl_string_array_initializer;
  p->destructor = jcntrl_string_array_desctructor;
  p->allocator = jcntrl_string_array_allocaltor;
  p->deleter = jcntrl_string_array_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_string_array, jcntrl_abstract_array,
                          get_ntuple);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_string_array, jcntrl_abstract_array,
                          resize);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_string_array,
                                   jcntrl_string_array_init_func)

jcntrl_string_array *jcntrl_string_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_string_array);
}

void jcntrl_string_array_delete(jcntrl_string_array *p)
{
  jcntrl_shared_object_delete(jcntrl_string_array_object(p));
}

jcntrl_shared_object *jcntrl_string_array_object(jcntrl_string_array *p)
{
  return jcntrl_abstract_array_object(jcntrl_string_array_abstract(p));
}

jcntrl_abstract_array *jcntrl_string_array_abstract(jcntrl_string_array *p)
{
  return &p->abstract;
}

jcntrl_string_array *jcntrl_string_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_string_array, obj);
}

jcntrl_size_type jcntrl_string_array_get_ntuple(jcntrl_string_array *p)
{
  return jcntrl_abstract_array_get_ntuple(jcntrl_string_array_abstract(p));
}

int jcntrl_string_array_resize(jcntrl_string_array *p, jcntrl_size_type ntuple)
{
  return jcntrl_abstract_array_resize(jcntrl_string_array_abstract(p), ntuple);
}

jcntrl_char_array *jcntrl_string_array_get(jcntrl_string_array *p,
                                           jcntrl_size_type index)
{
  JCNTRL_ASSERT(p);

  if (index < 0 || index >= p->narray) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return NULL;
  }

  return p->array[index];
}

jcntrl_char_array *jcntrl_string_array_get_copy(jcntrl_string_array *p,
                                                jcntrl_size_type index)
{
  jcntrl_size_type n;
  jcntrl_char_array *newp;
  jcntrl_char_array *src;

  src = jcntrl_string_array_get(p, index);
  if (!src)
    return NULL;

  newp = jcntrl_char_array_new();
  if (!newp)
    return NULL;

  n = jcntrl_char_array_get_ntuple(src);
  if (!jcntrl_char_array_resize(newp, n)) {
    jcntrl_char_array_delete(newp);
    return NULL;
  }

  if (!jcntrl_char_array_copy(newp, jcntrl_char_array_data(src), n, 0, 0)) {
    jcntrl_char_array_delete(newp);
    return NULL;
  }
  return newp;
}

const char *jcntrl_string_array_get_cstr(jcntrl_string_array *p,
                                         jcntrl_size_type index,
                                         jcntrl_size_type *len)
{
  jcntrl_char_array *t;

  t = jcntrl_string_array_get(p, index);
  if (!t)
    return NULL;

  if (len)
    *len = jcntrl_char_array_get_ntuple(t);
  return jcntrl_char_array_get(t);
}

int jcntrl_string_array_set(jcntrl_string_array *p, jcntrl_size_type index,
                            jcntrl_char_array *data)
{
  JCNTRL_ASSERT(p);

  if (index < 0 || index >= p->narray) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  if (p->array[index])
    jcntrl_char_array_delete(p->array[index]);
  p->array[index] = NULL;
  if (data)
    p->array[index] = jcntrl_char_array_take_ownership(data);
  return 1;
}

int jcntrl_string_array_set_copy(jcntrl_string_array *p, jcntrl_size_type index,
                                 jcntrl_data_array *data)
{
  jcntrl_size_type n;
  jcntrl_char_array *newp;

  if (!data)
    return jcntrl_string_array_set(p, index, NULL);

  JCNTRL_ASSERT(jcntrl_char_array_copyable(data));

  if (index < 0 || index >= p->narray) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  newp = jcntrl_char_array_new();
  if (!newp)
    return 0;

  n = jcntrl_data_array_get_ntuple(data);
  if (!jcntrl_char_array_resize(newp, n)) {
    jcntrl_char_array_delete(newp);
    return 0;
  }

  if (!jcntrl_char_array_copy(newp, data, n, 0, 0)) {
    jcntrl_char_array_delete(newp);
    return 0;
  }

  if (p->array[index])
    jcntrl_char_array_delete(p->array[index]);
  p->array[index] = newp;
  return 1;
}

int jcntrl_string_array_set_cstr(jcntrl_string_array *p, jcntrl_size_type index,
                                 const char *string, jcntrl_size_type len)
{
  jcntrl_data_array *data;
  jcntrl_static_cstr_array str;
  jcntrl_static_cstr_array_init_base(&str, string, len);

  data = jcntrl_static_cstr_array_data(&str);
  return jcntrl_string_array_set_copy(p, index, data);
}

static int jcntrl_string_array_check_indices(jcntrl_size_type ndest,
                                             jcntrl_size_type nsrc,
                                             jcntrl_size_type *ntuple,
                                             jcntrl_size_type idest,
                                             jcntrl_size_type iskip)
{
  JCNTRL_ASSERT(ntuple);
  JCNTRL_ASSERT(*ntuple >= 0);
  if (idest < 0 || idest >= ndest) {
    jcntrl_raise_index_error(__FILE__, __LINE__, idest);
    return 0;
  }
  if (iskip < 0 || iskip >= nsrc) {
    jcntrl_raise_index_error(__FILE__, __LINE__, iskip);
    return 0;
  }

  if (jcntrl_s_sub_overflow(ndest, idest, &ndest))
    return 1;
  if (jcntrl_s_sub_overflow(nsrc, iskip, &nsrc))
    return 1;

  if (ndest < *ntuple)
    *ntuple = ndest;
  if (nsrc < *ntuple)
    *ntuple = nsrc;
  return 1;
}

int jcntrl_string_array_copy(jcntrl_string_array *dest,
                             jcntrl_string_array *src, jcntrl_size_type ntuple,
                             jcntrl_size_type idest, jcntrl_size_type iskip)
{
  jcntrl_size_type nd, ns;
  nd = jcntrl_string_array_get_ntuple(dest);
  ns = jcntrl_string_array_get_ntuple(src);

  if (!jcntrl_string_array_check_indices(nd, ns, &ntuple, idest, iskip))
    return 0;
  if (ntuple <= 0)
    return 1;

  for (jcntrl_size_type i = 0; i < ntuple; ++i) {
    jcntrl_char_array *c;
    c = jcntrl_string_array_get(src, iskip + i);
    if (!jcntrl_string_array_set(dest, idest + i, c))
      return 0;
  }

  return 1;
}

int jcntrl_string_array_deep_copy(jcntrl_string_array *dest,
                                  jcntrl_string_array *src,
                                  jcntrl_size_type ntuple,
                                  jcntrl_size_type idest,
                                  jcntrl_size_type iskip)
{
  jcntrl_size_type nd, ns;
  nd = jcntrl_string_array_get_ntuple(dest);
  ns = jcntrl_string_array_get_ntuple(src);

  if (!jcntrl_string_array_check_indices(nd, ns, &ntuple, idest, iskip))
    return 0;
  if (ntuple <= 0)
    return 1;

  for (jcntrl_size_type i = 0; i < ntuple; ++i) {
    jcntrl_char_array *c;
    jcntrl_data_array *d;
    c = jcntrl_string_array_get(src, iskip + i);
    d = c ? jcntrl_char_array_data(c) : NULL;
    if (!jcntrl_string_array_set_copy(dest, idest + i, d))
      return 0;
  }

  return 1;
}
