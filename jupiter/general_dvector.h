#ifndef JUPITER_GENERAL_DVECTOR_H
#define JUPITER_GENERAL_DVECTOR_H

#include "common.h"
#include "geometry/util.h"
#include "general_vector_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Array of double using general_vector_node.
 */
struct general_dvector
{
  double *const d; ///< Data
  struct general_vector_node node;
};
typedef struct general_dvector general_dvector;

static inline general_dvector *
general_dvector__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct general_dvector, node);
}

JUPITER_DECL
void *general_dvector__alloc(struct general_vector_node *a, int n);
JUPITER_DECL
void general_dvector__delete(struct general_vector_node *a);
JUPITER_DECL
void general_dvector__copy(struct general_vector_node *to, int ts,
                           struct general_vector_node *from, int fs, int n);

static inline void general_dvector__assign(struct general_vector_node *to,
                                           struct general_vector_node *from)
{
  general_dvector *vf, *vt;
  vf = general_dvector__getter(from);
  vt = general_dvector__getter(to);

  *(double **)&vt->d = vf->d;
}

static inline void general_dvector__nullify(struct general_vector_node *a)
{
  general_dvector *v;
  v = general_dvector__getter(a);

  *(double **)&v->d = NULL;
}

static inline void general_dvector__swap(struct general_vector_node *p, int i,
                                         struct general_vector_node *q, int j,
                                         void *arg)
{
  struct general_dvector *pp;
  struct general_dvector *qq;
  double t;

  pp = general_dvector__getter(p);
  qq = general_dvector__getter(q);
  t = pp->d[i];
  pp->d[i] = qq->d[j];
  qq->d[j] = t;
}

typedef int general_dvector_comp(double a, double b, void *arg);

struct general_dvector_comp1_data
{
  general_dvector_comp *cmp;
  void *arg;
};

static inline int general_dvector__comp1(struct general_vector_node *p, int i,
                                         struct general_vector_node *q, int j,
                                         void *arg)
{
  struct general_dvector *pp, *qq;
  struct general_dvector_comp1_data *d;
  pp = general_dvector__getter(p);
  qq = general_dvector__getter(q);
  d = (struct general_dvector_comp1_data *)arg;
  return d->cmp(pp->d[i], qq->d[j], d->arg);
}

static inline struct general_dvector_comp1_data
general_dvector_comp1_new(general_dvector_comp *cmp, void *arg)
{
  return (struct general_dvector_comp1_data){.cmp = cmp, .arg = arg};
}

struct general_dvector_compn_data
{
  double needle;
  general_dvector_comp *cmp;
  void *arg;
};

static inline int general_dvector__compn(struct general_vector_node *n, int i,
                                         void *needle)
{
  struct general_dvector *v;
  struct general_dvector_compn_data *d;
  v = general_dvector__getter(n);
  d = (struct general_dvector_compn_data *)needle;
  return d->cmp(v->d[i], d->needle, d->arg);
}

static inline struct general_dvector_compn_data
general_dvector_compn_new(general_dvector_comp *cmp, void *arg, double needle)
{
  return (struct general_dvector_compn_data){
    .cmp = cmp,
    .arg = arg,
    .needle = needle,
  };
}

static inline const struct general_vector_callbacks *
general_dvector_funcs_set(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = general_dvector__alloc,
    .copy = general_dvector__copy,
    .assign = general_dvector__assign,
    .deleter = general_dvector__delete,
    .nullify = general_dvector__nullify,
    .swap = general_dvector__swap,
    .comp1 = general_dvector__comp1,
    .compn = general_dvector__compn,
    .merge = NULL,
  };
  return f;
}

#define general_dvector_funcs() general_vector_funcs(general_dvector_funcs_set)

static inline void general_dvector_init(general_dvector *v)
{
  general_vector_node_init(&v->node, general_dvector_funcs());
}

static inline void general_dvector_clear(general_dvector *v)
{
  general_vector_node_clear(&v->node, general_dvector_funcs());
}

static inline int general_dvector_n(general_dvector *v) { return v->node.n; }

static inline int general_dvector_resize(general_dvector *v, int n, int copy)
{
  general_dvector t;
  general_dvector_init(&t);
  return general_vector_node_resize(&v->node, &t.node, n, copy,
                                    general_dvector_funcs());
}

static inline int general_dvector_reserve(general_dvector *v, int n)
{
  general_dvector t;
  general_dvector_init(&t);
  return general_vector_node_reserve(&v->node, &t.node, n,
                                     general_dvector_funcs());
}

static inline int general_dvector_share(general_dvector *t, general_dvector *f)
{
  return general_vector_node_share(&t->node, &f->node, general_dvector_funcs());
}

static inline int general_dvector_copy(general_dvector *t, general_dvector *f)
{
  return general_vector_node_copy(&t->node, &f->node, t->node.n,
                                  general_dvector_funcs());
}

/**
 * Resize @p t to enough size to store the contents of @p f, and then copy. This
 * function always removes @p t from the sharing list, even if the size of @p t
 * is already enough.
 */
static inline int general_dvector_acopy(general_dvector *t, general_dvector *f)
{
  if (general_dvector_resize(t, f->node.n, 0))
    return 1;

  general_dvector_copy(t, f);
  return 0;
}

static inline void general_dvector_sort_range(general_dvector *t, int is,
                                              int ie,
                                              general_dvector_comp *comp,
                                              void *arg)
{
  struct general_dvector_comp1_data d;
  d = general_dvector_comp1_new(comp, arg);
  general_vector_node_sort_range(&t->node, is, ie, general_dvector_funcs(), &d);
}

static inline void general_dvector_sort(general_dvector *t,
                                        general_dvector_comp *comp, void *arg)
{
  struct general_dvector_comp1_data d;
  d = general_dvector_comp1_new(comp, arg);
  general_vector_node_sort(&t->node, general_dvector_funcs(), NULL);
}

static inline int general_dvector_bsearch_range(general_dvector *t, int is,
                                                int ie,
                                                general_dvector_comp *comp,
                                                double value, void *arg)
{
  struct general_dvector_compn_data d;
  d = general_dvector_compn_new(comp, arg, value);
  return general_vector_node_bsearch_range(&t->node, is, ie,
                                           general_dvector_funcs(), &d);
}

static inline int general_dvector_bsearch(general_dvector *t,
                                          general_dvector_comp *comp,
                                          double value, void *arg)
{
  struct general_dvector_compn_data d;
  d = general_dvector_compn_new(comp, arg, value);
  return general_vector_node_bsearch(&t->node, general_dvector_funcs(), &d);
}

static inline int general_dvector_lsearch_range(general_dvector *t, int is,
                                                int ie,
                                                general_dvector_comp *comp,
                                                double value, void *arg)
{
  struct general_dvector_compn_data d;
  d = general_dvector_compn_new(comp, arg, value);
  return general_vector_node_lsearch_range(&t->node, is, ie,
                                           general_dvector_funcs(), &d);
}

static inline int general_dvector_lsearch(general_dvector *t,
                                          general_dvector_comp *comp,
                                          double value, void *arg)
{
  struct general_dvector_compn_data d;
  d = general_dvector_compn_new(comp, arg, value);
  return general_vector_node_lsearch(&t->node, general_dvector_funcs(), &d);
}

static inline int general_dvector_uniq(general_dvector *t,
                                       general_dvector_comp *comp, void *arg)
{
  struct general_dvector_comp1_data d;
  struct general_dvector tmp;
  d = general_dvector_comp1_new(comp, arg);
  general_dvector_init(&tmp);
  return general_vector_node_uniq(&t->node, &tmp.node, general_dvector_funcs(),
                                  &d);
}

static inline int general_dvector_merge(general_dvector *outp,
                                        general_dvector *s1,
                                        general_dvector *s2,
                                        general_dvector_comp *comp, void *arg)
{
  struct general_dvector_comp1_data d;
  d = general_dvector_comp1_new(comp, arg);
  return general_vector_node_merge(&outp->node, &s1->node, &s2->node,
                                   general_dvector_funcs(), &d);
}

#ifdef __cplusplus
}
#endif

#endif
