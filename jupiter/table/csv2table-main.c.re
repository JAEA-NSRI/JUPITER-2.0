/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "table.h"
#include "csv2table.h"

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:indent:string = "  ";
  re2c:define:YYCURSOR = "p.cursor";
  re2c:define:YYMARKER = "p.marker";
  re2c:define:YYCTXMARKER = "p.ctxmarker";
  // re2c:define:YYLIMIT = "p.limit";
 */

typedef struct
{
  int argc;
  const char *progn;
  const char *input;
  const char *output;
  int input_stdin;
  int output_stdout;
  int verbose;
  int quiet;
  int interpolate;
  int adjust_coordinate;
  int force_rectilinear;
  int force_sum_const_map;
  double tolarence;
} csv2tab_options;

typedef struct
{
  char *buf;
  const char *cursor;
  const char *marker;
  const char *ctxmarker;
  const char *token;
} csv2tab_parser;

int csv2tab_optparse(csv2tab_options *opts, int argc, char **argv);
void csv2tab_help(const char *progn);
void csv2tab_print_table(const char *progn,
                         const char *input_file, csv2tab_data *data);

int csv2tab_asprintf(char **buf, const char *format, ...);
int csv2tab_vasprintf(char **buf, const char *format, va_list ap);

void csv2tab_msgf(const char *progn, const char *catname,
                  const char *filen, long line, long column,
                  const char *message, ...);
void csv2tab_vmsgf(const char *progn, const char *catname,
                   const char *filen, long line, long column,
                   const char *message, va_list ap);

void csv2tab_errorf(const char *progn, const char *filen,
                    long line, long column, const char *message, ...);
void csv2tab_verrorf(const char *progn, const char *filen,
                     long line, long column, const char *message, va_list ap);
void csv2tab_warnf(const char *progn, const char *filen,
                   long line, long column, const char *message, ...);
void csv2tab_vwarnf(const char *progn, const char *filen,
                     long line, long column, const char *message, va_list ap);

int main(int argc, char **argv)
{
  table_data *table = NULL;
  csv2tab_data *c2t_data = NULL;
  csv2tab_options opts;
  csv2tab_error e;
  csv2tab_stat_node *snode;
  table_error te;
  FILE *input = NULL;
  long l;
  long c;
  int r;

  r = 1;

  if (csv2tab_optparse(&opts, argc, argv)) goto error;

  if (!opts.input_stdin) {
    if (!opts.input) {
      csv2tab_errorf(opts.progn, NULL, 0, 0, "No input file specified");
      goto error;
    }
    if (strcmp(opts.input, "") == 0) {
      opts.input = NULL;
      opts.input_stdin = 1;
    }
  } else {
    opts.input = NULL;
  }
  if (!opts.output) {
    if (opts.output_stdout) {
      csv2tab_errorf(opts.progn, NULL, 0, 0, "Cannot write binary data to stdout");
    } else {
      csv2tab_errorf(opts.progn, NULL, 0, 0, "No output file specified");
    }
    return EXIT_FAILURE;
  } else if (strcmp(opts.output, "") == 0) {
    csv2tab_errorf(opts.progn, NULL, 0, 0, "Output file name is empty");
    goto error;
  }

  table = table_alloc();
  c2t_data = csv2tab_init();

  if (!table || !c2t_data) {
    csv2tab_errorf(opts.progn, NULL, 0, 0, "Memory allocation failed");
    goto error;
  }
  csv2tab_set_x_column(c2t_data, 0);
  csv2tab_set_y_column(c2t_data, 1);
  csv2tab_set_v_column(c2t_data, 2);
  csv2tab_set_adjust_coordinates(c2t_data, opts.adjust_coordinate);
  csv2tab_set_tolerance(c2t_data, opts.tolarence);
  if (opts.force_rectilinear) {
    csv2tab_set_force_geometry(c2t_data, TABLE_GEOMETRY_RECTILINEAR);
  }
  if (opts.force_sum_const_map) {
    csv2tab_set_force_geometry(c2t_data, TABLE_GEOMETRY_SUM_CONSTANT);
  }
  csv2tab_set_interpolation(c2t_data, opts.interpolate);

  if (opts.input) {
    errno = 0;
    input = fopen(opts.input, "rb");
    if (!input || errno != 0) {
      char *e = strerror(errno);
      csv2tab_errorf(opts.progn, opts.input, 0, 0, "cannot open: %s", e);
      goto error;
    }
  } else {
    input = stdin;
  }
  if (opts.input_stdin) {
    csv2tab_msgf(opts.progn, NULL, NULL, 0, 0, "Input CSV data here and put EOF to end");
  }

  l = 0;
  c = 0;
  table_set_interp_mode(table, TABLE_INTERP_BARYCENTRIC);
  csv2tab_convert(c2t_data, input, table, &e, &l, &c, &te);
  if (opts.verbose) {
    csv2tab_print_table(opts.progn, opts.input ? opts.input : "-", c2t_data);
  }
  if (!opts.quiet) {
    size_t si;
    const char *ifn;
    ifn = opts.input;
    if (!ifn) ifn = "-";
    si = 0;
    snode = csv2tab_get_stats(c2t_data);
    while (snode) {
      csv2tab_node *n;
      csv2tab_status st;
      long line;
      double x, y;
      n = csv2tab_get_node_by_stat(snode);
      if (!n) continue;
      line = csv2tab_get_node_source_line(n);
      x = csv2tab_get_node_x(n);
      y = csv2tab_get_node_y(n);
      st = csv2tab_get_node_flag(n);
      if (line > 0) { /* defined in CSV file */
        if ((st & CSV2TAB_STAT_INF) != 0) {
          csv2tab_msgf(opts.progn, NULL, ifn, line, 0,
                       "at (%g, %g): Required value(s) are infinite", x, y);
          si++;
        }
        if ((st & CSV2TAB_STAT_NAN) != 0) {
          csv2tab_msgf(opts.progn, NULL, ifn, line, 0,
                       "at (%g, %g): Required value(s) are not-a-number",
                       x, y);
          si++;
        }
        if ((st & CSV2TAB_STAT_MULTIPLE_XY) != 0) {
          csv2tab_msgf(opts.progn, NULL, ifn, line, 0,
                       "at (%g, %g): Defined twice or more", x, y);
          si++;
        }
        if ((st & CSV2TAB_STAT_DISCARD) != 0) {
          csv2tab_msgf(opts.progn, NULL, ifn, line, 0,
                       "at (%g, %g): This value is discarded", x, y);
          si++;
        }
      } else {
        if ((st & CSV2TAB_STAT_NAN) != 0) {
          csv2tab_msgf(opts.progn, NULL, ifn, 0, 0,
                       "at (%g, %g): This position not defined", x, y);
          si++;
        }
      }
      snode = csv2tab_status_next(snode);
    }
  }
  if (e != CSV2TAB_SUCCESS) {
    if (e == CSV2TAB_ERR_SYS) {
      char *es = strerror(errno);
      csv2tab_errorf(opts.progn, opts.input, l, c, "%s", es);
    } else if (e == CSV2TAB_ERR_INIT_TABLE) {
      char *es;
      int re;
      re = table_aerrorstr(&es, te, errno);
      if (re >= 0) {
        csv2tab_errorf(opts.progn, opts.input, l, c, "%s", es);
        free(es);
      } else {
        csv2tab_errorf(opts.progn, opts.input, l, c, "%s",
                       table_errorstr(te));
      }
    } else {
      csv2tab_errorf(opts.progn, opts.input, l, c, "%s",
                     csv2tab_errorstr(e));
    }
    goto error;
  }

  te = table_set_title(table, opts.input, 0);
  if (te == TABLE_ERR_NOMEM) {
    csv2tab_errorf(opts.progn, NULL, 0, 0, "Memory allocation failed");
    goto error;
  }

  errno = 0;
  te = table_write_binary(table, opts.output);
  r = errno;

  if (te != TABLE_SUCCESS) {
    char *e;
    const char *ce;
    int bs;

    bs = table_aerrorstr(&e, te, r);
    if (bs >= 0) {
      ce = e;
    } else {
      if (te == TABLE_ERR_SYS) {
        ce = strerror(r);
      } else {
        ce = table_errorstr(te);
      }
      e = NULL;
    }
    csv2tab_errorf(opts.progn, opts.output, 0, 0, "%s", ce);
    free(e);
    goto error;
  }

  r = 0;

 error:
  table_free(table);
  csv2tab_free(c2t_data);
  if (input && input != stdin) {
    fclose(input);
  }

  if (r != 0) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

/*!re2c
  end = "\x00";
*/

int csv2tab_optparse(csv2tab_options *opts, int argc, char **argv)
{
  int i;
  int c;
  int ign;
  csv2tab_parser p;
  char **v;
  const char **loc;
  double *dloc;
  int *farg;

  TABLE_ASSERT(opts && argv);

  c = argc;
  v = argv;

  farg = NULL;
  dloc = NULL;

  if (c == 0) return 1;

  memset(&p, 0, sizeof(csv2tab_parser));
  memset(opts, 0, sizeof(csv2tab_options));
  opts->progn = v[0];
  opts->tolarence = 0.0;
  ign = 0;

  for (i = 1; i < argc; ++i) {
    p.token = v[i];
    p.cursor = p.token;

    for (;;) {
      p.token = p.cursor;
      /*!re2c
        re2c:yyfill:enable = 0;
        re2c:indent:top = 3;

        *    { goto file; }
        "--" { goto double_hyphen; }
        "-"  { goto hyphen; }
       */
      TABLE_UNREACHABLE();
    }
    TABLE_UNREACHABLE();

  hyphen:
    if (ign) goto file;
    p.token = p.cursor;
    loc = NULL;
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;
        *   { p.cursor = p.token; break; }
        end { goto stdfile; }
      */
      TABLE_UNREACHABLE();
    }
    for (;;) {
      p.token = p.cursor;
      /*!re2c
        re2c:indent:top = 3;
        *   { goto invalid; }
        end { p.cursor = p.token; break; }
        "h" { goto help; }
        "o" { loc = &opts->output; break; }
        "t" { dloc = &opts->tolarence; break; }
        "r" { opts->force_rectilinear = 1; continue; }
        "s" { opts->force_sum_const_map = 1; continue; }
        "v" { opts->verbose = 1; continue; }
        "q" { opts->quiet = 1; continue; }
      */
      TABLE_UNREACHABLE();
    }
    if (loc || dloc) {
      p.token = p.cursor;
      /*!re2c
        re2c:indent:top = 3;
        *   { p.cursor = p.token; goto argument_here; }
        end { goto argument_next; }
      */
      TABLE_UNREACHABLE();
    }
    continue; /* to next argument */

  double_hyphen:
    if (ign) goto file;
    p.token = p.cursor;
    loc = NULL;
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;
        *   { goto invalid; }
        end { ign = 1; break; }
        "output"      { goto long_output; }
        "help"/end    { goto help; }
        "tolarence"/end { dloc = &opts->tolarence; break; }
        "verbose"/end { opts->verbose = 1; break; }
        "quiet"/end   { opts->quiet = 1; break; }
        "interpolate"/end { opts->interpolate = 1; break; }
        "adjust-coordinate"/end { opts->adjust_coordinate = 1; break; }
        "rectilinear"/end { opts->force_rectilinear = 1; break; }
        "sum-constant"/end { opts->force_sum_const_map = 1; break; }
      */
      TABLE_UNREACHABLE();

    long_output:
      loc = &opts->output;
      farg = &opts->output_stdout;
      break;
    }
    if (loc || dloc) {
      p.token = p.cursor;
      /*!re2c
        re2c:indent:top = 3;
        * { goto invalid; }
        "="    { p.token = p.cursor; goto argument_here; }
        "\x00" { goto argument_next; }
      */
      TABLE_UNREACHABLE();
    }
    continue; /* to next argument */

  invalid:
    csv2tab_errorf(v[0], NULL, 0, 0, "invalid argument: %s", v[i]);
    goto help;

  stdfile:
    if (ign) goto file;
    if (!(opts->input || opts->input_stdin )) {
      opts->input_stdin = 1;
    } else if (!(opts->output || opts->output_stdout)) {
      opts->output_stdout = 1;
    } else {
      csv2tab_errorf(v[0], NULL, 0, 0,
                    "extranous filename given, ignored: %s", v[i]);
    }
    continue; /* to next argument */

  file:
    if (!(opts->input || opts->input_stdin)) {
      opts->input = v[i];
    } else if (!(opts->output || opts->output_stdout)) {
      opts->output = v[i];
    } else {
      csv2tab_errorf(v[0], NULL, 0, 0,
                     "extranous filename given, ignored: %s", v[i]);
    }
    continue; /* to next argument */

  argument_next:
    if (i < argc - 1) {
      i++;
      p.token = v[i];
    } else {
      csv2tab_errorf(v[0], NULL, 0, 0,
                     "no parameter specified for: %s", v[i]);
      goto help;
    }

  argument_here:
    if (loc)  goto string_option;
    if (dloc) goto double_option;
    goto invalid;

  string_option:
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;
        * { break; }
        "-"/end { if (farg) { *farg = 1; *loc = NULL; loc = NULL; } break; }
      */
    }
    if (loc) {
      *loc = p.token;
    }
    loc = NULL;
    farg = NULL;
    continue;

  double_option:
    {
      double d;
      char *tmp;

      d = strtod(p.token, &tmp);
      if (*tmp == '\0') {
        *dloc = d;
      } else {
        csv2tab_errorf(v[0], NULL, 0, 0,
                       "Invalid double value: %s", p.token);
        goto help;
      }
      dloc = NULL;
    }
    continue;
  }
  if (opts->force_rectilinear && opts->force_sum_const_map) {
    csv2tab_errorf(v[0], NULL, 0, 0,
                   "Cannot use --rectilinear (-r) and --sum-const (-s) at same time");
    goto help;
  }

  return 0;

 help:
  csv2tab_help(v[0]);
  return 1;
}

void csv2tab_help(const char *progn)
{
  fprintf(stderr,
          "Usage: %s [input] [output]\n"
          "\n"
          "Options:\n"
          "  -h, --help          Show help (this message)\n"
          "  -o, --output=OUT    Output filename\n"
          "  -v, --verbose       Print sorted table\n"
          "  -q, --quiet         Do not print diagnostics\n"
          "\n"
          "  -t, --torelance=N   Set double-precision tolerance to N\n"
          "                      (default is 0)\n"
          "  --interpolate       Enable interpolation for missing value\n"
          "  --adjust-coordinate Adjust coordinate to match requirement of\n"
          "                      sum-constant map\n"
          "  -r, --rectilinear   Force to parse as rectilinear\n"
          "  -s, --sum-constant  Force to parse as sum-constant map\n"
          "      --              Stop parse arguments\n"
          "\n",
          progn);
}

int csv2tab_asprintf(char **buf, const char *format, ...)
{
  int r;
  va_list ap;
  va_start(ap, format);
  r = csv2tab_vasprintf(buf, format, ap);
  va_end(ap);
  return r;
}

int csv2tab_vasprintf(char **buf, const char *format, va_list ap)
{
#if defined(_GNU_SOURCE)
  /*
   * glibc and BSD libc
   */
  return vasprintf(buf, format, ap);

#elif defined(_ISOC99_SOURCE) ||                                \
  (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
  defined(_BSD_SOURCE) ||                                       \
  (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) ||           \
  (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) ||   \
  (defined(_MSC_VER) && _MSC_VER >= 1900)
  /*
   * C99
   */

  va_list c;
  char *p;
  int n;
  if (!buf) return -1;
  va_copy(c, ap);
  p = NULL;
  n = vsnprintf(p, 0, format, ap);
  if (n < 0) return n;
  p = (char*)malloc(sizeof(char) * (n + 1));
  if (!p) return -1;
  n = vsnprintf(p, n + 1, format, c);
  *buf = p;
  return n;

#elif defined(_MSC_VER)
  /*
   * Old Visual Studio
   */

  int r, len;
  size_t size;
  char *str;
  /* _vscprintf tells you how big the buffer needs to be */
  len = _vscprintf(format, ap);
  if (len == -1) {
    return -1;
  }
  size = (size_t)len + 1;
  str = malloc(size);
  if (!str) {
    return -1;
  }
  /* _vsprintf_s is the "secure" version of vsprintf */
  r = _vsprintf_s(str, len + 1, format, ap);
  if (r == -1) {
    free(str);
    return -1;
  }
  *buf = str;
  return r;

#else
# error "We need to be C99 is available at least."
#endif
}

void csv2tab_verrorf(const char *progn,
                     const char *filen, long line, long column,
                     const char *message, va_list ap)
{
  csv2tab_vmsgf(progn, "error", filen, line, column, message, ap);
}


void csv2tab_errorf(const char *progn,
                    const char *filen, long line, long column,
                    const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  csv2tab_verrorf(progn, filen, line, column, message, ap);
  va_end(ap);
}

void csv2tab_vwarnf(const char *progn,
                    const char *filen, long line, long column,
                    const char *message, va_list ap)
{
  csv2tab_vmsgf(progn, "warning", filen, line, column, message, ap);
}

void csv2tab_warnf(const char *progn, const char *filen,
                   long line, long column, const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  csv2tab_vwarnf(progn, filen, line, column, message, ap);
  va_end(ap);
}

void csv2tab_msgf(const char *progn, const char *catname,
                  const char *filen, long line, long column,
                  const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  csv2tab_vmsgf(progn, catname, filen, line, column, message, ap);
  va_end(ap);
}

void csv2tab_vmsgf(const char *progn, const char *catname,
                   const char *filen, long line, long column,
                   const char *message, va_list ap)
{
  va_list aq;
  const char *cfc;
  char *cfa = NULL;
  const char *fnsc;
  char *fnsa = NULL;
  char *buf;
  int r;

  if (!progn) progn = "csv2table";

  if (catname) {
    r = csv2tab_asprintf(&cfa, "%s: ", catname);
    if (r > 0) {
      cfc = cfa;
    } else {
      cfa = NULL;
      cfc = "";
    }
  } else {
    cfc = "";
  }
  if (filen) {
    if (line > 0) {
      if (column > 0) {
        r = csv2tab_asprintf(&fnsa, "%s(l%ld,c%ld): ", filen, line, column);
      } else {
        r = csv2tab_asprintf(&fnsa, "%s(l%ld): ", filen, line);
      }
    } else {
      r = csv2tab_asprintf(&fnsa, "%s: ", filen);
    }
    if (r > 0) {
      fnsc = fnsa;
    } else {
      fnsa = NULL;
      fnsc = "(error): ";
    }
  } else {
    fnsc = "";
  }

  va_copy(aq, ap);
  r = csv2tab_vasprintf(&buf, message, ap);
  if (r < 0) {
    fprintf(stderr, "%s: %s%s", progn, cfc, fnsc);
    vfprintf(stderr, message, aq);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "%s: %s%s%s\n", progn, cfc, fnsc, buf);
    free(buf);
  }
  free(fnsa);
  free(cfa);
  va_end(aq);
}

void csv2tab_print_table(const char *progn,
                         const char *input_file, csv2tab_data *data)
{
  csv2tab_node *xn;
  csv2tab_node *yn;
  size_t xi;
  size_t yi;

  xn = csv2tab_get_node_root(data);
  xi = 0;
  if (xn) {
    fprintf(stdout,
            "#- Sorted Table List -- %s\n"
            "#\n"
            "#  Flags:\n"
            "#    F: Value is infinite.\n"
            "#    N: Value is not-a-number (or not defined).\n"
            "#    M: Defined for twice or more for same (x, y) value\n"
            "#    O: Overflowed when reading (huge absolute value)\n"
            "#    U: Underflowed when reading (changed to 0)\n"
            "#    S: Could not build as sum-is-const mapping\n"
            "#       (flagged at first element only)\n"
            "#    E: Not found in CSV\n"
            "#    I: Value is interpolated\n"
            "#\n"
            "#  Values showen here are shortened, and\n"
            "#  we do not recommend to use these values as\n"
            "#  a (new) CSV data\n",
            input_file);
  }
  while (xn) {
    yn = xn;
    yi = 0;
    fprintf(stdout, "\n# %8s %8s %25s %5s %5s %7s %s\n",
            "X-cord", "Y-cord", "Value", "X-idx", "Y-idx", "Line No", "Flag");
    while (yn) {
      int i;
      csv2tab_status st;
      char f[9];
      memset(f, 0, 9);
      i = 0;
      st = csv2tab_get_node_flag(yn);
      if ((st & CSV2TAB_STAT_INF) != 0) {
        f[i++] = 'F';
      }
      if ((st & CSV2TAB_STAT_NAN) != 0) {
        f[i++] = 'N';
      }
      if ((st & CSV2TAB_STAT_MULTIPLE_XY) != 0) {
        f[i++] = 'M';
      }
      if ((st & CSV2TAB_STAT_OVERFLOW) != 0) {
        f[i++] = 'O';
      }
      if ((st & CSV2TAB_STAT_UNDERFLOW) != 0) {
        f[i++] = 'U';
      }
      if ((st & CSV2TAB_STAT_SUMC_NONCONSTRAINT) != 0) {
        f[i++] = 'S';
      }
      if (csv2tab_get_node_source_line(yn) == 0) {
        f[i++] = 'E';
      }
      if ((st & CSV2TAB_STAT_INTERPOLATE) != 0) {
        f[i++] = 'I';
      }
      if (f[0] == '\0') {
        f[0] = '-';
      }
      fprintf(stdout, "  %8.5g %8.5g %25.17g %5zu %5zu %7ld %s\n",
              csv2tab_get_node_x(yn),
              csv2tab_get_node_y(yn),
              csv2tab_get_node_value(yn),
              xi, yi,
              csv2tab_get_node_source_line(yn), f);
      yn = csv2tab_get_node_y_next(yn);
      ++yi;
    }
    xn = csv2tab_get_node_x_next(xn);
    ++xi;
  }
}
