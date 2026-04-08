#ifndef JUPITER_COMPONENT_VECTOR_H
#define JUPITER_COMPONENT_VECTOR_H

#include "common.h"
#include "component_data_defs.h"
#include "component_vector_defs.h"
#include "general_vector_node.h"
#include "geometry/util.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline struct component_vector *
component_vector__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct component_vector, node);
}

JUPITER_DECL
void *component_vector__alloc(struct general_vector_node *a, int n);
JUPITER_DECL
void component_vector__delete(struct general_vector_node *a);
JUPITER_DECL
void component_vector__copy(struct general_vector_node *to, int ts,
                            struct general_vector_node *from, int fs, int n);

static inline void component_vector__assign(struct general_vector_node *to,
                                            struct general_vector_node *from)
{
  struct component_vector *vf, *vt;
  vf = component_vector__getter(from);
  vt = component_vector__getter(to);

  *(struct component_data ***)&vt->d = vf->d;
}

static inline void component_vector__nullify(struct general_vector_node *a)
{
  struct component_vector *v;
  v = component_vector__getter(a);

  *(struct component_data ***)&v->d = NULL;
}

static inline void component_vector__swap(struct general_vector_node *p, int i,
                                          struct general_vector_node *q, int j,
                                          void *arg)
{
  struct component_vector *pp, *qq;
  struct component_data *t;

  pp = component_vector__getter(p);
  qq = component_vector__getter(q);
  t = pp->d[i];
  pp->d[i] = qq->d[j];
  qq->d[j] = t;
}

/**
 * @return negative when a < b, 0 when a == b, positive when a > b.
 */
typedef int component_vector_comp1(const struct component_data *a,
                                   const struct component_data *b, void *arg);

/**
 * @retval 0 keep @p a,
 * @retval 1 keep @p b.
 */
typedef int component_vector_merge1(const struct component_data *a,
                                    const struct component_data *b, void *arg);

struct component_vector_comp1_data
{
  component_vector_comp1 *cmp;
  component_vector_merge1 *merge;
  void *arg;
};

static inline int component_vector__comp1(struct general_vector_node *p, int i,
                                          struct general_vector_node *q, int j,
                                          void *arg)
{
  struct component_vector *pp, *qq;
  struct component_vector_comp1_data *d;
  pp = component_vector__getter(p);
  qq = component_vector__getter(q);
  d = (struct component_vector_comp1_data *)arg;
  return d->cmp(pp->d[i], qq->d[j], d->arg);
}

static inline int component_vector__merge1(struct general_vector_node *p, int i,
                                           struct general_vector_node *q, int j,
                                           void *arg)
{
  struct component_vector *pp, *qq;
  struct component_vector_comp1_data *d;
  pp = component_vector__getter(p);
  qq = component_vector__getter(q);
  d = (struct component_vector_comp1_data *)arg;
  return d->merge(pp->d[i], qq->d[j], d->arg);
}

typedef int component_vector_compn(const struct component_data *a, void *arg);

struct component_vector_compn_data
{
  component_vector_compn *cmp;
  void *arg;
};

static inline int component_vector__compn(struct general_vector_node *n, int i,
                                          void *arg)
{
  struct component_vector *v;
  struct component_vector_compn_data *d;
  v = component_vector__getter(n);
  d = (struct component_vector_compn_data *)arg;
  return d->cmp(v->d[i], d->arg);
}

static inline const struct general_vector_callbacks *
component_vector__funcs(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = component_vector__alloc,
    .copy = component_vector__copy,
    .assign = component_vector__assign,
    .deleter = component_vector__delete,
    .nullify = component_vector__nullify,
    .swap = component_vector__swap,
    .comp1 = component_vector__comp1,
    .compn = component_vector__compn,
    .merge = component_vector__merge1,
  };
  return f;
}

#define component_vector_funcs() general_vector_funcs(component_vector__funcs)

//----

static inline void component_vector_init(struct component_vector *vec)
{
  general_vector_node_init(&vec->node, component_vector_funcs());
}

static inline void component_vector_clear(struct component_vector *vec)
{
  general_vector_node_clear(&vec->node, component_vector_funcs());
}

static inline void component_vector_share(struct component_vector *to,
                                          struct component_vector *from)
{
  general_vector_node_share(&to->node, &from->node, component_vector_funcs());
}

static inline int component_vector_resize(struct component_vector *vec,
                                          int ncompo, int copy)
{
  component_vector tmp;
  component_vector_init(&tmp);
  return general_vector_node_resize(&vec->node, &tmp.node, ncompo, copy,
                                    component_vector_funcs());
}

static inline void component_vector_set(struct component_vector *vec, int index,
                                        struct component_data *d)
{
  vec->d[index] = d;
}

static inline struct component_data *
component_vector_get(struct component_vector *vec, int index)
{
  return vec->d[index];
}

static inline int component_vector_ncompo(struct component_vector *vec)
{
  return vec->node.n;
}

static inline void component_vector_sort_base(struct component_vector *vec,
                                              component_vector_comp1 *cmp,
                                              void *arg)
{
  struct component_vector_comp1_data d = {
    .cmp = cmp,
    .arg = arg,
  };
  general_vector_node_sort(&vec->node, component_vector_funcs(), &d);
}

static inline int component_vector_lsearch_base(struct component_vector *vec,
                                                component_vector_compn *cmp,
                                                void *arg)
{
  struct component_vector_compn_data d = {
    .cmp = cmp,
    .arg = arg,
  };
  return general_vector_node_lsearch(&vec->node, component_vector_funcs(), &d);
}

static inline int component_vector_bsearch_base(struct component_vector *vec,
                                                component_vector_compn *cmp,
                                                void *arg)
{
  struct component_vector_compn_data d = {
    .cmp = cmp,
    .arg = arg,
  };
  return general_vector_node_bsearch(&vec->node, component_vector_funcs(), &d);
}

static inline int component_vector_merge_base(struct component_vector *outp,
                                              struct component_vector *v1,
                                              struct component_vector *v2,
                                              component_vector_comp1 *cmp,
                                              component_vector_merge1 *merge,
                                              void *arg)
{
  struct component_vector tmp;
  struct component_vector_comp1_data d = {
    .cmp = cmp,
    .merge = merge,
    .arg = arg,
  };

  component_vector_init(&tmp);
  if (!general_vector_node_merge(&tmp.node, &v1->node, &v2->node,
                                 component_vector_funcs(), arg)) {
    component_vector_clear(&tmp);
    return 0;
  }
  component_vector_share(outp, &tmp);
  component_vector_clear(&tmp);
  return 1;
}

#ifdef __cplusplus
}
#endif

#endif
