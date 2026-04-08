#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "overflow.h"
#include "shared_object_priv.h"
#include "shared_object.h"
#include "defs.h"

#include <jupiter/geometry/rbtree.h>

static struct geom_rbtree *jcntrl_shared_object_tree_root = NULL;

#define jcntrl_shared_object_data_entry(ptr) \
  geom_rbtree_entry(ptr, struct jcntrl_shared_object_data, tree)

static int jcntrl_shared_object_data_comp(struct geom_rbtree *ta,
                                          struct geom_rbtree *tb)
{
  struct jcntrl_shared_object_data *a, *b;
  a = jcntrl_shared_object_data_entry(ta);
  b = jcntrl_shared_object_data_entry(tb);
  return strcmp(a->name, b->name);
}

int jcntrl_shared_object_data_install(jcntrl_shared_object_data *data)
{
  struct geom_rbtree *nroot;

  /* Mandatory functions */
  JCNTRL_ASSERT(data);
  JCNTRL_ASSERT(data->name);
  JCNTRL_ASSERT(data->funcs.downcast);

  if (!jcntrl_shared_object_tree_root) {
    jcntrl_shared_object_tree_root = &data->tree;
    return 1;
  }

  nroot = jcntrl_shared_object_tree_root;
  nroot = geom_rbtree_insert(nroot, &data->tree, jcntrl_shared_object_data_comp,
                             NULL);
  if (!nroot)
    return 0;

  jcntrl_shared_object_tree_root = nroot;
  return 1;
}

#define jcntrl_shared_object__ancestor_metadata_init NULL

static void *jcntrl_shared_object_downcast_impl(jcntrl_shared_object *p)
{
  return p;
}

static void jcntrl_shared_object_init_funcs(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_shared_object_downcast_impl;
}

const jcntrl_shared_object_data *jcntrl_shared_object_metadata_init(void)
{
  const jcntrl_shared_object_data *ret;
  jcntrl_shared_object__data_init(&ret, jcntrl_shared_object,
                                  jcntrl_shared_object_init_funcs);
  return ret;
}

static void jcntrl_shared_object_dummy_init_func(jcntrl_shared_object_funcs *p)
{
}

const jcntrl_shared_object_data *jcntrl_shared_object_data_for(const char *name)
{
  jcntrl_shared_object_data tmp;
  struct geom_rbtree *t;

  if (!jcntrl_shared_object_tree_root)
    return NULL;

  jcntrl_shared_object_data_init__core(&tmp, NULL, 0, name, NULL,
                                       jcntrl_shared_object_dummy_init_func);
  t = geom_rbtree_find(jcntrl_shared_object_tree_root, &tmp.tree,
                       jcntrl_shared_object_data_comp);
  if (!t)
    return NULL;
  return jcntrl_shared_object_data_entry(t);
}

static void *
jcntrl_shared_object_call_init(jcntrl_shared_object *p,
                               const jcntrl_shared_object_data *objp)
{
  if (objp->ancestor)
    if (!jcntrl_shared_object_call_init(p, objp->ancestor))
      return NULL;

  if (objp->funcs.initializer)
    if (!objp->funcs.initializer(p))
      return NULL;
  return p;
}

int jcntrl_shared_object_static_init(jcntrl_shared_object *obj,
                                     const jcntrl_shared_object_data *p)
{
  int r;
  r = !!jcntrl_shared_object_call_init(obj, p);
  obj->metadata = p;
  obj->ref_count = -1;
  return r;
}

void *jcntrl_shared_object_new_by_meta(const jcntrl_shared_object_data *p)
{
  jcntrl_shared_object *obj;

  JCNTRL_ASSERT(p);

  if (!p->funcs.allocator || !p->funcs.deleter) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Cannot create object");
    return NULL;
  }

  obj = p->funcs.allocator();
  if (!obj)
    return NULL;

  if (!jcntrl_shared_object_call_init(obj, p)) {
    p->funcs.deleter(obj);
    return NULL;
  }

  obj->metadata = p;
  obj->ref_count = 1;
  return p->funcs.downcast(obj);
}

void *jcntrl_shared_object_downcast_by_meta(const jcntrl_shared_object_data *p,
                                            jcntrl_shared_object *obj)
{
  const jcntrl_shared_object_data *objp;

  JCNTRL_ASSERT(p);

  if (!obj)
    return NULL;

  objp = obj->metadata;
  while (objp) {
    if (objp == p)
      return objp->funcs.downcast(obj);
    objp = objp->ancestor;
  }
  return NULL;
}

int jcntrl_shared_object_is_a_by_meta(const jcntrl_shared_object_data *p,
                                      jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_data_is_a(p, obj->metadata);
}

jcntrl_shared_object *jcntrl_shared_object_delete(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  JCNTRL_ASSERT(obj->ref_count != 0);

  if (obj->ref_count <= 1) {
    const jcntrl_shared_object_data *objp = obj->metadata;
    const jcntrl_shared_object_data *p;

    p = objp;
    while (p) {
      if (p->funcs.destructor)
        p->funcs.destructor(obj);
      p = p->ancestor;
    }
    if (obj->ref_count == 1) {
      if (objp->funcs.deleter)
        objp->funcs.deleter(obj);
    }
    return NULL;
  }
  obj->ref_count -= 1;
  return obj;
}

jcntrl_shared_object *
jcntrl_shared_object_take_ownership(jcntrl_shared_object *obj)
{
  int n;

  JCNTRL_ASSERT(obj);
  JCNTRL_ASSERT(obj->ref_count >= 0); // not allowed for static initialized

  if (jcntrl_i_add_overflow(obj->ref_count, 1, &n)) {
    return NULL;
  }
  obj->ref_count = n;
  return obj;
}

jcntrl_shared_object *
jcntrl_shared_object_release_ownership(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_delete(obj);
}

const jcntrl_shared_object_data *
jcntrl_shared_object_class(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  JCNTRL_ASSERT(obj->metadata);
  return obj->metadata;
}

const char *jcntrl_shared_object_class_name(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  JCNTRL_ASSERT(obj->metadata);
  return obj->metadata->name;
}

int jcntrl_shared_object_refcount(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  return obj->ref_count;
}

int jcntrl_shared_object_is_shared(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  return jcntrl_shared_object_refcount(obj) > 1;
}

int jcntrl_shared_object_is_static(jcntrl_shared_object *obj)
{
  JCNTRL_ASSERT(obj);
  return jcntrl_shared_object_refcount(obj) < 0;
}

static struct jcntrl_shared_object_virtual_stack
  *jcntrl_shared_object_vstack_root = NULL;

#ifdef _OPENMP
#pragma omp threadprivate(jcntrl_shared_object_vstack_root)
#endif

void jcntrl_shared_object_virtual_stack_push(
  struct jcntrl_shared_object_virtual_stack *entry)
{
  entry->next = jcntrl_shared_object_vstack_root;
  jcntrl_shared_object_vstack_root = entry;
}

void jcntrl_shared_object_virtual_stack_pop(void)
{
  JCNTRL_ASSERT(jcntrl_shared_object_vstack_root);
  jcntrl_shared_object_vstack_root = jcntrl_shared_object_vstack_root->next;
}

/* Returning pointer is thread local only in OpenMP manner */
const struct jcntrl_shared_object_virtual_stack *
jcntrl_shared_object_virtual_stack_top(void)
{
  return jcntrl_shared_object_vstack_root;
}
