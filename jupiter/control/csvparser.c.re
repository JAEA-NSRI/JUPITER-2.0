/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

/*!re2c
  re2c:indent:string = "  ";

  nl = ('\r\n'|'\n'|'\r');
  comma = ',';
  quote = '"';
 */

#include "csvparser.h"
#include "data_array.h"
#include "error.h"
#include "jupiter/re2c_lparser/re2c_lparser.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "defs.h"
#include "static_array.h"
#include "string_array.h"

#include <stddef.h>
#include <errno.h>
#include <stdlib.h>

struct jcntrl_csvparser
{
  jcntrl_shared_object object;
  FILE *fp;
  int opened;
  struct re2c_lparser lparser;
  jcntrl_char_array *filename_d;
  jcntrl_string_array *row_cells;
  jcntrl_size_type next_column_index;
  int error;
};
#define jcntrl_csvparser__ancestor jcntrl_shared_object
#define jcntrl_csvparser__dnmem object
JCNTRL_VTABLE_NONE(jcntrl_csvparser);

static jcntrl_csvparser *
jcntrl_csvparser_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_csvparser, obj);
}

static void *jcntrl_csvparser_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_csvparser_downcast_impl(obj);
}

static int jcntrl_csvparser_initializer(jcntrl_shared_object *obj)
{
  jcntrl_csvparser *p = jcntrl_csvparser_downcast_impl(obj);
  p->fp = NULL;
  p->opened = 0;
  re2c_lparser_init(&p->lparser);
  p->filename_d = NULL;
  p->row_cells = jcntrl_string_array_new();
  p->next_column_index = 0;
  p->error = 0;
  if (!p->row_cells)
    return 0;
  return 1;
}

static void jcntrl_csvparser_destructor(jcntrl_shared_object *obj)
{
  jcntrl_csvparser *p = jcntrl_csvparser_downcast_impl(obj);
  re2c_lparser_clean(&p->lparser);
  jcntrl_csvparser_close_stream(p);
  if (p->filename_d)
    jcntrl_char_array_delete(p->filename_d);
  if (p->row_cells)
    jcntrl_string_array_delete(p->row_cells);
}

static jcntrl_shared_object *jcntrl_csvparser_allocator(void)
{
  jcntrl_csvparser *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_csvparser);
  return p ? jcntrl_csvparser_object(p) : NULL;
}

static void jcntrl_csvparser_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static void jcntrl_csvparser_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_csvparser_downcast_v;
  p->initializer = jcntrl_csvparser_initializer;
  p->destructor = jcntrl_csvparser_destructor;
  p->allocator = jcntrl_csvparser_allocator;
  p->deleter = jcntrl_csvparser_deleter;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_csvparser, jcntrl_csvparser_init_func)

jcntrl_csvparser *jcntrl_csvparser_new(void)
{
  return jcntrl_shared_object_new(jcntrl_csvparser);
}

void jcntrl_csvparser_delete(jcntrl_csvparser *p)
{
  jcntrl_shared_object_delete(jcntrl_csvparser_object(p));
}

jcntrl_shared_object *jcntrl_csvparser_object(jcntrl_csvparser *p)
{
  return &p->object;
}

jcntrl_csvparser *jcntrl_csvparser_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_csvparser, obj);
}

int jcntrl_csvparser_set_input_file(jcntrl_csvparser *p, const char *filename)
{
  jcntrl_data_array *d;
  jcntrl_static_cstr_array fn;
  jcntrl_size_type l;

  if (!filename)
    return jcntrl_csvparser_set_input_file_d(p, NULL);

  l = strlen(filename);
  if (jcntrl_s_add_overflow(l, 1, &l))
    return 0;

  jcntrl_static_cstr_array_init_base(&fn, filename, l);
  d = jcntrl_static_cstr_array_data(&fn);

  return jcntrl_csvparser_set_input_file_d(p, d);
}

int jcntrl_csvparser_set_input_file_d(jcntrl_csvparser *p,
                                      jcntrl_data_array *filename)
{
  intmax_t iv;
  int ierr;
  jcntrl_size_type l, cs;

  l = 0;

  if (filename) {
    JCNTRL_ASSERT(jcntrl_char_array_copyable(filename));

    l = jcntrl_data_array_get_ntuple(filename);
  }
  if (l <= 0) {
    if (p->filename_d)
      jcntrl_char_array_delete(p->filename_d);
    p->filename_d = NULL;
    return 0;
  }

  ierr = 0;
  iv = jcntrl_data_array_get_ivalue(filename, l - 1, &ierr);
  if (ierr)
    return 0;

  cs = l;
  if (iv != '\0') {
    if (jcntrl_s_add_overflow(l, 1, &cs))
      return 0;
  }

  if (!p->filename_d) {
    p->filename_d = jcntrl_char_array_new();
    if (!p->filename_d)
      return 0;
  }

  if (!jcntrl_char_array_resize(p->filename_d, cs))
    return 0;

  return jcntrl_char_array_copy(p->filename_d, filename, l, 0, 0);
}

static void jcntrl_csvparser_reset(jcntrl_csvparser *p)
{
  re2c_lparser_clean(&p->lparser);
  re2c_lparser_init(&p->lparser);
  jcntrl_string_array_resize(p->row_cells, 0);
  p->error = 0;
}

void jcntrl_csvparser_set_stream(jcntrl_csvparser *p, FILE *fp)
{
  jcntrl_csvparser_close_stream(p);
  p->fp = fp;
  p->opened = 0;
  jcntrl_csvparser_reset(p);
}

void jcntrl_csvparser_close_stream(jcntrl_csvparser *p)
{
  if (p->opened && p->fp)
    fclose(p->fp);
  p->fp = NULL;
}

int jcntrl_csvparser_error(jcntrl_csvparser *p) { return p->error; }

int jcntrl_csvparser_eof(jcntrl_csvparser *p)
{
  if (p->lparser.buf)
    return re2c_lparser_eof(&p->lparser);
  return 0;
}

/**
 * The filename is NUL-terminated
 */
const char *jcntrl_csvparser_get_input_file(jcntrl_csvparser *p)
{
  if (!p->filename_d)
    return NULL;
  return jcntrl_char_array_get(p->filename_d);
}

int jcntrl_csvparser_open_file(jcntrl_csvparser *p)
{
  const char *fn;

  if (p->fp)
    return 1;

  fn = jcntrl_csvparser_get_input_file(p);
  if (!fn || strlen(fn) == 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Empty filename given");
    return 0;
  }

  p->fp = fopen(fn, "rb");
  if (!p->fp) {
    jcntrl_raise_errno_error(fn, 0, errno, "Could not open file");
    return 0;
  }

  p->opened = 1;
  jcntrl_csvparser_reset(p);
  return 1;
}

int jcntrl_csvparser_is_open(jcntrl_csvparser *p) { return !!p->fp; }

static int jcntrl_csvparser_fill(jcntrl_csvparser *p, int n)
{
  int r;
  const char *fn;
  ptrdiff_t offt;

  offt = 0;
  fn = jcntrl_csvparser_get_input_file(p);
  r = re2c_lparser_fill(&p->lparser, p->fp, fn, n, &offt);
  if (r != 0) {
    if (r < 0) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    } else {
      jcntrl_raise_errno_error(fn, p->lparser.line, errno, "Failed to read");
    }
    return 0;
  }

  if (offt != 0) {
    jcntrl_size_type ncells = jcntrl_string_array_get_ntuple(p->row_cells);
    for (jcntrl_size_type i = 0; i < ncells; ++i) {
      jcntrl_size_type l;
      const char *op;
      jcntrl_char_array *cp;
      cp = jcntrl_string_array_get(p->row_cells, i);
      if (!cp)
        continue;

      op = jcntrl_char_array_get(cp);
      l = jcntrl_char_array_get_ntuple(cp);
      if (!op)
        continue;

      jcntrl_char_array_bind(cp, op + offt, l);
    }
  }
  return 1;
}

static void jcntrl_csvparser_clear_rows(jcntrl_csvparser *p)
{
  jcntrl_size_type n = jcntrl_string_array_get_ntuple(p->row_cells);
  for (jcntrl_size_type i = 0; i < n; ++i)
    jcntrl_string_array_set(p->row_cells, i, NULL);
}

int jcntrl_csvparser_rewind(jcntrl_csvparser *p)
{
  const char *fn;
  JCNTRL_ASSERT(jcntrl_csvparser_is_open(p));

  fn = jcntrl_csvparser_get_input_file(p);

  if (fseek(p->fp, 0, SEEK_SET)) {
    jcntrl_raise_errno_error(fn, 0, errno, "Failed to seek");
    return 0;
  }

  re2c_lparser_clean(&p->lparser);
  re2c_lparser_init(&p->lparser);
  jcntrl_csvparser_clear_rows(p);
  return jcntrl_csvparser_fill(p, 0);
}

static int jcntrl_csvparser_check_eof(jcntrl_csvparser *p)
{
  if (re2c_lparser_eof(&p->lparser))
    return 0;
  return 1;
}

static void jcntrl_csvparser_goto_tok(jcntrl_csvparser *p)
{
  JCNTRL_ASSERT(p && p->lparser.tok);

  p->lparser.cur = p->lparser.tok;
  p->lparser.line = p->lparser.tokline;
  p->lparser.col = p->lparser.tokcol;
}

static int jcntrl_csvparser_start_cell(jcntrl_csvparser *p)
{
  jcntrl_char_array *d;
  jcntrl_size_type l;

  l = jcntrl_string_array_get_ntuple(p->row_cells);
  if (p->next_column_index >= l) {
    if (jcntrl_s_add_overflow(p->next_column_index, 10, &l)) {
      if (jcntrl_s_add_overflow(p->next_column_index, 1, &l)) {
        p->error = 1;
        return 0;
      }
    }

    if (!jcntrl_string_array_resize(p->row_cells, l)) {
      p->error = 1;
      return 0;
    }
  }

  d = jcntrl_string_array_get(p->row_cells, p->next_column_index);
  if (!d) {
    d = jcntrl_char_array_new();
    if (!d) {
      p->error = 1;
      return 0;
    }

    if (!jcntrl_string_array_set(p->row_cells, p->next_column_index, d)) {
      jcntrl_char_array_delete(d);
      p->error = 1;
      return 0;
    }
  }

  if (!jcntrl_char_array_bind(d, p->lparser.cur, 1)) {
    jcntrl_char_array_delete(d);
    p->error = 1;
    return 0;
  }

  jcntrl_char_array_delete(d);
  return 1;
}

static void jcntrl_csvparser_complete_row(jcntrl_csvparser *p)
{
  jcntrl_string_array_resize(p->row_cells, p->next_column_index);
  p->next_column_index = 0;
}

static jcntrl_char_array *jcntrl_csvparser_make_cell(jcntrl_csvparser *p)
{
  const char *cp;
  jcntrl_size_type l;
  jcntrl_char_array *d;

  d = jcntrl_string_array_get(p->row_cells, p->next_column_index);
  if (!d) {
    if (!jcntrl_csvparser_start_cell(p))
      return NULL;

    d = jcntrl_string_array_get(p->row_cells, p->next_column_index);
  }
  JCNTRL_ASSERT(d);

  cp = jcntrl_char_array_get(d);
  JCNTRL_ASSERT(cp);

  l = p->lparser.tok - cp;
  if (l > 0) {
    jcntrl_char_array_bind(d, cp, l);
  } else {
    jcntrl_char_array_resize(d, 0);
  }
  ++p->next_column_index;
  return d;
}

static jcntrl_char_array *jcntrl_csvparser_make_qcell(jcntrl_csvparser *p)
{
  char *base;
  char *cur;
  char *mrk;
  char *tok;
  char *dst;
  char *lim;
  jcntrl_char_array *d;

  d = jcntrl_csvparser_make_cell(p);
  if (!d)
    return NULL;

  if (jcntrl_char_array_get_ntuple(d) == 0)
    return d;

  base = jcntrl_char_array_get_writable(d);
  if (!base)
    return NULL;

  lim = base + jcntrl_char_array_get_ntuple(d);
  cur = base;
  dst = cur;
  while (cur < lim) {
    tok = cur;
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "cur";
      re2c:define:YYMARKER = "mrk";
      re2c:yyfill:enable = 0;
      re2c:indent:top = 2;

      * {
        *dst = *tok;
        ++dst;
        continue;
      }
      quote { goto quote; }
    */
    JCNTRL_UNREACHABLE();

  quote:
    if (cur >= lim)
      break;

    tok = cur;
    /*!re2c
      re2c:indent:top = 2;

      * { dst = tok; break; }
      quote { dst = tok; continue; }
     */
    JCNTRL_UNREACHABLE();
  }

  if (!jcntrl_char_array_resize(d, dst - base))
    return NULL;
  return d;
}

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "p->lparser.cur";
  re2c:define:YYLIMIT = "p->lparser.lim";
  re2c:define:YYMARKER = "p->lparser.mrk";
  re2c:define:YYCTXMARKER = "p->lparser.ctxmrk";
  re2c:define:YYFILL:naked = 1;
  re2c:define:YYFILL = "{ if (!jcntrl_csvparser_fill(p, @@)) goto fillerror; }";
  re2c:yyfill:enable = 1;
*/

jcntrl_char_array *jcntrl_csvparser_read_cell(jcntrl_csvparser *p)
{
  int nl;
  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(jcntrl_csvparser_is_open(p));

  if (p->next_column_index == 0) {
    p->lparser.lexeme = p->lparser.cur;
    jcntrl_csvparser_clear_rows(p);
  }

  if (re2c_lparser_eof(&p->lparser))
    goto eof;

  if (!p->lparser.buf) {
    if (!jcntrl_csvparser_fill(p, 0))
      return NULL;
  }

  nl = 0;

  re2c_lparser_start_token(&p->lparser);
  for (;;) {
    /*!re2c
      re2c:indent:top = 2;

      * { if (!jcntrl_csvparser_check_eof(p)) return NULL; break; }
      quote { goto quote; }
      comma { return jcntrl_csvparser_make_cell(p); }
      nl { jcntrl_csvparser_complete_row(p); return NULL; }
     */
  }

  jcntrl_csvparser_goto_tok(p);
  if (!jcntrl_csvparser_start_cell(p))
    return NULL;

  for (;;) {
    re2c_lparser_loc_upd_utf8(&p->lparser);
    re2c_lparser_start_token(&p->lparser);
    /*!re2c
      re2c:indent:top = 2;

      * { if (!jcntrl_csvparser_check_eof(p)) break; continue; }
      quote { goto syntax_error; }
      comma { return jcntrl_csvparser_make_cell(p); }
      nl    {
        jcntrl_csvparser_goto_tok(p);
        return jcntrl_csvparser_make_cell(p);
      }
    */
  }
  return jcntrl_csvparser_make_cell(p);

quote:
  if (!jcntrl_csvparser_start_cell(p))
    return NULL;

  for (;;) {
    re2c_lparser_loc_upd_utf8(&p->lparser);
    re2c_lparser_start_token(&p->lparser);
    /*!re2c
      re2c:indent:top = 2;

      * { if (!jcntrl_csvparser_check_eof(p)) goto incomplete; continue; }
      quote quote { continue; }
      quote / nl  { return jcntrl_csvparser_make_qcell(p); }
      quote comma { return jcntrl_csvparser_make_qcell(p); }
      quote { goto incomplete; }
    */
  }
  JCNTRL_UNREACHABLE();
  p->error = 1;
  return NULL;

eof:
  jcntrl_csvparser_complete_row(p);
  return NULL;

incomplete:
  p->error = 1;
  jcntrl_raise_argument_error(jcntrl_csvparser_get_input_file(p),
                              p->lparser.tokline, "Unclosed quoted cell");
  return NULL;

syntax_error:
  p->error = 1;
  jcntrl_raise_argument_error(jcntrl_csvparser_get_input_file(p),
                              p->lparser.line, "CSV syntax error");
  return NULL;

fillerror:
  p->error = 1;
  return NULL;
}

jcntrl_char_array *jcntrl_csvparser_read_cell_copy(jcntrl_csvparser *p)
{
  jcntrl_size_type l;
  jcntrl_char_array *n;
  jcntrl_char_array *d;
  d = jcntrl_csvparser_read_cell(p);
  if (!d)
    return NULL;

  n = jcntrl_char_array_new();
  if (!n)
    return NULL;

  l = jcntrl_char_array_get_ntuple(d);
  if (!jcntrl_char_array_resize(n, l)) {
    jcntrl_char_array_delete(n);
    return NULL;
  }

  if (!jcntrl_char_array_copy(n, jcntrl_char_array_data(d), l, 0, 0)) {
    jcntrl_char_array_delete(n);
    return NULL;
  }
  return n;
}

jcntrl_string_array *jcntrl_csvparser_read_row(jcntrl_csvparser *p)
{
  while (jcntrl_csvparser_read_cell(p))
    /* nop */;
  if (p->error)
    return NULL;
  if (jcntrl_string_array_get_ntuple(p->row_cells) == 0 &&
      re2c_lparser_eof(&p->lparser))
    return NULL;
  return p->row_cells;
}

jcntrl_string_array *jcntrl_csvparser_read_row_copy(jcntrl_csvparser *p)
{
  jcntrl_size_type l;
  jcntrl_string_array *a, *b;

  a = jcntrl_csvparser_read_row(p);
  if (!a)
    return NULL;

  b = jcntrl_string_array_new();
  if (!b)
    return NULL;

  l = jcntrl_string_array_get_ntuple(a);
  if (!jcntrl_string_array_resize(b, l)) {
    jcntrl_string_array_delete(b);
    return NULL;
  }

  if (!jcntrl_string_array_deep_copy(b, a, l, 0, 0)) {
    jcntrl_string_array_delete(b);
    return NULL;
  }
  return b;
}
