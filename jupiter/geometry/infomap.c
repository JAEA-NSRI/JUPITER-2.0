
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geom_assert.h"
#include "defs.h"
#include "infomap.h"
#include "variant.h"
#include "list.h"

/**
 * @ingroup Geometry
 * @brief information map
 *
 * An information entry contains description, unit and a value.
 */
struct geom_info_map
{
  struct geom_list list;
  const char *description;
  const char *unit;
  geom_variant *value;
  char buf[];
};

#define geom_info_map_entry(ptr) \
  geom_list_entry(ptr, struct geom_info_map, list)

geom_info_map *geom_info_map_new(geom_error *e)
{
  geom_info_map *map;
  map = (geom_info_map *)malloc(sizeof(struct geom_info_map));
  if (!map) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  geom_list_init(&map->list);
  map->value = NULL;
  map->unit = NULL;
  map->description = NULL;
  return map;
}

int geom_info_map_is_head(geom_info_map *map)
{
  GEOM_ASSERT(map);

  return !(map->value);
}

static
void geom_info_map_delete(geom_info_map *map)
{
  if (!map) return;

  geom_list_delete(&map->list);

  if (map->value) {
    geom_variant_delete(map->value);
  }
  free(map);
}

void geom_info_map_delete_item(geom_info_map *map)
{
  if (!map) return;

  GEOM_ASSERT(!geom_info_map_is_head(map));

  geom_info_map_delete(map);
}

void geom_info_map_delete_all(geom_info_map *head)
{
  struct geom_list *lhead;
  struct geom_list *p;
  struct geom_list *n;
  geom_info_map *ip;

  if (!head) return;

  GEOM_ASSERT(geom_info_map_is_head(head));

  lhead = &head->list;
  geom_list_foreach_safe(p, n, lhead) {
    ip = geom_info_map_entry(p);
    geom_info_map_delete(ip);
  }
  geom_info_map_delete(head);
}

geom_info_map *
geom_info_map_append(geom_info_map *head, const geom_variant *value,
                     const char *description, const char *unit,
                     geom_error *e)
{
  ptrdiff_t sz;
  ptrdiff_t dsz;
  ptrdiff_t usz;
  geom_info_map *newp;
  char *cploc;

  GEOM_ASSERT(head);

  sz = 0;
  if (description) {
    dsz = strlen(description);
    dsz += 1;
    sz += dsz;
    if (sz <= 0 || dsz <= 0) {
      if (e) *e = GEOM_ERR_OVERFLOW;
      return NULL;
    }
  }
  if (unit) {
    usz = strlen(unit);
    usz += 1;
    sz += usz;
    if (sz <= 0 || usz <= 0) {
      if (e) *e = GEOM_ERR_OVERFLOW;
      return NULL;
    }
  }

  newp = (geom_info_map *)malloc(sizeof(struct geom_info_map) + sz);
  if (!newp) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  newp->value = geom_variant_dup(value, e);
  if (!newp->value) {
    free(newp);
    return NULL;
  }

  cploc = newp->buf;
  if (description) {
    strncpy(cploc, description, dsz);
    newp->description = cploc;
    cploc += dsz;
    *(cploc - 1) = '\0';
  } else {
    newp->description = NULL;
  }
  if (unit) {
    strncpy(cploc, unit, usz);
    newp->unit = cploc;
    cploc += usz;
    *(cploc - 1) = '\0';
  } else {
    newp->unit = NULL;
  }

  geom_list_insert_prev(&head->list, &newp->list);

  return newp;
}

geom_info_map *geom_info_map_next(geom_info_map *p)
{
  GEOM_ASSERT(p);

  return geom_info_map_entry(geom_list_next(&p->list));
}

geom_info_map *geom_info_map_prev(geom_info_map *p)
{
  GEOM_ASSERT(p);

  return geom_info_map_entry(geom_list_prev(&p->list));
}

const char *geom_info_map_get_description(geom_info_map *p)
{
  GEOM_ASSERT(p);

  return p->description;
}

const char *geom_info_map_get_unit(geom_info_map *p)
{
  GEOM_ASSERT(p);

  return p->unit;
}

const geom_variant *geom_info_map_get_value(geom_info_map *p)
{
  GEOM_ASSERT(p);

  return p->value;
}

struct geom_info_map_unit_token {
  struct geom_list list;
  const char *sp;
  const char *ep;
  enum geom_info_map_unit_token_type {
    TOKEN_UNIT,
    TOKEN_NUMBER,
    TOKEN_OP,
    TOKEN_HEAD,
    TOKEN_PAREN,
  } type;
};
#define geom_info_map_unit_token_entry(ptr) \
  geom_list_entry(ptr, struct geom_info_map_unit_token, list)

static const char *
geom_info_map_unit_token(const char *p, enum geom_info_map_unit_token_type *t)
{
  char ch;
  const char *m, *d;
  m = NULL;
  d = NULL;

  if (t) *t = TOKEN_HEAD;
  while ((ch = *(p++)) != '\0') {
    switch(ch) {
    case ' ': case '\t':
      if (m) return m;
      if (d) return d;
      continue;

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
      if (t) {
        *t = TOKEN_UNIT;
        t = NULL;
      }
      m = p;
      continue;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (t) {
        *t = TOKEN_NUMBER;
        t = NULL;
      }
      d = p;
      continue;
    case '[': case ']': case '{': case '}': case '(': case ')':
      if (t) {
        *t = TOKEN_PAREN;
        t = NULL;
      }
      break;
    default:
      if (t) {
        *t = TOKEN_OP;
        t = NULL;
      }
      break;
    }
    break;
  }
  if (m) return m;
  if (d) return d;
  return p;
}

static void
geom_info_map_unit_token_init(struct geom_info_map_unit_token *p)
{
  GEOM_ASSERT(p);

  geom_list_init(&p->list);
  p->sp = NULL;
  p->ep = NULL;
  p->type = TOKEN_HEAD;
}

static struct geom_info_map_unit_token *
geom_info_map_unit_token_new(void)
{
  struct geom_info_map_unit_token *p;
  p = (struct geom_info_map_unit_token *)
    malloc(sizeof(struct geom_info_map_unit_token));
  if (!p) return NULL;

  geom_info_map_unit_token_init(p);
  return p;
}

static void
geom_info_map_unit_token_delete(struct geom_info_map_unit_token *p)
{
  if (!p) return;

  geom_list_delete(&p->list);
  free(p);
}

static void
geom_info_map_unit_token_delete_all(struct geom_info_map_unit_token *head)
{
  struct geom_info_map_unit_token *pp;
  struct geom_list *p, *n, *h;

  h = &head->list;
  geom_list_foreach_safe(p, n, h) {
    pp = geom_info_map_unit_token_entry(p);
    geom_info_map_unit_token_delete(pp);
  }
}

static struct geom_info_map_unit_token *
geom_info_map_unit_token_next(struct geom_info_map_unit_token *p)
{
  return geom_info_map_unit_token_entry(geom_list_next(&p->list));
}

static struct geom_info_map_unit_token *
geom_info_map_unit_token_prev(struct geom_info_map_unit_token *p)
{
  return geom_info_map_unit_token_entry(geom_list_prev(&p->list));
}

static void
geom_info_map_unit_token_set(struct geom_info_map_unit_token *p,
                             const char *sp, const char *ep,
                             enum geom_info_map_unit_token_type type)
{
  GEOM_ASSERT(p);
  GEOM_ASSERT(sp);

  if (!ep) {
    ep = sp + strlen(sp);
  }
  p->sp = sp;
  p->ep = ep;
  p->type = type;
}

static struct geom_info_map_unit_token *
geom_info_map_unit_token_insert_next(struct geom_info_map_unit_token *prev,
                                     const char *sp, const char *ep,
                                     enum geom_info_map_unit_token_type type)
{
  struct geom_info_map_unit_token *p;
  p = geom_info_map_unit_token_new();
  if (!p) return NULL;

  geom_info_map_unit_token_set(p, sp, ep, type);
  geom_list_insert_next(&prev->list, &p->list);
  return p;
}

static struct geom_info_map_unit_token *
geom_info_map_unit_token_insert_prev(struct geom_info_map_unit_token *next,
                                     const char *sp, const char *ep,
                                     enum geom_info_map_unit_token_type type)
{
  struct geom_info_map_unit_token *p;
  p = geom_info_map_unit_token_new();
  if (!p) return NULL;

  geom_info_map_unit_token_set(p, sp, ep, type);
  geom_list_insert_prev(&next->list, &p->list);
  return p;
}

static int
geom_info_map_unit_token_cmp(struct geom_info_map_unit_token *token,
                             const char *str)
{
  ptrdiff_t n, m;
  n = strlen(str) + 1;
  m = token->ep - token->sp;
  if (m < n) {
    n = m;
  }
  if (n < 0) {
    return 0;
  }
  return strncmp(token->sp, str, n);
}

int
geom_info_map_convert_unit(char **buf, const char *base,
                           const char *length_unit, const char *geometry_expr)
{
  char *bb;
  char *bp;
  struct geom_info_map_unit_token head;
  struct geom_info_map_unit_token *cursor;
  struct geom_info_map_unit_token *tp;
  enum geom_info_map_unit_token_type type;
  struct geom_list *lp, *ln;
  geom_error err;
  int no_unit_base;
  int single_token_base;
  int r;
  const char *p, *e;

  GEOM_ASSERT(buf);
  GEOM_ASSERT(geometry_expr);

  geom_info_map_unit_token_init(&head);

  err = GEOM_SUCCESS;
  r = -1;

  no_unit_base = 0;
  single_token_base = 1;
  if (!base || base[0] == '\0') {
    base = "1";
    no_unit_base = 1;
  } else if (strcmp(base, "1") == 0) {
    no_unit_base = 1;
  } else {
    e = geom_info_map_unit_token(base, NULL);
    if (*e != '\0') {
      single_token_base = 0;
    }
  }

  p = geometry_expr;
  while (*p != '\0') {
    e = geom_info_map_unit_token(p, &type);
    tp = geom_info_map_unit_token_insert_prev(&head, p, e, type);
    if (!tp) goto clean;
    p = e;
  }

  geom_list_foreach_safe(lp, ln, &head.list) {
    int no_unit, single_token;
    const char *ins;
    cursor = geom_info_map_unit_token_entry(lp);
    if (cursor->type != TOKEN_UNIT) continue;
    ins = NULL;
    if (geom_info_map_unit_token_cmp(cursor, "I") == 0) {
      ins = base;
      no_unit = no_unit_base;
      single_token = single_token_base;
    } else if (geom_info_map_unit_token_cmp(cursor, "L") == 0) {
      ins = length_unit;
      no_unit = 0;
      single_token = 1;
    }
    if (ins) {
      struct geom_info_map_unit_token *pr, *nx;
      pr = geom_info_map_unit_token_prev(cursor);
      nx = geom_info_map_unit_token_next(cursor);
      if (no_unit) {
        if (pr->type != TOKEN_OP && nx->type == TOKEN_OP &&
            geom_info_map_unit_token_cmp(nx, "/") == 0) {
          tp = geom_info_map_unit_token_insert_prev(cursor, ins, NULL,
                                                    TOKEN_NUMBER);
          if (!tp) goto clean;
        } else {
          if (pr->type == TOKEN_OP) {
            if (geom_info_map_unit_token_cmp(pr, "/") != 0) {
              geom_info_map_unit_token_delete(pr);
            } else {
              if (nx->type == TOKEN_OP) {
                geom_info_map_unit_token_delete(nx);
                ln = geom_list_next(lp);
                nx = &head;
              }
            }
          }
        }
        if (nx->type == TOKEN_NUMBER) {
          geom_info_map_unit_token_delete(nx);
          ln = geom_list_next(lp);
        }
      } else {
        tp = geom_info_map_unit_token_insert_prev(cursor, ins, NULL,
                                                  TOKEN_UNIT);
        if (!tp) goto clean;

        if (!single_token && (pr != &head || nx != &head)) {
          tp = geom_info_map_unit_token_insert_prev(tp, "(", NULL,
                                                    TOKEN_PAREN);
          if (!tp) goto clean;

          tp = geom_info_map_unit_token_insert_prev(cursor, ")", NULL,
                                                    TOKEN_PAREN);
          if (!tp) goto clean;
        }
      }
      geom_info_map_unit_token_delete(cursor);
      continue;
    }
  }
  cursor = geom_info_map_unit_token_next(&head);
  while (cursor->type == TOKEN_OP) {
    tp = geom_info_map_unit_token_next(cursor);
    geom_info_map_unit_token_delete(cursor);
    cursor = tp;
  }
  cursor = geom_info_map_unit_token_prev(&head);
  while (cursor->type == TOKEN_OP) {
    tp = geom_info_map_unit_token_prev(cursor);
    geom_info_map_unit_token_delete(cursor);
    cursor = tp;
  }

  r = 0;
  geom_list_foreach(lp, &head.list) {
    cursor = geom_info_map_unit_token_entry(lp);
    r += cursor->ep - cursor->sp;
  }
  r++; /* for '\0' */
  if (r <= 0) {
    r = -1;
    goto clean;
  }

  bb = (char *)malloc(sizeof(char) * r);
  if (!bb) {
    r = -1;
    goto clean;
  }

  bp = bb;
  r = 0;
  geom_list_foreach(lp, &head.list) {
    int n;
    cursor = geom_info_map_unit_token_entry(lp);
    n = cursor->ep - cursor->sp;
    strncpy(bp, cursor->sp, n);
    r += n;
    bp = bb + r;
  }
  *bp = '\0';
  *buf = bb;

 clean:
  geom_info_map_unit_token_delete_all(&head);
  return r;
}
