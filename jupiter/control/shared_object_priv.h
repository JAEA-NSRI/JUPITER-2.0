/**
 * @brief Handler of shared object
 *
 * This is private module.
 */

#ifndef JUPITER_CONTROL_SHARED_OBJECT_PRIV_H
#define JUPITER_CONTROL_SHARED_OBJECT_PRIV_H

#include "defs.h"
#include "error.h"
#include "jupiter/geometry/rbtree.h"
#include "shared_object.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

JUPITER_CONTROL_DECL_BEGIN

/**
 * @brief Downcast function
 */
typedef void *jcntrl_shared_object_downcaster(jcntrl_shared_object *data);

typedef jcntrl_shared_object *jcntrl_shared_object_allocator(void);

typedef void jcntrl_shared_object_deleter(jcntrl_shared_object *data);

typedef int jcntrl_shared_object_initializer(jcntrl_shared_object *data);

typedef void jcntrl_shared_object_destructor(jcntrl_shared_object *data);

typedef void jcntrl_shared_object_virtual(jcntrl_shared_object *data,
                                          void *arg);

struct jcntrl_shared_object_vtable
{
  jcntrl_shared_object_virtual *func;
};

struct jcntrl_shared_object_funcs
{
  /// Allcator function
  /// (Allocate new object, if possible)
  jcntrl_shared_object_allocator *allocator;

  /// Deleter function
  /// (Delete the object, if possible)
  jcntrl_shared_object_deleter *deleter;

  /// Initializer function
  /// (Initialize the contents of the object)
  jcntrl_shared_object_initializer *initializer;

  /// Desctructor function
  /// (clear and deallocate contents without deallocate the object itself)
  jcntrl_shared_object_destructor *destructor;

  /// Downcast function
  jcntrl_shared_object_downcaster *downcast;

  /// Size of array @p vtable
  jcntrl_size_type vtable_size;

  /// virtual function table
  struct jcntrl_shared_object_vtable *const vtable;
};

/**
 * Internal shared object data
 */
struct jcntrl_shared_object_data
{
  /// Class name
  const char *name;

  /// Tree for seach entry by name
  struct geom_rbtree tree;

  /// NULL if no ancestors, otherwise pointer to ancestor class
  const struct jcntrl_shared_object_data *ancestor;

  /// Functions
  struct jcntrl_shared_object_funcs funcs;
};

/**
 * To add virtual functions in [class]:
 *
 * ```c
 * #define [class]__ancestor [ancestor of [class]]
 * enum [class]_vtable_names  // Actually, the enum type name is not used
 * {
 *   [class]_[virtual_function_1]_id = JCNTRL_VTABLE_START([class]),
 *   [class]_[virtual_function_2]_id,
 *   [class]_[virtual_function_3]_id,
 *   // ...
 *   [class]_[virtual_function_n]_id,
 *   JCNTRL_VTABLE_SIZE([class])  // This must be the last item
 * };
 *
 * struct [class]_[virtual_function_1]_args { ...; }
 *
 * static [inline] void
 * [class]_[virtual_function_1]__wrapper(jcntrl_shared_object *p, void *arg,
 *                                       [function_type] *overridden_func)
 * {
 *   struct [class]_[virtual_function_1]_args *p = arg;
 *   overridden_func(...);
 * }
 *
 * // virtual override in [your_class]:
 *
 * static ... [your_class]_[virtual_function_1]_impl(...)
 *            // suffix `_impl` required
 * {
 *   ...;
 * }
 *
 * JCNTRL_VIRTUAL_WRAP([your_class], [class], [virtual_function_1]);
 *
 * static void [your_class]_init_func(jcntrl_shared_object_funcs *funcs)
 * {
 *   // ...
 *   JCNTRL_VIRTUAL_WRAP_SET(funcs, [your_class], [class],
 *                           [virtual_function_1]);
 * }
 *
 *
 * // at function init of [your_class]:
 *
 * // virtual function definition for [class]:
 * void [class]_[virtual_function_1]([class] *obj, ...)
 * {
 *   struct [class]_[virtual_function_1]_args args = { ... };
 *   jcntrl_shared_object_call_virtual(&obj->...object, [class],
 *                                     [virtual_function_1], &args);
 * }
 * ```
 *
 * If your class does not add virtual function:
 *
 * ```c
 * JCNTRL_VTABLE_NONE([class]);
 * ```
 */
enum jcntrl_shared_object_vtable_names
{
  jcntrl_shared_object_vtable_size = 0,
};

#define JCNTRL_VTABLE_X(a, b) a(b)
#define JCNTRL_VTABLE_E(x) x
#define JCNTRL_VTABLE_ANCESTOR(cls) JCNTRL_VTABLE_E(cls##__ancestor)
#define JCNTRL_VTABLE_START_A(ancestor) ancestor##_vtable_size
#define JCNTRL_VTABLE_START(cls) \
  JCNTRL_VTABLE_X(JCNTRL_VTABLE_START_A, JCNTRL_VTABLE_ANCESTOR(cls))
#define JCNTRL_VTABLE_SIZE(cls) cls##_vtable_size
#define JCNTRL_VTABLE_NONE(cls) \
  enum cls##_vtable_names { cls##_vtable_size = JCNTRL_VTABLE_START(cls) }

#define JCNTRL_DOWNCAST_IMPL_T(cls, type, obj)                              \
  ((void)sizeof(&(((type *)0)->cls##__dnmem) == (jcntrl_shared_object *)0), \
   (type *)((char *)obj - offsetof(type, cls##__dnmem)))

/**
 * For following class:
 * ```
 * struct [class] {
 *   [ancestor] mem;
 * };
 * ```
 *
 * define `[class]_dnmem` as
 * ```c
 * #define [class]__dnmem mem.[ancestor]__dnmem
 * ```
 * or
 * ```
 * #define [class]__dnmem mem
 * ```
 * if [ancestor] is jcntrl_shared_object (`jcntrl_shared_object__dnmem` is not
 * defined).
 *
 * This macro is **not** type safe. Use only to implement the downcast function
 * of [class]. This macro also assumes that `struct [class]` is `[class]`, this
 * is used for checking defined member is `jcntrl_shared_object` entry.
 */
#define JCNTRL_DOWNCAST_IMPL(cls, obj) JCNTRL_DOWNCAST_IMPL_T(cls, cls, obj)

#define JCNTRL_METADATA_INIT(cls) cls##_metadata_init
#define JCNTRL_ANCESTOR_INIT(cls) \
  JCNTRL_VTABLE_X(JCNTRL_METADATA_INIT, JCNTRL_VTABLE_ANCESTOR(cls))

#define JCNTRL_VIRTUAL_WRAP_FN(inherit, fn, vcls, funcname)            \
  static void inherit##_##funcname##__wrapped(jcntrl_shared_object *p, \
                                              void *arg)               \
  {                                                                    \
    vcls##_##funcname##__wrapper(p, arg, fn);                          \
  }

#define JCNTRL_VIRTUAL_WRAP(inherit, vcls, funcname) \
  JCNTRL_VIRTUAL_WRAP_FN(inherit, inherit##_##funcname##_impl, vcls, funcname)

#define JCNTRL_VIRTUAL_WRAP_SET(p, inherit, vcls, funcname) \
  p->vtable[vcls##_##funcname##_id].func = inherit##_##funcname##__wrapped

/**
 * @brief Make shared data
 */
struct jcntrl_shared_object
{
  const struct jcntrl_shared_object_data *metadata; /*!< Class metadata */
  int ref_count;                                    /*!< Number of references */
};

JUPITER_CONTROL_DECL
int jcntrl_shared_object_data_install(jcntrl_shared_object_data *data);

typedef void jcntrl_shared_object_data_init_func(jcntrl_shared_object_funcs *p);

static inline void jcntrl_shared_object_data_init__core(
  jcntrl_shared_object_data *p, struct jcntrl_shared_object_vtable *vtab,
  jcntrl_size_type vtab_size, const char *name,
  const jcntrl_shared_object_data *ancestor,
  jcntrl_shared_object_data_init_func *init_func)
{
  memset(p, 0, sizeof(jcntrl_shared_object_data));
  p->name = name;
  geom_rbtree_init(&p->tree);
  *(struct jcntrl_shared_object_vtable **)&p->funcs.vtable = vtab;
  p->funcs.vtable_size = vtab_size;
  if (vtab)
    for (jcntrl_size_type it = 0; it < vtab_size; ++it)
      vtab[it].func = NULL;

  p->ancestor = ancestor;
  init_func(&p->funcs);
}

static inline void jcntrl_shared_object__data_init_base(
  const jcntrl_shared_object_data **outp, jcntrl_shared_object_data *p,
  struct jcntrl_shared_object_vtable *vtable, jcntrl_size_type vtable_size,
  int *init_flag, int *init_lock, const char *name,
  const jcntrl_shared_object_data *(*ancestor_init_func)(void),
  jcntrl_shared_object_data_init_func *init_func)
{
  if (!*init_flag) {
    const jcntrl_shared_object_data *ancestor;
    *init_lock = 1;
    ancestor = ancestor_init_func ? ancestor_init_func() : NULL;
    jcntrl_shared_object_data_init__core(p, vtable, vtable_size, name, ancestor,
                                         init_func);
    *init_flag = jcntrl_shared_object_data_install(p);
    *init_lock = 0;
  }
  if (*init_flag) {
    *outp = p;
  } else {
    *outp = NULL;
  }
}

#define jcntrl_shared_object__vtable_size(n) ((n > 0) ? n : 1)

#define jcntrl_shared_object__data_init(ret, cls, init_func)                   \
  do {                                                                         \
    static int cls##__data_init_lock = 0;                                      \
    static int cls##__data_init = 0;                                           \
    static struct jcntrl_shared_object_data cls##__data;                       \
    static struct jcntrl_shared_object_vtable                                  \
      cls##_vtable[jcntrl_shared_object__vtable_size(cls##_vtable_size)];      \
    JCNTRL_ASSERT(!cls##__data_init_lock);                                     \
    jcntrl_shared_object__data_init_base(ret, &cls##__data, cls##_vtable,      \
                                         cls##_vtable_size, &cls##__data_init, \
                                         &cls##__data_init_lock, #cls,         \
                                         JCNTRL_ANCESTOR_INIT(cls),            \
                                         init_func);                           \
  } while (0)

#define JCNTRL_SHARED_METADATA_INIT_DEFINE(cls, init_func) \
  JCNTRL_SHARED_METADATA_INIT_DECL(cls)                    \
  {                                                        \
    const jcntrl_shared_object_data *ret;                  \
    jcntrl_shared_object__data_init(&ret, cls, init_func); \
    return ret;                                            \
  }

static inline void *jcntrl_shared_object_default_upcast_impl(void *data,
                                                             ptrdiff_t offset)
{
  if (!data)
    return NULL;
  return (char *)data + offset;
}

#define jcntrl_shared_object_default_upcast(ptr, cls, member) \
  jcntrl_shared_object_default_upcast_impl(ptr, offsetof(cls, member))

static inline void *
jcntrl_shared_object_default_allocator_impl(jcntrl_size_type sz,
                                            const char *file, long line)
{
  void *p;
  JCNTRL_ASSERT(sz > 0);
  p = malloc(sz);
  if (!p) {
    jcntrl_raise_allocation_failed(file, line);
    return NULL;
  }
  return p;
}

/**
 * Returning pointer is pointer to object `type`.
 * Note that this function may return NULL.
 *
 * Your function is required to return a pointer to `jcntrl_shared_object`, so,
 * your implementaion should looks like:
 *
 * ```
 * type *data = jcntrl_shared_object_default_allocator(...);
 * return data ? &data->object : NULL;
 * ```
 */
#define jcntrl_shared_object_default_allocator(type)                           \
  ((type *)jcntrl_shared_object_default_allocator_impl(sizeof(type), __FILE__, \
                                                       __LINE__))

static inline void
jcntrl_shared_object_default_deleter(jcntrl_shared_object *obj)
{
  free(obj->metadata->funcs.downcast(obj));
}

//-----

/**
 * Returns true if @p inheritor inherits @p ancestor
 */
static inline int
jcntrl_shared_object_data_is_a(const jcntrl_shared_object_data *ancestor,
                               const jcntrl_shared_object_data *inheritor)
{
  while (inheritor) {
    if (inheritor == ancestor)
      return 1;
    inheritor = inheritor->ancestor;
  }
  return 0;
}

#define jcntrl_shared_object_data_is_cls(cls, objp) \
  jcntrl_shared_object_data_is_a(JCNTRL_METADATA_INIT(cls)(), objp)

/*
 * This function is initializer for static (function-local) variable object.
 *
 * Do not use other purposes.
 */
JUPITER_CONTROL_DECL
int jcntrl_shared_object_static_init(jcntrl_shared_object *obj,
                                     const jcntrl_shared_object_data *p);

struct jcntrl_shared_object_virtual_stack
{
  jcntrl_shared_object *obj;
  const jcntrl_shared_object_data *cls;
  jcntrl_size_type func_id;
  void *arg;
  struct jcntrl_shared_object_virtual_stack *next;
};

JUPITER_CONTROL_DECL
const struct jcntrl_shared_object_virtual_stack *
jcntrl_shared_object_virtual_stack_top(void);

JUPITER_CONTROL_DECL
void jcntrl_shared_object_virtual_stack_push(
  struct jcntrl_shared_object_virtual_stack *entry);

JUPITER_CONTROL_DECL
void jcntrl_shared_object_virtual_stack_pop(void);

static inline void jcntrl_shared_object_call_virtual(
  jcntrl_shared_object *obj, const jcntrl_shared_object_data *fcls,
  jcntrl_size_type id, void *arg, const char *file, int line, const char *funcn)
{
  struct jcntrl_shared_object_virtual_stack stk;
  const jcntrl_shared_object_data *p;
  JCNTRL_ASSERT(jcntrl_shared_object_data_is_a(fcls, obj->metadata));

  p = obj->metadata;
  while (p && id < p->funcs.vtable_size && !p->funcs.vtable[id].func)
    p = p->ancestor;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(id < p->funcs.vtable_size);
  JCNTRL_ASSERT(p->funcs.vtable[id].func);
  if (!p || id >= p->funcs.vtable_size || !p->funcs.vtable[id].func) {
    jcntrl_raise_pure_virtual_error(file, line, fcls, obj->metadata, funcn);
    return;
  }

  stk.cls = p;
  stk.obj = obj;
  stk.func_id = id;
  stk.arg = arg;
  jcntrl_shared_object_virtual_stack_push(&stk);
  p->funcs.vtable[id].func(obj, arg);
  jcntrl_shared_object_virtual_stack_pop();
}

/**
 * Calls specific super (ancestor) class's virtual function implementation from
 * the object passed for current virtual function call.
 *
 * Unlike in C++, you cannot call different functions from currently running
 * virtual functions (i.e., this function is more like super() in Python, Perl
 * or Ruby). This limitation just exists for logic safety.
 */
static inline void
jcntrl_shared_object_call_super(const jcntrl_shared_object_data *ancestor,
                                const jcntrl_shared_object_data *fcls,
                                jcntrl_size_type id, void *arg,
                                const char *file, int line, const char *funcn)
{
  const jcntrl_shared_object_data *p;
  const struct jcntrl_shared_object_virtual_stack *caller;
  struct jcntrl_shared_object_virtual_stack stk;

  caller = jcntrl_shared_object_virtual_stack_top();
  JCNTRL_ASSERT(caller);
  JCNTRL_ASSERT(caller->cls);
  JCNTRL_ASSERT(caller->obj);
  JCNTRL_ASSERT(id == caller->func_id);

  p = caller->cls;
  while (p && p != ancestor)
    p = p->ancestor;
  while (p && id < p->funcs.vtable_size && !p->funcs.vtable[id].func)
    p = p->ancestor;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(id < p->funcs.vtable_size);
  JCNTRL_ASSERT(p->funcs.vtable[id].func);
  if (!p || id >= p->funcs.vtable_size || !p->funcs.vtable[id].func) {
    jcntrl_raise_pure_virtual_error(file, line, fcls, ancestor, funcn);
    return;
  }

  stk.obj = caller->obj;
  stk.arg = arg;
  stk.cls = p;
  stk.func_id = id;
  jcntrl_shared_object_virtual_stack_push(&stk);
  p->funcs.vtable[id].func(caller->obj, arg);
  jcntrl_shared_object_virtual_stack_pop();
}

#define jcntrl_shared_object_call_virtual(obj, cls, func, arg)        \
  jcntrl_shared_object_call_virtual(obj, JCNTRL_METADATA_INIT(cls)(), \
                                    cls##_##func##_id, arg, __FILE__, \
                                    __LINE__, #func)

#define jcntrl_shared_object_call_super(ancestor, cls, func, arg)             \
  jcntrl_shared_object_call_super(ancestor, JCNTRL_METADATA_INIT(cls)(),      \
                                  cls##_##func##_id, arg, __FILE__, __LINE__, \
                                  #func)

JUPITER_CONTROL_DECL_END

#endif
