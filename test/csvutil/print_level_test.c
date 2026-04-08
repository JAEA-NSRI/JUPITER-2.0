
#include <jupiter/os/asprintf.h>
#include <jupiter/csvutil.h>
#include <jupiter/csv.h>
#include <jupiter/geometry/bitarray.h>
#include <jupiter/optparse.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static const char *errname(csv_error e)
{
  static char buf[80];
  switch (e) {
#define TEXTCON(prefix, text) \
  case prefix##text:          \
    return #text

    TEXTCON(CSV_ERR_, SUCC);
    TEXTCON(CSV_ERR_, EOF);
    TEXTCON(CSV_ERR_, MIX_QUOTE);
    TEXTCON(CSV_ERR_, 2BIG);
    TEXTCON(CSV_ERR_, DATA_AFTER_CONT);
    TEXTCON(CSV_ERR_, BLOCK_COMMENT_AFTER_CONT);
    TEXTCON(CSV_ERR_, FOPEN);
    TEXTCON(CSV_ERR_, NOMEM);
    TEXTCON(CSV_ERR_, CMDLINE_INVAL);
    TEXTCON(CSV_ERR_, CMDLINE_RANGE);
    TEXTCON(CSV_ERR_, SYS);
    TEXTCON(CSV_ERR_, MPI);
    TEXTCON(CSV_ERR_, GEOMETRY);
    TEXTCON(CSV_ERR_, SERIALIZE);
  }

  if ((int)e == -1)
    return "<-1>";

  snprintf(buf, 79, "<Unexpect: %d>", (int)e);
  buf[79] = '\0';
  return buf;
}

static const char *bitname(csv_error_level i)
{
  static char buf[80];
  switch (i) {
    TEXTCON(CSV_EL_, DEBUG);
    TEXTCON(CSV_EL_, INFO);
    TEXTCON(CSV_EL_, WARN);
    TEXTCON(CSV_EL_, ERROR);
    TEXTCON(CSV_EL_, FATAL);
  case CSV_EL_MAX:
    return "<MAX>";
  }

  snprintf(buf, 79, "<Unexpect: %d>", (int)i);
  buf[79] = '\0';
  return buf;
}

static int bitcomp(jupiter_print_levels act, jupiter_print_levels got)
{
  jupiter_print_levels l;
  int first = 1;
  int ret = 1;
  for (int i = 0; i < CSV_EL_MAX; ++i) {
    int actbit = geom_bitarray_element_get(act.levels, i);
    int gotbit = geom_bitarray_element_get(got.levels, i);
    if (actbit != gotbit) {
      ret = 0;
      if (first) {
        printf("fail:");
        first = 0;
      }
      if (actbit) {
        printf(" -%s", bitname((csv_error_level)i));
      } else {
        printf(" +%s", bitname((csv_error_level)i));
      }
    }
  }
  if (!ret) {
    printf("\n");
  } else {
    printf("pass:");
    for (int i = 0; i < CSV_EL_MAX; ++i) {
      int gotbit = geom_bitarray_element_get(got.levels, i);
      if (gotbit)
        printf(" %s", bitname((csv_error_level)i));
    }
    printf("\n");
  }
  return ret;
}

static int do_test(int argc, char **argv, const char *def,
                   jupiter_print_levels deflvls, csv_error retv,
                   jupiter_print_levels exlevels, int print_rank)
{
  int rt = 1;
  jupiter_options opts;
  csv_error gotv;
  jupiter_options_init(&opts, def, deflvls);
  set_jupiter_print_rank(1);
  set_jupiter_print_levels(jupiter_print_levels_non_debug());

  printf("testing:\n");
  for (int i = 1; i < argc; ++i) {
    printf(" [%d] = \"%s\"", i, argv[i]);
  }
  printf("\n");

  if ((gotv = jupiter_optparse(&opts, &argc, &argv)) != retv) {
    printf("fail: return expect `%s`, but got `%s`\n", errname(retv),
           errname(gotv));
    rt = 0;
  }

  if (!bitcomp(exlevels, opts.print_levels))
    rt = 0;

  printf("%s: flag exp %d == got %d\n",
         (opts.print_my_rank == print_rank) ? "pass" : "fail",
         print_rank, opts.print_my_rank);
  if (opts.print_my_rank != print_rank)
    rt = 0;
  return rt;
}

int main(int argc, char **argv)
{
  int nargc;
  char *nargv[4];
  jupiter_print_levels deflvls;
  jupiter_print_levels explvls;
  int rt = EXIT_SUCCESS;

  nargc = 2;
  nargv[0] = argv[0];
  nargv[1] = "-print_level";
  nargv[2] = NULL;
  nargv[3] = NULL;

  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, -1, explvls, 1))
    rt = EXIT_FAILURE;

  nargc = 3;
  nargv[2] = "";
  nargv[3] = NULL;

  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-";

  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "+";

  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_all();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-,+debug";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_DEBUG, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-,+deBUG,+waRn,info,+erRor,+faTal";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_DEBUG, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_WARN, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_INFO, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_ERROR, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_FATAL, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "+deBUG,-info,+erRor,+faTal";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_DEBUG, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_WARN, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_ERROR, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_FATAL, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-debug";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_INFO, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_WARN, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_ERROR, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_FATAL, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "--";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_CMDLINE_INVAL, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-ddddd";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_CMDLINE_INVAL, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "-debug+fatal";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_CMDLINE_INVAL, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[1] = "-print_rank";
  nargv[2] = "all:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[1] = "-print_rank";
  nargv[2] = "all:-,1:+";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[1] = "-print_rank";

  nargv[2] = "all:-,0:+debug";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_DEBUG, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "aLl:-all+fatal,0:+debug";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  geom_bitarray_element_set(explvls.levels, CSV_EL_DEBUG, 1);
  geom_bitarray_element_set(explvls.levels, CSV_EL_FATAL, 1);
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "xyz";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_CMDLINE_INVAL, explvls, 1))
    rt = EXIT_FAILURE;

#define STR(x) #x
#define ESTR(x) STR(x)

  nargv[2] = "9223372036854775807"
             "9223372036854775807"
             "999:+";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_CMDLINE_INVAL, explvls, 1))
    rt = EXIT_FAILURE;

  {
    int r;
    char *buf;
    r = jupiter_asprintf(&buf, "%d:-", INT_MAX);
    if (r >= 0) {
      nargv[2] = buf;
      deflvls = jupiter_print_levels_non_debug();
      explvls = jupiter_print_levels_non_debug();
      if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 0))
        rt = EXIT_FAILURE;
      free(buf);
    } else {
      fprintf(stderr, "fail: Memory allocation failed\n");
      rt = EXIT_FAILURE;
    }
  }

  nargv[2] = "0:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "0..1:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "1..2:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 0))
    rt = EXIT_FAILURE;

  nargv[2] = "all:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_zero();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 1))
    rt = EXIT_FAILURE;

  nargv[2] = "2..0:-";
  deflvls = jupiter_print_levels_non_debug();
  explvls = jupiter_print_levels_non_debug();
  if (!do_test(nargc, nargv, "0", deflvls, CSV_ERR_SUCC, explvls, 0))
    rt = EXIT_FAILURE;

  return rt;
}
