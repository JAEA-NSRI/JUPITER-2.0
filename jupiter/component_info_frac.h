#ifndef JUPITER_COMPONENT_INFO_FRAC_H
#define JUPITER_COMPONENT_INFO_FRAC_H

/**
 * Array of pair of component and fraction.
 */

#include "common.h"
#include "component_info.h"
#include "component_info_defs.h"
#include "component_info_frac_defs.h"
#include "general_vector_node.h"
#include "geometry/util.h"

static inline struct component_info_frac *
component_info_frac__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct component_info_frac, node);
}

JUPITER_DECL
void *component_info_frac__alloc(struct general_vector_node *a, int n);
JUPITER_DECL
void component_info_frac__delete(struct general_vector_node *a);
JUPITER_DECL
void component_info_frac__copy(struct general_vector_node *to, int ts,
                               struct general_vector_node *from, int fs, int n);

static inline void component_info_frac__assign(struct general_vector_node *to,
                                               struct general_vector_node *from)
{
  struct component_info_frac *vf, *vt;
  vf = component_info_frac__getter(from);
  vt = component_info_frac__getter(to);

  *(struct component_info_frac_data **)&vt->d = vf->d;
}

static inline void component_info_frac__nullify(struct general_vector_node *a)
{
  struct component_info_frac *v;
  v = component_info_frac__getter(a);

  *(struct component_info_frac_data **)&v->d = NULL;
}

static inline void component_info_frac__swap(struct general_vector_node *p,
                                             int i,
                                             struct general_vector_node *q,
                                             int j, void *arg)
{
  struct component_info_frac *pp, *qq;
  struct component_info_frac_data t;

  pp = component_info_frac__getter(p);
  qq = component_info_frac__getter(q);

  t = pp->d[i];
  pp->d[i] = qq->d[j];
  qq->d[j] = t;
}

/**
 * @return negative when a < b, 0 when a == b, positive when a > b.
 */
typedef int component_info_frac_comp1(const struct component_info_frac_data *a,
                                      const struct component_info_frac_data *b,
                                      void *arg);

/**
 * @retval 0 keep @p a.
 * @retval 1 keep @p b.
 *
 * You can modify the argument.
 */
typedef int component_info_frac_merge1(struct component_info_frac_data *a,
                                       struct component_info_frac_data *b,
                                       void *arg);

struct component_info_frac_comp1_data
{
  component_info_frac_comp1 *cmp;
  component_info_frac_merge1 *merge;
  void *arg;
};

static inline int component_info_frac__comp1(struct general_vector_node *p,
                                             int i,
                                             struct general_vector_node *q,
                                             int j, void *arg)
{
  struct component_info_frac *pp, *qq;
  struct component_info_frac_comp1_data *d;
  pp = component_info_frac__getter(p);
  qq = component_info_frac__getter(q);
  d = (struct component_info_frac_comp1_data *)arg;
  return d->cmp(&pp->d[i], &qq->d[j], d->arg);
}

static inline int component_info_frac__merge1(struct general_vector_node *p,
                                              int i,
                                              struct general_vector_node *q,
                                              int j, void *arg)
{
  struct component_info_frac *pp, *qq;
  struct component_info_frac_comp1_data *d;
  pp = component_info_frac__getter(p);
  qq = component_info_frac__getter(q);
  d = (struct component_info_frac_comp1_data *)arg;
  if (d->merge)
    return d->merge(&pp->d[i], &qq->d[j], d->arg);
  return 0;
}

typedef int component_info_frac_compn(const struct component_info_frac_data *a,
                                      void *arg);

struct component_info_frac_compn_data
{
  component_info_frac_compn *cmp;
  void *arg;
};

static inline int component_info_frac__compn(struct general_vector_node *p,
                                             int i, void *arg)
{
  struct component_info_frac *pp;
  struct component_info_frac_compn_data *d;
  pp = component_info_frac__getter(p);
  d = (struct component_info_frac_compn_data *)arg;
  return d->cmp(&pp->d[i], d->arg);
}

static inline const struct general_vector_callbacks *
component_info_frac__funcs(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = component_info_frac__alloc,
    .copy = component_info_frac__copy,
    .assign = component_info_frac__assign,
    .deleter = component_info_frac__delete,
    .nullify = component_info_frac__nullify,
    .swap = component_info_frac__swap,
    .comp1 = component_info_frac__comp1,
    .compn = component_info_frac__compn,
    .merge = component_info_frac__merge1,
  };
  return f;
}

#define component_info_frac_funcs() \
  general_vector_funcs(component_info_frac__funcs)

//----

static inline void component_info_frac_init(struct component_info_frac *f)
{
  general_vector_node_init(&f->node, component_info_frac_funcs());
}

static inline void component_info_frac_clear(struct component_info_frac *f)
{
  general_vector_node_clear(&f->node, component_info_frac_funcs());
}

static inline void component_info_frac_share(struct component_info_frac *to,
                                             struct component_info_frac *from)
{
  general_vector_node_share(&to->node, &from->node,
                            component_info_frac_funcs());
}

static inline int component_info_frac_resize(struct component_info_frac *f,
                                             int n, int copy)
{
  struct component_info_frac tmp;
  component_info_frac_init(&tmp);
  return general_vector_node_resize(&f->node, &tmp.node, n, copy,
                                    component_info_frac_funcs());
}

static inline void component_info_frac_seti(struct component_info_frac *info,
                                            int index, int compo)
{
  info->d[index].comp = (struct component_info_data){.d = NULL, .id = compo};
}

static inline void component_info_frac_setc(struct component_info_frac *info,
                                            int index, struct component_data *d)
{
  info->d[index].comp = (struct component_info_data){.d = d, .id = 0};
}

static inline void component_info_frac_setf(struct component_info_frac *info,
                                            int index,
                                            component_info_frac_type f)
{
  info->d[index].fraction = f;
}

static inline struct component_info_data *
component_info_frac_getcp(struct component_info_frac *info, int index)
{
  return &info->d[index].comp;
}

static inline int component_info_frac_geti(struct component_info_frac *info,
                                           int index)
{
  return component_info_data_geti(component_info_frac_getcp(info, index));
}

static inline struct component_data *
component_info_frac_getc(struct component_info_frac *info, int index)
{
  return component_info_data_getc(component_info_frac_getcp(info, index));
}

static inline component_info_frac_type
component_info_frac_getf(struct component_info_frac *info, int index)
{
  return info->d[index].fraction;
}

static inline int component_info_frac_ncompo(struct component_info_frac *info)
{
  return info->node.n;
}

/**
 * Comparison function for component_info_frac_sort()
 */
static inline int
component_info_frac_sort_comp1(const struct component_info_frac_data *a,
                               const struct component_info_frac_data *b,
                               void *arg)
{
  return component_info_sort_comp1(&a->comp, &b->comp, arg);
}

/**
 * Comparison function for component_info_frac_merge()
 */
static inline int
component_info_frac_merge_comp1(const struct component_info_frac_data *a,
                                const struct component_info_frac_data *b,
                                void *arg)
{
  return component_info_merge_comp1(&a->comp, &b->comp, arg);
}

/**
 * Merger for component_info_frac_merge()
 */
static inline int
component_info_frac_merge_merge1(struct component_info_frac_data *a,
                                 struct component_info_frac_data *b, void *arg)
{
  if (component_info_merge_merge1(&a->comp, &b->comp, arg)) {
    b->fraction += a->fraction;
    return 1;
  } else {
    a->fraction += b->fraction;
    return 0;
  }
}

struct component_info_frac_sort_compn_data
{
  struct component_info_sort_compn_data d;
};

static inline int
component_info_frac_sort_compn(const struct component_info_frac_data *a,
                               void *arg)
{
  struct component_info_frac_sort_compn_data *d;
  d = (struct component_info_frac_sort_compn_data *)arg;
  return component_info_sort_compn(&a->comp, &d->d);
}

/**
 * @brief sort component info array in specified order.
 */
static inline void
component_info_frac_sort_base(struct component_info_frac *info,
                              component_info_frac_comp1 *cmp, void *arg)
{
  struct component_info_frac_comp1_data d = {
    .cmp = cmp,
    .merge = NULL,
    .arg = arg,
  };
  general_vector_node_sort(&info->node, component_info_frac_funcs(), &d);
}

/**
 * @brief sort component info array in ascending order on component indices.
 */
static inline void component_info_frac_sort(struct component_info_frac *info)
{
  component_info_frac_sort_base(info, component_info_frac_sort_comp1, NULL);
}

/**
 * @brief find the array index of given condition in component_info_frac array
 *
 * This function uses binary search. Array needs to be sorted in the way of
 * given comparison.
 */
static inline int
component_info_frac_find_sorted_base(struct component_info_frac *info,
                                     component_info_frac_compn *comp, void *arg)
{
  struct component_info_frac_compn_data d = {
    .cmp = comp,
    .arg = arg,
  };
  return general_vector_node_bsearch(&info->node, component_info_frac_funcs(),
                                     &d);
}

/**
 * @brief find the array index of a component index in component_info_frac
 * array.
 * @param ic Component index to look for
 * @param info Array to find
 * @return index of the array
 * @retval -1 not found
 */
static inline int
component_info_frac_find_sorted(int ic, struct component_info_frac *info)
{
  struct component_info_frac_sort_compn_data sd = {.d = {.ic = ic}};
  return component_info_frac_find_sorted_base(info,
                                              component_info_frac_sort_compn,
                                              &sd);
}

/**
 * @brief find the array index of given condition in component_info_frac array.
 */
static inline int
component_info_frac_find_base(struct component_info_frac *info,
                              component_info_frac_compn *cmp, void *arg)
{
  struct component_info_frac_compn_data d = {
    .cmp = cmp,
    .arg = arg,
  };
  return general_vector_node_lsearch(&info->node, component_info_frac_funcs(),
                                     &d);
}

static inline int component_info_frac_find(int ic,
                                           struct component_info_frac *info)
{
  struct component_info_frac_sort_compn_data sd = {.d = {.ic = ic}};
  return component_info_frac_find_base(info, component_info_frac_sort_compn,
                                       &sd);
}

static inline int
component_info_frac_uniq_base(struct component_info_frac *info,
                              component_info_frac_comp1 *cmp,
                              component_info_frac_merge1 *merge, void *arg)
{
  int r;
  struct component_info_frac tmp;
  struct component_info_frac_comp1_data d = {
    .cmp = cmp,
    .merge = merge,
    .arg = arg,
  };

  component_info_frac_init(&tmp);
  return general_vector_node_uniq(&info->node, &tmp.node,
                                  component_info_frac_funcs(), &d);
}

/**
 * Unique component_info_frac arrays
 *
 * Fractions of duplicated IDs are get summed.
 */
static inline int component_info_frac_uniq(struct component_info_frac *info)
{
  return component_info_frac_uniq_base(info, component_info_frac_merge_comp1,
                                       component_info_frac_merge_merge1, NULL);
}

static inline int component_info_frac_merge_base(
  struct component_info_frac *dest, struct component_info_frac *src1,
  struct component_info_frac *src2, component_info_frac_comp1 *cmp,
  component_info_frac_merge1 *merge, void *arg)
{
  int r;
  struct component_info_frac tmp;
  struct component_info_frac_comp1_data d = {
    .cmp = cmp,
    .merge = merge,
    .arg = arg,
  };

  component_info_frac_init(&tmp);
  r = general_vector_node_merge(&tmp.node, &src1->node, &src2->node,
                                component_info_frac_funcs(), &d);
  if (r) {
    component_info_frac_clear(&tmp);
    return r;
  }

  component_info_frac_share(dest, &tmp);
  component_info_frac_clear(&tmp);
  return r;
}

/**
 * Merges component_info_frac arrays
 *
 * Fractions of duplicated IDs are get summed.
 */
static inline int component_info_frac_merge(struct component_info_frac *dest,
                                            struct component_info_frac *src1,
                                            struct component_info_frac *src2)
{
  return component_info_frac_merge_base(dest, src1, src2,
                                        component_info_frac_merge_comp1,
                                        component_info_frac_merge_merge1, NULL);
}

#ifdef __cplusplus
}
#endif

#endif
