/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>    /* Used in formatter. */

#include "geometry/list.h"
#include "geometry/alloc_list.h"
#include "csv.h"
#include "csvutil.h"

#include <jupiter/re2c_lparser/re2c_lparser.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

enum csv_constants {
  hashsize = 256,
};

struct csv_column
{
  csv_row *parent;
  char *value;
  long line;
  long cols;
  struct geom_list list;
};
#define csv_column_entry(p) geom_list_entry(p, struct csv_column, list)

struct csv_row
{
  struct geom_list list;
  struct geom_list hash_list;
  csv_data *parent;
  csv_column column_head;
};
#define csv_row_entry(p) geom_list_entry(p, struct csv_row, list)
#define csv_row_hash_entry(p) geom_list_entry(p, struct csv_row, hash_list)

struct csv_data
{
  geom_alloc_list *allocs;
  csv_row row_head;
  struct geom_list rowtable[hashsize];
};

struct csv_token
{
  struct geom_list list;
  char *start;
  const char *end;
  long line;
  long col;
};
#define csv_token_entry(p) \
  geom_list_entry(p, struct csv_token, list)

struct csv_parser
{
  csv_data *data;
  struct re2c_lparser lp;
  struct csv_token token_head;
  csv_row *row;
  long cs_line;
  long cs_column;
  enum quotestate { in_quote, out_quote, quote_notset } use_quote;
};
typedef struct csv_parser csv_parser;

static unsigned long
hashCSVKey(const char *string, int len)
{
  unsigned long i, r, c;

  CSVASSERT(string);
  CSVASSERT(len >= 0);

  i = 0;
  r = 0;
  c = len;
  while (*string != '\0' && i < c) {
    r = r * 97 + (unsigned long)*string;
    string++;
  }
  c = 0;
  for (i = 0; i < sizeof(unsigned long); ++i) {
    c = c * 11 + ((r >> (i * CHAR_BIT)) & 0xff);
  }
  return c % hashsize;
}

static void
initCSVColumn(csv_column *col, csv_row *row)
{
  CSVASSERT(col);
  CSVASSERT(row);

  geom_list_init(&col->list);
  col->cols = 0;
  col->line = 0;
  col->parent = row;
  col->value = NULL;
}

static void
initCSVRow(csv_row *row, csv_data *d)
{
  CSVASSERT(row);
  CSVASSERT(d);

  row->parent = d;
  initCSVColumn(&row->column_head, row);
  geom_list_init(&row->list);
  geom_list_init(&row->hash_list);
}

static csv_error initCSV(csv_data *d)
{
  int i;

  CSVASSERT(d);

  d->allocs = geom_alloc_list_new();
  if (!d->allocs) {
    return CSV_ERR_NOMEM;
  }
  initCSVRow(&d->row_head, d);
  for (i = 0; i < hashsize; ++i) {
    geom_list_init(&d->rowtable[i]);
  }
  return 0;
}

csv_data *allocateCSV(void)
{
  csv_data *p;
  int ret;

  p = (csv_data*)malloc(sizeof(csv_data));
  if (!p) {
    return NULL;
  }
  ret = initCSV(p);
  if (ret != CSV_ERR_SUCC) {
    free(p);
    return NULL;
  }

  return p;
}

void freeCSV(csv_data *d)
{
  if (!d) return;
  if (d->allocs) {
    geom_alloc_free_all(d->allocs);
  }
  free(d);
}

csv_row *newCSVRow(csv_data *d)
{
  csv_row *t;

  CSVASSERT(d);
  CSVASSERT(d->allocs);

  t = (csv_row*)malloc(sizeof(csv_row));
  if (!t) return NULL;

  if (geom_alloc_add(d->allocs, t, free) != GEOM_SUCCESS) {
    free(t);
    return NULL;
  }

  initCSVRow(t, d);
  return t;
}

csv_column *newCSVColumn(csv_row *r)
{
  csv_data *d;
  csv_column *t;

  CSVASSERT(r);

  d = r->parent;

  CSVASSERT(d);
  CSVASSERT(d->allocs);

  t = (csv_column*)malloc(sizeof(csv_column));
  if (!t) return NULL;

  if (geom_alloc_add(d->allocs, t, free) != GEOM_SUCCESS) {
    free(t);
    return NULL;
  }

  initCSVColumn(t, r);
  return t;
}

static const char *getCSVRowKey(csv_row *row)
{
  struct geom_list *lp;

  CSVASSERT(row);

  lp = geom_list_next(&row->column_head.list);
  if (lp != &row->column_head.list) {
    csv_column *c;

    c = csv_column_entry(lp);
    return c->value;
  }
  return NULL;
}

int appendCSVRow(csv_data *d, csv_row *row)
{
  const char *keystr;
  unsigned long l;

  CSVASSERT(d);
  CSVASSERT(row);
  CSVASSERT(d == row->parent);

  geom_list_insert_prev(&d->row_head.list, &row->list);

  keystr = getCSVRowKey(row);
  if (keystr) {
    l = hashCSVKey(keystr, strlen(keystr));
    geom_list_insert_prev(&d->rowtable[l], &row->hash_list);
  }

  return 0;
}

int appendCSVColumn(csv_row *row, csv_column *col)
{
  CSVASSERT(row);
  CSVASSERT(col);
  CSVASSERT(row == col->parent);

  geom_list_insert_prev(&row->column_head.list, &col->list);
  return 0;
}

#define YYDEBUG(i, c)

/**
 * This function resolves escapes of "", \" and \\.
 */
static char *unescapeCSVCellString(const char *stp, const char *endp, int *n)
{
  const char *tok;
  const char *cur;
  /* const char *mrk; */
  char *buf;
  char *dst;
  char *dlim;

  CSVASSERT(stp);
  CSVASSERT(endp);
  CSVASSERT(n);
  CSVASSERT(stp <= endp);

  *n = endp - stp + 1;
  buf = (char *)malloc(sizeof(char) * (*n));
  if (!buf) {
    *n = 0;
    return NULL;
  }

  tok = stp;
  cur = tok;
  dst = buf;
  dlim = buf + *n;

  while (cur < endp && dst < dlim) {
    tok = cur;
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "cur";
      re2c:define:YYMARKER = "mrk";
      re2c:yyfill:enable = 0;
      re2c:indent:top = 2;
      re2c:indent:string = "  ";

      * { *(dst++) = *tok; continue; }
      "\\\\" { *(dst++) = '\\'; continue; }
      "\"\"" { *(dst++) = '"'; continue; }
      "\\\"" { *(dst++) = '"'; continue; }
    */

    CSVUNREACHABLE();
  }
  *dst = '\0';
  return buf;
}

static int
csvAddToken(csv_parser *p, const char *startp, const char *endp,
            char *allocp, long line, long col, csv_error *err)
{
  struct csv_token *token;
  ptrdiff_t n;

  CSVASSERT(p);
  CSVASSERT(startp);
  CSVASSERT(endp);

  token = (struct csv_token *)malloc(sizeof(struct csv_token));
  if (!token) {
    if (err) *err = CSV_ERR_NOMEM;
    return 1;
  }

  n = endp - startp;
  if (!allocp) {
    allocp = (char *)malloc(sizeof(char) * n);
    memcpy(allocp, startp, n);
  } else {
    if (allocp != startp) {
      memmove(allocp, startp, n);
    }
  }

  geom_list_insert_prev(&p->token_head.list, &token->list);
  token->start = allocp;
  token->end = endp - startp + allocp;
  token->line = line;
  token->col = col;

  return 0;
}

enum csv_add_mode {
  ADD_CELL = 0x1,
  ADD_ROW  = 0x2,
  ADD_CELL_ROW = ADD_CELL | ADD_ROW,
};

static int
csvAddCellSupport(csv_parser *p, enum csv_add_mode mode, csv_error *err)
{
  struct geom_list *lp, *ln, *lh;
  char *buf;
  char *dst;
  csv_column *col;
  csv_row *row;
  ptrdiff_t n;
  long l, c;

  CSVASSERT(p);
  CSVASSERT(p->data);

  row = p->row;

  if (mode & ADD_CELL) {
    lh = &p->token_head.list;
    n = 0;

    if (!geom_list_empty(lh)) {
      geom_list_foreach(lp, lh) {
        struct csv_token *tok;
        tok = csv_token_entry(lp);
        n += tok->end - tok->start;
        if (n < 0) {
          if (err) *err = CSV_ERR_2BIG;
          return 1;
        }
      }
      if (n + 1 < 0) {
        if (err) *err = CSV_ERR_2BIG;
        return 1;
      }

      buf = (char *)malloc(sizeof(char) * (n + 1));
      if (!buf) {
        if (err) *err = CSV_ERR_NOMEM;
        return 1;
      }

      if (geom_alloc_add(p->data->allocs, buf, free) != GEOM_SUCCESS) {
        if (err) *err = CSV_ERR_NOMEM;
        free(buf);
        return 1;
      }

      dst = buf;
      l = -1;
      c = -1;
      geom_list_foreach_safe(lp, ln, lh) {
        struct csv_token *tok;
        tok = csv_token_entry(lp);

        if (l < 0) {
          l = tok->line;
          c = tok->col;
        }
        n = tok->end - tok->start;
        memcpy(dst, tok->start, n);
        dst += n;

        geom_list_delete(lp);
        free(tok->start);
        free(tok);
      }
      *dst = '\0';

      if (!row) {
        row = newCSVRow(p->data);
        if (!row) {
          if (err) *err = CSV_ERR_NOMEM;
          return 1;
        }
        p->row = row;
      }
      col = newCSVColumn(row);
      if (!col) {
        if (err) *err = CSV_ERR_NOMEM;
        return 1;
      }
      col->value = buf;
      col->line = l;
      col->cols = c;
      appendCSVColumn(row, col);
    }
  }
  if (mode & ADD_ROW) {
    if (row) {
      appendCSVRow(p->data, row);
    }
    row = NULL;
  }
  p->row = row;

  return 0;
}

static int
csv_fill(csv_parser *p, FILE *stream, int n)
{
  ptrdiff_t off;
  int ret;

  off = 0;
  ret = re2c_lparser_fill(&p->lp, stream, NULL, n, &off);
  return ret;
}

csv_error
readCSV(FILE *stream, csv_data **data, long *error_line, long *error_col)
{
  csv_parser parser;
  char *tmp;
  int n;
  csv_error ret;

  CSVASSERT(stream);
  CSVASSERT(data);

  *data = NULL;

  parser.data = allocateCSV();
  if (!parser.data) return CSV_ERR_NOMEM;

  parser.row = NULL;
  parser.cs_line = -1;
  parser.cs_column = -1;
  geom_list_init(&parser.token_head.list);
  parser.token_head.start = NULL;
  parser.token_head.end   = NULL;
  parser.use_quote = quote_notset;

  ret = CSV_ERR_SUCC;
  re2c_lparser_init(&parser.lp);
  if (re2c_lparser_fill(&parser.lp, stream, NULL, 1, NULL) < 0) goto error;

start:
  re2c_lparser_start_token(&parser.lp);
  /*!re2c
    re2c:define:YYCTYPE = "unsigned char";
    re2c:define:YYCURSOR = "parser.lp.cur";
    re2c:define:YYLIMIT = "parser.lp.lim";
    re2c:define:YYMARKER = "parser.lp.mrk";
    re2c:define:YYFILL:naked = 1;
    re2c:define:YYFILL = "{ if (csv_fill(&parser, stream, @@) < 0) goto error; }";
    re2c:yyfill:enable = 1;
    re2c:indent:top = 1;
    re2c:indent:string = "  ";

    space = [ \t];
    separator = space*","space*;
    nl = space* ("\r\n" | "\r" | "\n");
    end = "\x00";

    * {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (!parser.lp.lexeme) {
        if (parser.use_quote == in_quote) {
          ret = CSV_ERR_MIX_QUOTE;
          goto error;
        }
        parser.use_quote = out_quote;
        parser.lp.lexeme = parser.lp.tok;
        parser.cs_line = parser.lp.tokline;
        parser.cs_column = parser.lp.tokcol;
      }
      goto start;
    }

    separator {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
        parser.lp.lexeme = NULL;
      }
      if (csvAddCellSupport(&parser, ADD_CELL, &ret)) goto error;
      parser.use_quote = quote_notset;
      goto start;
    }

    space* / "--" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.tok == parser.lp.cur) {
        parser.lp.cur++;
        goto start;
      }
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      parser.lp.lexeme = NULL;
      if (csvAddCellSupport(&parser, ADD_CELL_ROW, &ret)) goto error;
      parser.use_quote = quote_notset;
      goto line_comment;
    }

    space* "/""*" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      parser.lp.lexeme = NULL;
      goto block_comment;
    }

    "\"" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.use_quote == out_quote) {
        ret = CSV_ERR_MIX_QUOTE;
        goto error;
      }
      parser.use_quote = in_quote;
      parser.lp.lexeme = parser.lp.cur;
      parser.cs_line = parser.lp.line;
      parser.cs_column = parser.lp.col;
      goto instring;
    }

    space* "&" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      parser.lp.lexeme = NULL;
      goto continuing;
    }

    nl   {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      parser.lp.lexeme = NULL;
      if (csvAddCellSupport(&parser, ADD_CELL_ROW, &ret)) goto error;
      parser.use_quote = quote_notset;
      goto start;
    }

    end  {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      if (parser.lp.lexeme && parser.lp.lexeme < parser.lp.tok) {
        if (csvAddToken(&parser, parser.lp.lexeme, parser.lp.tok, NULL,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      parser.lp.lexeme = NULL;
      if (csvAddCellSupport(&parser, ADD_CELL_ROW, &ret)) goto error;
      parser.use_quote = quote_notset;
      goto end;
    }

    space* {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto start;
    }
  */

  CSVUNREACHABLE();

instring:
  re2c_lparser_start_token(&parser.lp);
  /*!re2c
    re2c:indent:top = 1;

    * {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto instring;
    }

    ("\"\"" | "\\\"") {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto instring;
    }

    "\"" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      tmp = unescapeCSVCellString(parser.lp.lexeme, parser.lp.tok, &n);
      if (!tmp) {
        ret = CSV_ERR_NOMEM;
        goto error;
      }
      parser.lp.lexeme = NULL;
      if (n > 0) {
        n--;
        if (csvAddToken(&parser, tmp, tmp + n, tmp,
                        parser.cs_line, parser.cs_column, &ret)) goto error;
      }
      goto start;
    }

    nl {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto instring;
    }

    end {
      ret = CSV_ERR_EOF;
      goto error;
    }
  */

  CSVUNREACHABLE();

line_comment:
  re2c_lparser_start_token(&parser.lp);
  /*!re2c
    re2c:indent:top = 1;

    * {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto line_comment;
    }

    end {
      goto end;
    }

    nl {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto start;
    }
  */

  CSVUNREACHABLE();

block_comment:
  re2c_lparser_start_token(&parser.lp);
  /*!re2c
    re2c:indent:top = 1;

    * {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto block_comment;
    }

    end {
      ret = CSV_ERR_EOF;
      goto error;
    }

    "*""/" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto start;
    }
  */

  CSVUNREACHABLE();

continuing:
  re2c_lparser_start_token(&parser.lp);

  /*!re2c
    re2c:indent:top = 1;

    * {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      ret = CSV_ERR_DATA_AFTER_CONT;
      goto error;
    }

    end {
      ret = CSV_ERR_EOF;
      goto error;
    }

    space {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto continuing;
    }

    "/""*" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      ret = CSV_ERR_BLOCK_COMMENT_AFTER_CONT;
      goto error;
    }

    "--" {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto line_comment;
    }

    nl {
      re2c_lparser_loc_upd_utf8(&parser.lp);
      goto start;
    }
  */

  CSVUNREACHABLE();

end:
  *data = parser.data;
  ret = CSV_ERR_SUCC;
  goto clean;

clean:
  re2c_lparser_clean(&parser.lp);
  {
    struct geom_list *lp, *ln, *lh;
    lh = &parser.token_head.list;
    geom_list_foreach_safe(lp, ln, lh) {
      struct csv_token *t;
      t = csv_token_entry(lp);
      free(t->start);
      free(t);
    }
  }
  return ret;

error:
  if (error_line) *error_line = parser.lp.tokline;
  if (error_col)  *error_col = parser.lp.tokcol;
  if (parser.lp.readerr) {
    ret = parser.lp.readerr;
  }
  freeCSV(parser.data);
  goto clean;
}

csv_column *getNextColumn(csv_column *c)
{
  struct geom_list *lp;

  if (!c) return NULL;

  lp = geom_list_next(&c->list);
  c = csv_column_entry(lp);
  if (!c->parent || c == &c->parent->column_head) return NULL;
  return c;
}

csv_row *getNextRow(csv_row *r)
{
  struct geom_list *lp;

  if (!r) return NULL;

  lp = geom_list_next(&r->list);
  r = csv_row_entry(lp);
  if (!r->parent || r == &r->parent->row_head) return NULL;
  return r;
}

csv_column *getPrevColumn(csv_column *c)
{
  struct geom_list *lp;

  if (!c) return NULL;

  lp = geom_list_prev(&c->list);
  c = csv_column_entry(lp);
  if (!c->parent || c == &c->parent->column_head) return NULL;
  return c;
}

csv_row *getPrevRow(csv_row *r)
{
  struct geom_list *lp;

  if (!r) return NULL;

  lp = geom_list_prev(&r->list);
  r = csv_row_entry(lp);
  if (!r->parent || r == &r->parent->row_head) return NULL;
  return r;
}

csv_row *getRowOfCSV(csv_data *d, int index)
{
  struct geom_list *lp;
  csv_row *r;

  CSVASSERT(d);

  r = &d->row_head;
  lp = &r->list;
  if (index >= 0) {
    lp = geom_list_next(lp);
    while (--index >= 0) {
      lp = geom_list_next(lp);
      if (lp == &r->list) break;
    }
  } else if (index < 0) {
    lp = geom_list_prev(lp);
    while (++index < 0) {
      lp = geom_list_prev(lp);
      if (lp == &r->list) break;
    }
  }
  if (lp == &r->list) return NULL;
  return csv_row_entry(lp);
}

csv_column *getColumnOfCSV(csv_row *d, int index)
{
  struct geom_list *lp;
  csv_column *r;

  CSVASSERT(d);

  r = &d->column_head;
  lp = &r->list;
  if (index >= 0) {
    lp = geom_list_next(lp);
    while (--index >= 0) {
      lp = geom_list_next(lp);
      if (lp == &r->list) break;
    }
  } else if (index < 0) {
    lp = geom_list_prev(lp);
    while (++index < 0) {
      lp = geom_list_prev(lp);
      if (lp == &r->list) break;
    }
  }
  if (lp == &r->list) return NULL;
  return csv_column_entry(lp);
}

const char *getCSVValue(csv_column *c)
{
  CSVASSERT(c);
  return c->value;
}

csv_error setCSVValue(csv_column *c, const char *text, size_t len)
{
  csv_data *data;
  char *t;

  CSVASSERT(c);
  CSVASSERT(c->parent);

  data = c->parent->parent;
  CSVASSERT(data);
  CSVASSERT(data->allocs);

  t = (char *)malloc(sizeof(char) * (len + 1));
  if (!t) return CSV_ERR_NOMEM;

  if (geom_alloc_add(data->allocs, t, free) != GEOM_SUCCESS) {
    return CSV_ERR_NOMEM;
  }

  strncpy(t, text, len);
  t[len] = '\0';

  c->value = t;

  return CSV_ERR_SUCC;
}

long getCSVTextLineOrigin(csv_column *c)
{
  if (!c) return -1;
  return c->line;
}

long getCSVTextColumnOrigin(csv_column *c)
{
  if (!c) return -1;
  return c->cols;
}

void setCSVTextOrigin(csv_column *col, long l, long c)
{
  if (!col) return;
  col->line = l;
  col->cols = c;
}

csv_row *findCSVRow(csv_data *d, const char *keystr, int len)
{
  unsigned long l;
  struct geom_list *lp, *lh;
  csv_row *r;
  const char *c;

  CSVASSERT(d);
  CSVASSERT(keystr);
  CSVASSERT(len >= 0);

  l = hashCSVKey(keystr, len);
  lp = &d->rowtable[l];
  lh = lp;
  geom_list_foreach(lp, lh) {
    r = csv_row_hash_entry(lp);
    c = getCSVRowKey(r);
    if (c) {
      if (strncmp(c, keystr, len) == 0) {
        return r;
      }
    }
  }
  return NULL;
}

csv_row *findCSVRowNext(csv_row *o)
{
  unsigned long l;
  csv_data *d;
  csv_row *r;
  const char *c;
  const char *keystr;
  struct geom_list *lp, *ls, *lh;
  int len;

  CSVASSERT(o);

  d = o->parent;

  CSVASSERT(d);

  keystr = getCSVRowKey(o);

  CSVASSERT(keystr);

  len = strlen(keystr) + 1;
  l = hashCSVKey(keystr, len);

  lh = &d->rowtable[l];
  ls = geom_list_next(&o->hash_list);
  geom_list_foreach_range(lp, ls, lh) {
    r = csv_row_hash_entry(lp);
    c = getCSVRowKey(o);
    if (c) {
      if (strncmp(c, keystr, len) == 0) {
        return r;
      }
    }
  }
  return NULL;
}

void dumpCSV(FILE *stream, csv_data *t) {
  csv_row *r;
  csv_column *c;
  const char *cc;
  int ir, ic;

  ir = 1;
  r = getRowOfCSV(t, 0);
  while (r) {
    ic = 1;
    c = getColumnOfCSV(r, 0);
    fprintf(stream, "# -------------------------------------------------\n");
    fprintf(stream, "# CSVRow:CSVCol:TxtLno:TxtCol:HV:Cell\n");
    fprintf(stream, "# -------------------------------------------------\n");
    while (c) {
      cc = getCSVValue(c);
      fprintf(stream, "# %6d:%6d:%6ld:%6ld:%02lx:%s:\n",
              ir, ic,
              getCSVTextLineOrigin(c),
              getCSVTextColumnOrigin(c),
              hashCSVKey(cc, strlen(cc)), cc);
      c = getNextColumn(c);
      ic++;
    }
    r = getNextRow(r);
    ir++;
  }
  fprintf(stream, "# -------------------------------------------------\n");
}

