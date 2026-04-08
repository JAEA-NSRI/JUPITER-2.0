/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#include "csvtmpl_format.h"
#include "csvutil.h"
#include "os/asprintf.h"

/*!re2c
  format_regex = '%'[-+ #0]*([1-9][0-9]*)?('.'[0-9]+)?[a-zA-Z];
*/

struct tval {
  char mode;
  union tval_data {
    int d;
    char *s;
  } data;
};

struct eval {
  char e;
  struct tval *ev;
  struct eval *p;
  struct eval *n;
};

/*
 * On some architectures (such as x86_64),
 * pointer to va_list cannot be handled correctly. So...
 */
struct vapp {
  va_list ap;
};

static struct tval *add_tval(char type, struct vapp *ap)
{
  struct tval *p = (struct tval*)malloc(sizeof(struct tval));
  if (p) {
    switch (type) {
    case 'd':
      p->data.d = va_arg(ap->ap, int);
      break;
    case 's':
      p->data.s = va_arg(ap->ap, char *);
      break;
    }
    p->mode = type;
  }
  return p;
}

static void free_tval(struct tval **t, size_t nt)
{
  size_t i;
  if (t) {
    for(i = 0; i != nt; ++i) {
      free(t[i]);
    }
  }
}

static struct eval *
add_eval(struct tval **t, size_t nt, char c, struct eval *l)
{
  struct eval *p;
  struct tval *tp;
  int ind;

  if (!t) { return NULL; }

  ind = -1;
  if (c >= 'A' && c <= 'Z') {
    ind = c - 'A';
  } else if (c >= 'a' && c <= 'z') {
    ind = c - 'a' + 26;
  }
  if (ind < 0 || (size_t)ind >= nt) { return NULL; }

  tp = t[ind];
  if (!tp) { return NULL; }

  p = (struct eval*)malloc(sizeof(struct eval));
  if (p) {
    p->e = c;
    p->ev = tp;
    p->p = l;
    p->n = NULL;
    if (l) {
      l->n = p;
    }
  }
  return p;
}

static void free_eval(struct eval *s)
{
  struct eval *n;
  if (!s) return;
  while(s) {
    n = s->n;
    free(s);
    s = n;
  }
}

#define TVAL_SIZE 52
static int char_to_tval_idx(unsigned char ch)
{
  unsigned char base;
  int offs;
  struct sp {
    const unsigned char *p;
  } p;

  p.p = &ch;
  for (;;) {
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "p.p";
      re2c:define:YYMARKER = "p.m";
      re2c:yyfill:enable = 0;
      re2c:indent:top = 2;
      re2c:indent:string = "  ";

      * { return -1; }

      // Splitted by EBCDIC coding (In ASCII, it is continuous through A to Z)
      [A-I] { base = 'A'; offs =  0; break; }
      [J-R] { base = 'J'; offs =  9; break; }
      [S-Z] { base = 'S'; offs = 18; break; }
      [a-i] { base = 'a'; offs = 26; break; }
      [j-r] { base = 'j'; offs = 35; break; }
      [s-z] { base = 's'; offs = 44; break; }
    */

    CSVUNREACHABLE();
    return -1;
  }

  return ch - base + offs;
}

/* Format integers */
int format_integersv(char **retp, const char *fmt,
                     const char *format_keys, va_list ap)
{
  struct sp {
    const char *t;
    const char *p;
    const char *m;
  } p;

  struct vapp app;

  char *buf, *bufo;
  int pass;
  int cnt;
  char mode;

  /* flags */
  int left_adj, alter, plus, fsp;
  char pad_char;

  int columns, precs;
  int c, w, lp, nd;
  struct tval *embed_val;
  int evv;

  struct tval *tval_a[TVAL_SIZE]; /* A-Z, a-z */
  struct eval *eval_s, *eval_p;

  va_copy(app.ap, ap);

  CSVASSERT(fmt);

  eval_s = NULL;
  eval_p = NULL;
  for(cnt = 0; cnt < TVAL_SIZE; cnt++) {
    tval_a[cnt] = NULL;
  }

  mode = 'd';
  p.p = format_keys;
  for (;;) {
    struct tval *pp;

    p.t = p.p;
    /*!re2c
      re2c:indent:top = 2;

      * { goto error; }
      " " { continue; }
      '\x00' { break; }

      [A-Za-z] { goto addval; }
      "[d]" { mode = 'd'; continue; }
      "[s]" { mode = 's'; continue; }
    */

    CSVUNREACHABLE();
    return -1;

  addval:
    pp = add_tval(mode, &app);
    if (!pp) {
      free_tval(tval_a, TVAL_SIZE);
      return -1;
    }

    tval_a[char_to_tval_idx(*p.t)] = pp;
  }

  p.p = fmt;

  eval_s = NULL;
  eval_p = NULL;
  for (;;) {
    p.t = p.p;
    /*!re2c
      re2c:indent:top = 2;

      * { continue; }
      '\x00' { break; }
      '%%'   { continue; }
      format_regex {
        struct eval *e = add_eval(tval_a, TVAL_SIZE, *(p.p - 1), eval_p);
        if (!e) { goto error; }
        if (!eval_s) eval_s = e;
        eval_p = e;
        continue;
      }
    */
  }

  for (pass = 0; pass < 2; ++pass) {
    p.p = fmt;
    eval_p = eval_s;

    if (pass) {
      buf = malloc(sizeof(char) * (cnt + 1));
      if (!buf) goto error;
      bufo = buf;
    } else {
      cnt = 0;
    }

    for (;;) {
      p.t = p.p;

      /*!re2c
        re2c:indent:top = 3;

        * {
          if (pass) {
            *(buf++) = *p.t;
          } else {
            cnt++;
          }
          continue;
        }

        "\x00" {
          if (pass) {
            *(buf++) = '\0';
          } else {
            cnt++;
          }
          break;
        }

        "%%" {
          if (pass) {
            *(buf++) = '%';
          } else {
            cnt++;
          }
          continue;
        }

        "%" { goto percent; }
      */
      continue;

    percent:
      left_adj = 0;
      plus = 0;
      alter = 0; /* this flag has no effect */
      pad_char = ' ';
      fsp = 0;
      if (!eval_p) goto error;
      embed_val = eval_p->ev;
      eval_p = eval_p->n;

      switch(embed_val->mode) {
      case 'd':
        goto fmt_int;
      case 's':
        goto fmt_str;
      }
      CSVUNREACHABLE();

    fmt_int:
      evv = embed_val->data.d;

      /* parse flags. */
      for (;;) {
        p.t = p.p;
        /*!re2c
          re2c:indent:top = 4;
          *   { break; }
          "\x00" { goto error; }
          "-" { left_adj = 1; continue; }
          "+" { plus = 1; continue; }
          "#" { alter = 1; continue; }
          " " { fsp = 1; continue; }
          "0" { pad_char = '0'; continue; }
        */
      }

      if (plus) pad_char = ' ';
      p.p = p.t;
      columns = 0;
      precs = 0;

      /* padding */
      for (;;) {
        if (columns < 0) { goto error; }
        p.t = p.p;
        /*!re2c
          re2c:indent:top = 4;
          *     { break; }
          "\x00" { goto error; }
          [0-9] { columns = columns * 10 + (*p.t - '0'); continue; }
        */
      }
      switch(embed_val->mode) {
      case 'd':
        if (fsp && columns > 0) { columns--; }
        break;
      default:
        CSVUNREACHABLE();
      }

      if (*p.t == '.') {
        /* precisions */
        for (;;) {
          if (precs < 0) { goto error; }
          p.t = p.p;
          /*!re2c
            re2c:indent:top = 5;
            *     { break; }
            "\x00" { goto error; }
            [0-9] { precs = precs * 10 + (*p.t - '0'); continue; }
          */
        }
        if (precs > 0) {
          pad_char = ' ';
        }
      }

      /*
       * count number of charactors to be written.
       */
      if (evv != 0) {
        int it = (evv < 0) ? -evv : evv;
        nd = 0;
        while (it > 0) {
          nd++;
          it = it / 10;
        }
      } else {
        nd = 1;
      }
      c = (nd < precs) ? precs : nd;
      if (plus || evv < 0) {
        c++;
      }
      if (left_adj) {
        lp = 0;
      } else {
        if (columns > c) {
          lp = columns - c;
        } else {
          lp = 0;
        }
      }
      w = (c < columns) ? columns : c;
      if (fsp && columns > 0) w = w + 1;

      /* embed number */
      p.p = p.t;
      for (;;) {
        p.t = p.p;
        /*!re2c
          re2c:indent:top = 4;
          *      { goto error; }
          "\x00" { goto error; }
          [a-zA-Z]  {
            if (pass) {
              int t;
              if (!plus && fsp && evv >= 0) {
                *(buf++) = ' '; w--;
              }
              if (pad_char != ' ' && evv < 0) {
                *(buf++) = '-'; c--; w--;
              }
              while (lp-- > 0) { *(buf++) = pad_char; w--; }
              if (pad_char == ' ' && evv < 0) {
                *(buf++) = '-'; c--; w--;
              } else if (plus) {
                *(buf++) = '+'; c--; w--;
              }
              while (c > nd) { *(buf++) = '0'; c--; w--; }
              t = c;
              if (evv < 0) evv = -evv;
              while (c > 0)  {
                *(buf+c-1) = evv % 10 + '0';
                c--; w--; evv /= 10;
              }
              buf += t;
              while (w > 0) {
                *(buf++) = ' ';
                w--;
              }
            } else {
              cnt += w;
            }
            break;
          }
        */
      }
      continue; /* make sure in loop */

    fmt_str:
      /* String format does not allow setting width and precision */
      for (;;) {
        p.t = p.p;
        /*!re2c
          re2c:indent:top = 4;
          *      { goto error; }
          "\x00" { goto error; }
          [a-zA-Z]  {
            int n;
            n = strlen(embed_val->data.s);
            if (n < 0) { goto error; }
            if (pass) {
              strcpy(buf, embed_val->data.s);
              buf += n;
            } else {
              cnt += n;
            }
            break;
          }
        */
      }
      continue; /* make sure in loop */
    }

    if (!retp) {
      break;
    }
  }

  free_tval(tval_a, TVAL_SIZE);
  free_eval(eval_s);
  va_end(app.ap);

  if (retp) {
    *retp = bufo;
  }
  return cnt - 1;

 error:
  free_tval(tval_a, TVAL_SIZE);
  free_eval(eval_s);
  va_end(app.ap);
  return -1;
}

int format_integers(char **retp, const char *fmt,
                    const char *format_keys, ...)
{
  int r;
  va_list ap;
  va_start(ap, format_keys);
  r = format_integersv(retp, fmt, format_keys, ap);
  va_end(ap);
  return r;
}

int make_glob_pattern(char **retp, const char *fmt)
{
  char *buf;
  char *bufc;
  int sz;
  struct sp {
    const char *t;
    const char *p;
    const char *m;
  } p;
  int pass;
  int recurr;

  sz = 0;
  recurr = 0;
  for (pass = 0; pass < 2; ++pass) {
    p.p = fmt;
    if (pass) {
      buf = (char *)malloc(sizeof(char) * sz);
      if (!buf) return -1;
      bufc = buf;
    }
    sz = 0;
    for (;;) {
      p.t = p.p;
      /*!re2c
        re2c:indent:top = 3;

        *      { recurr = 0; if (pass) { *bufc++ = *p.t; } sz++; continue; }
        '\x00' { recurr = 0; if (pass) { *bufc++ = '\0'; } sz++; break; }
        '%%'   {
          recurr = 0;
          if (pass) {
            *bufc++ = '%';
          }
          sz += 1;
          continue;
        }
        format_regex {
          if (!recurr) {
            if (pass) {
              *bufc++ = '*';
            }
            sz += 1;
          }
          recurr = 1;
          continue;
        }
      */

      CSVUNREACHABLE();
      return -1;
    }
    if (!retp) break;
  }

  *retp = buf;
  return sz;
}

int format_integers_match(const char *scan, const char *fmt,
                          int ndata, format_integers_match_data *data)
{
  struct sp {
    const char *t;
    const char *p;
    const char *m;
  } p;
  const char *scan_token;
  const char *scan_cursor;

  int i;
  int r;
  int tval_a[TVAL_SIZE]; /* A-Z, a-z */

  for (i = 0; i < TVAL_SIZE; ++i) {
    tval_a[i] = -1;
  }

  for (i = 0; i < ndata; ++i) {
    tval_a[char_to_tval_idx(data[i].key)] = i;
  }

  for (i = 0; i < ndata; ++i) {
    if (data[i].string) {
      *(data[i].string) = NULL;
    }
    data[i].matched = 0;
    data[i].err = 0;
  }

  r = 1;
  p.p = fmt;
  scan_cursor = scan;
  for (;;) {
    p.t = p.p;
    scan_token = scan_cursor;

    /*!re2c
      re2c:indent:top = 2;

      * {
        scan_cursor++;
        if (*scan_token != *p.t) {
          r = 0;
          break;
        }
        continue;
      }

      "\x00" {
        if (*scan_token != '\0') {
          r = 0;
        }
        break;
      }

      "%%" {
        scan_cursor++;
        if (*scan_token != '%') {
          r = 0;
          break;
        }
        continue;
      }

      format_regex { goto match_value; }
    */

    CSVUNREACHABLE();
    return -1;

  match_value:
    {
      char ch;
      const char *fmt_token;
      const char *fmt_cursor;
      format_integers_match_data *cur_data;

      ch = *(p.p - 1);
      cur_data = &data[tval_a[char_to_tval_idx(ch)]];

      fmt_token = p.t;
      fmt_cursor = p.p;

      switch (cur_data->type) {
      case FORMAT_MATCH_INT:
        {
          int v;
          int n;
          int f;

          n = 0;
          f = 0;
          v = 0;
          p.p = scan_cursor;
          for (;;) {
            p.t = p.p;
            /*!re2c
              re2c:indent:top = 6;
              *   { goto int_unmatch; }
              " " { continue; }
              "-" { if (n || f) goto int_unmatch; n = 1; continue; }
              "+" { if (n || f) goto int_unmatch; f = 1; continue; }
              [0-9] { v = *p.t - '0'; goto int_cont; }
            */
            CSVUNREACHABLE();
            return -1;

          int_cont:
            p.t = p.p;
            /*!re2c
              re2c:indent:top = 6;

              *  { goto int_match; }
              [0-9] {
                v = v * 10 + (*p.t - '0');
                if (v < 0) goto overflow;
                goto int_cont;
              }
            */
            CSVUNREACHABLE();
            return -1;

          overflow:
            v = INT_MAX;
            for (;;) {
              p.t = p.p;
              /*!re2c
                re2c:indent:top = 7;

                * { break; }
                [0-9] { continue; }
              */

              CSVUNREACHABLE();
              return -1;
            }
            cur_data->err = ERANGE;

          int_match:
            scan_cursor = p.t;
            cur_data->matched = 1;
            cur_data->value = v;
            break;

          int_unmatch:
            r = 0;
            break;
          }
        }
        break;

      case FORMAT_MATCH_STR:
        {
          p.p = scan_cursor;
          for (;;) {
            p.t = p.p;
            /*!re2c
              re2c:indent:top = 6;

              * { goto str_unmatch; }
              " " { continue; }
              [A-Za-z][A-Za-z0-9]* { goto str_match; }
             */
            CSVUNREACHABLE();
            return -1;

          str_match:
            scan_cursor = p.p;
            if (cur_data->string) {
              if (cur_data->matched) {
                free(*cur_data->string);
              }
              r = jupiter_asprintf(cur_data->string, "%.*s", p.p - p.t, p.t);
              if (r < 0) {
                *cur_data->string = NULL;
                cur_data->err = ENOMEM;
              }
              r = 1;
            }
            cur_data->matched = 1;
            break;

          str_unmatch:
            r = 0;
            break;
          }
        }
        break;

      default:
        CSVUNREACHABLE();
        return -1;
      }

      if (!r) break;

      p.p = fmt_cursor;
      p.t = fmt_token;
    }
    continue;
  }

  return r;
}
