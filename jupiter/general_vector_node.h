#ifndef JUPTIER_GENERAL_VECTOR_NODE_H
#define JUPTIER_GENERAL_VECTOR_NODE_H

#include "common.h"
#include "geometry/list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shareable vector template
 */

struct general_vector_node;

struct general_vector_callbacks
{
  /// Allocator function, return NULL failed, non-NULL success.
  void *(*alloc)(struct general_vector_node *node, int n);

  /// Copy function, copy contents @p from starting @p fi to @p to starting @p
  /// ti, size by @p n elements. An overlapped copy shall be handled correctly,
  /// using functions like memmove().
  void (*copy)(struct general_vector_node *to, int ti,
               struct general_vector_node *from, int fi, int n);

  /// Assign function, assign same pointer @p from to @p to
  void (*assign)(struct general_vector_node *to,
                 struct general_vector_node *from);

  /// Deleter function
  void (*deleter)(struct general_vector_node *p);

  /// Clear function (sets NULL without deallocation)
  void (*nullify)(struct general_vector_node *p);

  /// Swap index i of array p and j of array q
  void (*swap)(struct general_vector_node *p, int i,
               struct general_vector_node *q, int j, void *arg);

  /// Compare index i of array p and j of array q
  /// - please return positive when i > j, 0 when i == j and negative when i < j
  int (*comp1)(struct general_vector_node *p, int i,
               struct general_vector_node *q, int j, void *arg);

  /// Compare index i with needle (the type of needle is choice)
  int (*compn)(struct general_vector_node *p, int i, void *needle);

  /// Merge selector for index i of array p and j of array q
  /// - please return 0 for keeping left(p) and 1 for keeping right(q)
  /// - This function is allowed to be NULL while merging or uniqifying, in this
  ///   case, left will be used
  int (*merge)(struct general_vector_node *p, int i,
               struct general_vector_node *q, int j, void *arg);
};

typedef const struct general_vector_callbacks *
general_vector_funcs_initializer(struct general_vector_callbacks *f);

static inline const struct general_vector_callbacks *
general_vector_funcs__init(general_vector_funcs_initializer *init,
                           struct general_vector_callbacks *f)
{
  return init(f);
}

#define general_vector_funcs(initializer) \
  general_vector_funcs__init(initializer, \
                             &(struct general_vector_callbacks){NULL})

/**
 * @brief Node for definition of general vector
 *
 * Usage:
 *
 * ```c
 * struct your_vector {
 *   your_type *const your_data; // recommend to be const
 *   struct general_vector_node node;
 * };
 *
 * // define internal implementations:
 *
 * // define getter to return your_vector from node.
 * struct your_vector *your_vector__getter(struct general_vector_node *p)
 * {
 *   return geom_container_of(p, struct your_vector, node);
 * }
 *
 * void *your_vector__alloc(struct general_vector_node *p, int n)
 * {
 *   struct your_vector *d = your_vector__getter(p);
 *   *((your_type **)&d->your_data) = malloc(sizeof(your_type) * n);
 *   return d->your_data;
 * }
 *
 * void *your_vector__assign(struct general_vector_node *to, void *from)
 * {
 *   struct your_vector *tt = your_vector__getter(to);
 *   struct your_vector *tf = your_vector__getter(from);
 *   *((your_type **)&tt->your_data) = tf->your_data;
 * }
 *
 * void *your_vector__nullify(struct general_vector_node *p)
 * {
 *   struct your_vector *d = your_vector__getter(p);
 *   *((your_type **)&tt->your_data) = NULL;
 * }
 *
 * void *your_vector__delete(struct general_vector_node *p)
 * {
 *   struct your_vector *d = your_vector__getter(p);
 *   free(d->your_data);
 *   d->your_data = NULL;
 * }
 *
 * // and define nesessary functions.
 *
 * const struct general_vector_funcs *your_vector_funcs(void)
 * {
 *   return ...; // return internal implementations
 * }
 *
 * // define API functions using general_vector_node_* functions:
 * void your_vector_init(struct your_vector *v)
 * {
 *   v->your_data = NULL;
 *   general_vector_node_init(&v->node, your_vector_funcs());
 * }
 *
 * void your_vector_clear(struct your_vector *v)
 * {
 *   general_vector_node_clear(&v->node, your_vector_funcs());
 * }
 *
 * void your_vector_alloc(struct your_vector *v, int n)
 * {
 *   general_vector_node_alloc(&v->node, n, your_vector_funcs());
 * }
 *
 * // ...
 * ```
 */
struct general_vector_node
{
  const int n;             ///< Length of vector
  struct geom_list shared; ///< Shared link node (note: share link is weak)
};
#define general_vector_node_entry(ptr) \
  geom_list_entry(ptr, struct general_vector_node, shared)

#define general_vector_node_set_n(node, value) ((*(int *)&(node)->n) = (value))

static inline struct general_vector_node *
general_vector_node_init(struct general_vector_node *node,
                         const struct general_vector_callbacks *funcs)
{
  general_vector_node_set_n(node, 0);
  geom_list_init(&node->shared);
  funcs->nullify(node);
  return node;
}

/**
 * @brief deallocate (if not shared) and clear node.
 */
static inline void
general_vector_node_clear(struct general_vector_node *node,
                          const struct general_vector_callbacks *funcs)
{
  if (geom_list_empty(&node->shared)) {
    funcs->deleter(node);
  } else {
    geom_list_delete(&node->shared);
  }
  funcs->nullify(node);
  general_vector_node_set_n(node, 0);
}

/**
 * @brief share array in nodes
 * @retval 1 always return 1 (never fail).
 */
static inline int
general_vector_node_share(struct general_vector_node *to,
                          struct general_vector_node *from,
                          const struct general_vector_callbacks *funcs)
{
  general_vector_node_clear(to, funcs);
  geom_list_insert_prev(&from->shared, &to->shared);
  funcs->assign(to, from);
  general_vector_node_set_n(to, from->n);
  return 1;
}

/**
 * @brief allocate array
 * @retval 0 failed (allocation failed)
 * @retval 1 success
 *
 * If you want to keep unchanged when failed, use general_vector_node_resize().
 */
static inline int
general_vector_node_alloc(struct general_vector_node *node, int n,
                          const struct general_vector_callbacks *funcs)
{
  general_vector_node_clear(node, funcs);
  if (!funcs->alloc(node, n)) {
    funcs->nullify(node);
    general_vector_node_set_n(node, 0);
    return 0;
  }
  general_vector_node_set_n(node, n);
  return 1;
}

static inline int
general_vector_node_clip_index(struct general_vector_node *node, int s)
{
  if (s < 0)
    return 0;
  if (s >= node->n)
    return node->n - 1;
  return s;
}

/**
 * Copies max @p n elements (if @p n is negative, unlimited)
 */
static inline int
general_vector_node_copy(struct general_vector_node *to,
                         struct general_vector_node *from, int n,
                         const struct general_vector_callbacks *funcs)
{
  if (n < 0)
    n = from->n;

  if (from->n < n)
    n = from->n;

  if (to->n < n)
    n = to->n;

  if (n > 0)
    funcs->copy(to, 0, from, 0, n);
  return n;
}

static inline int
general_vector_node_copy_range(struct general_vector_node *to, int ts, int te,
                               struct general_vector_node *from, int fs, int fe,
                               const struct general_vector_callbacks *funcs)
{
  int n, nt, nf;
  ts = general_vector_node_clip_index(to, ts);
  te = general_vector_node_clip_index(to, te);
  fs = general_vector_node_clip_index(from, fs);
  fe = general_vector_node_clip_index(from, fe);
  nt = te - ts + 1;
  nf = fe - fs + 1;
  n = (nt > nf) ? nf : nt;
  if (n <= 0)
    return 0;

  funcs->copy(to, ts, from, fs, n);
  return n;
}

/**
 * @brief Resize vector in size of @p n.
 * @retval 0 failed
 * @retval 1 success
 *
 * Node will be detached when successfully resized.
 *
 * @p tmp_node will be automatically cleared.
 */
static inline int general_vector_node_resize(
  struct general_vector_node *node, struct general_vector_node *tmp_node, int n,
  int copy, const struct general_vector_callbacks *funcs)
{
  if (!general_vector_node_alloc(tmp_node, n, funcs))
    return 0;

  if (copy)
    general_vector_node_copy(tmp_node, node, -1, funcs);

  general_vector_node_clear(node, funcs);
  general_vector_node_share(node, tmp_node, funcs);
  general_vector_node_clear(tmp_node, funcs);
  return 1;
}

/**
 * @brief Resize vector in size of @p n, without detaching.
 */
static inline int general_vector_node_resize_shared(
  struct general_vector_node *node, struct general_vector_node *tmp_node, int n,
  int copy, const struct general_vector_callbacks *funcs)
{
  struct geom_list *lp, *ln, *lh;
  struct general_vector_node *m;

  if (geom_list_empty(&node->shared))
    return general_vector_node_resize(node, tmp_node, n, copy, funcs);

  m = general_vector_node_entry(geom_list_next(&node->shared));
  if (!general_vector_node_resize(node, tmp_node, n, copy, funcs))
    return 0;

  geom_list_insert_prev(&m->shared, &node->shared);

  lh = &m->shared;
  geom_list_foreach_safe (lp, ln, lh) {
    struct general_vector_node *o;
    o = general_vector_node_entry(lp);
    general_vector_node_clear(o, funcs);
    general_vector_node_share(o, node, funcs);
  }
  general_vector_node_clear(m, funcs);
  general_vector_node_share(m, node, funcs);

  return 1;
}

/**
 * @brief Allocate @p n vector if smaller.
 *
 * When resize, the node will be detached.
 */
static inline int
general_vector_node_reserve(struct general_vector_node *node,
                            struct general_vector_node *tmp_node, int n,
                            const struct general_vector_callbacks *funcs)
{
  if (node->n >= n)
    return 1;

  return general_vector_node_resize(node, tmp_node, n, 1, funcs);
}

/**
 * @brief Sort array in given range
 */
static inline void
general_vector_node_sort_range(struct general_vector_node *node, int is, int ie,
                               const struct general_vector_callbacks *funcs,
                               void *arg)
{
  /* insertion sort */
  int i = is;
  while (i <= ie && i < node->n) {
    int j = i;
    for (; j > is; --j) {
      if (funcs->comp1(node, j - 1, node, j, arg) <= 0)
        break;
      funcs->swap(node, j - 1, node, j, arg);
    }
    ++i;
  }
}

/**
 * @brief Sort whole array
 */
static inline void
general_vector_node_sort(struct general_vector_node *node,
                         const struct general_vector_callbacks *funcs,
                         void *arg)
{
  general_vector_node_sort_range(node, 0, node->n - 1, funcs, arg);
}

/**
 * @brief Performs binary search in given region
 * @return found index, -1 not found.
 *
 * Comparison must be same way when sorted
 */
static inline int general_vector_node_bsearch_range(
  struct general_vector_node *node, int is, int ie,
  const struct general_vector_callbacks *funcs, void *needle)
{
  if (is < 0)
    is = 0;
  if (ie >= node->n)
    ie = node->n - 1;
  while (is <= ie) {
    int m = (is + ie) / 2;
    if (funcs->compn(node, m, needle) > 0) {
      ie = m - 1;
    } else if (funcs->compn(node, m, needle) < 0) {
      is = m + 1;
    } else {
      return m;
    }
  }
  return -1;
}

/**
 * @brief Performs binary search in whole array
 * @return found index, -1 not found.
 *
 * Comparison must be same way when sorted
 */
static inline int
general_vector_node_bsearch(struct general_vector_node *node,
                            const struct general_vector_callbacks *funcs,
                            void *needle)
{
  return general_vector_node_bsearch_range(node, 0, node->n - 1, funcs, needle);
}

/**
 * @brief Performs linear search in given region
 * @return found index, -1 not found.
 */
static inline int general_vector_node_lsearch_range(
  struct general_vector_node *node, int is, int ie,
  const struct general_vector_callbacks *funcs, void *needle)
{
  if (is < 0)
    is = 0;
  if (ie >= node->n)
    ie = node->n - 1;
  for (; is <= ie; ++is) {
    if (funcs->compn(node, is, needle) == 0)
      return is;
  }
  return -1;
}

/**
 * @brief Performs linear search in whole array
 * @return found index, -1 not found.
 */
static inline int
general_vector_node_lsearch(struct general_vector_node *node,
                            const struct general_vector_callbacks *funcs,
                            void *needle)
{
  return general_vector_node_lsearch_range(node, 0, node->n - 1, funcs, needle);
}

#undef general_vector_node_set_n

/**
 * @brief Uniqify array
 * @retval 1 success
 * @retval 0 failed (allocation error)
 */
JUPITER_DECL
int general_vector_node_uniq(struct general_vector_node *node,
                             struct general_vector_node *tmp,
                             const struct general_vector_callbacks *funcs,
                             void *arg);

/**
 * @brief Merge (and uniqify) array
 * @retval 1 success
 * @retval 0 failed (allocation error)
 */
JUPITER_DECL
int general_vector_node_merge(struct general_vector_node *outp,
                              struct general_vector_node *src1,
                              struct general_vector_node *src2,
                              const struct general_vector_callbacks *funcs,
                              void *arg);

#ifdef __cplusplus
}
#endif

#endif
