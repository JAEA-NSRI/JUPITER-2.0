
#ifndef TABLE_TREE_H
#define TABLE_TREE_H

#include "table.h"

JUPITER_TABLE_DECL_START

/* inline function requires C99 */

struct table_tree
{
  struct table_tree *left;
  struct table_tree *right;
  struct table_tree *parent;
};
typedef struct table_tree table_tree;

#define table_tree_container_of(ptr,str,mem) \
  (str *)((char*)ptr - offsetof(str, mem))

static inline
table_tree *table_tree_left(table_tree *t)
{
  return t->left;
}

static inline
table_tree *table_tree_right(table_tree *t)
{
  return t->right;
}

static inline
table_tree *table_tree_parent(table_tree *t)
{
  return t->parent;
}

static inline
void table_tree_init(table_tree *t)
{
  t->parent = NULL;
  t->left = NULL;
  t->right = NULL;
}

static inline
void table_tree_disjoint(table_tree *child)
{
  if (child->parent) {
    if (child->parent->left == child) {
      child->parent->left = NULL;
    } else if (child->parent->right == child) {
      child->parent->right = NULL;
    }
  }
  child->parent = NULL;
}

static inline
void table_tree_chain_left(table_tree *parent, table_tree *child)
{
  if (parent->left) {
    table_tree_disjoint(parent->left);
  }
  if (child) {
    table_tree_disjoint(child);
  }
  parent->left = child;
  child->parent = parent;
}

static inline
void table_tree_chain_right(table_tree *parent, table_tree *child)
{
  if (parent->right) {
    table_tree_disjoint(parent->right);
  }
  if (child) {
    table_tree_disjoint(child);
  }
  parent->right = child;
  child->parent = parent;
}

/*
 * This function is recursive call, so may not be inlined.
 * inline keyword is added for avoid warning about unused function.
 *
 * see documentation for -Wunneeded-internal-declaration of clang
 * or gcc.
 */
static inline void
table_tree_foreach(table_tree *t, void *a, size_t dep, int is_left,
                   void (*f)(table_tree *t, void *a, size_t dep, int is_left))
{
  table_tree *tl, *tr;
  f(t, a, dep, is_left);
  tl = table_tree_left(t);
  if (tl) {
    table_tree_foreach(tl, a, dep + 1, 1, f);
  }
  tr = table_tree_right(t);
  if (tr) {
    table_tree_foreach(tr, a, dep + 1, 0, f);
  }
}

JUPITER_TABLE_DECL_END

#endif
