

#include "data_array.h"
#include "defs.h"
#include "field_variable.h"
#include "geometry.h"
#include "grid_data.h"
#include "jupiter/geometry/vector.h"
#include "mask_data.h"
#include "overflow.h"
#include "shared_object.h"
#include "struct_data.h"
#include "information.h"
#include "shared_object_priv.h"
#include "error.h"
#include "executive.h"

#include <jupiter/geometry/error.h>
#include <jupiter/geometry/rbtree.h>
#include <jupiter/serializer/error.h>
#include <jupiter/table/table.h>

#ifdef JCNTRL_USE_MPI
#include <mpi.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct jcntrl_information_data
{
  enum jcntrl_information_datatype type;
  union jcntrl_information_unified
  {
    int index;
    int range[2];
    int index3[3];
    int extent[6];
    double rval;
    geom_vec3 vector;
    jcntrl_shared_object *data;
    const jcntrl_shared_object_data *objecttype;
    const char *const_string;
    char *alloc_string;
  } d;
};

/**
 * @brief Stores information
 */
struct jcntrl_information_entry
{
  struct geom_rbtree tree;
  enum jcntrl_info info;
  struct jcntrl_information_data data;
};
#define jcntrl_information_tree_entry(ptr) \
  geom_rbtree_entry(ptr, struct jcntrl_information_entry, tree)

struct jcntrl_information
{
  struct jcntrl_shared_object object;
  struct geom_rbtree *root;
};
#define jcntrl_information__ancestor jcntrl_shared_object
#define jcntrl_information__dnmem object
JCNTRL_VTABLE_NONE(jcntrl_information);

static jcntrl_information *
jcntrl_information_downcast_impl(jcntrl_shared_object *p)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_information, p);
}

static void *jcntrl_information_downcast_v(jcntrl_shared_object *p)
{
  return jcntrl_information_downcast_impl(p);
}

static int jcntrl_information_init(jcntrl_shared_object *obj)
{
  jcntrl_information *info = jcntrl_information_downcast_impl(obj);
  info->root = NULL;
  return 1;
}

static void jcntrl_information_destructor(jcntrl_shared_object *obj)
{
  jcntrl_information *info = jcntrl_information_downcast_impl(obj);
  jcntrl_information_clear(info);
}

static jcntrl_shared_object *jcntrl_information_allocator(void)
{
  jcntrl_information *info;
  info = jcntrl_shared_object_default_allocator(jcntrl_information);
  return info ? jcntrl_information_object(info) : NULL;
}

static void jcntrl_information_deleter(jcntrl_shared_object *obj)
{
  if (obj->metadata == jcntrl_information_metadata_init()) {
    jcntrl_information *info = jcntrl_information_downcast_impl(obj);
    if (jcntrl_information_is_true(info, JCNTRL_INFO_LOCKED))
      return;
  }
  jcntrl_shared_object_default_deleter(obj);
}

static void jcntrl_information_funcs(jcntrl_shared_object_funcs *p)
{
  p->allocator = jcntrl_information_allocator;
  p->deleter = jcntrl_information_deleter;
  p->initializer = jcntrl_information_init;
  p->destructor = jcntrl_information_destructor;
  p->downcast = jcntrl_information_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_information, jcntrl_information_funcs)

static void jcntrl_information_data_init(struct jcntrl_information_data *data)
{
  memset(&data->d, 0, sizeof(union jcntrl_information_unified));
  data->type = JCNTRL_IDATATYPE_INVALID;
}

static void
jcntrl_information_entry_init(struct jcntrl_information_entry *entry)
{
  geom_rbtree_init(&entry->tree);
  jcntrl_information_data_init(&entry->data);
  entry->info = 0;
}

static struct jcntrl_information_entry *jcntrl_information_entry_new(void)
{
  struct jcntrl_information_entry *e;

  e = (struct jcntrl_information_entry *)malloc(
    sizeof(struct jcntrl_information_entry));
  if (!e) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }

  jcntrl_information_entry_init(e);
  return e;
}

static void jcntrl_information_clean_data(struct jcntrl_information_data *data)
{
  if (data->type == JCNTRL_IDATATYPE_STRING) {
    free(data->d.alloc_string);
  } else if (data->type == JCNTRL_IDATATYPE_OBJECT) {
    jcntrl_shared_object_delete(data->d.data);
  }
}

static void
jcntrl_information_clean_entry(struct jcntrl_information_entry *entry)
{
  jcntrl_information_clean_data(&entry->data);
}

static void
jcntrl_information_entry_delete(struct jcntrl_information_entry *entry)
{
  jcntrl_information_clean_entry(entry);
  free(entry);
}

static void jcntrl_information_remove_all_entries(jcntrl_information *info)
{
  while (info->root) {
    struct geom_rbtree *new_root, *del;
    struct jcntrl_information_entry *e;

    del = info->root;
    new_root = geom_rbtree_delete(del, del, NULL);
    info->root = new_root;

    e = jcntrl_information_tree_entry(del);
    jcntrl_information_entry_delete(e);
  }
}

static void jcntrl_information_delete_object(void *obj)
{
  jcntrl_information *info;
  info = (jcntrl_information *)obj;

  jcntrl_information_remove_all_entries(info);
  free(info);
}

static int jcntrl_information_entry_comp(struct geom_rbtree *a,
                                         struct geom_rbtree *b)
{
  struct jcntrl_information_entry *ea, *eb;
  ea = jcntrl_information_tree_entry(a);
  eb = jcntrl_information_tree_entry(b);
  return (int)eb->info - (int)ea->info;
}

jcntrl_information *jcntrl_information_new(void)
{
  return jcntrl_shared_object_new(jcntrl_information);
}

jcntrl_information *jcntrl_information_unlink(jcntrl_information *info)
{
  JCNTRL_ASSERT(info);

  if (jcntrl_shared_object_delete(jcntrl_information_object(info))) {
    return info;
  }
  return NULL;
}

jcntrl_information_datatype
jcntrl_information_get_required_type(jcntrl_info key)
{
  switch (key) {
  case JCNTRL_INFO_REQUEST_UPDATE_INFORMATION:
  case JCNTRL_INFO_REQUEST_UPDATE_DATA:
  case JCNTRL_INFO_REQUEST_UPDATE_EXTENT:
  case JCNTRL_INFO_REQUEST_CHECK_LOOP:
    return JCNTRL_IDATATYPE_REQUEST;
  case JCNTRL_INFO_REQUIREMENTS_FILLED:
    return JCNTRL_IDATATYPE_BOOL;
  case JCNTRL_INFO_DATA_OBJECT:
    return JCNTRL_IDATATYPE_OBJECT;
  case JCNTRL_INFO_DATATYPE:
    return JCNTRL_IDATATYPE_OBJECTTYPE;
  case JCNTRL_INFO_REQUIRED_DATATYPE:
    return JCNTRL_IDATATYPE_DATATYPE;
  case JCNTRL_INFO_DATA_EXTENT:
  case JCNTRL_INFO_WHOLE_EXTENT:
  case JCNTRL_INFO_PIECE_EXTENT:
    return JCNTRL_IDATATYPE_EXTENT;
  case JCNTRL_INFO_UPDATE_TIME:
    return JCNTRL_IDATATYPE_FLOAT;
  case JCNTRL_INFO_LOOP_MARKER:
  case JCNTRL_INFO_DATA_UP_TO_DATE:
    return JCNTRL_IDATATYPE_BOOL;
  case JCNTRL_INFO_LOCKED:
    return JCNTRL_IDATATYPE_BOOL;

  case JCNTRL_INFO_ERROR_NUMBER:
  case JCNTRL_INFO_ERROR_ERRNO:
  case JCNTRL_INFO_ERROR_GEOMETRY:
  case JCNTRL_INFO_ERROR_MPI:
  case JCNTRL_INFO_ERROR_TABLE:
  case JCNTRL_INFO_ERROR_SERIALIZER:
    return JCNTRL_IDATATYPE_INTEGER;
  case JCNTRL_INFO_ERROR_SOURCE_FILE:
    return JCNTRL_IDATATYPE_CSTRING;
  case JCNTRL_INFO_ERROR_SOURCE_LINE:
    return JCNTRL_IDATATYPE_INTEGER;
  case JCNTRL_INFO_ERROR_MESSAGE:
    return JCNTRL_IDATATYPE_CSTRING;
  case JCNTRL_INFO_ERROR_EXEC_UP:
  case JCNTRL_INFO_ERROR_EXEC_DOWN:
    return JCNTRL_IDATATYPE_CSTRING;
  case JCNTRL_INFO_ERROR_TYPE_REQUIRED:
  case JCNTRL_INFO_ERROR_TYPE_GIVEN:
    return JCNTRL_IDATATYPE_DATATYPE;
  case JCNTRL_INFO_ERROR_OBJECT_GIVEN:
    return JCNTRL_IDATATYPE_OBJECTTYPE;
  case JCNTRL_INFO_ERROR_INFO_KEY:
  case JCNTRL_INFO_ERROR_INFO_TYPE:
    return JCNTRL_IDATATYPE_INTEGER;
  case JCNTRL_INFO_ERROR_ELTYPE_EXPECT:
  case JCNTRL_INFO_ERROR_ELTYPE_REQUEST:
    return JCNTRL_IDATATYPE_INTEGER;
  case JCNTRL_INFO_ERROR_INDEX:
    return JCNTRL_IDATATYPE_INTEGER;
  case JCNTRL_INFO_ERROR_VIRTUAL_BASE:
  case JCNTRL_INFO_ERROR_CLASS_CALLING:
    return JCNTRL_IDATATYPE_OBJECTTYPE;
  case JCNTRL_INFO_ERROR_FUNCNAME:
    return JCNTRL_IDATATYPE_CSTRING;
  case JCNTRL_INFO_ERROR_CSV_HEADER:
    return JCNTRL_IDATATYPE_OBJECT;
  }

  JCNTRL_UNREACHABLE();
  return JCNTRL_IDATATYPE_INVALID;
}

jcntrl_shared_object *jcntrl_information_object(jcntrl_information *info)
{
  return &info->object;
}

jcntrl_information *jcntrl_information_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_information, object);
}

int jcntrl_information_has(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_entry e;

  JCNTRL_ASSERT(info);

  if (!info->root)
    return 0;

  e.info = key;
  if (geom_rbtree_find(info->root, &e.tree, jcntrl_information_entry_comp)) {
    return 1;
  } else {
    return 0;
  }
}

static struct jcntrl_information_entry *
jcntrl_information_find_entry(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_entry e;
  struct geom_rbtree *found;

  JCNTRL_ASSERT(info);

  if (!info->root)
    return NULL;

  e.info = key;
  found = geom_rbtree_find(info->root, &e.tree, jcntrl_information_entry_comp);
  if (found) {
    return jcntrl_information_tree_entry(found);
  }
  return NULL;
}

int jcntrl_information_remove(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_entry *e;

  JCNTRL_ASSERT(info);

  if (!info->root) {
    return 1;
  }

  if (jcntrl_information_is_true(info, JCNTRL_INFO_LOCKED)) {
    return 0;
  }

  e = jcntrl_information_find_entry(info, key);
  if (!e) {
    return 1;
  }

  info->root = geom_rbtree_delete(info->root, &e->tree, NULL);
  jcntrl_information_entry_delete(e);

  return 1;
}

int jcntrl_information_clear(jcntrl_information *info)
{
  JCNTRL_ASSERT(info);

  if (!info->root) {
    return 1;
  }

  if (jcntrl_information_is_true(info, JCNTRL_INFO_LOCKED)) {
    return 0;
  }

  jcntrl_information_remove_all_entries(info);
  return 1;
}

typedef int setter_function(union jcntrl_information_unified *p, void *arg);
typedef int getter_function(union jcntrl_information_unified *p, void *arg);

static struct jcntrl_information_entry *
jcntrl_information_add_entry(jcntrl_information *info,
                             struct jcntrl_information_entry *entry)
{
  if (!info->root) {
    info->root = &entry->tree;
    return entry;
  } else {
    struct geom_rbtree *new_root;
    new_root = geom_rbtree_insert(info->root, &entry->tree,
                                  jcntrl_information_entry_comp, NULL);
    if (new_root) {
      info->root = new_root;
      return entry;
    } else {
      return NULL;
    }
  }
  JCNTRL_UNREACHABLE();
}

static int jcntrl_information_data_set(struct jcntrl_information_data *data,
                                       jcntrl_info key,
                                       jcntrl_information_datatype type,
                                       setter_function *setter, void *arg)
{
  union jcntrl_information_unified tmpdata;
  int ret;
  JCNTRL_ASSERT(type == jcntrl_information_get_required_type(key));
  if (type != jcntrl_information_get_required_type(key)) {
    jcntrl_raise_information_type_error(__FILE__, __LINE__, key, type);
    return 1;
  }
  ret = setter(&tmpdata, arg);
  if (ret) {
    jcntrl_information_clean_data(data);
    data->type = type;
    data->d = tmpdata;
  }
  return ret;
}

static int jcntrl_information_entry_set(struct jcntrl_information_entry *entry,
                                        jcntrl_info key,
                                        jcntrl_information_datatype type,
                                        setter_function *setter, void *arg)
{
  entry->info = key;
  return jcntrl_information_data_set(&entry->data, key, type, setter, arg);
}

static int jcntrl_information_set(jcntrl_information *info, jcntrl_info key,
                                  jcntrl_information_datatype type, int force,
                                  setter_function *setter, void *arg)
{
  struct jcntrl_information_entry *entry;

  JCNTRL_ASSERT(info);
  JCNTRL_ASSERT(setter);

  if (!force) {
    if (jcntrl_information_is_true(info, JCNTRL_INFO_LOCKED)) {
      jcntrl_raise_information_locked_error(__FILE__, __LINE__);
      return 0;
    }
  }

  entry = jcntrl_information_find_entry(info, key);
  if (entry) {
    return jcntrl_information_entry_set(entry, key, type, setter, arg);
  } else {
    entry = jcntrl_information_entry_new();
    if (entry) {
      int retval;
      retval = jcntrl_information_entry_set(entry, key, type, setter, arg);
      if (!retval) {
        jcntrl_information_entry_delete(entry);
        return retval;
      }
      jcntrl_information_add_entry(info, entry);
      return retval;
    } else {
      return 0;
    }
  }
  JCNTRL_UNREACHABLE();
}

static int
jcntrl_information_data_get(struct jcntrl_information_data *data,
                            jcntrl_info key,
                            jcntrl_information_datatype requested_type,
                            getter_function *getter, void *arg)
{
  JCNTRL_ASSERT(requested_type == jcntrl_information_get_required_type(key));
  if (!data) {
    return getter(NULL, arg);
  }
  JCNTRL_ASSERT(data->type == requested_type);
  return getter(&data->d, arg);
}

static int
jcntrl_information_entry_get(struct jcntrl_information_entry *entry,
                             jcntrl_info key,
                             jcntrl_information_datatype requested_type,
                             getter_function *getter, void *arg)
{
  if (!entry) {
    return jcntrl_information_data_get(NULL, key, requested_type, getter, arg);
  }
  return jcntrl_information_data_get(&entry->data, key, requested_type, getter,
                                     arg);
}

static int jcntrl_information_get(jcntrl_information *info, jcntrl_info key,
                                  jcntrl_information_datatype requested_type,
                                  getter_function *getter, void *arg)
{
  struct jcntrl_information_entry *entry;

  entry = jcntrl_information_find_entry(info, key);
  return jcntrl_information_entry_get(entry, key, requested_type, getter, arg);
}

static int
jcntrl_information_request_setter(union jcntrl_information_unified *p,
                                  void *arg)
{
  return 1;
}

int jcntrl_information_set_request(jcntrl_information *info, jcntrl_info key)
{
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_REQUEST, 0,
                                jcntrl_information_request_setter, NULL);
}

struct jcntrl_information_bool_arg
{
  int value;
  int notfound;
};

static int jcntrl_information_bool_setter(union jcntrl_information_unified *p,
                                          void *arg)
{
  struct jcntrl_information_bool_arg *a;
  a = (struct jcntrl_information_bool_arg *)arg;
  p->index = !!a->value;
  return 1;
}

static int jcntrl_information_bool_getter(union jcntrl_information_unified *p,
                                          void *arg)
{
  struct jcntrl_information_bool_arg *a;
  a = (struct jcntrl_information_bool_arg *)arg;
  if (p) {
    a->notfound = 0;
    a->value = p->index;
  } else {
    a->notfound = 1;
    a->value = 0;
  }
  return 1;
}

int jcntrl_information_is_true(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_bool_arg a;
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_BOOL,
                         jcntrl_information_bool_getter, &a);
  return a.value;
}

int jcntrl_information_is_false(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_bool_arg a;
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_BOOL,
                         jcntrl_information_bool_getter, &a);
  return !a.notfound && !a.value;
}

int jcntrl_information_is_falsey(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_bool_arg a;
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_BOOL,
                         jcntrl_information_bool_getter, &a);
  return !a.value;
}

int jcntrl_information_set_bool(jcntrl_information *info, jcntrl_info key,
                                int value)
{
  struct jcntrl_information_bool_arg a;
  a.value = value;
  a.notfound = 0;
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_BOOL, 0,
                                jcntrl_information_bool_setter, &a);
}

struct jcntrl_information_integer_arg
{
  int value;
  int (*func_notfound)(void *data);
  void *func_arg;
};

static int
jcntrl_information_integer_getter(union jcntrl_information_unified *d,
                                  void *arg)
{
  struct jcntrl_information_integer_arg *a;
  a = (struct jcntrl_information_integer_arg *)arg;
  if (d) {
    a->value = d->index;
    return 1;
  }
  a->value = a->func_notfound(a->func_arg);
  return 1;
}

static int
jcntrl_information_integer_setter(union jcntrl_information_unified *d,
                                  void *arg)
{
  struct jcntrl_information_integer_arg *a;
  a = (struct jcntrl_information_integer_arg *)arg;
  d->index = a->value;
  return 1;
}

static int default_integer_notfound_d(void *data) { return *(int *)data; }
static int default_integer_notfound_0(void *data) { return 0; }

int jcntrl_information_get_integer(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_integer_arg a;
  a.func_notfound = default_integer_notfound_0;
  a.func_arg = NULL;
  a.value = 0;
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_INTEGER,
                         jcntrl_information_integer_getter, &a);
  return a.value;
}

int jcntrl_information_get_integer_nf(jcntrl_information *info, jcntrl_info key,
                                      int (*func_if_notfound)(void *),
                                      void *arg, int value_notfound)
{
  struct jcntrl_information_integer_arg a;
  if (func_if_notfound) {
    a.func_notfound = func_if_notfound;
    a.func_arg = arg;
  } else {
    a.func_notfound = default_integer_notfound_d;
    a.func_arg = &value_notfound;
  }
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_INTEGER,
                         jcntrl_information_integer_getter, &a);
  return a.value;
}

int jcntrl_information_set_integer(jcntrl_information *info, jcntrl_info key,
                                   int value)
{
  struct jcntrl_information_integer_arg a;
  a.value = value;
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_INTEGER, 0,
                                jcntrl_information_integer_setter, &a);
}

struct jcntrl_information_datatype_arg
{
  jcntrl_datatype value;
};

static int
jcntrl_information_datatype_getter(union jcntrl_information_unified *d,
                                   void *arg)
{
  struct jcntrl_information_datatype_arg *a;
  a = (struct jcntrl_information_datatype_arg *)arg;
  if (d) {
    a->value = d->index;
    return 1;
  }
  a->value = JCNTRL_DATATYPE_INVALID;
  return 1;
}

static int
jcntrl_information_datatype_setter(union jcntrl_information_unified *d,
                                   void *arg)
{
  struct jcntrl_information_datatype_arg *a;
  a = (struct jcntrl_information_datatype_arg *)arg;
  d->index = a->value;
  return 1;
}

jcntrl_datatype jcntrl_information_get_datatype(jcntrl_information *info,
                                                jcntrl_info key)
{
  struct jcntrl_information_datatype_arg a;
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_DATATYPE,
                         jcntrl_information_datatype_getter, &a);
  return a.value;
}

int jcntrl_information_set_datatype(jcntrl_information *info, jcntrl_info key,
                                    jcntrl_datatype value)
{
  struct jcntrl_information_datatype_arg a;

  JCNTRL_ASSERT(value != JCNTRL_DATATYPE_INVALID);
  if (value == JCNTRL_DATATYPE_INVALID) {
    jcntrl_raise_argument_error(
      __FILE__, __LINE__,
      "You cannot set INVALID datatype to jcntrl_information");
    return 0;
  }

  a.value = value;
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_DATATYPE, 0,
                                jcntrl_information_datatype_setter, &a);
}

struct jcntrl_information_objecttype_arg
{
  const jcntrl_shared_object_data *value;
};

static int
jcntrl_information_objecttype_getter(union jcntrl_information_unified *d,
                                     void *arg)
{
  struct jcntrl_information_objecttype_arg *p;
  p = (struct jcntrl_information_objecttype_arg *)arg;
  if (d)
    p->value = d->objecttype;
  return 1;
}

static int
jcntrl_information_objecttype_setter(union jcntrl_information_unified *d,
                                     void *arg)
{
  struct jcntrl_information_objecttype_arg *p;
  p = (struct jcntrl_information_objecttype_arg *)arg;
  d->objecttype = p->value;
  return 1;
}

const jcntrl_shared_object_data *
jcntrl_information_get_objecttype(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_objecttype_arg p = {NULL};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_OBJECTTYPE,
                         jcntrl_information_objecttype_getter, &p);
  return p.value;
}

#undef jcntrl_information_set_objecttype
int jcntrl_information_set_objecttype(jcntrl_information *info, jcntrl_info key,
                                      const jcntrl_shared_object_data *value)
{
  struct jcntrl_information_objecttype_arg p = {value};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_OBJECTTYPE, 0,
                                jcntrl_information_objecttype_setter, &p);
}

struct jcntrl_information_intptr_arg
{
  const int *iptr;
};

static int jcntrl_information_clen_ivec_setter(int *dest, void *arg,
                                               jcntrl_size_type len)
{
  struct jcntrl_information_intptr_arg *p;
  p = (struct jcntrl_information_intptr_arg *)arg;
  memcpy(dest, p->iptr, len * sizeof(int));
  return 1;
}

static int jcntrl_information_clen_ivec_getter(const int *src, void *arg)
{
  struct jcntrl_information_intptr_arg *p;
  p = (struct jcntrl_information_intptr_arg *)arg;
  p->iptr = src;
  return 1;
}

static int jcntrl_information_range_setter(union jcntrl_information_unified *d,
                                           void *arg)
{
  return jcntrl_information_clen_ivec_setter(d->range, arg, 2);
}

static int jcntrl_information_range_getter(union jcntrl_information_unified *d,
                                           void *arg)
{
  if (d)
    return jcntrl_information_clen_ivec_getter(d->range, arg);
  return 1;
}

const int *jcntrl_information_get_range(jcntrl_information *info,
                                        jcntrl_info key)
{
  struct jcntrl_information_intptr_arg a = {NULL};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_INDEX_RANGE,
                         jcntrl_information_range_getter, &a);
  return a.iptr;
}

int jcntrl_information_set_range(jcntrl_information *info, jcntrl_info key,
                                 const int range[2])
{
  struct jcntrl_information_intptr_arg a = {range};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_INDEX_RANGE, 0,
                                jcntrl_information_range_setter, &a);
}

int jcntrl_information_set_range2(jcntrl_information *info, jcntrl_info key,
                                  int begin, int end)
{
  int range[2] = {begin, end};
  return jcntrl_information_set_range(info, key, range);
}

static int jcntrl_information_extent_setter(union jcntrl_information_unified *d,
                                            void *arg)
{
  return jcntrl_information_clen_ivec_setter(d->extent, arg, 6);
}

static int jcntrl_information_extent_getter(union jcntrl_information_unified *d,
                                            void *arg)
{
  if (d)
    return jcntrl_information_clen_ivec_getter(d->extent, arg);
  return 1;
}

const int *jcntrl_information_get_extent(jcntrl_information *info,
                                         jcntrl_info key)
{
  struct jcntrl_information_intptr_arg a = {NULL};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_EXTENT,
                         jcntrl_information_extent_getter, &a);
  return a.iptr;
}

int jcntrl_information_set_extent(jcntrl_information *info, jcntrl_info key,
                                  const int extent[6])
{
  struct jcntrl_information_intptr_arg a = {extent};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_EXTENT, 0,
                                jcntrl_information_extent_setter, &a);
}

int jcntrl_information_set_extent6(jcntrl_information *info, jcntrl_info key,
                                   int x1, int x2, int x3, int x4, int x5,
                                   int x6)
{
  int extent[6] = {x1, x2, x3, x4, x5, x6};
  return jcntrl_information_set_extent(info, key, extent);
}

static int jcntrl_information_index_setter(union jcntrl_information_unified *d,
                                           void *arg)
{
  return jcntrl_information_clen_ivec_setter(d->index3, arg, 3);
}

static int jcntrl_information_index_getter(union jcntrl_information_unified *d,
                                           void *arg)
{
  if (d)
    return jcntrl_information_clen_ivec_getter(d->index3, arg);
  return 1;
}

const int *jcntrl_information_get_index(jcntrl_information *info,
                                        jcntrl_info key)
{
  struct jcntrl_information_intptr_arg a = {NULL};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_INDEX,
                         jcntrl_information_index_getter, &a);
  return a.iptr;
}

int jcntrl_information_set_index(jcntrl_information *info, jcntrl_info key,
                                 const int index[3])
{
  struct jcntrl_information_intptr_arg a = {index};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_INDEX, 0,
                                jcntrl_information_index_setter, &a);
}

int jcntrl_information_set_index3(jcntrl_information *info, jcntrl_info key,
                                  int x1, int x2, int x3)
{
  int index[3] = {x1, x2, x3};
  return jcntrl_information_set_index(info, key, index);
}

struct jcntrl_information_float_arg
{
  double value;
};

static int jcntrl_information_float_setter(union jcntrl_information_unified *d,
                                           void *arg)
{
  struct jcntrl_information_float_arg *p;
  p = (struct jcntrl_information_float_arg *)arg;
  d->rval = p->value;
  return 1;
}

static int jcntrl_information_float_getter(union jcntrl_information_unified *d,
                                           void *arg)
{
  struct jcntrl_information_float_arg *p;
  p = (struct jcntrl_information_float_arg *)arg;
  if (d)
    p->value = d->rval;
  return 1;
}

double jcntrl_information_get_float(jcntrl_information *info, jcntrl_info key)
{
  struct jcntrl_information_float_arg p = {0.0};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_FLOAT,
                         jcntrl_information_float_getter, &p);
  return p.value;
}

int jcntrl_information_set_float(jcntrl_information *info, jcntrl_info key,
                                 double value)
{
  struct jcntrl_information_float_arg p = {value};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_FLOAT, 0,
                                jcntrl_information_float_setter, &p);
}

struct jcntrl_information_vector_arg
{
  geom_vec3 value;
};

static int jcntrl_information_vector_setter(union jcntrl_information_unified *d,
                                            void *arg)
{
  struct jcntrl_information_vector_arg *p;
  p = (struct jcntrl_information_vector_arg *)arg;
  d->vector = p->value;
  return 1;
}

static int jcntrl_information_vector_getter(union jcntrl_information_unified *d,
                                            void *arg)
{
  struct jcntrl_information_vector_arg *p;
  p = (struct jcntrl_information_vector_arg *)arg;
  if (d)
    p->value = d->vector;
  return 1;
}

geom_vec3 jcntrl_information_get_vector(jcntrl_information *info,
                                        jcntrl_info key)
{
  struct jcntrl_information_vector_arg p = {geom_vec3_c(0.0, 0.0, 0.0)};
  jcntrl_information_get(info, key, JCNTRL_IDATATYPE_VECTOR,
                         jcntrl_information_vector_getter, &p);
  return p.value;
}

int jcntrl_information_set_vector(jcntrl_information *info, jcntrl_info key,
                                  geom_vec3 vec)
{
  struct jcntrl_information_vector_arg p = {vec};
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_VECTOR, 0,
                                jcntrl_information_vector_setter, &p);
}

struct jcntrl_information_shared_object_arg
{
  jcntrl_shared_object *value;
};

static int
jcntrl_information_shared_object_getter(union jcntrl_information_unified *d,
                                        void *arg)
{
  struct jcntrl_information_shared_object_arg *a;
  a = (struct jcntrl_information_shared_object_arg *)arg;
  if (d) {
    a->value = d->data;
  } else {
    a->value = NULL;
  }
  return 1;
}

static int
jcntrl_information_shared_object_setter(union jcntrl_information_unified *d,
                                        void *arg)
{
  struct jcntrl_information_shared_object_arg *a;
  a = (struct jcntrl_information_shared_object_arg *)arg;
  d->data = a->value;
  return 1;
}

jcntrl_shared_object *jcntrl_information_get_object(jcntrl_information *info,
                                                    jcntrl_info key)
{
  struct jcntrl_information_shared_object_arg a;
  if (!jcntrl_information_get(info, key, JCNTRL_IDATATYPE_OBJECT,
                              jcntrl_information_shared_object_getter, &a)) {
    return NULL;
  }
  return a.value;
}

int jcntrl_information_set_object(jcntrl_information *info, jcntrl_info key,
                                  jcntrl_shared_object *object)
{
  struct jcntrl_information_shared_object_arg a;
  JCNTRL_ASSERT(object);
  a.value = object;
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_OBJECT, 0,
                                jcntrl_information_shared_object_setter, &a);
}

struct jcntrl_information_const_string_arg
{
  const char *text;
};

static int
jcntrl_information_const_string_getter(union jcntrl_information_unified *d,
                                       void *arg)
{
  struct jcntrl_information_const_string_arg *a;
  a = (struct jcntrl_information_const_string_arg *)arg;
  if (d) {
    a->text = d->const_string;
  } else {
    a->text = NULL;
  }
  return 1;
}

static int jcntrl_information_string_getter(union jcntrl_information_unified *d,
                                            void *arg)
{
  struct jcntrl_information_const_string_arg *a;
  a = (struct jcntrl_information_const_string_arg *)arg;
  if (d) {
    a->text = d->alloc_string;
  } else {
    a->text = NULL;
  }
  return 1;
}

static int
jcntrl_information_const_string_setter(union jcntrl_information_unified *d,
                                       void *arg)
{
  struct jcntrl_information_const_string_arg *a;
  a = (struct jcntrl_information_const_string_arg *)arg;
  d->const_string = a->text;
  return 1;
}

const char *jcntrl_information_get_string(jcntrl_information *info,
                                          jcntrl_info key)
{
  struct jcntrl_information_const_string_arg arg;
  arg.text = NULL;
  if (jcntrl_information_get_required_type(key) == JCNTRL_IDATATYPE_STRING) {
    jcntrl_information_get(info, key, JCNTRL_IDATATYPE_STRING,
                           jcntrl_information_string_getter, &arg);
  } else {
    jcntrl_information_get(info, key, JCNTRL_IDATATYPE_CSTRING,
                           jcntrl_information_const_string_getter, &arg);
  }
  return arg.text;
}

int jcntrl_information_set_const_string(jcntrl_information *info,
                                        jcntrl_info key, const char *str)
{
  struct jcntrl_information_const_string_arg arg;
  arg.text = str;
  return jcntrl_information_set(info, key, JCNTRL_IDATATYPE_CSTRING, 0,
                                jcntrl_information_const_string_setter, &arg);
}

struct jcntrl_information_string_arg
{
  char *astr;
};

static int jcntrl_information_string_setter(union jcntrl_information_unified *d,
                                            void *arg)
{
  struct jcntrl_information_string_arg *a;
  a = (struct jcntrl_information_string_arg *)arg;
  d->alloc_string = a->astr;
  return 1;
}

int jcntrl_information_set_string(jcntrl_information *info, jcntrl_info key,
                                  const char *str)
{
  int r;
  struct jcntrl_information_string_arg arg;
  jcntrl_size_type l;

  l = strlen(str);
  if (l < 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "String length overflow");
    return 0;
  }

  if (jcntrl_s_add_overflow(l, 1, &l))
    return 0;

  arg.astr = (char *)malloc(l * sizeof(char));
  if (!arg.astr) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return 0;
  }

  strcpy(arg.astr, str);

  r = jcntrl_information_set(info, key, JCNTRL_IDATATYPE_STRING, 0,
                             jcntrl_information_string_setter, &arg);
  if (!r)
    free(arg.astr);
  return r;
}

static int
jcntrl_information_entry_set_request(struct jcntrl_information_entry *entry,
                                     jcntrl_info key)
{
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_REQUEST,
                                      jcntrl_information_request_setter, NULL);
}

static int
jcntrl_information_entry_set_bool(struct jcntrl_information_entry *entry,
                                  jcntrl_info key, int value)
{
  struct jcntrl_information_bool_arg arg;
  arg.value = !!value;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_BOOL,
                                      jcntrl_information_bool_setter, &arg);
}

static int
jcntrl_information_entry_set_cstr(struct jcntrl_information_entry *entry,
                                  jcntrl_info key, const char *str)
{
  struct jcntrl_information_const_string_arg arg;
  arg.text = str;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_CSTRING,
                                      jcntrl_information_const_string_setter,
                                      &arg);
}

static int
jcntrl_information_entry_set_int(struct jcntrl_information_entry *entry,
                                 jcntrl_info key, int value)
{
  struct jcntrl_information_integer_arg arg;
  arg.value = value;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_INTEGER,
                                      jcntrl_information_integer_setter, &arg);
}

static int
jcntrl_information_entry_set_datatype(struct jcntrl_information_entry *entry,
                                      jcntrl_info key, jcntrl_datatype value)
{
  struct jcntrl_information_datatype_arg arg;
  arg.value = value;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_DATATYPE,
                                      jcntrl_information_datatype_setter, &arg);
}

static int
jcntrl_information_entry_set_objecttype(struct jcntrl_information_entry *entry,
                                        jcntrl_info key,
                                        const jcntrl_shared_object_data *value)
{
  struct jcntrl_information_objecttype_arg arg;
  arg.value = value;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_OBJECTTYPE,
                                      jcntrl_information_objecttype_setter,
                                      &arg);
}

static int
jcntrl_information_entry_set_object(struct jcntrl_information_entry *entry,
                                    jcntrl_info key,
                                    jcntrl_shared_object *value)
{
  struct jcntrl_information_shared_object_arg arg;
  arg.value = value;
  return jcntrl_information_entry_set(entry, key, JCNTRL_IDATATYPE_OBJECT,
                                      jcntrl_information_shared_object_setter,
                                      &arg);
}

/*
 * error information generator function (these functions are declared
 * in error.h).
 */

static jcntrl_error_callback *jcntrl_current_error_callback = NULL;
static void *jcntrl_current_error_callback_data = NULL;

jcntrl_error_callback *jcntrl_error_callback_set(jcntrl_error_callback *func,
                                                 void *data)
{
  jcntrl_error_callback *old;
  old = jcntrl_current_error_callback;
  jcntrl_current_error_callback = func;
  jcntrl_current_error_callback_data = data;
  return old;
}

jcntrl_error_callback *jcntrl_error_callback_get(void)
{
  return jcntrl_current_error_callback;
}

int jcntrl_error_run_callback(jcntrl_information *info)
{
  if (jcntrl_current_error_callback) {
    return jcntrl_current_error_callback(jcntrl_current_error_callback_data,
                                         info);
  }
  return 0;
}

static void
jcntrl_information_init_static(jcntrl_information *info,
                               struct jcntrl_information_entry *lock)
{
  jcntrl_shared_object_static_init(jcntrl_information_object(info),
                                   jcntrl_information_metadata_init());
  jcntrl_information_init(jcntrl_information_object(info));
  jcntrl_information_entry_init(lock);
  jcntrl_information_entry_set_bool(lock, JCNTRL_INFO_LOCKED, 1);
  jcntrl_information_add_entry(info, lock);
}

int jcntrl_raise_allocation_failed(const char *file, int line)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line, lock;
  struct jcntrl_information_entry message;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_ALLOCATE);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Allocation Failed");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_information_locked_error(const char *file, int line)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line, lock;
  struct jcntrl_information_entry message;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_INFORMATION_LOCKED);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Attempted to modify a locked information");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_overflow_error(const char *file, int line)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, efile, eline, message, lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&efile);
  jcntrl_information_entry_init(&eline);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_OVERFLOW);
  jcntrl_information_entry_set_cstr(&efile, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&eline, JCNTRL_INFO_ERROR_SOURCE_LINE, line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Arithmetic overflow detected");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &efile);
  jcntrl_information_add_entry(&info, &eline);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_argument_error(const char *file, int line, const char *message)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line;
  struct jcntrl_information_entry emsg, lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&emsg);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_ARGUMENT);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&emsg, JCNTRL_INFO_ERROR_MESSAGE, message);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &emsg);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_index_error(const char *file, int line, jcntrl_size_type index)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line;
  struct jcntrl_information_entry message, eindex, lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_init(&eindex);
  jcntrl_information_entry_init(&lock);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_INDEX);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Index out-of-range");
  jcntrl_information_entry_set_int(&eindex, JCNTRL_INFO_ERROR_INDEX, index);
  jcntrl_information_entry_set_bool(&lock, JCNTRL_INFO_LOCKED, 1);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  jcntrl_information_add_entry(&info, &eindex);
  jcntrl_information_add_entry(&info, &lock);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_loop_detected_error(const char *file, int line,
                                     const char *upstream_executive_name,
                                     const char *downstream_executive_name)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line;
  struct jcntrl_information_entry message, up_name, down_name, lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_init(&up_name);
  jcntrl_information_entry_init(&down_name);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_LOOP_DETECTED);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Connection loop detected");
  jcntrl_information_entry_set_cstr(&up_name, JCNTRL_INFO_ERROR_EXEC_UP,
                                    upstream_executive_name);
  jcntrl_information_entry_set_cstr(&down_name, JCNTRL_INFO_ERROR_EXEC_DOWN,
                                    downstream_executive_name);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  jcntrl_information_add_entry(&info, &up_name);
  jcntrl_information_add_entry(&info, &down_name);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_upstream_type_error(
  const char *file, int line, const char *upstream_executive_name,
  const char *downstream_executive_name, jcntrl_datatype required,
  jcntrl_datatype provided,
  const jcntrl_shared_object_data *object_type_provided)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line;
  struct jcntrl_information_entry message, up_name, down_name;
  struct jcntrl_information_entry prov_entry, req_entry, prov_o_entry;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_init(&up_name);
  jcntrl_information_entry_init(&down_name);
  jcntrl_information_entry_init(&prov_entry);
  jcntrl_information_entry_init(&prov_o_entry);
  jcntrl_information_entry_init(&req_entry);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_DATATYPE_ERROR);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Provided data type from upstream "
                                    "executive does not fit the input "
                                    "requirement");
  jcntrl_information_entry_set_cstr(&up_name, JCNTRL_INFO_ERROR_EXEC_UP,
                                    upstream_executive_name);
  jcntrl_information_entry_set_cstr(&down_name, JCNTRL_INFO_ERROR_EXEC_DOWN,
                                    downstream_executive_name);
  jcntrl_information_entry_set_datatype(&prov_entry,
                                        JCNTRL_INFO_ERROR_TYPE_GIVEN, provided);
  jcntrl_information_entry_set_objecttype(&prov_o_entry,
                                          JCNTRL_INFO_ERROR_OBJECT_GIVEN,
                                          object_type_provided);
  jcntrl_information_entry_set_datatype(&req_entry,
                                        JCNTRL_INFO_ERROR_TYPE_REQUIRED,
                                        required);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  jcntrl_information_add_entry(&info, &up_name);
  jcntrl_information_add_entry(&info, &down_name);
  jcntrl_information_add_entry(&info, &prov_entry);
  jcntrl_information_add_entry(&info, &prov_o_entry);
  jcntrl_information_add_entry(&info, &req_entry);
  return jcntrl_error_run_callback(&info);
}

static int jcntrl_raise_external_library_error(const char *file, int line,
                                               jcntrl_error_code err,
                                               jcntrl_info extinfocode,
                                               int exterrcode, const char *msg)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line;
  struct jcntrl_information_entry exterrcode_entry, msg_entry, lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&exterrcode_entry);
  jcntrl_information_entry_init(&msg_entry);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER, err);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_int(&exterrcode_entry, extinfocode, exterrcode);
  jcntrl_information_entry_set_cstr(&msg_entry, JCNTRL_INFO_ERROR_MESSAGE, msg);
  jcntrl_information_entry_set_bool(&lock, JCNTRL_INFO_LOCKED, 1);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &exterrcode_entry);
  jcntrl_information_add_entry(&info, &msg_entry);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_errno_error(const char *file, int line, int errno_code,
                             const char *msg)
{
  if (!msg)
    msg = strerror(errno_code);
  return jcntrl_raise_external_library_error(file, line, JCNTRL_ERROR_ERRNO,
                                             JCNTRL_INFO_ERROR_ERRNO,
                                             errno_code, msg);
}

int jcntrl_raise_table_error(const char *file, int line, int table_error_code,
                             const char *msg)
{
  if (!msg)
    msg = table_errorstr(table_error_code);
  return jcntrl_raise_external_library_error(file, line, JCNTRL_ERROR_TABLE,
                                             JCNTRL_INFO_ERROR_TABLE,
                                             table_error_code, msg);
}

int jcntrl_raise_geom_error(const char *file, int line, int geom_error_code,
                            const char *msg)
{
  if (!msg)
    msg = geom_strerror(geom_error_code);
  return jcntrl_raise_external_library_error(file, line, JCNTRL_ERROR_GEOMETRY,
                                             JCNTRL_INFO_ERROR_GEOMETRY,
                                             geom_error_code, msg);
}

int jcntrl_raise_serializer_error(const char *file, int line,
                                  int serializer_error_code, const char *msg)
{
  if (!msg)
    msg = msgpackx_strerror(serializer_error_code);
  return jcntrl_raise_external_library_error(file, line,
                                             JCNTRL_ERROR_SERIALIZER,
                                             JCNTRL_INFO_ERROR_SERIALIZER,
                                             serializer_error_code, msg);
}

int jcntrl_raise_mpi_error(const char *file, int line, int mpi_error_code,
                           const char *msg)
{
#ifdef JCNTRL_USE_MPI
  char mpiemsg[MPI_MAX_ERROR_STRING];
  int msglen;
  if (!msg) {
    MPI_Error_string(mpi_error_code, mpiemsg, &msglen);
    mpiemsg[msglen] = '\0';
    msg = mpiemsg;
  }
#else
  msg = "MPI error raised, but MPI is not linked";
#endif
  return jcntrl_raise_external_library_error(file, line, JCNTRL_ERROR_MPI,
                                             JCNTRL_INFO_ERROR_MPI,
                                             mpi_error_code, msg);
}

int jcntrl_raise_information_type_error(const char *file, int line,
                                        jcntrl_info key,
                                        jcntrl_information_datatype requested)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line, lock;
  struct jcntrl_information_entry message, info_key, requested_type;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_init(&info_key);
  jcntrl_information_entry_init(&requested_type);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_INFORMATION_TYPE);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Invalid type used for specified "
                                    "information key. This error is "
                                    "considered bug");
  jcntrl_information_entry_set_int(&info_key, JCNTRL_INFO_ERROR_INFO_KEY, key);
  jcntrl_information_entry_set_int(&requested_type, JCNTRL_INFO_ERROR_INFO_TYPE,
                                   requested);
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &message);
  jcntrl_information_add_entry(&info, &info_key);
  jcntrl_information_add_entry(&info, &requested_type);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_element_type_error(const char *file, int line,
                                    jcntrl_element_type expected,
                                    jcntrl_element_type requested)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line, lock;
  struct jcntrl_information_entry el_expected, el_requested, message;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&el_expected);
  jcntrl_information_entry_init(&el_requested);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_ELEMENT_TYPE);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_int(&el_expected,
                                   JCNTRL_INFO_ERROR_ELTYPE_EXPECT, expected);
  jcntrl_information_entry_set_int(&el_requested,
                                   JCNTRL_INFO_ERROR_ELTYPE_REQUEST, requested);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Requested array type does not match to "
                                    "data array. This error is considered bug");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &el_expected);
  jcntrl_information_add_entry(&info, &el_requested);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_pure_virtual_error(const char *file, int line,
                                    const jcntrl_shared_object_data *base,
                                    const jcntrl_shared_object_data *thisp,
                                    const char *funcname)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, source_line, lock;
  struct jcntrl_information_entry base_cls, this_cls, fname, message;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&source_line);
  jcntrl_information_entry_init(&base_cls);
  jcntrl_information_entry_init(&this_cls);
  jcntrl_information_entry_init(&fname);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_PURE_VIRTUAL);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_int(&source_line, JCNTRL_INFO_ERROR_SOURCE_LINE,
                                   line);
  jcntrl_information_entry_set_objecttype(&base_cls,
                                          JCNTRL_INFO_ERROR_VIRTUAL_BASE, base);
  jcntrl_information_entry_set_objecttype(&this_cls,
                                          JCNTRL_INFO_ERROR_CLASS_CALLING,
                                          thisp);
  jcntrl_information_entry_set_cstr(&fname, JCNTRL_INFO_ERROR_FUNCNAME,
                                    funcname);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "Pure virtual (unbound function) called");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &source_line);
  jcntrl_information_add_entry(&info, &base_cls);
  jcntrl_information_add_entry(&info, &this_cls);
  jcntrl_information_add_entry(&info, &fname);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

int jcntrl_raise_csv_header_error(const char *file, const char *required_name,
                                  jcntrl_data_array *csv_header_name,
                                  jcntrl_size_type index)
{
  jcntrl_information info;
  struct jcntrl_information_entry errcode, source_file, lock;
  struct jcntrl_information_entry exec_up, header_name, idx, message;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&errcode);
  jcntrl_information_entry_init(&source_file);
  jcntrl_information_entry_init(&exec_up);
  jcntrl_information_entry_init(&header_name);
  jcntrl_information_entry_init(&idx);
  jcntrl_information_entry_init(&message);
  jcntrl_information_entry_set_int(&errcode, JCNTRL_INFO_ERROR_NUMBER,
                                   JCNTRL_ERROR_CSV_HEADER_MISMATCH);
  jcntrl_information_entry_set_cstr(&source_file, JCNTRL_INFO_ERROR_SOURCE_FILE,
                                    file);
  jcntrl_information_entry_set_cstr(&exec_up, JCNTRL_INFO_ERROR_EXEC_UP,
                                    required_name);
  if (csv_header_name) {
    jcntrl_shared_object *obj;
    obj = jcntrl_data_array_object(csv_header_name);
    jcntrl_information_entry_set_object(&header_name,
                                        JCNTRL_INFO_ERROR_CSV_HEADER, obj);
  } else {
    jcntrl_information_entry_set_object(&header_name,
                                        JCNTRL_INFO_ERROR_CSV_HEADER, NULL);
  }
  jcntrl_information_entry_set_int(&idx, JCNTRL_INFO_ERROR_INDEX, index);
  jcntrl_information_entry_set_cstr(&message, JCNTRL_INFO_ERROR_MESSAGE,
                                    "CSV header does not match to upstream");
  jcntrl_information_add_entry(&info, &errcode);
  jcntrl_information_add_entry(&info, &source_file);
  jcntrl_information_add_entry(&info, &exec_up);
  jcntrl_information_add_entry(&info, &header_name);
  jcntrl_information_add_entry(&info, &idx);
  jcntrl_information_add_entry(&info, &message);
  return jcntrl_error_run_callback(&info);
}

/*
 * Executive supplemental functions
 *
 * These functions are declared in executive.h.
 */

int jcntrl_executive_update(jcntrl_executive *exe)
{
  jcntrl_information info;
  struct jcntrl_information_entry update_info, update_extent, update_data;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&update_info);
  jcntrl_information_entry_init(&update_extent);
  jcntrl_information_entry_init(&update_data);
  jcntrl_information_entry_set_request(&update_info,
                                       JCNTRL_INFO_REQUEST_UPDATE_INFORMATION);
  jcntrl_information_entry_set_request(&update_extent,
                                       JCNTRL_INFO_REQUEST_UPDATE_EXTENT);
  jcntrl_information_entry_set_request(&update_data,
                                       JCNTRL_INFO_REQUEST_UPDATE_DATA);
  jcntrl_information_add_entry(&info, &update_info);
  jcntrl_information_add_entry(&info, &update_extent);
  jcntrl_information_add_entry(&info, &update_data);
  return jcntrl_executive_update_by_request(exe, &info);
}

int jcntrl_executive_update_information(jcntrl_executive *exe)
{
  jcntrl_information info;
  struct jcntrl_information_entry update_info;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&update_info);
  jcntrl_information_entry_set_request(&update_info,
                                       JCNTRL_INFO_REQUEST_UPDATE_INFORMATION);
  jcntrl_information_add_entry(&info, &update_info);
  return jcntrl_executive_update_by_request(exe, &info);
}

int jcntrl_executive_update_extent(jcntrl_executive *exe)
{
  jcntrl_information info;
  struct jcntrl_information_entry update_extent;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&update_extent);
  jcntrl_information_entry_set_request(&update_extent,
                                       JCNTRL_INFO_REQUEST_UPDATE_EXTENT);
  jcntrl_information_add_entry(&info, &update_extent);
  return jcntrl_executive_update_by_request(exe, &info);
}

int jcntrl_executive_update_data(jcntrl_executive *exe)
{
  jcntrl_information info;
  struct jcntrl_information_entry update_data;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&update_data);
  jcntrl_information_entry_set_request(&update_data,
                                       JCNTRL_INFO_REQUEST_UPDATE_DATA);
  jcntrl_information_add_entry(&info, &update_data);
  return jcntrl_executive_update_by_request(exe, &info);
}

int jcntrl_executive_check_loop(jcntrl_executive *exe)
{
  jcntrl_information info;
  struct jcntrl_information_entry check_loop;
  struct jcntrl_information_entry lock;
  jcntrl_information_init_static(&info, &lock);
  jcntrl_information_entry_init(&check_loop);
  jcntrl_information_entry_set_request(&check_loop,
                                       JCNTRL_INFO_REQUEST_CHECK_LOOP);
  jcntrl_information_add_entry(&info, &check_loop);
  return jcntrl_executive_update_by_request(exe, &info);
}
