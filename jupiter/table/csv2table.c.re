/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */
/* Converts CSV to Binary data */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "table.h"
#include "csv2table.h"

struct csv2tab_parser
{
  char *buf;
  size_t bufsz;
  const char *cursor;
  const char *marker;
  const char *ctxmarker;
  const char *limit;
  const char *token;
  const char *lexeme;
  long line;
  long column;
  int eof;
};
typedef struct csv2tab_parser csv2tab_parser;

/**
 * @brief single node of csv2tab data
 */
struct csv2tab_node {
  double x;  ///< X (first independent) value
  double y;  ///< Y (second independent) value
  double v;  ///< Result (dependent) value
  long src_line; ///< Line number of source input where defines this entry
  csv2tab_stat_node *stat;
  csv2tab_node *xnext;
  csv2tab_node *ynext;
  csv2tab_node *xprev;
  csv2tab_node *yprev;
  csv2tab_node *next; /* Allocation next */
  csv2tab_node *prev; /* Allocation prev */
};

struct csv2tab_data {
  int xcol;
  int ycol;
  int vcol;
  double tol;
  int adjust_coordinate;
  int interpolate;
  table_geometry force_geom;
  csv2tab_node *root;
  csv2tab_stat_node *stats;
};

struct csv2tab_stat_node {
  csv2tab_status stat;
  csv2tab_node *node;
  csv2tab_stat_node *next;
};

enum csv2tab_add_mode {
  CSV2TAB_ADD_X,
  CSV2TAB_ADD_Y,
  CSV2TAB_ADD_V,
};
typedef enum csv2tab_add_mode csv2tab_add_mode;

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:indent:string = "  ";
  re2c:define:YYCURSOR = "p.cursor";
  re2c:define:YYMARKER = "p.marker";
  re2c:define:YYCTXMARKER = "p.ctxmarker";
  re2c:define:YYLIMIT = "p.limit";
*/

static csv2tab_stat_node *csv2tab_stat_new(csv2tab_stat_node **root)
{
  csv2tab_stat_node *v;

  TABLE_ASSERT(root);

  v = (csv2tab_stat_node *)calloc(sizeof(csv2tab_stat_node), 1);
  if (!v) return NULL;

  v->stat = CSV2TAB_STAT_CLEAR;
  v->node = NULL;

  v->next = *root;
  *root = v;
  return v;
}

static int csv2tab_set_status_flag(csv2tab_node *node,
                                   csv2tab_stat_node **sroot,
                                   csv2tab_status flag, csv2tab_error *e)
{
  csv2tab_stat_node *sn;

  TABLE_ASSERT(node);

  sn = node->stat;
  if (!sn) {
    sn = csv2tab_stat_new(sroot);
    if (!sn) {
      if (e) *e = CSV2TAB_ERR_NOMEM;
      return 1;
    }
  }
  sn->stat |= flag;
  sn->node = node;
  node->stat = sn;
  return 0;
}

static int csv2tab_fill(FILE *input, csv2tab_parser *p, int lcount, csv2tab_error *e)
{
  char *l;
  const char *cl;
  enum {
    BUFSIZE = 32
  };
  char *nbuf;
  size_t reqs;
  size_t nsz;
  size_t rsz;
  size_t bsz;
  ptrdiff_t loff;
  ptrdiff_t noff;
  size_t ls;

  TABLE_ASSERT(input);
  TABLE_ASSERT(p);

  reqs = (BUFSIZE < lcount) ? lcount : BUFSIZE;
  if (p->buf && p->cursor) {
    cl = p->cursor;
    if (p->marker && p->marker < cl) cl = p->marker;
    if (p->ctxmarker && p->ctxmarker < cl) cl = p->ctxmarker;
    if (p->token && p->token < cl) cl = p->token;
    if (p->lexeme && p->lexeme < cl) cl = p->lexeme;
    loff = cl - p->buf;
    l = p->buf + loff;
    ls = p->limit - l;
    reqs += ls + 1;
  } else { /* new */
    p->bufsz = 0;
    l = NULL;
    loff = 0;
    ls = 0;
  }

  nsz = 0;
  if (!p->buf || p->bufsz < reqs) {
    nbuf = (char *)realloc(p->buf, sizeof(char) * reqs);
    if (!nbuf) {
      if (e) *e = CSV2TAB_ERR_NOMEM;
      return 1;
    }
    nsz = reqs;
  } else {
    nbuf = p->buf;
    nsz = p->bufsz;
  }
  if (loff > 0 && ls > 0) {
    l = nbuf + loff;
    memmove(nbuf, l, ls);
  }

  l = nbuf + ls;
  bsz = reqs - ls;
  if (p->eof) {
    rsz = 0;
  } else {
    rsz = fread(l, sizeof(char), bsz, input);
  }
  if (rsz < bsz) {
    p->eof = 1;
  }

  noff = nbuf - p->buf - loff;
  if (p->cursor) p->cursor = p->cursor + noff;
  if (p->marker) p->marker = p->marker + noff;
  if (p->ctxmarker) p->ctxmarker = p->ctxmarker + noff;
  if (p->token) p->token = p->token + noff;
  if (p->lexeme) p->lexeme = p->lexeme + noff;

  l = l + rsz;
  if (p->eof) {
    *l++ = '\0';
  }
  p->limit = l;
  p->buf = nbuf;
  p->bufsz = nsz;
  if (!p->cursor) p->cursor = p->buf;

  while (l < p->buf + p->bufsz) {
    *l++ = '\0';
  }

  return 0;
}

static double csv2tab_strtod(const char *fstp, const char *limp, const char **m)
{
  ptrdiff_t sz;
  char *buf;
  char *rt;
  double r;
  int e;

  sz = limp - fstp;
  if (sz <= 0) {
    if (m) *m = fstp;
    return 0.0;
  }

  sz += 1; /* space for '\0' */
  buf = (char *)calloc(sizeof(char), sz);
  if (!buf) {
    if (m) *m = NULL;
    return 0.0;
  }
  strncpy(buf, fstp, sz);

  errno = 0;
  r = strtod(buf, &rt);
  e = errno;
  if (m) {
    if (rt) {
      *m = fstp + (rt - buf);
    } else {
      *m = NULL;
    }
  }
  free(buf);
  errno = e;

  return r;
}

static csv2tab_node *csv2tab_new_node(csv2tab_node **root)
{
  csv2tab_node *v;
  csv2tab_node *r;

  TABLE_ASSERT(root);

  v = (csv2tab_node *)calloc(sizeof(csv2tab_node), 1);
  if (!v) return NULL;

  v->v = NAN;
  v->x = NAN;
  v->y = NAN;
  v->src_line = 0;
  v->stat = NULL;

  r = *root;
  if (!r) {
    *root = v;
  } else {
    v->next = r->next;
    v->prev = r;
    r->next = v;
    if (v->next) {
      v->next->prev = v;
    }
  }
  return v;
}

static int csv2tab_addcell(csv2tab_node **root,
                           csv2tab_stat_node **sroot, csv2tab_node **curr,
                           const char *tok, const char *cur,
                           long srcl, csv2tab_add_mode mode,
                           csv2tab_error *err)
{
  const char *rst;
  double v;
  csv2tab_node *ncur;
  int e;

  TABLE_ASSERT(root);
  TABLE_ASSERT(curr);

  errno = 0;
  v = csv2tab_strtod(tok, cur, &rst);
  if (rst != cur) {
    if (err) *err = CSV2TAB_ERR_FLOAT_FORMAT;
    return 1;
  }
  e = errno;

  ncur = *curr;
  if (!ncur) {
    ncur = csv2tab_new_node(root);
    if (!ncur) {
      if (err) *err = CSV2TAB_ERR_NOMEM;
      return 1;
    }
  }

  switch(mode) {
  case CSV2TAB_ADD_V:
    ncur->v = v;
    break;
  case CSV2TAB_ADD_X:
    ncur->x = v;
    break;
  case CSV2TAB_ADD_Y:
    ncur->y = v;
    break;
  default:
    TABLE_UNREACHABLE();
    break;
  }
  if (e == ERANGE) {
    if (v == 0.0) {
      csv2tab_set_status_flag(ncur, sroot, CSV2TAB_STAT_UNDERFLOW, err);
    } else {
      csv2tab_set_status_flag(ncur, sroot, CSV2TAB_STAT_OVERFLOW, err);
    }
  }
  ncur->src_line = srcl;

  *curr = ncur;
  return 0;
}

static int csv2tab_addcell_i(csv2tab_node **root,
                             csv2tab_stat_node **sroot, csv2tab_node **curr,
                             const char *tok, const char *cur, int srcl,
                             int column_index, int xidx, int yidx, int vidx,
                             csv2tab_error *err)
{
  int r;
  r = 0;

  /* Note that all or 2 of xidx, yidx and vidx can be same value */
  if (column_index == xidx) {
    r = csv2tab_addcell(root, sroot, curr, tok, cur, srcl, CSV2TAB_ADD_X, err);
  }
  if (r == 0 && column_index == yidx) {
    r = csv2tab_addcell(root, sroot, curr, tok, cur, srcl, CSV2TAB_ADD_Y, err);
  }
  if (r == 0 && column_index == vidx) {
    r = csv2tab_addcell(root, sroot, curr, tok, cur, srcl, CSV2TAB_ADD_V, err);
  }
  return r;
}


static int csv2tab_readcsv(csv2tab_node **node,
                           csv2tab_stat_node **sroot, FILE *input,
                           int xidx, int yidx, int vidx,
                           csv2tab_error *errinfo,
                           long *errline, long *errcol)
{
  csv2tab_node *curr_node;
  csv2tab_parser p;
  csv2tab_error e;
  int ic;
  int ir;
  int r;

  TABLE_ASSERT(input);

  memset(&p, 0, sizeof(csv2tab_parser));
  p.line = 1;
  p.column = 1;
  curr_node = NULL;

  ir = 1;
  ic = 0;

  e = CSV2TAB_SUCCESS;
  csv2tab_fill(input, &p, 1, &e);
  if (e != CSV2TAB_SUCCESS) goto error;

  for (;;) {
    p.token = p.cursor;
    p.marker = NULL;
    p.ctxmarker = NULL;
    /*!re2c
      re2c:indent:top = 2;
      re2c:yyfill:enable = 1;
      re2c:define:YYFILL:naked = 1;
      re2c:define:YYFILL = "{ if (csv2tab_fill(input, &p, @@, &e)) break; }";

      nl = ("\n"|"\r\n"|"\r");
      sp = (" "|"\t");
      sep = ",";
      quo = "\"";
      eof = "\x00";
      pnd = "#";

      *      { p.cursor = p.token; goto cell; }
      eof    { break; }
      pnd    { p.column += p.cursor - p.token; goto comment; }
      sp     { p.column += p.cursor - p.token; continue; }
      nl     { goto newline; }
      sep    { p.column += p.cursor - p.token; ic++; continue; }
      quo    { p.column += p.cursor - p.token; goto quot; }
     */
    TABLE_UNREACHABLE();

  newline:
    p.line++;
    p.column = 1;
    ir++;
    ic = 0;
    curr_node = NULL;
    continue;

  comment:
    p.token = p.cursor;
    /*!re2c
      re2c:indent:top = 2;

      *   { p.column++; goto comment; }
      eof { break; }
      nl  { p.line++; p.column = 0; continue; }
    */
    TABLE_UNREACHABLE();

  quot:
    {
      long l = p.line;
      long c = p.column;

      p.lexeme = p.cursor;
      for (;;) {
        p.token = p.cursor;
        /*!re2c
          re2c:indent:top = 4;

          *       { c++; continue; }
          eof     { goto unexp_eof; }
          nl      { l++; c = 1; continue; }
          quo quo { c += p.cursor - p.token; continue; }
          quo     { c += p.cursor - p.token; break; }
        */
        TABLE_UNREACHABLE();
      }
      csv2tab_addcell_i(node, sroot, &curr_node, p.lexeme, p.token, p.line,
                        ic, xidx, yidx, vidx, &e);
      if (e != CSV2TAB_SUCCESS) {
        break;
      }

      p.line = l;
      p.column = c;
      p.lexeme = NULL;
    }
    continue;

  cell:
    {
      long c = p.column - (p.cursor - p.token);
      p.lexeme = p.token;
      for (;;) {
        p.token = p.cursor;
        /*!re2c
          re2c:indent:top = 4;

          * { p.column += 1; continue; }
          sp* "\x00" { break; }
          sp* nl     { break; }
          sp* sep    { break; }
        */
        TABLE_UNREACHABLE();
      }
      csv2tab_addcell_i(node, sroot, &curr_node, p.lexeme, p.token, p.line,
                        ic, xidx, yidx, vidx, &e);
      if (e != CSV2TAB_SUCCESS) {
        p.column = c;
        break;
      }
      p.cursor = p.token;
      p.lexeme = NULL;
    }
    continue;

  unexp_eof:
    e = CSV2TAB_ERR_EOF;
    break;
  }

  r = 0;

 error:
  if (errinfo) *errinfo = e;
  if (e != CSV2TAB_SUCCESS) {
    if (errline) *errline = p.line;
    if (errcol)  *errcol  = p.column;
    r = 1;
  }
  free(p.buf);

  return r;
}

/*
 * Swap the position of two nodes
 *
 * This is little complicated: https://stackoverflow.com/a/27041055
 */
static void csv2tab_swap_node(csv2tab_node *n1, csv2tab_node *n2)
{
  csv2tab_node *t;
  csv2tab_node *v[4];

  if (n1 == n2) return;
  if (n2->next == n1) {
    t = n1;
    n1 = n2;
    n2 = t;
  }

  v[0] = n1->prev;
  v[1] = n2->prev;
  v[2] = n1->next;
  v[3] = n2->next;

  if ((n1->next == n2 && n2->prev == n1) ||
      (n1->prev == n2 && n2->next == n1)) {
    n1->prev = v[2];
    n2->prev = v[0];
    n1->next = v[3];
    n2->next = v[1];
  } else {
    n1->prev = v[1];
    n2->prev = v[0];
    n1->next = v[3];
    n2->next = v[2];
  }

  if (n1->prev) n1->prev->next = n1;
  if (n1->next) n1->next->prev = n1;
  if (n2->prev) n2->prev->next = n2;
  if (n2->next) n2->next->prev = n2;
}

static int csv2tab_sort(csv2tab_node **root, csv2tab_stat_node **sroot,
                        double tolerance, csv2tab_error *e)
{
  csv2tab_node *oroot;
  csv2tab_node *t;
  csv2tab_node *tn;
  csv2tab_status ee;
  size_t vnc;
  csv2tab_node *valr = NULL;
  csv2tab_node *valc = NULL;
  csv2tab_node *invc = NULL;
  csv2tab_node *troot;
  int r;

  TABLE_ASSERT(root);
  r = 0;

  oroot = *root;
  t = oroot;
  vnc = 0;
  while (t) {
    tn = t->next;
    ee = CSV2TAB_STAT_CLEAR;
    if (isnan(t->x)) {
      ee |= CSV2TAB_STAT_NANX;
    }
    if (isnan(t->y)) {
      ee |= CSV2TAB_STAT_NANY;
    }
    if (isinf(t->x)) {
      ee |= CSV2TAB_STAT_INFX;
    }
    if (isinf(t->y)) {
      ee |= CSV2TAB_STAT_INFY;
    }

    if (isfinite(t->x) && isfinite(t->y)) {
      vnc++;
      if (!valr) valr = t;
      if (valc) {
        valc->next = t;
      }
      valc = t;
    } else {
      t->next = invc;
      invc = t;
    }

    if (ee != CSV2TAB_STAT_CLEAR) {
      r = csv2tab_set_status_flag(t, sroot, ee, e);
    }
    t = tn;
  }
  /* Finalize */
  if (valc) {
    valc->next = NULL;
  }
  if (r != 0) {
    goto error;
  }
  if (vnc == 0) {
    if (e) *e = CSV2TAB_ERR_NODATA;
    r = 1;
    goto error;
  }

  t = valr;
  troot = t;
  t = t->next;
  while (t) {
    csv2tab_node *tx, *ty, *txp, *txo, *typ, *tt;
    txo = NULL;
    tx  = NULL;
    txp = troot;
    if (t->x - txp->x >= -tolerance) {
      while (1) {
        tx = txp;
        txp = tx->xnext;
        if (!txp) break;
        if (fabs(t->x - tx->x) <= tolerance) break;
        if (t->x < (txp->x - tolerance)) break;
        txo = tx;
      }
    }
    /* Equal to x */
    if (tx && fabs(t->x - tx->x) <= tolerance) {
      ty  = NULL;
      typ = tx;
      if (t->y - typ->y >= -tolerance) {
        while (1) {
          ty = typ;
          typ = ty->ynext;
          if (!typ) break;
          /* Data has multiple data for same (X, Y) */
          if (fabs(t->y - ty->y) <= tolerance) {
            if (!csv2tab_test_node_flag(ty, CSV2TAB_STAT_MULTIPLE_XY)) {
              r = csv2tab_set_status_flag(t, sroot,
                                          CSV2TAB_STAT_MULTIPLE_XY, e);
            }
            break;
          }
          if (t->y < (typ->y - tolerance)) break;
        }
      }
      /* insert between ty and typ */
      if (ty) {
        ty->ynext = t;
        t->ynext = typ;
        t->xnext = tx;
      } else {
        /* update x base (tx) to t */
        t->ynext = typ;
        t->xnext = typ->xnext;
        tt = txo;
        while (tt) {
          tt->xnext = t;
          tt = tt->ynext;
        }
        if (tx == troot) {
          troot = t;
        }
      }
    } else {
      if (tx) {
        tx->xnext = t;
        t->xnext = txp;
      } else {
        troot = t;
        t->xnext = txp;
      }
    }
    t = t->next;
  }

  /* swap valr by troot */
  csv2tab_swap_node(valr, troot);

  /* fast forward to the end */
  if (valc == valr || valc == troot) {
    while (valc->next) {
      valc = valc->next;
    }
  }
  valr = troot;
  r = 0;

  error:
  /* Concatenate two links */
  if (valr) {
    TABLE_ASSERT(valc);
    valc->next = invc;
  } else {
    valr = invc;
  }
  *root = valr;

  return r;
}

static void csv2tab_insert_left(csv2tab_node **root,
                                csv2tab_node *insert, csv2tab_node *node)
{
  csv2tab_node *yp;
  TABLE_ASSERT(root);
  TABLE_ASSERT(insert);
  TABLE_ASSERT(node);

  /*
   *                    * ----- yp            -- yp
   *                    |        |
   *  yp ---- node  ->  |     insert -- node  -- yy
   *  |                 |        |
   *  yp->yprev         * --- yp->yprev       y value
   */
  yp = node->xprev;
  insert->xnext = node;
  node->xprev = insert;
  if (yp) {
    if (yp->yprev) {
      yp->yprev->ynext = insert;
    }
    insert->yprev = yp->yprev;
    yp->yprev = insert;
  }
  insert->ynext = yp;
  if (*root == yp || *root == node) {
    *root = insert;
  }
}

static void csv2tab_insert_below(csv2tab_node **root,
                                 csv2tab_node *insert, csv2tab_node *node)
{
  csv2tab_node *yp;
  TABLE_ASSERT(root);
  TABLE_ASSERT(insert);
  TABLE_ASSERT(node);

  /*
   *                (nc) ---- node -------.  -- yy
   *                            |         |
   *  yp ---- node ->  yp -- insert ---.  |   -- yp
   *                            |       \ |
   *                       node->yprev -- *    y value
   */
  yp = node->xprev;
  if (yp) {
    yp->xnext = insert;
  }
  insert->xprev = yp;
  node->xprev = NULL;
  insert->xnext = node->xnext;
  if (node->yprev) {
    node->yprev->ynext = insert;
  }
  insert->yprev = node->yprev;
  insert->ynext = node;
  node->yprev = insert;
  if (node == *root) {
    *root = insert;
  }
}

static void csv2tab_interpolate(csv2tab_node *node)
{
  enum { north = 0, east, south, west };
  csv2tab_node *nadj[4];
  double v[4];
  double d[4];
  double r;
  double t;
  double ds;
  int i;

  TABLE_ASSERT(node);

  nadj[north]  = node->ynext;
  nadj[east]   = node->xnext;
  nadj[south]  = node->yprev;
  nadj[west]   = node->xprev;

  for (i = 0; i < 4; ++i) {
    if (nadj[i]) {
      v[i] = nadj[i]->v;
    } else {
      v[i] = NAN;
    }
    d[i] = INFINITY;
  }
  if (nadj[north]) d[north] = nadj[north]->y - node->y;
  if (nadj[south]) d[south] = node->y - nadj[south]->y;
  if (nadj[east ]) d[west ] = nadj[east ]->x - node->x;
  if (nadj[west ]) d[east ] = node->x - nadj[west ]->x;

  /* Swap to use as weight. */
  t = d[south];
  d[south] = d[north];
  d[north] = t;

  t = d[east];
  d[east] = d[west];
  d[west] = t;

  ds = 0.0;
  r = NAN;
  for (i = 0; i < 4; ++i) {
    if (isfinite(v[i]) && isfinite(d[i])) {
      if (isnan(r)) r = 0.0;
      r += v[i] * d[i];
      ds += d[i];
    }
  }
  node->v = r / ds;
}

static int csv2tab_build_mesh(csv2tab_node **root, csv2tab_stat_node **sroot,
                              table_data *table, double tolerance,
                              int interpolate, int adj_coord,
                              table_geometry forcing_geometry,
                              csv2tab_error *e, table_error *te)
{
  csv2tab_node *xn;
  csv2tab_node *yn;
  csv2tab_node *xnp;
  csv2tab_node *ynp;
  csv2tab_node *n;
  csv2tab_node *groot;
  double yp;
  double yy;
  table_index xs;
  table_index ys;
  table_size is;
  double *yt;
  double *xt;
  double *vt;
  table_geometry gg;
  int r;
  table_error tep;
  r = 0;

  TABLE_ASSERT(root);
  TABLE_ASSERT(sroot);

  xt = NULL;
  yt = NULL;
  vt = NULL;

  /* geometrical root (typ. x = 0, y = 0) node */
  groot = *root;

  /* Make double linked list */
  xnp = NULL;
  xn = groot;
  while (xn) {
    yn = xn;
    ynp = NULL;
    while (yn) {
      yn->yprev = ynp;
      yn->xprev = xnp;
      yn->xnext = xn->xnext;
      ynp = yn;
      yn = yn->ynext;
    }
    xnp = xn;
    xn = xn->xnext;
  }

  /* First, generate complete rectiliner net */
  xnp = groot;
  xn = groot->xnext;
  while (xn) {
    csv2tab_node *yl;
    ynp = xnp;
    yl = yn;
    yn = xn;
    while (ynp && yn) {
      yp = ynp->y;
      yy = yn->y;
      yl = yn;
      /* Remove duplicated node */
      if (csv2tab_test_node_flag(yn, CSV2TAB_STAT_MULTIPLE_XY)) {
        csv2tab_node *t;
        t = yn->yprev;
        t->ynext = yn->ynext;
        yn->ynext->yprev = t;
        yn = yn->ynext;
        continue;
      }
      if (yy < (yp - tolerance)) {
        n = csv2tab_new_node(root);
        if (!n) {
          if (e) *e = CSV2TAB_ERR_NOMEM;
          r = 1;
        } else {
          csv2tab_node *t1, *t2, *xt, *xtp;
          n->x = ynp->x;
          n->y = yy;
          yn->xprev = ynp;
          ynp->xnext = yn;
          csv2tab_insert_left(&groot, n, yn);

          t1 = ynp->xprev;
          t2 = n;
          ynp = n;

          while (t1) {
            n = t1->yprev;
            /* find matching point on left side and connect to it if found */
            if (n) {
              xtp = n;
              while (xtp) {
                xt = xtp;
                xtp = xtp->yprev;
              }
              while (xt != n->ynext) {
                if (fabs(xt->y - yy) <= tolerance) {
                  xt->xnext = t2;
                  t2->xprev = n;
                  break;
                }
                xt = xt->ynext;
              }
              if (xt != n->ynext) break;
            }
            n = csv2tab_new_node(root);
            if (!n) {
              if (e) *e = CSV2TAB_ERR_NOMEM;
              r = 1;
              break; /* Cannot continue the work */
            }
            n->x = t1->x;
            n->y = t2->y;
            t2->xprev = t1;
            csv2tab_insert_left(&groot, n, t2);
            t1 = t1->xprev;
            t2 = n;
          }
        }
      } else if (yp < (yy - tolerance)) {
        n = csv2tab_new_node(root);
        if (!n) {
          if (e) *e = CSV2TAB_ERR_NOMEM;
          r = 1;
        } else {
          n->x = yn->x;
          n->y = ynp->y;
          yn->xprev = ynp;
          csv2tab_insert_below(&groot, n, yn);
          yn = n;
        }
      } else {
        ynp->xnext = yn;
        yn->xprev = ynp;
      }
      ynp = ynp->ynext;
      yn = yn->ynext;
    }
    /* fill tops */
    yn = yl;
    while (ynp) {
      n = csv2tab_new_node(root);
      if (!n) {
        if (e) *e = CSV2TAB_ERR_NOMEM;
        r = 1;
      } else {
        n->x = yl->x;
        n->y = ynp->y;
        n->yprev = yl;
        yl->ynext = n;
        n->xnext = yl->xnext;
        n->xprev = ynp;
        ynp->xnext = n;
      }
      yl = n;
      ynp = ynp->ynext;
    }
    xnp = xn;
    xn = xn->xnext;
  }
  if (r != 0) goto error;

  /* build x and y array */
  xs = 0;
  xn = groot;
  while (xn) {
    xn = xn->xnext;
    xs++;
  }
  ys = 0;
  yn = groot;
  while (yn) {
    yn = yn->ynext;
    ys++;
  }
  if (xs < 0 || ys < 0) {
    if (e) *e = CSV2TAB_ERR_NOMEM;
    goto error;
  }
  xt = (double *) calloc(sizeof(double), xs);
  yt = (double *) calloc(sizeof(double), ys);
  if (!xt || !yt) {
    if (e) *e = CSV2TAB_ERR_NOMEM;
    r = 1;
    goto error;
  }
  is = 0;
  xn = groot;
  while (xn) {
    xt[is] = xn->x;
    xn = xn->xnext;
    is++;
  }
  is = 0;
  yn = groot;
  while (yn) {
    yt[is] = yn->y;
    yn = yn->ynext;
    is++;
  }

  /* Detect sum-const map */
  switch (forcing_geometry) {
  case TABLE_GEOMETRY_RECTILINEAR:
  case TABLE_GEOMETRY_SUM_CONSTANT:
    gg = forcing_geometry;
    break;
  default:
    gg = TABLE_GEOMETRY_INVALID;
  }
  if (gg != TABLE_GEOMETRY_RECTILINEAR) {
    is = table_calc_data_size(TABLE_GEOMETRY_SUM_CONSTANT, xs, ys);
    do {
      table_index ii, jj, ts;
      double t;

      if (is == (table_size)-1) {
        if (gg == TABLE_GEOMETRY_SUM_CONSTANT) {
          if (e) *e = CSV2TAB_ERR_GEOMETRY_NOT_APPLICABLE;
          r = 1;
          goto error;
        }
        gg = TABLE_GEOMETRY_RECTILINEAR;
        break;
      }

      /* Assume */
      TABLE_ASSERT(xs == ys);

      yn = groot;
      ynp = NULL;
      while (yn) {
        ynp = yn;
        yn = yn->ynext;
      }
      ii = 0;
      xnp = ynp;
      xn = xnp->xnext;
      while (xn && (isfinite(xn->v) || isfinite(xnp->v))) {
        xnp = xn;
        xn = xn->xnext;
        ii++;
        if (ii > 4) break;
      }
      /* values are filled like RECTILINEAR */
      if (ii > 4) {
        gg = TABLE_GEOMETRY_RECTILINEAR;
        break;
      }

      vt = (double *) calloc(sizeof(double), xs);
      if (!vt) {
        if (e) *e = CSV2TAB_ERR_NOMEM;
        r = 1;
        goto error;
      }
      ii = 0;
      jj = ys - 1;
      t = 0.0;
      for (; ii < xs; ++ii, --jj) {
        vt[ii] = xt[ii] + yt[jj];
        t += vt[ii];
      }
      for (ii = 0; ii < xs; ++ii) {
        vt[ii] = vt[ii] * xs - t;
      }
      t = 0.0;
      for (ii = 0; ii < xs; ++ii) {
        t += vt[ii] * vt[ii];
      }
      free(vt);
      vt = NULL;
      /*
       * t is RMS here. (t is almost 1e-20 when vt is varied within 1 plus
       * or minus DBL_EPSILON and 100 elements)
       */
      t /= (double)xs * xs;
      if (t > (xt[0] + yt[ys - 1]) * 1e-11) {
        csv2tab_set_status_flag(groot, sroot, CSV2TAB_STAT_SUMC_NONCONSTRAINT,
                                e);
        gg = TABLE_GEOMETRY_RECTILINEAR;
        break;
      }

      gg = TABLE_GEOMETRY_SUM_CONSTANT;

      yn = groot;
      ii = 0;
      ts = ys;
      while (yn) {
        xn = yn;
        for (; ii < ts; ++ii) {
          if (!xn) break;
          if (ii >= is) break;
          xn = xn->xnext;
        }
        /* Disjoint the table here */
        if (xn && ii == ts) {
          if (xn->yprev) {
            xn->yprev->ynext = NULL;
          }
          if (xn->xprev) {
            xn->xprev->xnext = NULL;
          }
          xn->yprev = NULL;
          xn->xprev = NULL;
        }
        while (xn) {
          csv2tab_set_status_flag(xn, sroot, CSV2TAB_STAT_DISCARD, e);
          xn = xn->xnext;
        }
        ii = ts;
        ys--;
        ts = ts + ys;
        yn = yn->ynext;
      }
      /* ys is modified. Revert to xs */
      ys = xs;

      /* Adjust coordinates */
      if (adj_coord) {
        double sum;
        sum = xt[0] + xt[xs - 1];
        for (ii = 0; ii < xs; ++ii) {
          yt[ii] = sum - xt[xs - ii - 1];
        }
      }

    } while(0);
  }

  is = table_calc_data_size(gg, xs, ys);
  if (is != (table_size)-1) {
    table_index ii;

    vt = (double *)calloc(sizeof(double), is);
    if (!vt) {
      if (e) *e = CSV2TAB_ERR_NOMEM;
      r = 1;
      goto error;
    }

    yn = groot;
    ii = 0;
    while (yn) {
      xn = yn;
      while (xn) {
        if (interpolate && isnan(xn->v)) {
          csv2tab_interpolate(xn);
          if (isfinite(xn->v)) {
            csv2tab_set_status_flag(xn, sroot, CSV2TAB_STAT_INTERPOLATE, e);
          }
        }
        if (isnan(xn->v)) {
          csv2tab_set_status_flag(xn, sroot, CSV2TAB_STAT_NANV, e);
        } else if (isinf(xn->v)) {
          csv2tab_set_status_flag(xn, sroot, CSV2TAB_STAT_INFV, e);
        }
        TABLE_ASSERT(ii < is);
        vt[ii] = xn->v;
        ++ii;
        xn = xn->xnext;
      }
      yn = yn->ynext;
    }
  }

  switch (gg) {
  case TABLE_GEOMETRY_RECTILINEAR:
  case TABLE_GEOMETRY_SUM_CONSTANT:
    tep = table_init(table, NULL, gg, xs, ys, table_get_interp_mode(table),
                     xt, yt, vt);
    if (te) *te = tep;
    if (tep != TABLE_SUCCESS) {
      r = 1;
      if (e) *e = CSV2TAB_ERR_INIT_TABLE;
    }
    break;
  default:
    if (e) *e = CSV2TAB_ERR_GEOMETRY_UNKNOWN;
    r = 1;
    goto error;
  }
  r = 0;

 error:
  /* If root is changed (by creating new node),
   * reset allocation root to geometrical root.
   */
  if (*root != groot) {
    csv2tab_swap_node(*root, groot);
    *root = groot;
  }
  free(xt);
  free(yt);
  free(vt);

  return r;
}

const char *csv2tab_errorstr(csv2tab_error e)
{
  switch(e) {
  case CSV2TAB_SUCCESS:
    return "Success";
  case CSV2TAB_ERR_EOF:
    return "Unexpected EOF reached";
  case CSV2TAB_ERR_NOMEM:
    return "Memory allocation failed";
  case CSV2TAB_ERR_GEOMETRY_UNKNOWN:
    return "Geometry type could not be detected";
  case CSV2TAB_ERR_GEOMETRY_NOT_APPLICABLE:
    return "Specified geometry type is not applicable for this CSV data";
  case CSV2TAB_ERR_INVALID_TOKEN:
    return "Invalid token found";
  case CSV2TAB_ERR_FLOAT_FORMAT:
    return "Invalid floating point value";
  case CSV2TAB_ERR_NODATA:
    return "No valid data present";
  case CSV2TAB_ERR_SYS:
    return "System call error occured";
  case CSV2TAB_ERR_INIT_TABLE:
    return "Error while initialize table";
  default:
    return "Unknown error occured";
  }
}

csv2tab_data *csv2tab_init(void)
{
  csv2tab_data *v;
  v = (csv2tab_data *)calloc(sizeof(csv2tab_data), 1);
  if (!v) return NULL;

  v->root = NULL;
  v->xcol = 0;
  v->ycol = 1;
  v->vcol = 2;
  v->tol = 0.0;
  return v;
}

void csv2tab_free(csv2tab_data *obj)
{
  csv2tab_node *n;
  csv2tab_node *t;
  csv2tab_stat_node *sn;
  csv2tab_stat_node *st;

  if (!obj) return;

  n = obj->root;
  while (n) {
    t = n->next;
    free(n);
    n = t;
  }
  sn = obj->stats;
  while (sn) {
    st = sn->next;
    free(sn);
    sn = st;
  }
  free(obj);
}

void csv2tab_set_x_column(csv2tab_data *data, int index)
{
  if (!data || index < 0) return;
  data->xcol = index;
}

void csv2tab_set_y_column(csv2tab_data *data, int index)
{
  if (!data || index < 0) return;
  data->ycol = index;
}

void csv2tab_set_v_column(csv2tab_data *data, int index)
{
  if (!data || index < 0) return;
  data->vcol = index;
}

void csv2tab_set_tolerance(csv2tab_data *data, double tol)
{
  if (!data) return;
  if (!isfinite(tol)) tol = 1.e-12;
  if (tol < 0.0) tol = -tol;
  data->tol = tol;
}

double csv2tab_get_tolerance(csv2tab_data *data)
{
  if (!data) return NAN;
  return data->tol;
}

void csv2tab_set_force_geometry(csv2tab_data *data, table_geometry geom)
{
  if (!data) return;
  data->force_geom = geom;
}

static void csv2tab_set_flags(int *loc, int flag)
{
  TABLE_ASSERT(loc);

  if (flag) {
    *loc = 1;
  } else {
    *loc = 0;
  }
}

void csv2tab_set_interpolation(csv2tab_data *data, int interpolate)
{
  if (!data) return;
  csv2tab_set_flags(&data->interpolate, interpolate);
}

void csv2tab_set_adjust_coordinates(csv2tab_data *data, int adj_coord)
{
  if (!data) return;
  csv2tab_set_flags(&data->adjust_coordinate, adj_coord);
}

int csv2tab_convert(csv2tab_data *data, FILE *input, table_data *table,
                    csv2tab_error *errinfo,
                    long *errline, long *errcol,
                    table_error *terror)
{
  int r;

  TABLE_ASSERT(data);
  TABLE_ASSERT(table);

  r = csv2tab_readcsv(&data->root, &data->stats, input,
                      data->xcol, data->ycol, data->vcol,
                      errinfo, errline, errcol);
  if (r != 0) return r;

  r = csv2tab_sort(&data->root, &data->stats, data->tol, errinfo);
  if (r != 0) return r;

  r = csv2tab_build_mesh(&data->root, &data->stats, table, data->tol,
                         data->interpolate, data->adjust_coordinate,
                         data->force_geom, errinfo, terror);
  if (r != 0) return r;

  return r;
}

int csv2tab_test_node_flag(csv2tab_node *node, csv2tab_status flag)
{
  csv2tab_stat_node *sn;

  if (!node) return 0;

  sn = node->stat;
  if (!sn) return 0;

  return (sn->stat & flag) != 0;
}

csv2tab_status csv2tab_get_node_flag(csv2tab_node *node)
{
  csv2tab_stat_node *sn;

  if (!node) return CSV2TAB_STAT_CLEAR;

  sn = node->stat;
  if (!sn) return CSV2TAB_STAT_CLEAR;

  return sn->stat;
}

csv2tab_stat_node *csv2tab_get_stats(csv2tab_data *data)
{
  if (!data) return NULL;

  return data->stats;
}

csv2tab_stat_node *csv2tab_status_next(csv2tab_stat_node *stat)
{
  if (!stat) return NULL;
  return stat->next;
}

csv2tab_node *csv2tab_get_node_by_stat(csv2tab_stat_node *stat)
{
  if (!stat) return NULL;
  return stat->node;
}

csv2tab_stat_node *csv2tab_get_stat_by_node(csv2tab_node *node)
{
  if (!node) return NULL;
  return node->stat;
}

csv2tab_node *csv2tab_get_node_root(csv2tab_data *data)
{
  if (!data) return NULL;
  return data->root;
}

double csv2tab_get_node_value(csv2tab_node *node)
{
  if (!node) return NAN;
  return node->v;
}

double csv2tab_get_node_x(csv2tab_node *node)
{
  if (!node) return NAN;
  return node->x;
}

double csv2tab_get_node_y(csv2tab_node *node)
{
  if (!node) return NAN;
  return node->y;
}

csv2tab_node *csv2tab_get_node_x_next(csv2tab_node *node)
{
  if (!node) return NULL;
  return node->xnext;
}

csv2tab_node *csv2tab_get_node_y_next(csv2tab_node *node)
{
  if (!node) return NULL;
  return node->ynext;
}

long csv2tab_get_node_source_line(csv2tab_node *node)
{
  if (!node) return -1;
  return node->src_line;
}
