/*
 * YSE: ref: https://github.com/nyuichi/gc.h/blob/master/list.h
 *
 *   Reference source is licensed under 2-clause BSD license,
 *   we might need to obey its condition
 *    (retain copyright notice and license term here)
 */
/**
 * @file list.h
 * @brief General-purpose, double-linked list
 * @ingroup Geometry
 *
 * These functions provides double-linked list for any
 * struct type via embedding `struct geom_list`.
 */

#ifndef JUPITER_GEOMETRY_LIST_H
#define JUPITER_GEOMETRY_LIST_H

#include <stddef.h>

#include "util.h"

#ifdef __cplusplus
/*
 * WARNING: geom_list is not for use with C++.
 *          Highly recommend to use std::list<> instead.
 */
extern "C" {
#endif

/**
 * @brief General-purpose, double linked list
 * @ingroup Geometry
 *
 * Adds double-linked list feature in your struct.
 *
 * You should have 'head' entry which does not have any data. i.e.,
 * first list item should be put to next of 'head'. 'head' is used for
 * marker that list's begin/end.
 *
 *
 *     +-->[head]<-->[1]<-->[2]<-->[3]<--....-->[n]<--+
 *     |                                              |
 *     +----------------------------------------------+
 *
 *
 * The rule above is applied when you used `geom_list_foreach()`, which
 * will run the loop body for 'head' item. Deleting 'head' is *not*
 * errornous operation, but you will be stuck. Other operations are
 * all applicable to both actual items and the 'head'.
 *
 * You can refer the data from `geom_list` via
 * `geom_list_entry([ptr to geom_list], struct [type], [field name which is geom_list])`
 */
struct geom_list
{
  struct geom_list *next; /**< Next item */
  struct geom_list *prev; /**< Previous item */
};

/**
 * @brief Initialize list
 * @memberof geom_list
 * @param list list item to initialize
 *
 * Set next and previous item to itself.
 */
static inline void
geom_list_init(struct geom_list *list)
{
  list->next = list->prev = list;
}

/**
 * @brief Test list is empty
 * @memberof geom_list
 * @param head list head to test.
 * @return 1 if empty, 0 not empty
 *
 * Textually, should be applied for the list 'head'. If you call this
 * function for an list item (which `geom_list_init` is called), this
 * function will test whether that item is linked to another.
 */
static inline int
geom_list_empty(struct geom_list *head)
{
  return (head->prev == head);
}

/**
 * @brief Remove list entry
 * @memberof geom_list
 * @param entry list item to remove
 *
 * This function is used to define `geom_list_delete()`.
 *
 * @note To keep consistency, you should use `geom_list_delete()`
 *       instead.
 */
static inline void
geom_list_delete_no_init(struct geom_list *entry)
{
  entry->next->prev = entry->prev;
  entry->prev->next = entry->next;
}

/**
 * @brief Remove list entry
 * @memberof geom_list
 * @param entry list item to remove
 *
 * Removes `entry` from the list and clear the connection of `entry`.
 */
static inline void
geom_list_delete(struct geom_list *entry)
{
  geom_list_delete_no_init(entry);
  geom_list_init(entry);
}

/**
 * @brief Insert an item between two items.
 * @memberof geom_list
 * @param insert an item to be inserted
 * @param prev previous item
 * @param next next item
 *
 * `prev` and `next` must be adjucent.
 *
 * You should not call this function directly.
 */
static inline void
geom_list_insert_adjucent(struct geom_list *insert,
                          struct geom_list *prev, struct geom_list *next)
{
  next->prev = insert;
  insert->next = next;
  insert->prev = prev;
  prev->next = insert;
}

/**
 * @brief Insert item to next
 * @memberof geom_list
 * @param prev item to insert after
 * @param insert  insert item to insert
 *
 * If \p prev is 'head', this function will prepend \p insert to the
 * start of the list. Otherwise, this function will insert \p insert to
 * next of \p prev.
 *
 *     ...-->[prev]<-->[insert]<-->[original prev's next]<--...
 */
static inline void
geom_list_insert_next(struct geom_list *prev, struct geom_list *insert)
{
  geom_list_insert_adjucent(insert, prev, prev->next);
}

/**
 * @brief Insert item to previous
 * @memberof geom_list
 * @param next item to insert before
 * @param insert an item to be inserted
 *
 * If \p next is 'head', this function will append \p insert to the end
 * of the list. Otherwise, this function will insert \p insert to
 * previous of \p next.
 *
 *     ...-->[original next's prev]<-->[insert]<-->[next]<--...
 */
static inline void
geom_list_insert_prev(struct geom_list *next, struct geom_list *insert)
{
  geom_list_insert_adjucent(insert, next->prev, next);
}

/**
 * @brief Insert list to next
 * @memberof geom_list
 * @param prev item to insert after
 * @param insert a list to be inserted
 *
 * No 'head' assumed in the \p insert list. Therefore, list of \p insert
 * where
 *
 *     .-->[insert]<-->[1]<-->[2]<-->[3]<--....-->[n]<--.
 *     |                                                |
 *     '------------------------------------------------'
 *
 * will inserted as
 *
 *     ...-->[prev]<-->[insert]<-->[1]<-->[2]<--.
 *                                              |
 *       .--------------------------------------'
 *       |
 *       '-->[3]<--....-->[n]<-->[original prev's next]<--...
 *
 * If \p insert is a 'head', you may want to remove it after insertion:
 *
 *     geom_list_insert_list_next(prev, insert);
 *     geom_list_delete(insert);
 *
 */
static inline void
geom_list_insert_list_next(struct geom_list *prev, struct geom_list *insert)
{
  insert->prev->next = prev->next;
  prev->next->prev = insert->prev;
  prev->next = insert;
  insert->prev = prev;
}

/**
 * @brief Insert list to next
 * @memberof geom_list
 * @param next item to insert after
 * @param insert a list to be inserted
 *
 * No 'head' assumed in the \p insert list. Therefore, list of \p insert
 * where
 *
 *     .-->[insert]<-->[1]<-->[2]<-->[3]<--....-->[n]<--.
 *     |                                                |
 *     '------------------------------------------------'
 *
 * will inserted as
 *
 *     ...-->[original next's prev]<-->[insert]<-->[1]<-->[2]<--.
 *                                                              |
 *                 .--------------------------------------------'
 *                 |
 *                 '-->[3]<--....-->[n]<-->[next]<--...
 *
 * If \p insert is a 'head', you may want to remove it after insertion:
 *
 *     geom_list_insert_list_next(prev, insert);
 *     geom_list_delete(insert);
 *
 */
static inline void
geom_list_insert_list_prev(struct geom_list *next, struct geom_list *insert)
{
  geom_list_insert_list_next(next->prev, insert);
}

/**
 * @brief Get list entry
 * @relates geom_list
 * @param ptr  pointer to the list item
 * @param type base struct type name
 * @param field field (member) name of `type`
 * @return pointer to base struct
 */
#define geom_list_entry(ptr, type, field) \
  geom_container_of(ptr, type, field)

/**
 * @brief Get list next
 * @memberof geom_list
 * @param p list item to get.
 * @return next item.
 *
 * If p is end of the list, 'head' item will be returned.
 */
static inline struct geom_list *
geom_list_next(struct geom_list *p)
{
  return p->next;
}

/**
 * @brief Get list previous
 * @memberof geom_list
 * @param p list item to get.
 * @return previous item.
 *
 * If p is begin of the list, 'head' item will be returned.
 */
static inline struct geom_list *
geom_list_prev(struct geom_list *p)
{
  return p->prev;
}

/**
 * @brief Ranged list item foreach
 * @relates geom_list
 * @param p iterator variable
 * @param s start item (inclusive)
 * @param e end item (exclusive)
 */
#define geom_list_foreach_range(p, s, e)   \
  for ((p) = (s); (p) != (e); (p) = geom_list_next(p))

/**
 * @brief Ranged list item foreach, safe version
 * @relates geom_list
 * @param p iterator variable
 * @param n next iterator variable
 * @parem s start item (inclusive)
 * @param e end item (exclusive)
 *
 * Use this version when the loop modifies `p->next`.
 */
#define geom_list_foreach_range_safe(p, n, s, e)         \
  for ((p) = (s), (n) = geom_list_next(p);               \
       (p) != (e); (p) = (n), (n) = geom_list_next(p))

/**
 * @brief List item foreach
 * @relates geom_list
 * @param p iterator variable
 * @param head List head
 */
#define geom_list_foreach(p, head) \
  geom_list_foreach_range(p, geom_list_next(head), head)

/**
 * @brief List item foreach, safe version
 * @relates geom_list
 * @param p iterator variable
 * @param n next iterator variable
 * @param head List head
 *
 * Use this version when the loop modifies `p->next`.
 */
#define geom_list_foreach_safe(p, n, head)                          \
  geom_list_foreach_range_safe(p, n, geom_list_next(head), head)

/**
 * @brief Sorts list
 * @memberof geom_list
 * @param head head of the list
 * @param cmp Comparison function
 *
 * @p cmp should negative, 0 and positive value for la > lb, la == lb
 * and la < lb respectively.
 *
 * This function uses selection sort.
 */
static inline
void geom_list_sort(struct geom_list *head,
                    int (*cmp)(struct geom_list *la, struct geom_list *lb))
{
  struct geom_list *lp, *ln;
  geom_list_foreach(lp, head) {
    struct geom_list *min = lp;
    geom_list_foreach_range(ln, geom_list_next(lp), head) {
      if (cmp(min, ln) < 0) {
        min = ln;
      }
    }
    if (min != lp) {
      geom_list_delete_no_init(min);
      geom_list_insert_prev(lp, min);
      lp = min; /* to be geom_list_next(...) == lp */
    }
  }
}

/**
   @example "geometry/list.c"
   We consider a `struct foo` contains 1 `int` data,

   @code{c}
   struct foo {
     int a;
   };
   @endcode

   to be linked each other to form a list.

   First, add field of `struct geom_list` to the `struct foo`:

   @code{c}
   struct foo {
     struct geom_list list;
     int a;
   };
   @endcode

   (You can put `list` after the member `a`. No matter where `struct
   geom_list` is located, and, of course, multiple `struct
   geom_list`s in one `struct` is also acceptable to form
   double-linked list in two or more directions)

   (note that insert field of `struct geom_list` as a value, not
   a pointer)

   To get pointer of `struct geom_list`, it's done simply by taking
   its address:

   @code{c}
   struct foo obj;
   struct geom_list *ptr = &obj.list;

   geom_list_init(&obj.list);
   @endcode

   To get pointer of `struct foo` from `struct geom_list`, use
   `geom_list_entry()` macro:

   @code{c}
   struct foo *fooptr;
   struct geom_list *ptr = geom_list_*somewhat*(...);

   fooptr = geom_list_entry(ptr, struct foo, list);
   @endcode

   Here, the each arguments means that `ptr` was the item in `struct
   foo` and field of `list`. `geom_list_entry()` computes the offset
   of `ptr` and `fooptr` from given structure and field name (at
   compile time); C standard macro `offsetof()` does this trick. This
   is why you can put `struct geom_list` into any location of `struct
   foo` and/or multiple `struct geom_list`s.

   Complete example here:
 */

#ifdef __cplusplus
}
#endif

#endif
