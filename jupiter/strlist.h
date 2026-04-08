#ifndef JUPITER_STRLIST_H
#define JUPITER_STRLIST_H

#include "common.h"
#include "geometry/list.h"
#include "geometry/util.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This header file is valid only in C. (invalid in C++) */

struct jupiter_strlist_head
{
  struct geom_list list;
  const size_t len;
};
typedef struct jupiter_strlist_head jupiter_strlist_head;

struct jupiter_strlist
{
  struct jupiter_strlist_head node;
  char buf[];
};
typedef struct jupiter_strlist jupiter_strlist;

#define jupiter_strlist_head_entry(ptr) \
  geom_list_entry(ptr, struct jupiter_strlist_head, list)

JUPITER_DECL
jupiter_strlist *jupiter_strlist_node_new(size_t length);

/**
 * Remove @p node from a list (if member of any) and free the node
 */
JUPITER_DECL
void jupiter_strlist_free(jupiter_strlist *node);

static inline int jupiter_strlist__is_node(jupiter_strlist_head *h)
{
  return h->len != (size_t)-1;
}

static inline jupiter_strlist *jupiter_strlist_entry(struct geom_list *lp)
{
  jupiter_strlist_head *h;
  h = jupiter_strlist_head_entry(lp);
  if (jupiter_strlist__is_node(h))
    return geom_container_of(h, jupiter_strlist, node);
  return NULL;
}

static inline void jupiter_strlist_head_init(jupiter_strlist_head *h)
{
  geom_list_init(&h->list);
  *(size_t *)&h->len = (size_t)-1;
}

static inline int jupiter_strlist_is_empty(jupiter_strlist_head *h)
{
  return geom_list_empty(&h->list);
}

/**
 * Append given @p node at the end of @p head.
 *
 * @note If @p node is a member of a list, delete first using
 * jupiter_strlist_delete().
 */
static inline void jupiter_strlist_append(jupiter_strlist_head *head,
                                          jupiter_strlist *node)
{
  geom_list_insert_prev(&head->list, &node->node.list);
}

/**
 * Prepend given @p node at the start of @p head.
 *
 * @note If @p node is a member of a list, delete first using
 * jupiter_strlist_delete().
 */
static inline void jupiter_strlist_prepend(jupiter_strlist_head *head,
                                           jupiter_strlist *node)
{
  geom_list_insert_next(&head->list, &node->node.list);
}

/**
 * Insert given @p node at immediately before @p next.
 *
 * @note If @p node is member of a list, delete first using
 * jupiter_strlist_delete().
 */
static inline void jupiter_strlist_insert_prev(jupiter_strlist *next,
                                               jupiter_strlist *node)
{
  geom_list_insert_prev(&next->node.list, &node->node.list);
}

/**
 * Insert given @p node at immediately after @p prev.
 *
 * @note If @p node is member of a list, delete first using
 * jupiter_strlist_delete().
 */
static inline void jupiter_strlist_insert_next(jupiter_strlist *prev,
                                               jupiter_strlist *node)
{
  geom_list_insert_next(&prev->node.list, &node->node.list);
}

/**
 * Add all member of @p list to last of @p head.
 *
 * Entries of @p list will moved to @p head and removed from @p list.
 */
static inline void jupiter_strlist_append_list(jupiter_strlist_head *head,
                                               jupiter_strlist_head *list)
{
  geom_list_insert_list_prev(&head->list, &list->list);
  geom_list_delete(&list->list);
}

/**
 * Add all member of @p list to first of @p head.
 *
 * Entries of @p list will moved to @p head and removed from @p list.
 */
static inline void jupiter_strlist_prepend_list(jupiter_strlist_head *head,
                                                jupiter_strlist_head *list)
{
  geom_list_insert_list_next(&head->list, &list->list);
  geom_list_delete(&list->list);
}

/**
 * Insert all member of @p list before @p next.
 *
 * Entries of @p list will moved to @p next and removed from @p list.
 */
static inline void jupiter_strlist_insert_list_prev(jupiter_strlist *next,
                                                    jupiter_strlist_head *list)
{
  geom_list_insert_list_prev(&next->node.list, &list->list);
  geom_list_delete(&list->list);
}

/**
 * Insert all member of @p list after @p prev.
 *
 * Entries of @p list will moved to @p prev and removed from @p list.
 */
static inline void jupiter_strlist_insert_list_next(jupiter_strlist *prev,
                                                    jupiter_strlist_head *list)
{
  geom_list_insert_list_next(&prev->node.list, &list->list);
  geom_list_delete(&list->list);
}

/**
 * Returns the first node of list @p h, NULL if empty
 */
static inline jupiter_strlist *jupiter_strlist_first(jupiter_strlist_head *h)
{
  return jupiter_strlist_entry(geom_list_next(&h->list));
}

/**
 * Returns the last node of list @p h, NULL if empty
 */
static inline jupiter_strlist *jupiter_strlist_last(jupiter_strlist_head *h)
{
  return jupiter_strlist_entry(geom_list_prev(&h->list));
}

/**
 * Returns the next node of @p n, NULL if it is last node
 */
static inline jupiter_strlist *jupiter_strlist_next(jupiter_strlist *n)
{
  return jupiter_strlist_entry(geom_list_next(&n->node.list));
}

/**
 * Returns the previous node of @p n, NULL if it is first node
 */
static inline jupiter_strlist *jupiter_strlist_prev(jupiter_strlist *n)
{
  return jupiter_strlist_entry(geom_list_prev(&n->node.list));
}

#define jupiter_strlist_foreach(lp, head) \
  for (lp = jupiter_strlist_first(head); lp; lp = jupiter_strlist_next(lp))

#define jupiter_strlist_reverse_foreach(lp, head) \
  for (lp = jupiter_strlist_last(head); lp; lp = jupiter_strlist_prev(lp))

#define jupiter_strlist__foreach_safe_base(lp, ln, head, f, n) \
  for (lp = f(head), ln = n(lp); lp; lp = ln, ln = ln ? n(ln) : NULL)

#define jupiter_strlist_foreach_safe(lp, ln, head)                        \
  jupiter_strlist__foreach_safe_base(lp, ln, head, jupiter_strlist_first, \
                                     jupiter_strlist_next)

#define jupiter_strlist_reverse_foreach_safe(lp, ln, head)               \
  jupiter_strlist__foreach_safe_base(lp, ln, head, jupiter_strlist_last, \
                                     jupiter_strlist_prev)

#define jupiter_strlist__foreach_range_base(lp, ls, le, n) \
  for (lp = ls, le = n(le); lp && lp != le; lp = n(lp))

#define jupiter_strlist_foreach_range(lp, ls, le) \
  jupiter_strlist__foreach_range_base(lp, ls, le, jupiter_strlist_next)

#define jupiter_strlist_reverse_foreach_range(lp, ls, le) \
  jupiter_strlist__foreach_range_base(lp, ls, le, jupiter_strlist_prev)

#define jupiter_strlist__foreach_range_safe_base(lp, ln, ls, le, n) \
  for (lp = ls, ln = n(lp), le = n(le); lp && lp != le;             \
       lp = ln, ln = lp ? n(ln) : NULL)

#define jupiter_strlist_foreach_range_safe(lp, ln, ls, le) \
  jupiter_strlist__foreach_range_safe_base(lp, ln, ls, le, jupiter_strlist_next)

#define jupiter_strlist_reverse_foreach_range_safe(lp, ln, ls, le) \
  jupiter_strlist__foreach_range_safe_base(lp, ln, ls, le, jupiter_strlist_prev)

/**
 * Returns the list head of given node, NULL if not found (i.e., dangling
 * node)
 *
 * This function searches over the list, so this function has cost of O(N).
 */
static inline jupiter_strlist_head *
jupiter_strlist_get_head(jupiter_strlist *node)
{
  struct geom_list *lp, *lh;
  lh = &node->node.list;

  geom_list_foreach (lp, lh) {
    jupiter_strlist_head *h;
    h = jupiter_strlist_head_entry(lp);
    if (!jupiter_strlist__is_node(h))
      return h;
  }
  return NULL;
}

/**
 * Remove @p node from list
 *
 * @note @p node will not be freed. You can insert @p node at another location
 * of list or another list.
 *
 * @node Use jupiter_strlist_free() to free dangling nodes.
 */
static inline void jupiter_strlist_delete(jupiter_strlist *node)
{
  geom_list_delete(&node->node.list);
}

static inline void jupiter_strlist_free_all(jupiter_strlist_head *head)
{
  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe (lp, ln, lh) {
    jupiter_strlist *l = jupiter_strlist_entry(lp);
    if (!l)
      continue;

    jupiter_strlist_free(l);
  }
}

/**
 * Allocate new strlist node with copying given string.
 *
 * @note buffer allocated in size of @p string including NUL.
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_dup_s(const char *string);

/**
 * Allocate new strlist node with given node (aka. clone node).
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_dup_l(jupiter_strlist *l);

/**
 * Allocate new strlist node with C-style formatting
 *
 * @note buffer allocated in size of result string including NUL.
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_asprintf(const char *format, ...);

/**
 * Allocate new strlist node with C-style formatting
 *
 * @note buffer allocated in size of result string including NUL.
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_vasprintf(const char *format, va_list ap);

typedef int jupiter_strlist_foreach_func(jupiter_strlist *l, void *arg);

/**
 * @brief Iterate over specific range of string list
 * @param from start node (inclusive)
 * @param to   last node (inclusive)
 * @param func function to call
 * @param arg  extra function parameter(s)
 *
 * @p from and @p to must be a member of same list. However if @p from == @p to,
 * this function calls @p func for it even if it is not a member of any list
 * (i.e., dangling node).
 *
 * If the range @p from and @p to include 'head' (i.e., @p to is before @p
 * from), it will be just skipped.
 *
 * If you want to modify the list within @p func, use 'safe' version.
 *
 * If @p func return non-0 value, the loop will be aborted.
 */
static inline void
jupiter_strlist_foreach_range_f(jupiter_strlist *from, jupiter_strlist *to,
                                jupiter_strlist_foreach_func *func, void *arg)
{
  if (from == to) {
    func(from, arg);
  } else {
    struct geom_list *lp, *ls, *le;
    ls = &from->node.list;
    le = geom_list_next(&to->node.list);
    geom_list_foreach_range (lp, ls, le) {
      jupiter_strlist *l;
      l = jupiter_strlist_entry(lp);
      if (!l)
        continue;

      if (func(l, arg))
        break;
    }
  }
}

static inline void
jupiter_strlist_foreach_range_safe_f(jupiter_strlist *from, jupiter_strlist *to,
                                     jupiter_strlist_foreach_func *func,
                                     void *arg)
{
  if (from == to) {
    func(from, arg);
  } else {
    struct geom_list *lp, *ln, *ls, *le;
    ls = &from->node.list;
    le = geom_list_next(&to->node.list);
    geom_list_foreach_range_safe (lp, ln, ls, le) {
      jupiter_strlist *l;
      l = jupiter_strlist_entry(lp);
      if (!l)
        continue;

      if (func(l, arg))
        break;
    }
  }
}

/**
 * Perform distribute processing in node-basis strategy using OpenMP
 *
 * @param from start node (inclusive)
 * @param to   last node (inclusive)
 * @param func function to call in parallel
 * @param arg  Argument to func
 *
 * This function does not initiate parallel section. This function should be
 * called in a single section (synchronized or unsynchronzied) or in a task. @p
 * arg from calling thread will be passed, so @p arg cannot have threadprivate
 * resource of executing thread. If you want, you have to allocate an array of
 * pointer (data) for each threads indexed by omp_get_thread_num().
 *
 * The return value of @p func will be ignored.
 *
 * Tasks generated by this function are not guaranteed to be executed before
 * returning. Use barrier or synchronized single section to complete all tasks.
 *
 * @note a task will be generated even if @p from == @p to.
 */
static inline void
jupiter_strlist_foreach_range_p(jupiter_strlist *from, jupiter_strlist *to,
                                jupiter_strlist_foreach_func *func, void *arg)
{
  if (from == to) {
#ifdef _OPENMP
#pragma omp task
#endif
    func(from, arg);
  } else {
    struct geom_list *lp, *ls, *le;
    ls = &from->node.list;
    le = geom_list_next(&to->node.list);
    geom_list_foreach_range (lp, ls, le) {
      jupiter_strlist *l;

      l = jupiter_strlist_entry(lp);
      if (!l)
        continue;

#ifdef _OPENMP
#pragma omp task
#endif
      func(l, arg);
    }
  }
}

/**
 * Safe version of jupiter_strlist_foreach_range_p().
 */
static inline void
jupiter_strlist_foreach_range_safe_p(jupiter_strlist *from, jupiter_strlist *to,
                                     jupiter_strlist_foreach_func *func,
                                     void *arg)
{
  if (from == to) {
#ifdef _OPENMP
#pragma omp task
#endif
    func(from, arg);
  } else {
    struct geom_list *lp, *ln, *ls, *le;
    ls = &from->node.list;
    le = geom_list_next(&to->node.list);
    geom_list_foreach_range_safe (lp, ln, ls, le) {
      jupiter_strlist *l;

      l = jupiter_strlist_entry(lp);
      if (!l)
        continue;

#ifdef _OPENMP
#pragma omp task
#endif
      func(l, arg);
    }
  }
}

static inline void jupiter_strlist_foreach__all_f(
  jupiter_strlist_head *head, jupiter_strlist_foreach_func *func, void *arg,
  void (*range_func)(jupiter_strlist *from, jupiter_strlist *to,
                     jupiter_strlist_foreach_func *func, void *arg))
{
  jupiter_strlist *f, *t;
  f = jupiter_strlist_first(head);
  t = jupiter_strlist_last(head);
  if (f && t)
    range_func(f, t, func, arg);
}

static inline void
jupiter_strlist_foreach_all(jupiter_strlist_head *head,
                            jupiter_strlist_foreach_func *func, void *arg)
{
  jupiter_strlist_foreach__all_f(head, func, arg,
                                 jupiter_strlist_foreach_range_f);
}

static inline void
jupiter_strlist_foreach_all_safe(jupiter_strlist_head *head,
                                 jupiter_strlist_foreach_func *func, void *arg)
{
  jupiter_strlist_foreach__all_f(head, func, arg,
                                 jupiter_strlist_foreach_range_safe_f);
}

static inline void
jupiter_strlist_foreach_all_p(jupiter_strlist_head *head,
                              jupiter_strlist_foreach_func *func, void *arg)
{
  jupiter_strlist_foreach__all_f(head, func, arg,
                                 jupiter_strlist_foreach_range_p);
}

static inline void jupiter_strlist_foreach_all_safe_p(
  jupiter_strlist_head *head, jupiter_strlist_foreach_func *func, void *arg)
{
  jupiter_strlist_foreach__all_f(head, func, arg,
                                 jupiter_strlist_foreach_range_safe_p);
}

/**
 * Get allocated size of node
 */
static inline size_t jupiter_strlist_size(jupiter_strlist *node)
{
  return node->node.len;
}

static inline int jupiter_strlist__incl_size(jupiter_strlist *l, void *arg)
{
  *(size_t *)arg += jupiter_strlist_size(l);
  return 0;
}

static inline size_t jupiter_strlist_size_list(jupiter_strlist *from,
                                               jupiter_strlist *to)
{
  size_t sz = 0;
  jupiter_strlist_foreach_range_f(from, to, jupiter_strlist__incl_size, &sz);
  return sz;
}

static inline size_t jupiter_strlist_size_all(jupiter_strlist_head *head)
{
  size_t sz = 0;
  jupiter_strlist_foreach_all(head, jupiter_strlist__incl_size, &sz);
  return sz;
}

/**
 * Get nul-terminated string length
 *
 * @sa strnlen() (not in C standard, but in POSIX standard)
 *
 * If returned value is equal to jupiter_strlist_size(), the string is **not**
 * nul-terminated.
 */
JUPITER_DECL
size_t jupiter_strlist_length(jupiter_strlist *node);

JUPITER_DECL
size_t jupiter_strlist_length_list(jupiter_strlist *from, jupiter_strlist *to);

JUPITER_DECL
size_t jupiter_strlist_length_all(jupiter_strlist_head *head);

/**
 * strstr() for jupiter_strlist node (with support strings without nul).
 *
 * returns -1 if not found.
 *
 * This function uses Rabin-Karp algorithm. While there are many algorithms can
 * implement strstr(), so your libc's implementation may be faster.
 */
JUPITER_DECL
ptrdiff_t jupiter_strlist_strstr(jupiter_strlist *haystack, const char *needle);

JUPITER_DECL
ptrdiff_t jupiter_strlist_strstr_l(jupiter_strlist *haystack,
                                   jupiter_strlist *needle);

/**
 * strchr() for jupiter_strlist node (with support string without nul)
 */
JUPITER_DECL
ptrdiff_t jupiter_strlist_strchr(jupiter_strlist *haystack, char ch);

/**
 * Joins from @p from to @p to and return single node.
 * @p from and @p to is inclusive.
 *
 * Returned list node is not member of any list.
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_join_list(jupiter_strlist *from,
                                           jupiter_strlist *to,
                                           const char *sep);

JUPITER_DECL
jupiter_strlist *jupiter_strlist_join_all(jupiter_strlist_head *list,
                                          const char *sep);

typedef const char *jupiter_strlist_split_comp(const char **next, const char *p,
                                               size_t len, void *arg);

/**
 * Splist string of @p n with @p comp function
 *
 * @p comp expects functions like strstr() or strchr().
 *
 * @warning @p n must be nul-terminated, if @p comp uses function expecting
 * nul-terminated string, such as strstr() or strchr().
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_split_f(jupiter_strlist *n,
                                         jupiter_strlist_split_comp *comp,
                                         void *arg);

/**
 * Split @p n into nodes, using strstr(), replaces list @p n.
 *
 * @warning The string in @p n must be null-terminated.
 */
JUPITER_DECL
jupiter_strlist *jupiter_strlist_split_s(jupiter_strlist *n, const char *pat);

JUPITER_DECL
jupiter_strlist *jupiter_strlist_split_ch(jupiter_strlist *n, char ch);

#ifdef __cplusplus
}
#endif

#endif
