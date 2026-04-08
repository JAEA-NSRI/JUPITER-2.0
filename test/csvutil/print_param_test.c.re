/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <jupiter/re2c_lparser/re2c_lparser.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/manager.h>
#include <jupiter/csv.h>
#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/mat43.h>
#include <jupiter/geometry/vector.h>
#include <jupiter/print_param_vecmat.h>
#include <jupiter/print_param_core.h>
#include <jupiter/print_param_comp.h>
#include <jupiter/print_param_basic.h>
#include <jupiter/csvutil.h>
#include <jupiter/struct.h>
#include <jupiter/os/asprintf.h>

#include <float.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#define SHARED_FILE_NAME "print_param_test.txt"
#define RANK_FILE_NAME "print_param_test.%d.txt"

static int for_any_rank(int cond);
static int for_all_rank(int cond);
static char *rank_file_name(void);
static int write_message(char *rank_fn, flags *flg);
static int test_message(char *rank_fn, flags *flg);

int main(int argc, char **argv)
{
  char *rank_fn;
  int r = EXIT_SUCCESS;
  flags flg;

#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif

  memset(&flg, 0, sizeof(flags));
  flg.fp = stderr;
#ifdef JUPITER_MPI
  flg.list_fp_mpi = MPI_FILE_NULL;
  flg.list_fp_comm = MPI_COMM_NULL;
#endif

  rank_fn = rank_file_name();
  if (!write_message(rank_fn, &flg))
    r = EXIT_FAILURE;

  if (flg.list_fp)
    fclose(flg.list_fp);
#ifdef JUPITER_MPI
  if (flg.list_fp_mpi != MPI_FILE_NULL)
    MPI_File_close(&flg.list_fp_mpi);
  if (flg.list_fp_comm != MPI_COMM_NULL)
    MPI_Comm_free(&flg.list_fp_comm);
#endif

  if (!test_message(rank_fn, &flg))
    r = EXIT_FAILURE;

  free(rank_fn);

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif
  return r;
}

static int for_any_rank(int cond)
{
#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
#endif
  return cond;
}

static int for_all_rank(int cond)
{
#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
#endif
  return cond;
}

static int fail(const char *ok, void *arg) { return 0; }

static char *rank_file_name(void)
{
  char *buf;
  int rank;
  int r;

  rank = 0;
#ifdef JUPITER_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

  r = jupiter_asprintf(&buf, RANK_FILE_NAME, rank);
  if (r < 0) {
    csvperror(__FILE__, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }
  return buf;
}

static int write_message(char *rank_fn, flags *flg)
{
  int nogood;
  int r;
  int rank;
  struct PP_charp_name_data n;
  struct PP_charp_value_data v;
  struct PP_charp_unit_data u;

  if (!rank_fn)
    return 0;

  rank = 0;
#ifdef JUPITER_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

  errno = 0;
  flg->list_fp_name = rank_fn;
  flg->list_fp = fopen(rank_fn, "wb");
  if (for_any_rank(!flg->list_fp)) {
    if (!flg->list_fp)
      csvperror(rank_fn, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, errno, 0,
                NULL);
    return 0;
  }

  r = OFF;
  nogood = OFF;

  print_param_header(flg, '-', 0, 3, PP_HEADER_LEN, "This is header");

  PP_charp_name_init(&n, "base");
  PP_charp_value_init(&v, "test", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "base (no unit)");
  PP_charp_value_init(&v, "test", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "base (fail with unit)");
  PP_charp_value_init(&v, "test", "(err)", fail, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "base (fail without unit)");
  PP_charp_value_init(&v, "test", "(err)", fail, NULL);
  PP_charp_unit_init(&u, "");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  nogood = OFF;

  PP_charp_name_init(&n,
                     "This is very very very very very very very long name");
  PP_charp_value_init(&v, "test", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "boooo");
  PP_charp_value_init(
    &v, "This is very very very very very very very very long value", "(err)",
    NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "multi");
  PP_charp_value_init(&v, "This is value with\nnew line\n", "(err)", NULL,
                      NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "multi4");
  PP_charp_value_init(&v, "This\nis\nvalue\nwith\nnew\nline", "(err)", NULL,
                      NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "multi5");
  PP_charp_value_init(&v, "This\nis\nvalue\nwith\nnew line", "(err)", NULL,
                      NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "This is name with\nnew line");
  PP_charp_value_init(&v, "1", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "This is ..................... name with\nnew line");
  PP_charp_value_init(&v, "This is value with\nnew line", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "This is somewhat longer name with ....");
  PP_charp_value_init(&v, "                  This\nis value with new line",
                      "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Null");
  PP_charp_value_init(&v, NULL, "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Both Null");
  PP_charp_value_init(&v, NULL, NULL, NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Left");
  PP_charp_value_init(&v, "1", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, -1, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Left-multi");
  PP_charp_value_init(&v, "1\n2\n33", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, -1, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Left-multi 2");
  PP_charp_value_init(&v, "  1\n222\n33\n", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, -1, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "This is Left-justified\nvalue");
  PP_charp_value_init(&v, "1", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, -1, &n.data, &v.data, &u.data, &nogood);

  print_param_header(flg, '-', 0, 3, PP_HEADER_LEN, "Types");

  PP_charp(flg, 0, "string", "value", "", 1, &nogood);
  PP_charp(flg, 0, "string (empty)", "", "", 1, &nogood);
  PP_charp(flg, 0, "string (null)", NULL, "", 1, &nogood);
  PP_int(flg, 0, "int", 0, "", 1, &nogood);
  PP_int(flg, 0, "int", 1, "", 1, &nogood);
  PP_int(flg, 0, "int", -1, "", 1, &nogood);
  PP_int(flg, 0, "int", INT_MIN, "", 1, &nogood);
  PP_int(flg, 0, "int", INT_MAX, "", 1, &nogood);
  PP_double(flg, 0, "double", 0.0, "", 1, &nogood);
  PP_double(flg, 0, "double", 1.0, "", 1, &nogood);
  PP_double(flg, 0, "double", -1.0, "", 1, &nogood);
  PP_double(flg, 0, "double", DBL_MAX, "", 1, &nogood);
  PP_double(flg, 0, "double", DBL_MIN, "", 1, &nogood);
#ifdef INFINITY
  PP_double(flg, 0, "double", INFINITY, "", 1, &nogood);
  PP_double(flg, 0, "double", -INFINITY, "", 1, &nogood);
#endif
#ifdef NAN
  PP_double(flg, 0, "double", NAN, "", 1, &nogood);
#endif
  PP_double_ns(flg, 0, "double, not set", 0.0, "", 1, &nogood);
  PP_double_ns(flg, 0, "double, not set", 1.0, "", 1, &nogood);
  PP_double_ns(flg, 0, "double, not set", -1.0, "", 1, &nogood);
  PP_double_ns(flg, 0, "double, not set", DBL_MAX, "", 1, &nogood);
  PP_double_ns(flg, 0, "double, not set", DBL_MIN, "", 1, &nogood);
#ifdef INFINITY
  PP_double_ns(flg, 0, "double, not set", INFINITY, "", 1, &nogood);
  PP_double_ns(flg, 0, "double, not set", -INFINITY, "", 1, &nogood);
#endif
#ifdef NAN
  PP_double_ns(flg, 0, "double, not set", NAN, "", 1, &nogood);
#endif
  PP_double_inf(flg, 0, "double, inf", 0.0, "", 1, &nogood);
  PP_double_inf(flg, 0, "double, inf", 1.0, "", 1, &nogood);
  PP_double_inf(flg, 0, "double, inf", -1.0, "", 1, &nogood);
  PP_double_inf(flg, 0, "double, inf", DBL_MAX, "", 1, &nogood);
  PP_double_inf(flg, 0, "double, inf", DBL_MIN, "", 1, &nogood);
#ifdef INFINITY
  PP_double_inf(flg, 0, "double, inf", INFINITY, "", 1, &nogood);
  PP_double_inf(flg, 0, "double, inf", -INFINITY, "", 1, &nogood);
#endif
#ifdef NAN
  PP_double_inf(flg, 0, "double, inf", NAN, "", 1, &nogood);
#endif

  PP_geom_vec2(flg, 0, "vec2", geom_vec2_c(0., 0.), "", 1, &nogood);
  PP_geom_vec2(flg, 0, "vec2", geom_vec2_c(1.5, -1.5), "", 1, &nogood);
  PP_geom_vec2(flg, 0, "vec2", geom_vec2_c(-1.5, 1), "", 1, &nogood);
  PP_geom_vec3(flg, 0, "vec3", geom_vec3_c(0., 0., 0.), "", 1, &nogood);
  PP_geom_vec3(flg, 0, "vec3", geom_vec3_c(1, -1.5, 0.), "", 1, &nogood);
  PP_geom_vec3(flg, 0, "vec3", geom_vec3_c(-1.5, 1.5, 0.), "", 1, &nogood);
  PP_geom_svec3(flg, 0, "svec3", geom_svec3_c(0, 0, 0), "", 1, &nogood);
  PP_geom_svec3(flg, 0, "svec3", geom_svec3_c(1, -1, 0), "", 1, &nogood);
  PP_geom_svec3(flg, 0, "svec3", geom_svec3_c(1, 0, -1), "", 1, &nogood);
  PP_geom_mat43(flg, 0, "mat43", geom_mat43_E(), "", 1, &nogood);

  {
    controllable_type value;
    jcntrl_executive_manager *manager;
    jcntrl_executive_manager_entry *entry;

    manager = jcntrl_executive_manager_new();
    entry = jcntrl_executive_manager_reserve(manager, "test-exec");

    value.exec = entry;
    value.current_value = 1.0;

    PP_controllable_type(flg, 0, "controllable", &value, "", 1, &nogood);

    entry = jcntrl_executive_manager_reserve(
      manager, "This-is-very-very-very-very-very-very-long-exec");

    value.exec = entry;
    value.current_value = 0.0;

    PP_controllable_type(flg, 0, "controllable", &value, "", 1, &nogood);

    value.exec = NULL;
    value.current_value = 2.0;

    PP_controllable_type(flg, 0, "controllable", &value, "", 1, &nogood);

    jcntrl_executive_manager_delete(manager);
  }

  fclose(flg->list_fp);
  flg->list_fp = NULL;

#ifdef JUPITER_MPI
  if ((r = MPI_File_open(MPI_COMM_WORLD, SHARED_FILE_NAME,
                         MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL,
                         &flg->list_fp_mpi)) != MPI_SUCCESS) {
    csvperror(SHARED_FILE_NAME, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
              NULL);
    return 0;
  }
  MPI_Comm_dup(MPI_COMM_WORLD, &flg->list_fp_comm);
  MPI_File_set_size(flg->list_fp_mpi, 0);

  PP_charp_name_init(&n, "Same MPI value");
  PP_charp_value_init(&v, "a", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Same\nMPI value");
  PP_charp_value_init(&v, "a", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Different MPI value");
  PP_charp_value_init(&v, (rank == 0) ? "0" : "1", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  PP_charp_name_init(&n, "Different\nMPI value");
  PP_charp_value_init(&v, (rank == 0) ? "0" : "1", "(err)", NULL, NULL);
  PP_charp_unit_init(&u, "u");
  pp_m_line_f(flg, 0, PP_BASELEN, &n.data, &v.data, &u.data, &nogood);

  MPI_File_close(&flg->list_fp_mpi);
  MPI_Comm_free(&flg->list_fp_comm);
  flg->list_fp_mpi = MPI_FILE_NULL;
  flg->list_fp_comm = MPI_COMM_NULL;
#endif

  return r == OFF;
}

struct parser
{
  const char *fn;
  struct re2c_lparser lp;
  /*!stags:re2c format = "const char *@@; "; */
};

static int fill(struct parser *p, FILE *fp, int n)
{
  int r;
  ptrdiff_t off;
  r = re2c_lparser_fill(&p->lp, fp, p->fn, n, &off);
  if (r < 0)
    return r;

  /*!stags:re2c format = "if (p->@@) p->@@ += off; "; */
  return r;
}

/*!re2c
  re2c:flags:tags = 1;
  re2c:api:style = free-form;
  re2c:tags:expression = "p->@@";
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "p->lp.cur";
  re2c:define:YYMARKER = "p->lp.mrk";
  re2c:define:YYCTXMARKER = "p->lp.ctxmrk";
  re2c:define:YYLIMIT = "p->lp.lim";
  re2c:define:YYFILL:naked = 1;
  re2c:define:YYFILL = "{ if (fill(p, fp, @@) < 0) goto error; }";
  re2c:yyfill:enable = 1;
  re2c:indent:string = "  ";

  nl = "\n";
  rank = ("[" @rank_s (" "* ("0"|[1-9][0-9]*)|"*"{5}) @rank_e "] ")?;

  nan_glibc = ([-+]" "*)?'nan';
  inf_glibc = ([-+]" "*)?'inf''inity'?;

  fnan = nan_glibc;
  enan = nan_glibc;
  gnan = nan_glibc;
  finf = inf_glibc;
  einf = inf_glibc;
  ginf = inf_glibc;

  // `%f` value
  //   `%.0f` may writes `1.` or `1` for 1.0            (glibc man says later).
  //   `%.4f` may writes `.0001` or `0.0001` for 0.0001 (glibc man says later).
  fvalue = ([-+]" "*)?([0-9]+"."?|[0-9]*"."[0-9]+);
  ffloat = fvalue|fnan|finf;

  // `%e` value
  evalue = fvalue [eE][+-]?[0-9]+;
  efloat = evalue|enan|einf;

  // `%g` value
  gvalue = evalue|fvalue;
  gfloat = gvalue|gnan|ginf;
*/

/*!max:re2c*/
#ifndef YYMAXFILL
#define YYMAXFILL 1
#endif

static void unmatch(struct parser *p)
{
  re2c_lparser_loc_upd_utf8(&p->lp);
  csvperrorf(p->fn, p->lp.tokline, p->lp.tokcol, CSV_EL_ERROR, NULL,
             "Text does not match");
}

static int check_rank_n(const char *rank_s, const char *rank_e, int rank,
                        struct parser *p);
static int check_text_len(const char *ts, const char *te, int exp,
                          struct parser *p)
{
  if (exp == te - ts)
    return 1;

  csvperrorf(p->fn, p->lp.tokline, p->lp.tokcol, CSV_EL_ERROR, NULL,
             "Length of text expected %d, but got %d", exp, te - ts);
  return 0;
}

static int check_rank_file(int rank, flags *flg, struct parser *p, FILE *fp)
{
  const char *rank_s, *rank_e, *ts, *te, *tt;
  int r = 1;
  int lenv;

  if (fill(p, fp, YYMAXFILL) < 0)
    goto error;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      re2c:indent:top = 2;

      rank @ts ("-"{3}) " This is header " "-"+ @te nl { break; }
      * { unmatch(p); return 0; }
    */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_HEADER_LEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "base: "" "* "test" @te " u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "base (no unit): "" "* "test" @te nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "base (fail with unit): "" "* "test" @te " u !!" nl
      { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "base (fail without unit): "" "* "test" @te " !!" nl
      { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * This is"(" very"{7})" long name: " nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* "test" @te " u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * boooo: " nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   This is"(" very"{8})" long value u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "multi: "" "* @tt "This is value with" @te " u" nl
      { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;
  lenv = tt - ts;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @te "new line" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "multi4: "" "* @tt "This" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  lenv = tt - ts;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "is" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "value" @te " u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "with" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "new" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "line" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " @ts "multi5: "" "* @tt "This" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  lenv = tt - ts;

  re2c_lparser_loc_upd_utf8(&p->lp);
  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "is" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "value" " "* @te " u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "with" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts " "* @tt "new line" @te nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, tt, lenv, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " "This is name with" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank "   " @ts "new line: " " "* "1" @te " u" nl { break; }
      * { unmatch(p); return 0; }
     */
  } while (0);
  if (!check_rank_n(rank_s, rank_e, rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  return r;

error:
  csvperror(p->fn, p->lp.line, p->lp.col, CSV_EL_ERROR, NULL, CSV_ERR_SYS,
            errno, 0, NULL);
  return 0;
}

static int check_mpi_file(int nprc, flags *flg, struct parser *p, FILE *fp)
{
  const char *rank_s, *rank_e, *ts, *tt, *te;
  int shared_rank = (nprc > 1) ? -1 : 0;
  int r = 1;
  if (fill(p, fp, YYMAXFILL) < 0)
    goto error;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      re2c:indent:top = 2;

      rank " * " @ts "Same MPI value: " " "* "a" @te " u" nl { break; }
      * { unmatch(p); return 0; }
    */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, shared_rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    /*!re2c
      rank " * " "Same" nl { break; }
      * { unmatch(p); return 0; }
    */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, shared_rank, p))
    r = 0;

  do {
    re2c_lparser_start_token(&p->lp);
    p->lp.tok = p->lp.cur;
    /*!re2c
      rank "   " @ts "MPI value: " " "* "a" @te " u" nl { break; }
      * { unmatch(p); return 0; }
    */
  } while (0);
  re2c_lparser_loc_upd_utf8(&p->lp);
  if (!check_rank_n(rank_s, rank_e, shared_rank, p))
    r = 0;
  if (!check_text_len(ts, te, PP_BASELEN, p))
    r = 0;

  for (int ir = 0; ir < nprc; ++ir) {
    do {
      re2c_lparser_start_token(&p->lp);
      /*!re2c
        re2c:indent:top = 3;

        rank " * " @ts "Different MPI value: " " "* @tt [0-9]+ @te " u" nl
        { break; }
        * { unmatch(p); return 0; }
      */
    } while (0);
    re2c_lparser_loc_upd_utf8(&p->lp);
    if (!check_rank_n(rank_s, rank_e, ir, p))
      r = 0;
    if (!check_rank_n(tt, te, (ir == 0) ? 0 : 1, p))
      r = 0;
    if (!check_text_len(ts, te, PP_BASELEN, p))
      r = 0;
  }

  for (int ir = 0; ir < nprc; ++ir) {
    do {
      re2c_lparser_start_token(&p->lp);
      /*!re2c
        re2c:indent:top = 3;

        rank " * " "Different" nl { break; }
        * { unmatch(p); return 0; }
      */
    } while (0);
    re2c_lparser_loc_upd_utf8(&p->lp);
    if (!check_rank_n(rank_s, rank_e, ir, p))
      r = 0;

    do {
      re2c_lparser_start_token(&p->lp);
      /*!re2c
        re2c:indent:top = 3;

        rank "   " @ts "MPI value: " " "* @tt [0-9]+ @te " u" nl { break; }
        * { unmatch(p); return 0; }
      */
    } while (0);
    re2c_lparser_loc_upd_utf8(&p->lp);
    if (!check_rank_n(rank_s, rank_e, ir, p))
      r = 0;
    if (!check_rank_n(tt, te, (ir == 0) ? 0 : 1, p))
      r = 0;
    if (!check_text_len(ts, te, PP_BASELEN, p))
      r = 0;
  }

  if (!re2c_lparser_eof(&p->lp)) {
    csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL, "Excess data found");
    r = 0;
  }

  return r;

error:
  csvperror(p->fn, p->lp.line, p->lp.col, CSV_EL_ERROR, NULL, CSV_ERR_SYS,
            errno, 0, NULL);
  return 0;
}

static int test_message(char *rank_fn, flags *flg)
{
  FILE *fp;
  struct parser p;
  int nprc = 1;
  int rank = 0;
  int rrnk;
  int rmpi;

  if (!rank_fn)
    return 0;

#ifdef JUPITER_MPI
  MPI_Comm_size(MPI_COMM_WORLD, &nprc);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

  errno = 0;
  fp = fopen(rank_fn, "rb");
  if (!fp) {
    csvperror(rank_fn, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, errno, 0, NULL);
    return 0;
  }
  re2c_lparser_init(&p.lp);
  p.fn = rank_fn;

  rrnk = check_rank_file(rank, flg, &p, fp);

  re2c_lparser_clean(&p.lp);
  fclose(fp);

  rmpi = 1;
#ifdef JUPITER_MPI
  if (rank == 0) {
    fp = fopen(SHARED_FILE_NAME, "rb");
    if (!fp) {
      csvperror(SHARED_FILE_NAME, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN,
                errno, 0, NULL);
      return 0;
    }

    re2c_lparser_init(&p.lp);
    p.fn = SHARED_FILE_NAME;

    rmpi = check_mpi_file(nprc, flg, &p, fp);

    re2c_lparser_clean(&p.lp);
    fclose(fp);
  }
#endif

  return rrnk && rmpi;
}

static int check_rank_n(const char *rank_s, const char *rank_e, int rank,
                        struct parser *p)
{
#ifdef JUPITER_MPI
  int r = 0;

  /*!re2c
    re2c:define:YYCTYPE = "unsigned char";
    re2c:define:YYCURSOR = "rank_s";
    re2c:define:YYLIMIT = "rank_e";
    re2c:yyfill:enable = 0;
  */
  while (rank_s < rank_e) {
    /*!re2c
      re2c:indent:top = 2;
      * { goto nodigit; }
      '*****' { if (rank_s == rank_e) goto common; goto nodigit; }
      ' '* / [0-9] { break; }
     */
  }

  while (rank_s < rank_e) {
    /*!re2c
      re2c:indent:top = 2;

      * { goto nodigit; }
      '0' { r = r * 10 + 0; continue; }
      '1' { r = r * 10 + 1; continue; }
      '2' { r = r * 10 + 2; continue; }
      '3' { r = r * 10 + 3; continue; }
      '4' { r = r * 10 + 4; continue; }
      '5' { r = r * 10 + 5; continue; }
      '6' { r = r * 10 + 6; continue; }
      '7' { r = r * 10 + 7; continue; }
      '8' { r = r * 10 + 8; continue; }
      '9' { r = r * 10 + 9; continue; }
    */
  }
  if (r == rank)
    return 1;

  if (rank < 0) {
    csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL,
               "Output expected to be common, but is got for rank %d", r);
  } else {
    csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL,
               "Output expected to be for rank %d, but got %d", rank, r);
  }
  return 0;

common:
  if (rank < 0)
    return 1;

  csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL,
             "Output expected to be rank for %d, but is got common", rank);
  return 0;

nodigit:
  csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL,
             "Rank number does not matched to non-digit char: %c", *rank_s);
  return 0;

#else /* no MPI */
  if (rank_s || rank_e) {
    csvperrorf(p->fn, p->lp.line, 0, CSV_EL_ERROR, NULL,
               "Rank number (for MPI) has been matched, but MPI is not linked");
    return 0;
  }
  return 1;
#endif
}
