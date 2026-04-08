/**
 * @file rbtree.h
 * @ingroup Geometry
 * @brief General-purpose Red-Black tree implemention
 *
 * @sa Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest and
 *     Clifford Stein, Introduction to Algorithms, Third
 *     Ed. pp. 308--338. ISBN 978-0-262-53305-8.
 */

#ifndef JUPITER_GEOMETRY_RBTREE_H
#define JUPITER_GEOMETRY_RBTREE_H

#include <stddef.h>

#include "util.h"

#ifdef __cplusplus
/*
 * WARNING: geom_rbtree is not for use with C++.
 *          Highly recommend to use std::map<> or std::set<> instead.
 *
 *          If the order is not important (geom_rbtree is always
 *          sorted), C++11 has std::unordered_map<> and
 *          std::unordered_set<>, which is definitely faster than
 *          geom_rbtree. std::undered_map<> is O(1) time on set and
 *          get operations in average, where geom_rbtree always
 *          requires O(log(N)) time on them.
 */
extern "C" {
#endif

/**
 * @memberof geom_rbtree
 * @brief Color in Red-Black tree
 */
enum geom_rbtree_color
{
  GEOM_RBTREE_RED,   ///< Red
  GEOM_RBTREE_BLACK, ///< Black
};

/**
 * @brief Red-Black tree data
 * @ingroup Geometry
 *
 * Data to implement Red-Black tree strcture to any data.
 *
 * Red-Black tree is one of binary tree algorithm to be used for
 * add, remove and search items efficiently.
 *
 * In Red-Black tree, each items do not have order; can be used for
 * unordered data set or map.
 *
 * You should keep **root node** properly. The **root node** may
 * change when add or remove items. If you pass an non-root node to
 * the argument which requires the root node, you'll break the tree
 * and `geom_rbtree_get_root()` won't return the correct pointer.
 */
struct geom_rbtree
{
  struct geom_rbtree *parent; ///< Parent Node
  struct geom_rbtree *left;   ///< Left child node
  struct geom_rbtree *right;  ///< Right child node
  enum geom_rbtree_color color; ///< Color
};

/**
 * @memberof geom_rbtree
 * @brief Red-Black tree node comparisation function prototype
 * @retval -1 `a` is less than `b`.
 * @retval  0 `a` is equal to `b`.
 * @retval  1 `a` is greter than `b`.
 */
typedef int geom_rbtree_compare_func(struct geom_rbtree *a, struct geom_rbtree *b);

/**
 * @memberof geom_rbtree
 * @brief Callback function to add additional work on rotation
 * @param root Current root of tree
 * @param x    Operation target
 */
typedef void geom_rbtree_rotate_callback(struct geom_rbtree *root, struct geom_rbtree *x);

/**
 * @memberof goem_rbtree
 * @brief Table of callback.
 *
 * These callbacks are called **before** corresponding operations are
 * done.
 *
 * For optimization matter, modification is not allowed.
 */
struct geom_rbtree_callback_table
{
  geom_rbtree_rotate_callback *const on_left_rotate;
  geom_rbtree_rotate_callback *const on_right_rotate;
};

/**
 * @memberof geom_rbtree
 * @brief Initialize the node as single node tree (aka. root node)
 * @param node A node to be init
 */
static inline
void geom_rbtree_init(struct geom_rbtree *node)
{
  node->color = GEOM_RBTREE_BLACK;
  node->parent = node;
  node->left = NULL;
  node->right = NULL;
}

/**
 * @memberof geom_rbtree
 * @brief Get left child of specified node
 * @param x Node to get.
 * @return left child, or NULL if x is NULL (i.e., NIL node)
 */
static inline struct geom_rbtree *
geom_rbtree_left(struct geom_rbtree *x)
{
  if (!x) return NULL;
  return x->left;
}

/**
 * @memberof geom_rbtree
 * @brief Get right child of specified node
 * @param x Node to get.
 * @return right child, or NULL if x is NULL (i.e., NIL node)
 */
static inline struct geom_rbtree *
geom_rbtree_right(struct geom_rbtree *x)
{
  if (!x) return NULL;
  return x->right;
}

/**
 * @memberof geom_rbtree
 * @brief Get parent of specified node
 * @param x Node to get.
 * @return parent node
 * @retval x when `x` is root node
 * @retval NULL when `x` is NULL (i.e., NIL node)
 */
static inline struct geom_rbtree *
geom_rbtree_parent(struct geom_rbtree *x)
{
  if (!x) return NULL;
  return x->parent;
}

/**
 * @memberof geom_rbtree
 * @brief Check whether specified node is the root node
 * @param x Node to test
 * @retval     0 `x` is not root node, or `x` is NIL.
 * @retval non-0 `x` is the root node
 */
static inline int
geom_rbtree_is_root(struct geom_rbtree *x)
{
  if (!x) return 0;
  return x->parent == x;
}

/**
 * @relates geom_rbtree
 * @brief Set node as the root node.
 * @param x Node to set
 *
 * Do not use this macro directly.
 */
#define geom_rbtree_set_root(x)  do { x->parent = x; } while(0)

/**
 * @memberof geom_rbtree
 * @brief Get root node.
 * @param x node to get
 * @return the root node
 *
 * @warning This function has \f$ O(\log n) \f$ cost.
 */
static inline struct geom_rbtree *
geom_rbtree_get_root(struct geom_rbtree *x)
{
  while (x != geom_rbtree_parent(x)) {
    x = geom_rbtree_parent(x);
  }
  return x;
}

/**
 * @memberof geom_rbtree
 * @brief Get color of the node
 * @param x node to get
 * @return the node color.
 * @retval GEOM_RBTREE_BLACK if `x` is NULL (because it's NIL-node).
 */
static inline enum geom_rbtree_color
geom_rbtree_color(struct geom_rbtree *x)
{
  if (!x) return GEOM_RBTREE_BLACK;
  return x->color;
}

/**
 * @memberof geom_rbtree
 * @brief Test specified node `x` is colored by `c`.
 * @param x Node to test
 * @param c Color to test
 * @retval non-0 if `x`'s color is `c`.
 * @retval     0 if `x`'s color is not `c`.
 *
 * @sa geom_rbtree_color
 */
static inline int
geom_rbtree_is_color(struct geom_rbtree *x, enum geom_rbtree_color c)
{
  return geom_rbtree_color(x) == c;
}

/**
 * @memberof geom_rbtree
 * @brief Test specified node `x` is RED node.
 * @param x Node to test
 * @return non-0 if `x`'s color is RED, 0 if not.
 *
 * This function is equivalent to
 * `geom_rbtree_is_color(x, GEOM_RBTREE_RED)`.
 *
 * @sa geom_rbtree_is_color
 */
static inline int
geom_rbtree_is_red(struct geom_rbtree *x)
{
  return geom_rbtree_is_color(x, GEOM_RBTREE_RED);
}

/**
 * @memberof geom_rbtree
 * @brief Test specified node `x` is BLACK node.
 * @param x Node to test
 * @return non-0 if `x`'s color is BLACK, 0 if not.
 *
 * This function is equivalent to
 * `geom_rbtree_is_color(x, GEOM_RBTREE_BLACK)`.
 *
 * @sa geom_rbtree_is_color
 */
static inline int
geom_rbtree_is_black(struct geom_rbtree *x)
{
  return geom_rbtree_is_color(x, GEOM_RBTREE_BLACK);
}

/**
 * @memberof geom_rbtree
 * @brief Test specified node `x` is BLACK and non-NIL node.
 * @param x Node to test
 * @return non-0 if `x`'s color is BLACK and not NIL-node, 0 if not.
 *
 * @sa geom_rbtree_is_color geom_rbtree_is_black
 */
static inline int
geom_rbtree_is_non_nil_black(struct geom_rbtree *x)
{
  if (!x) return 0;
  return geom_rbtree_is_color(x, GEOM_RBTREE_BLACK);
}

/**
 * @memberof geom_rbtree
 * @private
 * @brief Left Rotate the Tree.
 * @param root The root node.
 * @param x
 * @return The new root node.
 */
static inline struct geom_rbtree *
geom_rbtree_left_rotate(struct geom_rbtree *root, struct geom_rbtree *x,
                        const struct geom_rbtree_callback_table *callbacks)
{
  struct geom_rbtree *y;
  y = x->right;
  if (!y) return root;

  if (callbacks && callbacks->on_left_rotate) {
    callbacks->on_left_rotate(root, x);
  }

  x->right = y->left;
  if (y->left) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (geom_rbtree_is_root(x)) {
    root = y;
    geom_rbtree_set_root(y);
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
  return root;
}

/**
 * @memberof geom_rbtree
 * @private
 * @brief Right Rotate the Tree.
 * @param root The root node.
 * @param x
 * @return The new root node.
 */
static inline struct geom_rbtree *
geom_rbtree_right_rotate(struct geom_rbtree *root, struct geom_rbtree *y,
                         const struct geom_rbtree_callback_table *callbacks)
{
  struct geom_rbtree *x;
  x = y->left;
  if (!x) return root;

  if (callbacks && callbacks->on_right_rotate) {
    callbacks->on_right_rotate(root, y);
  }

  y->left = x->right;
  if (x->right) {
    x->right->parent = y;
  }
  x->parent = y->parent;
  if (geom_rbtree_is_root(y)) {
    root = x;
    geom_rbtree_set_root(x);
  } else if (y == y->parent->right) {
    y->parent->right = x;
  } else {
    y->parent->left = x;
  }
  x->right = y;
  y->parent = x;
  return root;
}

/**
 * @memberof geom_rbtree
 * @private
 * @brief Tree fixup on insertion of a node `z`.
 * @param root The root node
 * @param z Inserted node
 * @param callback functions
 * @return The new root node
 */
static inline struct geom_rbtree *
geom_rbtree_insert_fix(struct geom_rbtree *root, struct geom_rbtree *z,
                       const struct geom_rbtree_callback_table *callbacks)
{
  struct geom_rbtree *zp;
  struct geom_rbtree *zpp;
  struct geom_rbtree *y;

  if (geom_rbtree_is_root(z)) return root;

  zp = z->parent;
  while (geom_rbtree_is_red(zp)) {
    zpp = zp->parent;
    if (zp == zpp->left) {
      y = zpp->right;
      if (geom_rbtree_is_red(y)) {
        zp->color = GEOM_RBTREE_BLACK;
        y->color = GEOM_RBTREE_BLACK;
        zpp->color = GEOM_RBTREE_RED;
        z = zpp;
        zp = z->parent;
        zpp = zp->parent;
      } else {
        if (z == zp->right) {
          z = zp;
          root = geom_rbtree_left_rotate(root, z, callbacks);
          zp = z->parent;
          zpp = zp->parent;
        }
        zp->color = GEOM_RBTREE_BLACK;
        zpp->color = GEOM_RBTREE_RED;
        root = geom_rbtree_right_rotate(root, zpp, callbacks);
      }
    } else if (zp == zpp->right) {
      y = zpp->left;
      if (geom_rbtree_is_red(y)) {
        zp->color = GEOM_RBTREE_BLACK;
        y->color = GEOM_RBTREE_BLACK;
        zpp->color = GEOM_RBTREE_RED;
        z = zpp;
        zp = z->parent;
        zpp = zp->parent;
      } else {
        if (z == zp->left) {
          z = zp;
          root = geom_rbtree_right_rotate(root, z, callbacks);
          zp = z->parent;
          zpp = zp->parent;
        }
        zp->color = GEOM_RBTREE_BLACK;
        zpp->color = GEOM_RBTREE_RED;
        root = geom_rbtree_left_rotate(root, zpp, callbacks);
      }
    } else {
      break;
    }
    zp = z->parent;
  }
  root->color = GEOM_RBTREE_BLACK;
  return root;
}

/**
 * @memberof geom_rbtree
 * @brief Insert new node
 * @param root The root node
 * @param z A node to insert.
 * @param comp_func Comparison function between two nodes.
 * @param callbacks Callback function table.
 * @return The new root node, NULL if already has entry.
 */
static inline struct geom_rbtree *
geom_rbtree_insert(struct geom_rbtree *root, struct geom_rbtree *z,
                   geom_rbtree_compare_func *comp_func,
                   const struct geom_rbtree_callback_table *callbacks)
{
  int cmp;
  struct geom_rbtree *x;
  struct geom_rbtree *y;
  y = NULL;
  x = root;
  while (x) {
    y = x;
    cmp = comp_func(z, x);
    if (cmp < 0) {
      x = x->left;
    } else if (cmp == 0) {
      return NULL;
    } else {
      x = x->right;
    }
  }
  z->parent = y;
  if (!y) {
    geom_rbtree_set_root(z);
    root = z;
  } else if (comp_func(z, y) < 0) {
    y->left = z;
  } else {
    y->right = z;
  }
  z->color = GEOM_RBTREE_RED;
  z->right = NULL;
  z->left = NULL;
  return geom_rbtree_insert_fix(root, z, callbacks);
}

static inline struct geom_rbtree *
geom_rbtree_find(struct geom_rbtree *root, struct geom_rbtree *search,
                 geom_rbtree_compare_func *comp_func)
{
  int cmp;
  struct geom_rbtree *x;

  x = root;
  while (x) {
    cmp = comp_func(search, x);
    if (cmp == 0) {
      return x;
    } else if (cmp < 0) {
      x = x->left;
    } else {
      x = x->right;
    }
  }
  return x;
}

static inline struct geom_rbtree *
geom_rbtree_minimum(struct geom_rbtree *tree)
{
  struct geom_rbtree *p;
  p = NULL;
  while (tree) {
    p = tree;
    tree = tree->left;
  }
  return p;
}

static inline struct geom_rbtree *
geom_rbtree_maximum(struct geom_rbtree *tree)
{
  struct geom_rbtree *p;
  p = NULL;
  while (tree) {
    p = tree;
    tree = tree->right;
  }
  return p;
}

static inline struct geom_rbtree *
geom_rbtree_transplant(struct geom_rbtree *root,
                       struct geom_rbtree *u, struct geom_rbtree *v)
{
  if (u == root) { /* geom_rbtree_is_root(u) */
    root = v;
    if (v) geom_rbtree_set_root(v);
  } else if (u == u->parent->left) {
    if (v) v->parent = u->parent;
    u->parent->left = v;
  } else {
    if (v) v->parent = u->parent;
    u->parent->right = v;
  }
  return root;
}

static inline struct geom_rbtree *
geom_rbtree_delete_fix(struct geom_rbtree *root, struct geom_rbtree *x,
                       const struct geom_rbtree_callback_table *callbacks)
{
  struct geom_rbtree *w;
  struct geom_rbtree *xp;

  xp = x->parent;
  while (!geom_rbtree_is_root(x) && geom_rbtree_is_black(x)) {
    if (x == xp->left) {
      w = xp->right;
      if (geom_rbtree_is_red(w)) {
        w->color = GEOM_RBTREE_BLACK;
        xp->color = GEOM_RBTREE_RED;
        root = geom_rbtree_left_rotate(root, xp, callbacks);
        w = xp->right;
      }
      if (w &&
          geom_rbtree_is_black(w->left) && geom_rbtree_is_black(w->right)) {
        w->color = GEOM_RBTREE_RED;
        x = xp;
        xp = x->parent;
      } else {
        if (w && geom_rbtree_is_black(w->right)) {
          if (w->left) w->left->color = GEOM_RBTREE_BLACK;
          w->color = GEOM_RBTREE_RED;
          root = geom_rbtree_left_rotate(root, w, callbacks);
        }
        if (w) w->color = xp->color;
        xp->color = GEOM_RBTREE_BLACK;
        if (w && w->right) w->right->color = GEOM_RBTREE_BLACK;
        root = geom_rbtree_left_rotate(root, xp, callbacks);
        x = root;
        xp = NULL;
      }
    } else if (x == xp->right) {
      w = xp->left;
      if (geom_rbtree_is_red(w)) {
        w->color = GEOM_RBTREE_BLACK;
        xp->color = GEOM_RBTREE_RED;
        root = geom_rbtree_right_rotate(root, xp, callbacks);
        w = xp->left;
      }
      if (w &&
          geom_rbtree_is_black(w->left) && geom_rbtree_is_black(w->right)) {
        w->color = GEOM_RBTREE_RED;
        x = xp;
        xp = x->parent;
      } else {
        if (w && geom_rbtree_is_black(w->left)) {
          if (w->right) w->right->color = GEOM_RBTREE_BLACK;
          w->color = GEOM_RBTREE_RED;
          root = geom_rbtree_right_rotate(root, w, callbacks);
        }
        if (w) w->color = xp->color;
        xp->color = GEOM_RBTREE_BLACK;
        if (w && w->left) w->left->color = GEOM_RBTREE_BLACK;
        root = geom_rbtree_right_rotate(root, xp, callbacks);
        x = root;
        xp = NULL;
      }
    }
  }
  if (x) x->color = GEOM_RBTREE_BLACK;
  return root;
}

static inline struct geom_rbtree *
geom_rbtree_delete(struct geom_rbtree *root, struct geom_rbtree *z,
                   const struct geom_rbtree_callback_table *callbacks)
{
  enum geom_rbtree_color yocolor;
  struct geom_rbtree *x;
  struct geom_rbtree *y;

  y = z;
  yocolor = y->color;
  if (!z->left) {
    x = z->right;
    root = geom_rbtree_transplant(root, z, z->right);
  } else if (!z->right) {
    x = z->left;
    root = geom_rbtree_transplant(root, z, z->left);
  } else {
    y = geom_rbtree_minimum(z->right);
    yocolor = y->color;
    x = y->right;
    if (y->parent == z) {
      if (x) x->parent = y;
    } else {
      root = geom_rbtree_transplant(root, y, y->right);
      y->right = z->right;
      if (y->right) y->right->parent = y;
    }
    root = geom_rbtree_transplant(root, z, y);
    y->left = z->left;
    if (y->left) y->left->parent = y;
    y->color = z->color;
  }
  if (yocolor == GEOM_RBTREE_BLACK) {
    if (x) root = geom_rbtree_delete_fix(root, x, callbacks);
  }
  geom_rbtree_init(z);
  return root;
}

/**
 * @brief Get rbtree entry
 * @relates geom_rbtree
 * @param ptr pointer to the tree node
 * @param type base struct type name
 * @param mem field (member) name of `type`
 * @return Pointer to bese struct.
 */
#define geom_rbtree_entry(ptr, type, mem)       \
  geom_container_of(ptr, type, mem)


static inline struct geom_rbtree *
geom_rbtree_succ_next(struct geom_rbtree *lp)
{
  if (lp->right) {
    return geom_rbtree_minimum(lp->right);
  }
  while (!geom_rbtree_is_root(lp) && lp == lp->parent->right) {
    lp = lp->parent;
  }
  if (geom_rbtree_is_root(lp)) {
    return NULL;
  }
  return lp->parent;
}

#define geom_rbtree_foreach_succ(lp, root)                              \
  for(lp = geom_rbtree_minimum(root); lp; lp = geom_rbtree_succ_next(lp))


static inline struct geom_rbtree *
geom_rbtree_predec_next(struct geom_rbtree *lp)
{
  if (lp->left) {
    return geom_rbtree_maximum(lp->left);
  }
  while (!geom_rbtree_is_root(lp) && lp == lp->parent->left) {
    lp = lp->parent;
  }
  if (geom_rbtree_is_root(lp)) {
    return NULL;
  }
  return lp->parent;
}

#define geom_rbtree_foreach_predec(lp, root)                              \
  for(lp = geom_rbtree_maximum(root); lp; lp = geom_rbtree_predec_next(lp))

static inline struct geom_rbtree *
geom_rbtree_minimum_depth(struct geom_rbtree *tree, int *depth)
{
  struct geom_rbtree *p;
  p = NULL;
  while (tree) {
    p = tree;
    ++*depth;
    tree = tree->left;
  }
  return p;
}

static inline struct geom_rbtree *
geom_rbtree_succ_next_depth(struct geom_rbtree *lp, int *depth)
{
  if (lp->right) {
    return geom_rbtree_minimum_depth(lp->right, depth);
  }
  while (!geom_rbtree_is_root(lp) && lp == lp->parent->right) {
    lp = lp->parent;
    --*depth;
  }
  if (geom_rbtree_is_root(lp)) {
    *depth = -1;
    return NULL;
  }
  --*depth;
  return lp->parent;
}

#define geom_rbtree_foreach_succ_depth(lp, root, depth) \
  for (lp = geom_rbtree_minimum_depth(root, &depth); lp; \
       lp = geom_rbtree_succ_next_depth(lp, &depth))

#ifdef __cplusplus
}
#endif

#endif
