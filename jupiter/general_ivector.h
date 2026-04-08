#ifndef JUPITER_GENERAL_IVECTOR_H
#define JUPITER_GENERAL_IVECTOR_H

#include "common.h"
#include "geometry/util.h"
#include "general_vector_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Array of int using general_vector_node.
 */
struct general_ivector
{
  int *const d; ///< Data
  struct general_vector_node node;
};
typedef struct general_ivector general_ivector;

static inline general_ivector *
general_ivector__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct general_ivector, node);
}

JUPITER_DECL
void *general_ivector__alloc(struct general_vector_node *a, int n);
JUPITER_DECL
void general_ivector__delete(struct general_vector_node *a);
JUPITER_DECL
void general_ivector__copy(struct general_vector_node *to, int ts,
                           struct general_vector_node *from, int fs, int n);

static inline void general_ivector__assign(struct general_vector_node *to,
                                           struct general_vector_node *from)
{
  general_ivector *vf, *vt;
  vf = general_ivector__getter(from);
  vt = general_ivector__getter(to);

  *(int **)&vt->d = vf->d;
}

static inline void general_ivector__nullify(struct general_vector_node *a)
{
  general_ivector *v;
  v = general_ivector__getter(a);

  *(int **)&v->d = NULL;
}

typedef int general_ivector_comp(int a, int b, void *arg);

struct general_ivector_comp1_data
{
  general_ivector_comp *cmp;
  void *arg;
};

struct general_ivector_compn_data
{
  struct general_ivector_comp1_data d;
  int needle;
};

static inline void general_ivector__swap(struct general_vector_node *p, int i,
                                         struct general_vector_node *q, int j,
                                         void *arg)
{
  struct general_ivector *pp;
  struct general_ivector *qq;
  double t;

  pp = general_ivector__getter(p);
  qq = general_ivector__getter(q);
  t = pp->d[i];
  pp->d[i] = qq->d[j];
  qq->d[j] = t;
}

static inline int general_ivector__comp1(struct general_vector_node *p, int i,
                                         struct general_vector_node *q, int j,
                                         void *arg)
{
  struct general_ivector *pp, *qq;
  struct general_ivector_comp1_data *d;
  pp = general_ivector__getter(p);
  qq = general_ivector__getter(q);
  d = (struct general_ivector_comp1_data *)arg;
  return d->cmp(pp->d[i], qq->d[j], d->arg);
}

static inline int general_ivector__compn(struct general_vector_node *a, int i,
                                         void *needle)
{
  struct general_ivector *v;
  struct general_ivector_compn_data *d;

  v = general_ivector__getter(a);
  d = (struct general_ivector_compn_data *)needle;

  return d->d.cmp(v->d[i], d->needle, d->d.arg);
}

static inline struct general_ivector_comp1_data
general_ivector_comp1_new(general_ivector_comp *cmp, void *arg)
{
  return (struct general_ivector_comp1_data){.cmp = cmp, .arg = arg};
}

static inline struct general_ivector_compn_data
general_ivector_compn_new(general_ivector_comp *cmp, void *arg, int needle)
{
  return (struct general_ivector_compn_data){
    .d =
      {
        .cmp = cmp,
        .arg = arg,
      },
    .needle = needle,
  };
}

static inline const struct general_vector_callbacks *
general_ivector_funcs_set(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = general_ivector__alloc,
    .copy = general_ivector__copy,
    .assign = general_ivector__assign,
    .deleter = general_ivector__delete,
    .nullify = general_ivector__nullify,
    .swap = general_ivector__swap,
    .comp1 = general_ivector__comp1,
    .compn = general_ivector__compn,
    .merge = NULL,
  };
  return f;
}

#define general_ivector_funcs() general_vector_funcs(general_ivector_funcs_set)

static inline void general_ivector_init(general_ivector *v)
{
  general_vector_node_init(&v->node, general_ivector_funcs());
}

static inline void general_ivector_clear(general_ivector *v)
{
  general_vector_node_clear(&v->node, general_ivector_funcs());
}

static inline int general_ivector_n(general_ivector *v) { return v->node.n; }

static inline int general_ivector_resize(general_ivector *v, int n, int copy)
{
  general_ivector t;
  general_ivector_init(&t);
  return general_vector_node_resize(&v->node, &t.node, n, copy,
                                    general_ivector_funcs());
}

static inline int general_ivector_reserve(general_ivector *v, int n)
{
  general_ivector t;
  general_ivector_init(&t);
  return general_vector_node_reserve(&v->node, &t.node, n,
                                     general_ivector_funcs());
}

static inline int general_ivector_share(general_ivector *t, general_ivector *f)
{
  return general_vector_node_share(&t->node, &f->node, general_ivector_funcs());
}

static inline int general_ivector_copy(general_ivector *t, general_ivector *f)
{
  return general_vector_node_copy(&t->node, &f->node, t->node.n,
                                  general_ivector_funcs());
}

/**
 * Resize @p t to enough size to store the contents of @p f, and then copy. This
 * function always removes @p t from the sharing list, even if the size of @p t
 * is already enough.
 */
static inline int general_ivector_acopy(general_ivector *t, general_ivector *f)
{
  if (general_ivector_resize(t, f->node.n, 0))
    return 1;

  general_ivector_copy(t, f);
  return 0;
}

static inline void general_ivector_sort_range(general_ivector *t, int is,
                                              int ie,
                                              general_ivector_comp *comp,
                                              void *arg)
{
  struct general_ivector_comp1_data d;
  d = general_ivector_comp1_new(comp, arg);
  general_vector_node_sort_range(&t->node, is, ie, general_ivector_funcs(), &d);
}

static inline void general_ivector_sort(general_ivector *t,
                                        general_ivector_comp *comp, void *arg)
{
  struct general_ivector_comp1_data d;
  d = general_ivector_comp1_new(comp, arg);
  general_vector_node_sort(&t->node, general_ivector_funcs(), &d);
}

static inline int general_ivector_bsearch_range(general_ivector *t, int is,
                                                int ie,
                                                general_ivector_comp *comp,
                                                int value, void *arg)
{
  struct general_ivector_compn_data d;
  d = general_ivector_compn_new(comp, arg, value);
  return general_vector_node_bsearch_range(&t->node, is, ie,
                                           general_ivector_funcs(), &d);
}

static inline int general_ivector_bsearch(general_ivector *t,
                                          general_ivector_comp *comp, int value,
                                          void *arg)
{
  struct general_ivector_compn_data d;
  d = general_ivector_compn_new(comp, arg, value);
  return general_vector_node_bsearch(&t->node, general_ivector_funcs(), &d);
}

static inline int general_ivector_lsearch_range(general_ivector *t, int is,
                                                int ie,
                                                general_ivector_comp *comp,
                                                int value, void *arg)
{
  struct general_ivector_compn_data d;
  d = general_ivector_compn_new(comp, arg, value);
  return general_vector_node_lsearch_range(&t->node, is, ie,
                                           general_ivector_funcs(), &d);
}

static inline int general_ivector_lsearch(general_ivector *t,
                                          general_ivector_comp *comp, int value,
                                          void *arg)
{
  struct general_ivector_compn_data d;
  d = general_ivector_compn_new(comp, arg, value);
  return general_vector_node_lsearch(&t->node, general_ivector_funcs(), &d);
}

static inline int general_ivector_uniq(general_ivector *t,
                                       general_ivector_comp *comp, void *arg)
{
  struct general_ivector_comp1_data d;
  struct general_ivector tmp;
  d = general_ivector_comp1_new(comp, arg);
  general_ivector_init(&tmp);
  return general_vector_node_uniq(&t->node, &tmp.node, general_ivector_funcs(),
                                  &d);
}

static inline int general_ivector_merge(general_ivector *outp,
                                        general_ivector *s1,
                                        general_ivector *s2,
                                        general_ivector_comp *comp, void *arg)
{
  struct general_ivector_comp1_data d;
  d = general_ivector_comp1_new(comp, arg);
  return general_vector_node_merge(&outp->node, &s1->node, &s2->node,
                                   general_ivector_funcs(), &d);
}

#ifdef __cplusplus
}
#endif

#endif
