
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

#include "../geometry/rbtree.h"
#include "../geometry/list.h"

#include "defs.h"
#include "buffer.h"
#include "error.h"

struct msgpackx_buffer_object;

/**
 * @ingroup Serializer
 * @brief Flexible shared buffer pointer
 */
struct msgpackx_buffer
{
  struct geom_rbtree tree;
  struct geom_list list;
  struct msgpackx_buffer_object *obj;
  char *pointer;
  char *substr;
  char *max;
};
#define msgpackx_buffer_tree_entry(ptr)                 \
  geom_rbtree_entry(ptr, struct msgpackx_buffer, tree)

#define msgpackx_buffer_list_entry(ptr)                 \
  geom_list_entry(ptr, struct msgpackx_buffer, list)

/**
 * @ingroup Serializer
 * @brief Shared data
 */
struct msgpackx_buffer_object
{
  struct geom_rbtree *root;
  struct msgpackx_buffer buffer;
};

static int
msgpackx_buffer_list_is_head(msgpackx_buffer *lp)
{
  MSGPACKX_ASSERT(lp);
  MSGPACKX_ASSERT(lp->obj);

  return lp == &lp->obj->buffer;
}

static char *
msgpackx_buffer_tree_get_max(msgpackx_buffer *a, char *max)
{
  if (a->substr && a->substr > max) {
    return a->substr;
  }
  return max;
}

static char *
msgpackx_buffer_tree_max_comp(msgpackx_buffer *a, msgpackx_buffer *b)
{
  char *as;
  char *bs;
  as = a->max;
  bs = b->max;
  if (as < bs) {
    return bs;
  }
  return as;
}

static void
msgpackx_buffer_tree_on_left_rotate(struct geom_rbtree *root,
                                    struct geom_rbtree *x)
{
  struct geom_rbtree *lx, *y, *z, *yr;
  msgpackx_buffer *xb, *lxb, *yb, *zb, *yrb;

  y   = geom_rbtree_right(x);
  lx  = geom_rbtree_left(x);
  z   = geom_rbtree_left(y);
  yr  = geom_rbtree_right(y);
  xb  = msgpackx_buffer_tree_entry(x);
  lxb = lx ? msgpackx_buffer_tree_entry(lx) : NULL;
  yb  = y  ? msgpackx_buffer_tree_entry(y)  : NULL;
  yrb = yr ? msgpackx_buffer_tree_entry(yr) : NULL;
  if (z) {
    zb = msgpackx_buffer_tree_entry(z);
    xb->max = msgpackx_buffer_tree_max_comp(lxb, zb);
  } else {
    if (lxb) {
      xb->max = lxb->max;
    } else {
      xb->max = NULL;
    }
  }
  xb->max = msgpackx_buffer_tree_get_max(xb, xb->max);
  if (yrb) {
    yb->max = msgpackx_buffer_tree_max_comp(xb, yrb);
  } else {
    yb->max = xb->max;
  }
  yb->max = msgpackx_buffer_tree_get_max(yb, yb->max);
}

static void
msgpackx_buffer_tree_on_right_rotate(struct geom_rbtree *root,
                                     struct geom_rbtree *y)
{
  struct geom_rbtree *lx, *x, *z, *yr;
  msgpackx_buffer *xb, *lxb, *yb, *zb, *yrb;

  x   = geom_rbtree_left(y);
  lx  = geom_rbtree_left(x);
  z   = geom_rbtree_right(x);
  yr  = geom_rbtree_right(y);
  xb  = msgpackx_buffer_tree_entry(x);
  lxb = lx ? msgpackx_buffer_tree_entry(lx) : NULL;
  yb  = y  ? msgpackx_buffer_tree_entry(y)  : NULL;
  yrb = yr ? msgpackx_buffer_tree_entry(yr) : NULL;
  if (z) {
    zb = msgpackx_buffer_tree_entry(z);
    yb->max = msgpackx_buffer_tree_max_comp(yrb, zb);
  } else {
    if (yrb) {
      yb->max = yrb->max;
    } else {
      yb->max = NULL;
    }
  }
  yb->max = msgpackx_buffer_tree_get_max(yb, yb->max);
  if (lxb) {
    xb->max = msgpackx_buffer_tree_max_comp(lxb, yb);
  } else {
    xb->max = yb->max;
  }
  xb->max = msgpackx_buffer_tree_get_max(xb, xb->max);
}

static const struct geom_rbtree_callback_table
msgpackx_buffer_tree_callbacks = {
  .on_left_rotate  = msgpackx_buffer_tree_on_left_rotate,
  .on_right_rotate = msgpackx_buffer_tree_on_right_rotate,
};

static char *
msgpackx_buffer_tree_calculate_max(msgpackx_buffer *ptr,
                                   struct geom_rbtree *l,
                                   struct geom_rbtree *r)
{
  msgpackx_buffer *lb, *rb;
  char *max;
  if (ptr) {
    if (!l) l = geom_rbtree_left(&ptr->tree);
    if (!r) r = geom_rbtree_right(&ptr->tree);
  }
  if (l && r) {
    lb = msgpackx_buffer_tree_entry(l);
    rb = msgpackx_buffer_tree_entry(r);
    max = msgpackx_buffer_tree_max_comp(lb, rb);
  } else {
    if (l) {
      r = l;
    }
    if (r) {
      rb = msgpackx_buffer_tree_entry(r);
      max = rb->max;
    } else {
      max = NULL;
    }
  }
  if (ptr) {
    return msgpackx_buffer_tree_get_max(ptr, max);
  } else {
    return max;
  }
}

static void
msgpackx_buffer_tree_update_max_upwards(msgpackx_buffer *start)
{
  struct geom_rbtree *r;
  msgpackx_buffer *rb;

  start->max = msgpackx_buffer_tree_calculate_max(start, NULL, NULL);
  r = geom_rbtree_parent(&start->tree);
  while (r) {
    rb = msgpackx_buffer_tree_entry(r);
    if (rb->max >= start->max) break;
    rb->max = start->max;
    if (geom_rbtree_is_root(r)) break;
    r = geom_rbtree_parent(r);
  }
}

static void
msgpackx_buffer_tree_refresh_max_upwards(msgpackx_buffer *start)
{
  struct geom_rbtree *r;
  msgpackx_buffer *ptr;

  r = &start->tree;
  while (r) {
    ptr = msgpackx_buffer_tree_entry(r);
    ptr->max = msgpackx_buffer_tree_calculate_max(ptr, NULL, NULL);
    if (geom_rbtree_is_root(r)) break;
    r = geom_rbtree_parent(r);
  }
}

/*
 * Find the lowest base pointer where the last pointer greater than ptr.
 */
static msgpackx_buffer *
msgpackx_buffer_tree_find_min_base_of_last_greater(struct geom_rbtree *root,
                                                   char *ptr)
{
  struct geom_rbtree *p, *q;
  msgpackx_buffer *pb, *pq;
  char *t;

  p = root;
  pq = NULL;
  pb = NULL;
  while (p) {
    pb = msgpackx_buffer_tree_entry(p);
    if (pb->max < ptr) {
      break;
    }

    q = geom_rbtree_left(p);
    t = NULL;
    if (q) {
      pq = msgpackx_buffer_tree_entry(q);
      t = pq->max;
    }
    if (t >= ptr) {
      p = q;
    } else {
      if (pb->pointer < ptr) break;
      if (pb->substr > ptr) break;
      p = geom_rbtree_right(p);
    }
  }
  return pb;
}

static msgpackx_buffer *
msgpackx_buffer_tree_inside_or_right_ptr_next(msgpackx_buffer *lp, char *ptr)
{
  struct geom_list *h;
  struct geom_list *p;

  h = &lp->list;
  geom_list_foreach(p, h) {
    lp = msgpackx_buffer_list_entry(p);
    if (msgpackx_buffer_list_is_head(lp)) return NULL;
    if (lp->max <= ptr) continue;
    break;
  }
  if (p != h) {
    return lp;
  } else {
    return NULL;
  }
}

static void
msgpackx_buffer_init(msgpackx_buffer *p)
{
  geom_rbtree_init(&p->tree);
  geom_list_init(&p->list);
  p->obj = NULL;
  p->pointer = NULL;
  p->substr = NULL;
  p->max = NULL;
}

static struct msgpackx_buffer_object *
msgpackx_buffer_object_new(void)
{
  struct msgpackx_buffer_object *p;
  p = (struct msgpackx_buffer_object *)
    malloc(sizeof(struct msgpackx_buffer_object));
  if (!p) return NULL;

  p->root = NULL;
  msgpackx_buffer_init(&p->buffer);
  p->buffer.obj = p;
  return p;
}

static struct msgpackx_buffer_object *
msgpackx_buffer_object_reserve(struct msgpackx_buffer_object *obj,
                               ptrdiff_t size)
{
  char *np;
  ptrdiff_t off;

  if (size <= 0) return NULL;

  if (obj->buffer.max - obj->buffer.pointer >= size) {
    return obj;
  }

  np = realloc(obj->buffer.pointer, size);
  if (!np) return NULL;

  obj->buffer.max = np + size;
  if (obj->buffer.pointer) {
    off = np - obj->buffer.pointer;
    if (off == 0) return obj;

    obj->buffer.pointer = np;
    obj->buffer.substr += off;
  } else {
    obj->buffer.pointer = np;
    obj->buffer.substr = np;
    return obj;
  }

  if (obj->root) {
    msgpackx_buffer *bufp;
    struct geom_list *lp;

    geom_list_foreach(lp, &obj->buffer.list) {
      bufp = msgpackx_buffer_list_entry(lp);
      bufp->pointer += off;
      if (bufp->substr) bufp->substr += off;
      if (bufp->max)    bufp->max += off;
    }
  }

  return obj;
}

static struct msgpackx_buffer_object *
msgpackx_buffer_object_resize(struct msgpackx_buffer_object *obj,
                              ptrdiff_t size)
{
  MSGPACKX_ASSERT(obj);
  MSGPACKX_ASSERT(size >= 0);

  if (!msgpackx_buffer_object_reserve(obj, size)) return NULL;
  obj->buffer.substr = obj->buffer.pointer + size;

  return obj;
}

static ptrdiff_t
msgpackx_buffer_object_size(struct msgpackx_buffer_object *obj)
{
  if (!obj->buffer.pointer) return 0;
  return obj->buffer.substr - obj->buffer.pointer;
}

static void
msgpackx_buffer_object_delete(struct msgpackx_buffer_object *obj)
{
  if (!obj) return;

  if (obj->root) {
    struct geom_list *lp, *ln;
    msgpackx_buffer *bufp;

    geom_list_foreach_safe(lp, ln, &obj->buffer.list) {
      bufp = msgpackx_buffer_list_entry(lp);
      bufp->obj = NULL;
      msgpackx_buffer_delete(bufp);
    }
  }
  free(obj->buffer.pointer);
  free(obj);
}

static struct msgpackx_buffer_object *
msgpackx_buffer_object_make_space(struct msgpackx_buffer_object *obj,
                                  char *loc, ptrdiff_t size)
{
  msgpackx_buffer *pb;
  ptrdiff_t exsz;
  ptrdiff_t off;

  if (size < 0) {
    return NULL;
  }

  off = loc - obj->buffer.pointer;
  exsz = msgpackx_buffer_object_size(obj) + size;
  if (off < 0) {
    exsz += (-off);
  } else {
    if (loc >= obj->buffer.substr) {
      exsz += loc - obj->buffer.substr;
    }
  }
  obj = msgpackx_buffer_object_resize(obj, exsz);
  if (!obj) return NULL;

  if (off < 0) {
    pb = NULL;
    loc = obj->buffer.pointer;
  } else {
    loc = obj->buffer.pointer + off;
    pb = msgpackx_buffer_tree_find_min_base_of_last_greater(obj->root, loc);
  }

  if (!pb) {
    struct geom_rbtree *p;

    /* We need to move all pointers */
    p = geom_rbtree_minimum(obj->root);
    pb = msgpackx_buffer_tree_entry(p);
  }

  for (; pb; pb = msgpackx_buffer_tree_inside_or_right_ptr_next(pb, loc)) {
    if (pb->pointer > loc) {
      pb->pointer += size;
    }
    if (pb->substr > loc) {
      pb->substr += size;
    }
    if (pb->max) {
      pb->max += size;
    }
    msgpackx_buffer_tree_update_max_upwards(pb);
  }

  memmove(loc + size, loc, obj->buffer.substr - loc - size);
  return obj;
}

static struct msgpackx_buffer_object *
msgpackx_buffer_object_shrink_space(struct msgpackx_buffer_object *obj,
                                    char *loc, ptrdiff_t size)
{
  struct geom_rbtree *p;
  msgpackx_buffer *pb;
  ptrdiff_t exsz;
  ptrdiff_t off;

  if (size < 0) {
    return NULL;
  }
  if (loc < obj->buffer.pointer) return NULL;
  if (loc >= obj->buffer.substr) return NULL;

  off = loc - obj->buffer.pointer;
  exsz = msgpackx_buffer_object_size(obj) - size;
  if (loc >= obj->buffer.substr) {
    exsz += loc - obj->buffer.substr;
  }
  obj = msgpackx_buffer_object_resize(obj, exsz);
  if (!obj) return NULL;

  loc = obj->buffer.pointer + off;
  pb = msgpackx_buffer_tree_find_min_base_of_last_greater(obj->root, loc);

  if (pb) {
    p = &pb->tree;
  } else {
    /* We need to move all pointers */
    p = geom_rbtree_minimum(obj->root);
  }

  for (; p; p = geom_rbtree_succ_next(p)) {
    pb = msgpackx_buffer_tree_entry(p);
    if (pb->pointer > loc) {
      pb->pointer -= size;
    }
    if (pb->substr && pb->substr > loc) {
      pb->substr -= size;
      if (pb->substr < pb->pointer) {
        pb->substr = pb->pointer;
      }
    }
    if (pb->max > loc) {
      pb->max -= size;
    }
  }

  memmove(loc, loc + size, obj->buffer.substr - loc);
  return obj;
}

static int
msgpackx_buffer_comp(struct geom_rbtree *a, struct geom_rbtree *b)
{
  msgpackx_buffer *p, *q;
  p = msgpackx_buffer_tree_entry(a);
  q = msgpackx_buffer_tree_entry(b);

  /* Allow duplications */
  if (p->pointer < q->pointer) return -1;
  return 1;
}

static int
msgpackx_buffer_other_pointer_refers_same_buffer(msgpackx_buffer *p)
{
  if (!p->obj) return 0;

  if (geom_rbtree_is_root(&p->tree) &&
      !geom_rbtree_left(&p->tree) && !geom_rbtree_right(&p->tree)) {
    return 0;
  }
  return 1;
}

msgpackx_buffer *
msgpackx_buffer_new(void)
{
  msgpackx_buffer *p;

  p = (msgpackx_buffer *)malloc(sizeof(msgpackx_buffer));
  if (!p) return NULL;

  msgpackx_buffer_init(p);
  return p;
}

static msgpackx_buffer *
msgpackx_buffer_unlink(msgpackx_buffer *ptr)
{
  geom_list_delete(&ptr->list);

  if (ptr->obj) {
    struct geom_rbtree *root;
    struct geom_rbtree *np, *npr;
    struct geom_rbtree *l, *r, *p;
    msgpackx_buffer *pb;
    char *max;

    l = geom_rbtree_left(&ptr->tree);
    r = geom_rbtree_right(&ptr->tree);
    max = NULL;
    np = NULL;
    if (!l) {
      /*
       *      |                |
       *      o <- ptr =>      r <- ptr
       *     / \              / \
       * (NIL)  r         (NIL) (NIL)
       *
       * (r can be NIL here.)
       */
      if (r) {
        pb = msgpackx_buffer_tree_entry(r);
        pb->max = msgpackx_buffer_tree_get_max(pb, NULL);
      }
      np = r;
    } else if (!r) {
      /*
       *      |                |
       *      o <- ptr =>      l <- ptr
       *     / \              / \
       *    l  (NIL)      (NIL) (NIL)
       *
       * (l is not NIL.)
       */
      np = l;
      pb = msgpackx_buffer_tree_entry(l);
      pb->max = msgpackx_buffer_tree_get_max(pb, NULL);
    } else {
      np = geom_rbtree_left(r);
      if (!np) {
        /*
         *        |                |
         *        o <-ptr          r <-ptr
         *       / \        =>    / \
         *      l   r            l   np
         *         / \
         *     (NIL)  np
         */
        np = geom_rbtree_right(r);
        max = msgpackx_buffer_tree_calculate_max(NULL, l, np);
        pb = msgpackx_buffer_tree_entry(r);
        max = msgpackx_buffer_tree_get_max(pb, max);
        pb->max = max;

      } else {
        struct geom_rbtree *pl;
        /*
         *            |                             |
         *            o <-ptr                       np
         *           / \                           /  \
         *          l   r                         l    r
         *             / \                            / \
         *          ...   ...                      ...   ...
         *         /                      =>      /
         *        p (p can be equal to r)        p (p can be equal to r)
         *       / \                            / \
         *      np  pl                       npr   pl
         *     /  \
         *  (NIL)  npr
         */
        np = geom_rbtree_minimum(r);
        p = geom_rbtree_parent(np);
        npr = geom_rbtree_right(np);
        pl = geom_rbtree_right(p);
        max = msgpackx_buffer_tree_calculate_max(NULL, npr, pl);
        pb = msgpackx_buffer_tree_entry(p);
        pb->max = msgpackx_buffer_tree_get_max(pb, max);

        /* p is not root */
        p = geom_rbtree_parent(p);
        pb = msgpackx_buffer_tree_entry(p);
        msgpackx_buffer_tree_refresh_max_upwards(pb);

        max = msgpackx_buffer_tree_calculate_max(NULL, l, r);
        pb = msgpackx_buffer_tree_entry(np);
        max = msgpackx_buffer_tree_get_max(pb, max);
        pb->max = max;
      }
    }

    if (!geom_rbtree_is_root(&ptr->tree)) {
      p = geom_rbtree_parent(&ptr->tree);
      pb = msgpackx_buffer_tree_entry(p);
      l = geom_rbtree_left(p);
      r = geom_rbtree_right(p);
      if (l == &ptr->tree) {
        l = np;
      } else {
        r = np;
      }
      pb->max = msgpackx_buffer_tree_calculate_max(NULL, l, r);
      pb->max = msgpackx_buffer_tree_get_max(pb, pb->max);
      if (!geom_rbtree_is_root(p)) {
        p = geom_rbtree_parent(p);
        pb = msgpackx_buffer_tree_entry(p);
        msgpackx_buffer_tree_update_max_upwards(pb);
      }
    }

    root = ptr->obj->root;
    root = geom_rbtree_delete(root, &ptr->tree,
                              &msgpackx_buffer_tree_callbacks);
    ptr->obj->root = root;
    if (!root) { /* ptr were the last element */
      msgpackx_buffer_object_delete(ptr->obj);
      ptr->obj = NULL;
    }
  }
  return ptr;
}

void
msgpackx_buffer_delete(msgpackx_buffer *ptr)
{
  if (!ptr) return;

  msgpackx_buffer_unlink(ptr);
  free(ptr);
}

void
msgpackx_buffer_delete_all_referenced(msgpackx_buffer *ptr)
{
  if (!ptr) return;

  if (ptr->obj) {
    msgpackx_buffer_object_delete(ptr->obj);
  } else {
    msgpackx_buffer_delete(ptr);
  }
}

static msgpackx_buffer *
msgpackx_buffer_insert(msgpackx_buffer *ptr)
{
  struct geom_rbtree *root;
  struct geom_rbtree *n;
  msgpackx_buffer *nbuf;

  if (!ptr->obj) return NULL;

  root = ptr->obj->root;
  ptr->max = msgpackx_buffer_tree_get_max(ptr, NULL);
  root = geom_rbtree_insert(root, &ptr->tree, msgpackx_buffer_comp,
                            &msgpackx_buffer_tree_callbacks);
  if (!root) {
    return NULL;
  }

  ptr->obj->root = root;
  ptr->max = NULL;
  msgpackx_buffer_tree_update_max_upwards(ptr);
  n = geom_rbtree_succ_next(&ptr->tree);
  if (n) {
    nbuf = msgpackx_buffer_tree_entry(n);
    geom_list_insert_prev(&nbuf->list, &ptr->list);
  } else {
    geom_list_insert_prev(&ptr->obj->buffer.list, &ptr->list);
  }

  return ptr;
}

static enum msgpackx_buffer_pointer_validity
msgpackx_buffer_object_is_valid(struct msgpackx_buffer_object *obj,
                                char *testp)
{
  if (obj->buffer.pointer && obj->buffer.substr) {
    if (testp >= obj->buffer.pointer || testp < obj->buffer.substr) {
      return MSGPACKX_POINTER_VALID;
    }
    if (testp == obj->buffer.substr) {
      return MSGPACKX_POINTER_VALID_AS_END;
    }
    return MSGPACKX_POINTER_INVALID;
  } else {
    return MSGPACKX_POINTER_INVALID;
  }
}

enum msgpackx_buffer_pointer_validity
msgpackx_buffer_is_valid(msgpackx_buffer *ptr, void *testp)
{
  if (ptr->obj) {
    return msgpackx_buffer_object_is_valid(ptr->obj, testp);
  } else {
    return MSGPACKX_POINTER_INVALID;
  }
}

void *
msgpackx_buffer_pointer(msgpackx_buffer *ptr)
{
  return ptr->pointer;
}

void *
msgpackx_buffer_endp(msgpackx_buffer *ptr)
{
  if (ptr->substr) {
    return ptr->substr;
  } else {
    return msgpackx_buffer_pointer_root(ptr) + msgpackx_buffer_size(ptr);
  }
}

char *
msgpackx_buffer_make_cstr(msgpackx_buffer *ptr)
{
  char *p;
  ptrdiff_t sz;

  sz = msgpackx_buffer_size(ptr);
  if (sz <= 0) {
    return NULL;
  }

  sz *= sizeof(char);
  p = (char *)malloc(sz);
  if (!p) return NULL;

  memcpy(p, ptr->pointer, sz);
  return p;
}

ptrdiff_t
msgpackx_buffer_size(msgpackx_buffer *ptr)
{
  char *lp;

  if (!ptr->obj) return (ptrdiff_t)0;

  lp = ptr->substr;
  if (!lp) {
    lp = ptr->obj->buffer.substr;
  }
  return lp - ptr->pointer;
}

char *
msgpackx_buffer_pointer_root(msgpackx_buffer *ptr)
{
  if (!ptr->obj) return NULL;

  return ptr->obj->buffer.pointer;
}

char *
msgpackx_buffer_reserve(msgpackx_buffer *ptr, ptrdiff_t size)
{
  struct msgpackx_buffer_object *a, *p;
  if (!ptr->obj) {
    a = msgpackx_buffer_object_new();
    if (!a) return NULL;
  } else {
    a = ptr->obj;
  }
  p = msgpackx_buffer_object_reserve(a, size);
  if (!ptr->obj) {
    ptr->obj = a;
    ptr->pointer = a->buffer.pointer;
    ptr->substr = NULL;
    msgpackx_buffer_insert(ptr);
  }
  if (!p) {
    return NULL;
  }
  return ptr->pointer;
}

char *
msgpackx_buffer_resize(msgpackx_buffer *ptr, ptrdiff_t size)
{
  if (!msgpackx_buffer_reserve(ptr, size)) return NULL;
  if (!msgpackx_buffer_object_resize(ptr->obj, size)) return NULL;
  return msgpackx_buffer_pointer(ptr);
}

msgpackx_buffer *
msgpackx_buffer_for_whole_region(msgpackx_buffer *ptr)
{
  return msgpackx_buffer_substr(ptr, 0, -1, MSGPACKX_SEEK_SET);
}

int
msgpackx_buffer_is_substr(msgpackx_buffer *ptr)
{
  return (ptr->substr != NULL);
}

int
msgpackx_buffer_is_shared(msgpackx_buffer *a, msgpackx_buffer *b)
{
  return a->obj != NULL && a->obj == b->obj;
}

int msgpackx_buffer_any_shared(msgpackx_buffer *a)
{
  struct geom_list *lh, *lp;

  if (!a->obj)
    return 0;

  lh = &a->obj->buffer.list;
  for (lp = geom_list_next(lh); lp != lh; lp = geom_list_next(lp)) {
    if (lp != &a->list)
      return 1;
  }
  return 0;
}

int
msgpackx_buffer_is_overlapped(msgpackx_buffer *a, msgpackx_buffer *b)
{
  char *as, *ae, *bs, *be;

  if (msgpackx_buffer_is_shared(a, b)) {
    as = a->pointer;
    bs = b->pointer;
    ae = a->substr;
    be = b->substr;
    if (!ae || as == ae)
      ae = as + 1;
    if (!be || bs == be)
      be = bs + 1;
    MSGPACKX_ASSERT(as < ae);
    MSGPACKX_ASSERT(bs < be);
    if (ae < bs || be < as)
      return 0;
    return 1;
  }
  return 0;
}

int msgpackx_buffer_any_overlapped(msgpackx_buffer *a)
{
  struct geom_rbtree *tp;
  msgpackx_buffer *bp;
  char *as;
  char *ae;

  if (!a->obj)
    return 0;

  as = a->pointer;
  ae = a->substr;
  if (!ae)
    ae = as + 1;

  geom_rbtree_foreach_succ(tp, a->obj->root) {
    bp = msgpackx_buffer_tree_entry(tp);
    if (bp == a)
      continue;
    if (bp->pointer >= ae)
      break;

    if (bp->substr && bp->substr > as)
      return 1;
    if (bp->pointer >= as)
      return 1;
  }
  return 0;
}

msgpackx_buffer *
msgpackx_buffer_substr(msgpackx_buffer *ptr, ptrdiff_t offset, ptrdiff_t size,
                       enum msgpackx_buffer_seek_mode seek)
{
  msgpackx_buffer *p;

  MSGPACKX_ASSERT(ptr);

  if (!ptr->obj || !ptr->obj->buffer.pointer) {
    return NULL;
  }

  p = msgpackx_buffer_new();
  if (!p) return NULL;

  p->obj = ptr->obj;
  p->pointer = ptr->pointer;
  p->substr = ptr->substr;
  p->max = NULL;

  if (!msgpackx_buffer_insert(p)) {
    p->obj = NULL; /* Removing an item which is not in the tree will fail. */
    msgpackx_buffer_delete(p);
    return NULL;
  }

  msgpackx_buffer_goto(p, offset, size, seek);
  return p;
}

static void
msgpackx_buffer_resize_restricted(msgpackx_buffer *ptr, ptrdiff_t size,
                                  struct msgpackx_buffer_object *obj)
{
  char *osub;

  osub = ptr->substr;

  if (size >= 0) {
    ptr->substr = ptr->pointer + size;
    if (ptr->substr > obj->buffer.substr) {
      ptr->substr = obj->buffer.substr;
    }
  } else {
    ptr->substr = NULL;
  }

  if (osub > ptr->substr) {
    msgpackx_buffer_tree_refresh_max_upwards(ptr);
  } else {
    msgpackx_buffer_tree_update_max_upwards(ptr);
  }
}

msgpackx_buffer *
msgpackx_buffer_goto(msgpackx_buffer *ptr, ptrdiff_t offset, ptrdiff_t size,
                     enum msgpackx_buffer_seek_mode seek)
{
  struct geom_list *t;
  struct msgpackx_buffer_object *obj;
  msgpackx_buffer *p;

  switch(seek) {
  case MSGPACKX_SEEK_CUR:
    break;

  case MSGPACKX_SEEK_SET:
    offset = (ptr->obj->buffer.pointer + offset) - ptr->pointer;
    break;

  case MSGPACKX_SEEK_END:
    offset = (ptr->obj->buffer.substr + offset) - ptr->pointer;
    break;
  }

  obj = ptr->obj;
  t = NULL;
  if (offset != 0) {
    ptrdiff_t pdiff;
    if (offset < 0) {
      t = geom_list_prev(&ptr->list);
      pdiff = -1; /* (ptr->pointer + offset) < p->pointer */
    } else {
      t = geom_list_next(&ptr->list);
      pdiff =  1; /* (ptr->pointer + offset) > p->pointer */
    }
    if (t) {
      p = msgpackx_buffer_list_entry(t);
      if (!msgpackx_buffer_list_is_head(p)) {
        pdiff = pdiff * (ptr->pointer + offset - p->pointer);
        if (pdiff > 0) {
          msgpackx_buffer_unlink(ptr);
        } else {
          t = NULL;
        }
      } else {
        t = NULL;
      }
    }
  }
  ptr->pointer += offset;
  msgpackx_buffer_resize_restricted(ptr, size, obj);
  if (ptr->pointer < obj->buffer.pointer) {
    ptr->pointer = obj->buffer.pointer;
  }
  if (t) {
    msgpackx_buffer_insert(ptr);
  }

  return ptr;
}

msgpackx_buffer *
msgpackx_buffer_relocate(msgpackx_buffer *moving_p,
                         msgpackx_buffer *destination_p,
                         ptrdiff_t offset,
                         enum msgpackx_buffer_seek_mode seek)
{
  ptrdiff_t sz;

  sz = -1;
  if (msgpackx_buffer_is_substr(moving_p)) {
    sz = msgpackx_buffer_size(moving_p);
  }

  if (moving_p->obj != destination_p->obj) {
    msgpackx_buffer_unlink(moving_p);

    moving_p->obj = destination_p->obj;
    switch(seek) {
    case MSGPACKX_SEEK_CUR:
      moving_p->pointer = destination_p->pointer;
      break;
    case MSGPACKX_SEEK_SET:
      moving_p->pointer = destination_p->obj->buffer.pointer;
   break;
    case MSGPACKX_SEEK_END:
      moving_p->pointer = destination_p->obj->buffer.substr;
      break;
    }
    moving_p->pointer += offset;
    if (sz >= 0) {
      moving_p->substr = moving_p->pointer + sz;
    }

    msgpackx_buffer_insert(moving_p);

  } else {
    if (seek == MSGPACKX_SEEK_CUR) {
      offset += destination_p->pointer - moving_p->pointer;
    }
    msgpackx_buffer_goto(moving_p, offset, sz, seek);
  }
  return moving_p;
}

msgpackx_buffer *
msgpackx_buffer_increment(msgpackx_buffer *ptr)
{
  ptrdiff_t sz;

  sz = -1;
  if (msgpackx_buffer_is_substr(ptr)) {
    sz = msgpackx_buffer_size(ptr);
  }
  return msgpackx_buffer_goto(ptr, 1, sz, MSGPACKX_SEEK_CUR);
}

msgpackx_buffer *
msgpackx_buffer_decrement(msgpackx_buffer *ptr)
{
  ptrdiff_t sz;

  sz = -1;
  if (msgpackx_buffer_is_substr(ptr)) {
    sz = msgpackx_buffer_size(ptr);
  }
  return msgpackx_buffer_goto(ptr, -1, sz, MSGPACKX_SEEK_CUR);
}

msgpackx_buffer *
msgpackx_buffer_resize_substr(msgpackx_buffer *ptr, ptrdiff_t size)
{
  ptrdiff_t osz, sub;
  struct msgpackx_buffer_object *obj;

  if (size < 0) return NULL;

  osz = msgpackx_buffer_size(ptr);
  if (osz == size) {
    if (!ptr->substr) {
      ptr->substr = ptr->pointer + size;
      msgpackx_buffer_tree_update_max_upwards(ptr);
    }
    return ptr;
  }

  obj = ptr->obj;

  sub = size - osz;
  if (sub > 0) {
    if (!msgpackx_buffer_object_make_space(obj, ptr->pointer, sub)) {
      return NULL;
    }
  } else {
    if (!msgpackx_buffer_object_shrink_space(obj, ptr->pointer, -sub)) {
      return NULL;
    }
  }
  ptr->substr = ptr->pointer + size;
  msgpackx_buffer_tree_refresh_max_upwards(ptr);
  return ptr;
}

msgpackx_buffer *
msgpackx_buffer_raw_copy(msgpackx_buffer *dest,
                         const void *data, ptrdiff_t size,
                         enum msgpackx_buffer_copy_mode mode)
{
  ptrdiff_t dsz;
  struct msgpackx_buffer_object *obj;

  if (size <= 0) return NULL;

  obj = dest->obj;

  if (!obj || (mode & MSGPACKX_COPY_CREATE)) {
    msgpackx_buffer *p;
    obj = msgpackx_buffer_object_new();
    if (!obj) return NULL;
    if (!msgpackx_buffer_object_resize(obj, size)) {
      msgpackx_buffer_object_delete(obj);
      return NULL;
    }

    msgpackx_buffer_unlink(dest);
    dest->obj = obj;
    dest->pointer = obj->buffer.pointer;
 dest->substr = NULL;
    dest->max = msgpackx_buffer_tree_get_max(dest, NULL);

    memcpy(dest->pointer, data, size);
    p = msgpackx_buffer_insert(dest);
    if (!p) {
      return NULL;
    }

    return dest;
  }

  dsz = msgpackx_buffer_size(dest);
  if (dsz != size) {
    if (dsz < size) {
      if (mode & MSGPACKX_COPY_EXPAND) {
        if (!msgpackx_buffer_resize_substr(dest, size)) {
          return NULL;
        }
      } else {
        size = dsz;
      }
    } else {
      if (mode & MSGPACKX_COPY_SHRINK) {
        if (!msgpackx_buffer_resize_substr(dest, size)) {
          return NULL;
        }
      }
    }
  }
  memmove(dest->pointer, data, size);
  if (dsz > size && (mode & MSGPACKX_COPY_ZERO)) {
    char *loc, *endp;
    loc = dest->pointer;
    endp = loc + msgpackx_buffer_size(dest);
    loc += size;
    memset(loc, 0, endp - loc);
  }
  return dest;
}

msgpackx_buffer *
msgpackx_buffer_copy(msgpackx_buffer *dest, msgpackx_buffer *from,
                     enum msgpackx_buffer_copy_mode mode)
{
  ptrdiff_t fsz, dsz;
  struct msgpackx_buffer_object *obj;

  fsz = msgpackx_buffer_size(from);
  if (fsz <= 0) return NULL;

  obj = dest->obj;

  if (!obj || (mode & MSGPACKX_COPY_CREATE)) {
    msgpackx_buffer *p;

    if (obj && from == dest &&
        !msgpackx_buffer_other_pointer_refers_same_buffer(dest)) {
      /*
       * On, `msgpackx_buffer_copy(p, p, MSGPACKX_COPY_CREATE)`,
       * `msgpackx_buffer_unlink(dest)` may deallocates source buffer,
       * if `p` is the last pointing buffer.
       */

      char *fp;
      msgpackx_buffer_object_resize(obj, fsz);
      fp = from->pointer;
      msgpackx_buffer_goto(dest, 0, -1, MSGPACKX_SEEK_SET);
      if (dest->pointer != fp) {
        memmove(dest->pointer, fp, fsz);
      }
    } else {
      obj = msgpackx_buffer_object_new();
      if (!obj) return NULL;
      if (!msgpackx_buffer_object_resize(obj, fsz)) {
        msgpackx_buffer_object_delete(obj);
        return NULL;
      }

      msgpackx_buffer_unlink(dest);
      dest->obj = obj;
      dest->pointer = obj->buffer.pointer;
   dest->substr = NULL;
      dest->max = msgpackx_buffer_tree_get_max(dest, NULL);

      memcpy(dest->pointer, from->pointer, fsz);
      p = msgpackx_buffer_insert(dest);
      if (!p) {
        return NULL;
      }

      return dest;
    }
  }

  dsz = msgpackx_buffer_size(dest);
  if (fsz == dsz) {
    memmove(dest->pointer, from->pointer, fsz);
  } else {
    ptrdiff_t sub;

    if (fsz > dsz) {
      if (mode & MSGPACKX_COPY_EXPAND) {
        if (!msgpackx_buffer_resize_substr(dest, fsz)) {
          return NULL;
        }
        dsz = fsz;
      }
      memmove(dest->pointer, from->pointer, dsz);

    } else {
      char *fromp;

      fromp = NULL;
      if (mode & MSGPACKX_COPY_SHRINK) {
        sub = dsz - fsz;
        if ((from->pointer > dest->pointer &&
             from->pointer - dest->pointer < sub) ||
            (from->pointer + fsz > dest->pointer &&
             from->pointer + fsz - dest->pointer < sub)) {
          /* Shrinking destroys `from` data: create temporary */
          fromp = msgpackx_buffer_make_cstr(from);
          if (!fromp) return NULL;
        }
        if (!msgpackx_buffer_resize_substr(dest, fsz)) {
          free(fromp);
          return NULL;
        }
        dsz = fsz;
      }
      if (fromp) {
        memcpy(dest->pointer, fromp, fsz);
        free(fromp);
      } else {
        memmove(dest->pointer, from->pointer, fsz);
      }

      if ((dsz > fsz) && (mode & MSGPACKX_COPY_ZERO)) {
        char *loc, *endp;
        loc = dest->pointer;
        endp = loc + msgpackx_buffer_size(dest);
        loc += fsz;
        memset(loc, 0, endp - loc);
      }
    }
  }
  return dest;
}

msgpackx_buffer *
msgpackx_buffer_dup(msgpackx_buffer *src, enum msgpackx_buffer_dup_mode mode)
{
  msgpackx_buffer *b;

  if (mode == MSGPACKX_DUP_SUBSTR) {
    b = msgpackx_buffer_new();
    if (!b) return NULL;

    if (!msgpackx_buffer_copy(b, src, MSGPACKX_COPY_CREATE)) {
      msgpackx_buffer_delete(b);
      return NULL;
    }
    return b;
  } else if (mode == MSGPACKX_DUP_WHOLE) {
    char *p, *r;
    ptrdiff_t sz;

    b = msgpackx_buffer_substr(src, 0, -1, MSGPACKX_SEEK_SET);
    if (!b) return NULL;

    if (!msgpackx_buffer_copy(b, b, MSGPACKX_COPY_CREATE)) {
      msgpackx_buffer_delete(b);
      return NULL;
    }

    r = msgpackx_buffer_pointer_root(src);
    p = msgpackx_buffer_pointer(src);
    sz = -1;
    if (msgpackx_buffer_is_substr(src)) {
      sz = msgpackx_buffer_size(src);
    }
    msgpackx_buffer_goto(b, p - r, sz, MSGPACKX_SEEK_SET);

    return b;
  }
  return NULL;
}


int
msgpackx_buffer_printf(msgpackx_buffer *dest,
                       enum msgpackx_buffer_copy_mode mode,
                       const char *format, ...)
{
  va_list ap;
  int r;
  va_start(ap, format);
  r = msgpackx_buffer_vprintf(dest, mode, format, ap);
  va_end(ap);
  return r;
}

int
msgpackx_buffer_vprintf(msgpackx_buffer *dest,
                        enum msgpackx_buffer_copy_mode mode,
                        const char *format, va_list ap)
{
  /*
   * C99 supported platforms, POSIX 2001.12 and VS2015 or later.
   *
   * [CAUTION]: Some (v)snprintf implementations behave in different way
   *            from specified by C99, with ***same prototype***.
   */
  va_list c;
  int n;
  ptrdiff_t sz;
  struct msgpackx_buffer_object *obj;

  va_copy(c, ap);
  n = vsnprintf(NULL, 0, format, c);
  va_end(c);
  if (n < 0) return n;

  obj = dest->obj;
  if (!obj || (mode & MSGPACKX_COPY_CREATE)) {
    obj = msgpackx_buffer_object_new();
    if (!obj) return -1;
    if (!msgpackx_buffer_object_resize(obj, n + 1)) {
      msgpackx_buffer_object_delete(obj);
      return -1;
    }

    msgpackx_buffer_unlink(dest);
    dest->obj = obj;
    dest->pointer = obj->buffer.pointer;
    dest->substr = NULL;
    dest->max = msgpackx_buffer_tree_get_max(dest, NULL);
    if (!msgpackx_buffer_insert(dest)) {
      return -1;
    }
    sz = n;

  } else {
    sz = msgpackx_buffer_size(dest);
    if (sz > n) {
      if (mode & MSGPACKX_COPY_SHRINK) {
        if (!msgpackx_buffer_resize_substr(dest, n + 1)) {
          return -1;
        }
        sz = n;
      }
    } else if (sz <= n) {
      if (mode & MSGPACKX_COPY_EXPAND) {
        if (!msgpackx_buffer_resize_substr(dest, n + 1)) {
          return -1;
        }
      } else {
        n = sz - 1;
      }
    }
  }
  vsnprintf(dest->pointer, n + 1, format, ap);
  if ((mode & (MSGPACKX_COPY_SHRINK | MSGPACKX_COPY_EXPAND))) {
    /* remove '\0' */
    msgpackx_buffer_object_shrink_space(obj, dest->pointer + n + 1, 1);
  }
  if (sz > n && (mode & MSGPACKX_COPY_ZERO)) {
    memset(dest->pointer + n, 0, sz - n);
  }
  return n;
}

char *msgpackx_buffer_tree_dump(msgpackx_buffer *dump, int use_offset,
                                const char *prefix)
{
  msgpackx_buffer *buffer;
  msgpackx_buffer *lp;
  int r;

  struct msgpackx_buffer_object *obj;
  int depth;
  int max_depth;
  struct geom_rbtree *cur;
  uintptr_t px;
  int sz, szs, szo;
  char *retp;

  buffer = msgpackx_buffer_new();
  if (!buffer) return NULL;

  obj = dump->obj;
  if (!obj) {
    r = msgpackx_buffer_printf(buffer, MSGPACKX_COPY_CREATE,
                               "%s(null object)\n", prefix);
    if (r < 0) goto error;
    goto fin;
  }
  if (!msgpackx_buffer_reserve(buffer, 1024)) goto error;

  lp = msgpackx_buffer_substr(buffer, 0, 0, MSGPACKX_SEEK_SET);
  if (!lp) goto error;

  depth = -1;
  max_depth = -1;
  szs = 0;
  geom_rbtree_foreach_succ_depth(cur, obj->root, depth) {
    msgpackx_buffer *bp;
    if (depth > max_depth) max_depth = depth;
    bp = msgpackx_buffer_tree_entry(cur);
    px = (uintptr_t)bp;
    sz = 0;
    while (px != 0) {
      px >>= 4;
      sz++;
    }
    if (sz > szs) szs = sz;
  }
  sz = 0;
  px = (uintptr_t)obj->buffer.substr;
  while (px != 0) {
    px >>= 4;
    sz++;
  }
  sz  += 2;
  szs += 2;
  if (sz  < 5)  sz = 5;
  if (szs < 5) szs = 5;

  if (use_offset) {
    szo = 0;
    px = obj->buffer.substr - obj->buffer.pointer;
    while (px != 0) {
      px /= 10;
      szo++;
    }
    if (szo < 5) szo = 5;
    szo++;
  }

  depth = -1;
  geom_rbtree_foreach_succ_depth(cur, obj->root, depth) {
    struct geom_rbtree *p, *parent;
    msgpackx_buffer *bp;
    char *bop;
    int i, j;

    bp = msgpackx_buffer_tree_entry(cur);
    bop = bp->obj->buffer.pointer;
    if (use_offset) {
      r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                 "%s%*p (%*" PRIdMAX, prefix, szs, bp,
                                 szo, (intmax_t)(bp->pointer - bop));
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
      if (bp->substr) {
        r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                   ",%*" PRIdMAX, szo,
                                   (intmax_t)(bp->substr - bop));
      } else {
        r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                   ",%*s", szo, "(nil)");
      }
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
      if (bp->max) {
        r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                   ") max %*" PRIdMAX " ",
                                   szo, (intmax_t)(bp->max - bop));
      } else {
        r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                   ") max %*s ",
                                   szo, "(nil)");
      }
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);

    } else {
      r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND,
                                 "%s%*p (%*p, %*p) max %*p ", prefix,
                                 szs, bp, sz, bp->pointer,
                                 sz, bp->substr, sz, bp->max);
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
    }

    for (i = 0; i < (max_depth - depth); ++i) {
      r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "  ");
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
    }
    r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "@");
    if (r < 0) goto error;
    msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);

    p = cur;
    for (; !geom_rbtree_is_root(p); p = parent) {
      parent = geom_rbtree_parent(p);
      if (p == cur) {
        if (geom_rbtree_left(parent) == p) {
          r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "-.");
          i = 1;
        } else {
          r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "-'");
          i = 0;
        }
        if (r < 0) goto error;
        msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
      } else {
        if (geom_rbtree_left(parent) == p) {
          j = 1;
        } else {
          j = 0;
        }
        if ((i && j) || (!i && !j)) {
          r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "  ");
        } else {
          r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, " |");
        }
        if (r < 0) goto error;
        msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
        i = j;
      }
    }
    if (p == cur) {
      r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "--");
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
    }
    if (bp == dump) {
      r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, " <-- ptr");
      if (r < 0) goto error;
      msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
    }
    r = msgpackx_buffer_printf(lp, MSGPACKX_COPY_EXPAND, "\n");
    if (r < 0) goto error;
    msgpackx_buffer_goto(lp, r, 0, MSGPACKX_SEEK_CUR);
  }

fin:
  retp = msgpackx_buffer_make_cstr(buffer);
  msgpackx_buffer_delete_all_referenced(buffer);
  return retp;

error:
  msgpackx_buffer_delete_all_referenced(buffer);
  return NULL;
}
