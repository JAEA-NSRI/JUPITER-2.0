#include "strlist.h"
#include "csvutil.h"
#include "geometry/list.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

jupiter_strlist *jupiter_strlist_node_new(size_t length)
{
  jupiter_strlist *l;

  if (length == (size_t)-1)
    return NULL;

  l = (jupiter_strlist *)malloc(sizeof(jupiter_strlist) +
                                length * sizeof(l->buf[0]));
  if (!l)
    return NULL;

  jupiter_strlist_head_init(&l->node);
  *(size_t *)&l->node.len = length;
  l->buf[0] = '\0';
  return l;
}

void jupiter_strlist_free(jupiter_strlist *node)
{
  jupiter_strlist_delete(node);
  free(node);
}

jupiter_strlist *jupiter_strlist_dup_s(const char *string)
{
  int l = strlen(string) + 1;
  jupiter_strlist *n = jupiter_strlist_node_new(l);
  if (!n)
    return NULL;
  strcpy(n->buf, string);
  return n;
}

jupiter_strlist *jupiter_strlist_dup_l(jupiter_strlist *l)
{
  jupiter_strlist *n;
  n = jupiter_strlist_node_new(l->node.len);
  if (!n)
    return NULL;
  if (n->node.len != (size_t)-1 && n->node.len > 0)
    memcpy(n->buf, l->buf, n->node.len);
  return n;
}

jupiter_strlist *jupiter_strlist_asprintf(const char *format, ...)
{
  jupiter_strlist *l;
  va_list ap;
  va_start(ap, format);
  l = jupiter_strlist_vasprintf(format, ap);
  va_end(ap);
  return l;
}

jupiter_strlist *jupiter_strlist_vasprintf(const char *format, va_list ap)
{
#if defined(_BSD_SOURCE) ||                                   \
  (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) ||         \
  defined(_ISOC99_SOURCE) ||                                  \
  (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) || \
  defined(_UCRT) ||                                           \
  (defined(_MSC_VER) && _MSC_VER >= 1900) /* Visual Studio 2015 */
  /*
   * C99 supported platforms, POSIX 2001.12, Windows Universal CRT and
   * VS2015 or later.
   *
   * [CAUTION]: Some (v)snprintf implementations behave in different way
   *            from specified by C99, with ***same prototype***.
   */

  va_list c;
  jupiter_strlist *p;
  int n;
  if (!format)
    return NULL;
  va_copy(c, ap);
  n = vsnprintf(NULL, 0, format, ap);
  if (n < 0 || (n > 0 && n > SIZE_MAX - 1)) {
    va_end(c);
    return NULL;
  }
  p = jupiter_strlist_node_new(n + 1);
  if (!p) {
    va_end(c);
    return NULL;
  }
  n = vsnprintf(p->buf, n + 1, format, c);
  va_end(c);
  return p;

#else
  char *p;
  int n;
  jupiter_strlist *l;
  n = jupiter_vasprintf(&p, format, ap);
  if (n < 0)
    return NULL;
  if (n > 0 && n > SIZE_MAX - 1) {
    free(p);
    return NULL;
  }
  l = jupiter_strlist_node_new(n + 1);
  if (!l) {
    free(p);
    return NULL;
  }
  return l;
#endif
}

size_t jupiter_strlist_length(jupiter_strlist *node)
{
  const char *l = memchr(node->buf, '\0', node->node.len);
  return l ? l - node->buf : node->node.len;
}

static int jupiter_strlist__incl_length(jupiter_strlist *l, void *arg)
{
  *(size_t *)arg += jupiter_strlist_length(l);
  return 0;
}

size_t jupiter_strlist_length_list(jupiter_strlist *from, jupiter_strlist *to)
{
  size_t sz = 0;
  jupiter_strlist_foreach_range_f(from, to, jupiter_strlist__incl_length, &sz);
  return sz;
}

size_t jupiter_strlist_length_all(jupiter_strlist_head *head)
{
  size_t sz = 0;
  jupiter_strlist_foreach_all(head, jupiter_strlist__incl_length, &sz);
  return sz;
}

static const unsigned int hash_base = 256;
static const unsigned int hash_modulus = 101;

static unsigned int jupiter_strlist_hash_c(const char *needle, unsigned int hsh,
                                           size_t len, size_t *lenout)
{
  size_t i;
  for (i = 0; i < len && *needle; ++i, ++needle)
    hsh = (hsh + (unsigned char)*needle) * hash_base % hash_modulus;
  if (lenout)
    *lenout = i;
  return hsh;
}

static unsigned int jupiter_strlist_hash(const char *needle, size_t len,
                                         size_t *lenout)
{
  return jupiter_strlist_hash_c(needle, 0, len, lenout);
}

ptrdiff_t jupiter_strlist_strstr(jupiter_strlist *haystack, const char *needle)
{
  int nlen;
  size_t hlen;
  unsigned int nhsh, hhsh;
  nlen = strlen(needle);
  if (nlen <= 0)
    return -1;

  hlen = jupiter_strlist_length(haystack);
  if (hlen < nlen)
    return -1;

  nhsh = jupiter_strlist_hash(needle, nlen, NULL);
  for (size_t i = 0; i < hlen - nlen + 1; ++i) {
    size_t j;

    hhsh = jupiter_strlist_hash(&haystack->buf[i], nlen, NULL);
    if (nhsh != hhsh)
      continue;

    for (j = 0; j < nlen; ++j)
      if (haystack->buf[i + j] != needle[j])
        break;
    if (j == nlen)
      return i;
  }
  return -1;
}

ptrdiff_t jupiter_strlist_strstr_l(jupiter_strlist *haystack,
                                   jupiter_strlist *needle)
{
  size_t nlen, hlen;
  unsigned int nhsh, hhsh;

  hlen = jupiter_strlist_length(haystack);
  nhsh = jupiter_strlist_hash(needle->buf, needle->node.len, &nlen);
  if (hlen < nlen)
    return -1;

  for (size_t i = 0; i < hlen - nlen + 1; ++i) {
    size_t j;

    hhsh = jupiter_strlist_hash(&haystack->buf[i], nlen, NULL);
    if (nhsh != hhsh)
      continue;

    for (j = 0; j < nlen; ++j)
      if (haystack->buf[i + j] != needle->buf[j])
        break;
    if (j == nlen)
      return i;
  }
  return -1;
}

ptrdiff_t jupiter_strlist_strchr(jupiter_strlist *haystack, char ch)
{
  for (ptrdiff_t i = 0; i < haystack->node.len; ++i) {
    char chb = haystack->buf[i];
    if (chb == '\0')
      return -1;
    if (chb == ch)
      return i;
  }
  return -1;
}

struct jupiter_strlist_join_data
{
  size_t n;
  int lsep;
  const char *sep;
  char *buf;
};

static int jupiter_strlist__incl_join_size(jupiter_strlist *l, void *arg)
{
  size_t nl;
  struct jupiter_strlist_join_data *d;
  d = (struct jupiter_strlist_join_data *)arg;

  if (d->n != 0 && d->lsep > 0) {
    if (SIZE_MAX - d->lsep < d->n) {
      d->n = (size_t)-1;
      return 1;
    }
    d->n += d->lsep;
  }

  nl = jupiter_strlist_length(l);
  if (SIZE_MAX - nl < d->n) {
    d->n = (size_t)-1;
    return 1;
  }
  d->n += nl;
  return 0;
}

static int jupiter_strlist__perform_join(jupiter_strlist *l, void *arg)
{
  const char *s;
  struct jupiter_strlist_join_data *d;
  d = (struct jupiter_strlist_join_data *)arg;

  if (d->n != 0 && d->lsep > 0) {
    s = d->sep;
    while ((*d->buf++ = *s++))
      /* nop */;
    --d->buf;
  }

  s = l->buf;
  for (size_t i = 0; i < l->node.len; ++i) {
    if (*s == '\0')
      break;
    *d->buf++ = *s++;
  }

  d->n = 1;
  return 0;
}

static size_t jupiter_strlist_join_base(jupiter_strlist *from,
                                        jupiter_strlist *to, const char *sep,
                                        char *(*alloc)(size_t len, void *a),
                                        void *arg)
{
  char *buf;
  struct jupiter_strlist_join_data d = {
    .n = 0,
    .lsep = sep ? strlen(sep) : 0,
    .sep = sep,
    .buf = NULL,
  };

  jupiter_strlist_foreach_range_f(from, to, jupiter_strlist__incl_join_size,
                                  &d);
  if (d.n == (size_t)-1)
    return d.n;

  if (SIZE_MAX - 1 < d.n)
    return (size_t)-1;
  d.n += 1;

  buf = alloc(d.n, arg);
  if (!buf)
    return (size_t)-1;

  d.buf = buf;
  d.n = 0;
  jupiter_strlist_foreach_range_f(from, to, jupiter_strlist__perform_join, &d);
  *d.buf = '\0';

  return d.buf - buf;
}

struct list_out_data
{
  jupiter_strlist *lp;
};

static char *list_alloc(size_t len, void *a)
{
  struct list_out_data *p;
  p = (struct list_out_data *)a;
  p->lp = jupiter_strlist_node_new(len);
  if (!p->lp)
    return NULL;
  return p->lp->buf;
}

jupiter_strlist *jupiter_strlist_join_list(jupiter_strlist *from,
                                           jupiter_strlist *to, const char *sep)
{
  size_t n;
  struct list_out_data l = {NULL};
  n = jupiter_strlist_join_base(from, to, sep, list_alloc, &l);
  if (n == (size_t)-1)
    return NULL;
  return l.lp;
}

jupiter_strlist *jupiter_strlist_join_all(jupiter_strlist_head *list,
                                          const char *sep)
{
  jupiter_strlist *f, *t;
  f = jupiter_strlist_first(list);
  t = jupiter_strlist_last(list);
  if (f && t)
    return jupiter_strlist_join_list(f, t, sep);
  return NULL;
}

jupiter_strlist *jupiter_strlist_split_f(jupiter_strlist *n,
                                         jupiter_strlist_split_comp *comp,
                                         void *arg)
{
  const char *p, *next, *r;
  int delete_n = 0;
  jupiter_strlist_head lh;
  jupiter_strlist_head_init(&lh);

  p = n->buf;
  r = comp(&next, p, n->node.len, arg);
  if (!r)
    return n;

  for (; r; p = next, r = comp(&next, p, n->node.len - (p - n->buf), arg)) {
    jupiter_strlist *l;
    size_t sz = r - p + 1;

    l = jupiter_strlist_node_new(sz);
    if (!l) {
      jupiter_strlist_free_all(&lh);
      return NULL;
    }

    memcpy(l->buf, p, sz - 1);
    l->buf[sz - 1] = '\0';
    jupiter_strlist_append(&lh, l);
  }
  if (*p == '\0') {
    delete_n = 1;
  } else {
    memmove(n->buf, p, strlen(p) + 1);
  }

  geom_list_insert_list_prev(&n->node.list, &lh.list);

  if (delete_n)
    jupiter_strlist_free(n);

  n = jupiter_strlist_entry(geom_list_next(&lh.list));

  geom_list_delete(&lh.list);
  return n;
}

struct split_s_data
{
  const char *pat;
  int lpat;
};

static const char *split_s_comp(const char **next, const char *s, size_t l,
                                void *arg)
{
  const char *r;
  struct split_s_data *d = (struct split_s_data *)arg;
  r = strstr(s, d->pat);
  *next = r ? r + d->lpat : NULL;
  return r;
}

jupiter_strlist *jupiter_strlist_split_s(jupiter_strlist *n, const char *pat)
{
  struct split_s_data d = {pat};

  if (!pat || *pat == '\0')
    return n;

  d.lpat = strlen(pat);

  return jupiter_strlist_split_f(n, split_s_comp, &d);
}

struct split_ch_data
{
  char ch;
};

static const char *split_ch_comp(const char **next, const char *s, size_t l,
                                 void *arg)
{
  struct split_ch_data *d = (struct split_ch_data *)arg;
  const char *r;
  r = strchr(s, d->ch);
  *next = r ? r + 1 : NULL;
  return r;
}

jupiter_strlist *jupiter_strlist_split_ch(jupiter_strlist *n, char ch)
{
  struct split_ch_data d = {ch};
  return jupiter_strlist_split_f(n, split_ch_comp, &d);
}
