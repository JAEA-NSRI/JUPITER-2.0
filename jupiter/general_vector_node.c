#include "geometry/list.h"
#include "geometry/rbtree.h"
#include "general_vector_node.h"

#include <limits.h>
#include <stdlib.h>

struct general_vector_node_iary;

struct general_vector_node_inode
{
  struct geom_list list;
  struct geom_rbtree tree;
  struct general_vector_node *node;
  int index;
  struct general_vector_node_iary *root;
};
#define general_vector_node_inode_tree_entry(ptr) \
  geom_rbtree_entry(ptr, struct general_vector_node_inode, tree)
#define general_vector_node_inode_list_entry(ptr) \
  geom_list_entry(ptr, struct general_vector_node_inode, list)

static void
general_vector_node_inode_init(struct general_vector_node_inode *i,
                               struct general_vector_node_iary *root,
                               struct general_vector_node *node, int index)
{
  geom_list_init(&i->list);
  geom_rbtree_init(&i->tree);
  i->node = node;
  i->index = index;
  i->root = root;
}

struct general_vector_node_iary
{
  int n;
  const struct general_vector_callbacks *funcs;
  void *arg;
  struct general_vector_node_inode nodes[];
};

static struct general_vector_node_iary *
general_vector_node_iary_alloc(int n, const struct general_vector_callbacks *f,
                               void *arg)
{
  struct general_vector_node_iary *p;
  p = (struct general_vector_node_iary *)malloc(
    sizeof(struct general_vector_node_iary) +
    sizeof(struct general_vector_node_inode) * n);
  if (!p)
    return NULL;

  p->n = n;
  p->funcs = f;
  p->arg = arg;
  return p;
}

static int general_vector_node_inode_comp(struct geom_rbtree *a,
                                          struct geom_rbtree *b)
{
  struct general_vector_node_iary *r;
  struct general_vector_node_inode *ia, *ib;
  ia = general_vector_node_inode_tree_entry(a);
  ib = general_vector_node_inode_tree_entry(b);
  r = ia->root;
  return r->funcs->comp1(ia->node, ia->index, ib->node, ib->index, r->arg);
}

static int
general_vector_node_inode_unique_add(struct geom_list *lhead,
                                     struct geom_rbtree **troot,
                                     struct general_vector_node_inode *add)
{
  void *arg;
  const struct general_vector_callbacks *funcs;
  struct geom_rbtree *root, *t;
  int r = 1;
  root = *troot;

  funcs = add->root->funcs;
  arg = add->root->arg;
  t = NULL;
  if (root)
    t = geom_rbtree_find(root, &add->tree, general_vector_node_inode_comp);
  if (t) {
    r = 0;
    struct general_vector_node_inode *q;

    if (!funcs->merge)
      return r;

    q = general_vector_node_inode_tree_entry(t);
    if (!funcs->merge(q->node, q->index, add->node, add->index, arg))
      return r;

    geom_list_delete(&q->list);
    if (geom_list_empty(lhead))
      root = NULL;

    if (root)
      root = geom_rbtree_delete(root, t, NULL);
  }

  root = geom_rbtree_insert(root, &add->tree, general_vector_node_inode_comp,
                            NULL);
  geom_list_insert_prev(lhead, &add->list);

  *troot = root;
  return r;
}

static void
general_vector_node_inode_build(struct general_vector_node *dest,
                                struct geom_list *lhead,
                                const struct general_vector_callbacks *funcs)
{
  struct geom_list *lp;
  struct general_vector_node *c;
  int is, ie, ts;
  c = NULL;
  ts = 0;

  if (geom_list_empty(lhead))
    return;

  geom_list_foreach(lp, lhead) {
    struct general_vector_node_inode *p;
    p = general_vector_node_inode_list_entry(lp);
    if (!c || p->node != c || p->index != ie + 1) {
      if (c) {
        int te = ts + ie - is;
        general_vector_node_copy_range(dest, ts, te, c, is, ie, funcs);
        ts = te + 1;
      }
      c = p->node;
      is = p->index;
      ie = p->index;
    } else {
      ie = p->index;
    }
  }
  general_vector_node_copy_range(dest, ts, dest->n - 1, c, is, ie, funcs);
}

int general_vector_node_uniq(struct general_vector_node *node,
                             struct general_vector_node *tmp,
                             const struct general_vector_callbacks *funcs,
                             void *arg)
{
  struct geom_list lhead;
  struct geom_rbtree *root;
  struct general_vector_node_iary *inodes;
  struct general_vector_node_inode *lroot;
  int j;
  int is, ie, ts;

  if (node->n <= 1)
    return 0; /* always unique */

  if (node->n <= 2) {
    if (funcs->comp1(node, 0, node, 1, arg) != 0)
      return 1;

    if (funcs->merge && funcs->merge(node, 0, node, 1, arg)) {
      general_vector_node_copy_range(node, 0, 0, node, 1, 1, funcs);
    }
    return general_vector_node_resize(node, tmp, 1, 1, funcs);
  }

  inodes = general_vector_node_iary_alloc(node->n, funcs, arg);
  if (!inodes)
    return 0;

  root = NULL;
  geom_list_init(&lhead);
  j = 0;
  for (int i = 0; i < node->n; ++i) {
    struct general_vector_node_inode *p;
    p = &inodes->nodes[j];
    general_vector_node_inode_init(p, inodes, node, i);
    if (general_vector_node_inode_unique_add(&lhead, &root, p))
      ++j;
  }

  if (!general_vector_node_alloc(tmp, j, funcs)) {
    free(inodes);
    return 0;
  }

  general_vector_node_inode_build(tmp, &lhead, funcs);

  general_vector_node_clear(node, funcs);
  general_vector_node_share(node, tmp, funcs);
  general_vector_node_clear(tmp, funcs);
  free(inodes);
  return 1;
}

int general_vector_node_merge(struct general_vector_node *outp,
                              struct general_vector_node *src1,
                              struct general_vector_node *src2,
                              const struct general_vector_callbacks *funcs,
                              void *arg)
{
  struct geom_list lhead;
  struct geom_rbtree *root;
  struct general_vector_node_iary *inodes;
  int j;
  int n1 = src1->n;
  int n2 = src2->n;

  if (n1 <= 0 && n2 <= 0) {
    general_vector_node_clear(outp, funcs);
    return 0;
  }
  if (n1 < 0)
    n1 = 0;
  if (n2 < 0)
    n2 = 0;

  if (n2 > INT_MAX - n1)
    return 1;

  inodes = general_vector_node_iary_alloc(n1 + n2, funcs, arg);
  if (!inodes)
    return 0;

  root = NULL;
  geom_list_init(&lhead);
  j = 0;
  for (int ii = 0; ii < n1 + n2; ++ii) {
    int i;
    struct general_vector_node *node;
    struct general_vector_node_inode *p;
    struct geom_rbtree *t;

    if (ii < n1) {
      node = src1;
      i = ii;
    } else {
      node = src2;
      i = ii - n1;
    }

    p = &inodes->nodes[j];
    general_vector_node_inode_init(p, inodes, node, i);
    if (general_vector_node_inode_unique_add(&lhead, &root, p))
      ++j;
  }

  if (!general_vector_node_alloc(outp, j, funcs)) {
    free(inodes);
    return 0;
  }

  general_vector_node_inode_build(outp, &lhead, funcs);
  free(inodes);
  return 1;
}
