
#ifndef JUPTIER_TEST_UTIL_H
#define JUPTIER_TEST_UTIL_H

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

int test_compare_vasprintf(char **buf, const char *fmt, va_list ap);
int test_compare_asprintf(char **buf, const char *fmt, ...);

int test_compare_vprintf(const char *first_line, const char *cont_line,
                         const char *fmt, va_list ap);
int test_compare_printf(const char *first_line, const char *cont_line,
                        const char *fmt, ...);

int test_compare_base(int stat, const char *file, long line,
                      const char *description, const char *fail_detail);

int test_compare_fail(int stat, const char *file, long line,
                      const char *compare, const char *fmt, ...);

int test_compare_failv(int stat, const char *file, long line,
                       const char *compare, const char *fmt, va_list ap);

int test_print_bytes(const void *data, ptrdiff_t len);

int test_is_little_endian(void);

#define test_compare_xf(stat, comps, ...) \
  test_compare_fail((stat), __FILE__, __LINE__, (comps), __VA_ARGS__)

#define test_compare_cf(comps, ...) \
  test_compare_fail(!(comps), __FILE__, __LINE__, #comps, __VA_ARGS__)

#define test_compare_c(comps) \
  test_compare_fail(!(comps), __FILE__, __LINE__, #comps, NULL)

#define test_compare(a, b) test_compare_xf(!((a) == (b)), #a " == " #b, NULL)

#define test_compare_f(a, b, ...) \
  test_compare_xf(!((a) == (b)), #a " == " #b, __VA_ARGS__)

#define test_buf_cmp(cp, by) (memcmp(cp, by, sizeof(cp)) != 0)

#define bytes(...) ((unsigned char[]){__VA_ARGS__})

#define test_compare_bytes(brace_expr_or_str_of_bytes, bytes)                  \
  test_compare_fail(test_buf_cmp(brace_expr_or_str_of_bytes, bytes), __FILE__, \
                    __LINE__, #bytes " == " #brace_expr_or_str_of_bytes, NULL)

#define test_compare_bytes_f(brace_expr_or_str_of_bytes, bytes, ...)           \
  test_compare_fail(test_buf_cmp(brace_expr_or_str_of_bytes, bytes), __FILE__, \
                    __LINE__, #bytes " == " #brace_expr_or_str_of_bytes,       \
                    __VA_ARGS__)

#define test_compare_buffer_sz(brace_expr_or_str_of_bytes, mpx_buffer) \
  (test_compare(msgpackx_buffer_size(mpx_buffer),                      \
                sizeof(brace_expr_or_str_of_bytes)) ||                 \
   test_compare_bytes(brace_expr_or_str_of_bytes,                      \
                      msgpackx_buffer_pointer(mpx_buffer)))

#define test_compare_buffer(brace_expr_or_str_of_bytes, mpx_buffer) \
  test_compare_bytes(brace_expr_or_str_of_bytes,                    \
                     msgpackx_buffer_pointer(mpx_buffer))

#define test_compare_node(brace_expr_or_str_of_bytes, mpx_node) \
  test_compare_bytes(brace_expr_or_str_of_bytes,                \
                     msgpackx_node_get_data_pointer(mpx_node, NULL))

/* typed binary comparison */
int test_compare_typed(void *got, void *exp, void *arg, const char *description,
                       const char *file, long line,
                       int (*cmp)(void *g, void *e, void *a),
                       int (*got_printer)(char **b, void *d, void *a),
                       int (*exp_printer)(char **b, void *d, void *a),
                       int (*ext_printer)(char **b, void *g, void *e, void *a));

#define check_op_x(cond) (((int[1]){1})[(cond) ? 0 : 1] && 1)

#define check_op_str2(opstr, a, b) \
  (opstr[0] == a && opstr[1] == b && opstr[2] == '\0')

#define check_op_str12(opstr, a, b) \
  (opstr[0] == a && (opstr[1] == '\0' || (opstr[1] == b && opstr[2] == '\0')))

#define check_op_str1(opstr, a) (opstr[0] == a && opstr[1] == '\0')

#define check_op_str(opstr)                                             \
  check_op_x(                                                           \
    check_op_str2(opstr, '=', '=') || check_op_str2(opstr, '!', '=') || \
    check_op_str12(opstr, '<', '=') || check_op_str12(opstr, '>', '='))

enum test_compare_op
{
  test_compare_eq = 0,
  test_compare_ne = 1,
  test_compare_le = 2,
  test_compare_lt = 3,
  test_compare_ge = 4,
  test_compare_gt = 5,
};

static inline int test_compare_op_s(const char *str)
{
  if (check_op_str2(str, '=', '='))
    return test_compare_eq;
  if (check_op_str2(str, '!', '='))
    return test_compare_ne;
  if (check_op_str1(str, '<'))
    return test_compare_lt;
  if (check_op_str2(str, '<', '='))
    return test_compare_le;
  if (check_op_str1(str, '>'))
    return test_compare_gt;
  if (check_op_str2(str, '>', '='))
    return test_compare_ge;
  return -1;
}

int test_compare_ii_cmp(void *got, void *exp, void *arg);
int test_compare_iu_cmp(void *got, void *exp, void *arg);
int test_compare_ui_cmp(void *got, void *exp, void *arg);
int test_compare_uu_cmp(void *got, void *exp, void *arg);
int test_compare_dd_cmp(void *got, void *exp, void *arg);
int test_compare_eps_cmp(void *got, void *exp, void *arg);
int test_compare_ss_cmp(void *got, void *exp, void *arg);
int test_compare_ssn_cmp(void *got, void *exp, void *arg);
int test_compare_pd_cmp(void *got, void *exp, void *arg);

int test_compare_i_prn(char **buf, void *d, void *a);
int test_compare_u_prn(char **buf, void *d, void *a);
int test_compare_d_prn(char **buf, void *d, void *a);
int test_compare_s_prn(char **buf, void *d, void *a);
int test_compare_sn_prn(char **buf, void *d, void *a);
int test_compare_p_prn(char **buf, void *d, void *a);
int test_compare_pd_prn(char **buf, void *d, void *a);

int test_compare_eps_ext_prn(char **buf, void *g, void *e, void *a);
int test_compare_pd_ext_prn(char **buf, void *g, void *e, void *a);

#define test_compare_x_ii(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(&((intmax_t){got}), &((intmax_t){exp}),                  \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_ii_cmp, test_compare_i_prn, \
                      test_compare_i_prn, NULL))

#define test_compare_l_ii(got, op, exp, file, line) \
  test_compare_x_ii(got, op, exp, #got " " #op " " #exp, file, line)

#define test_compare_ii(got, op, exp) \
  test_compare_x_ii(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

#define test_compare_x_iu(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(&((intmax_t){got}), &((uintmax_t){exp}),                 \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_iu_cmp, test_compare_i_prn, \
                      test_compare_u_prn, NULL))

#define test_compare_l_iu(got, op, exp, file, line) \
  test_compare_x_iu(got, op, exp, #got " " #op " " #exp, file, line)

#define test_compare_iu(got, op, exp) \
  test_compare_x_iu(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

#define test_compare_x_ui(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(&((uintmax_t){got}), &((intmax_t){exp}),                 \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_ui_cmp, test_compare_u_prn, \
                      test_compare_i_prn, NULL))

#define test_compare_l_ui(got, op, exp, file, line) \
  test_compare_x_ui(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

#define test_compare_ui(got, op, exp) \
  test_compare_x_ui(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

#define test_compare_x_uu(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(&((uintmax_t){got}), &((uintmax_t){exp}),                \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_uu_cmp, test_compare_u_prn, \
                      test_compare_u_prn, NULL))

#define test_compare_l_uu(got, op, exp, file, line) \
  test_compare_x_uu(got, op, exp, #got " " #op " " #exp, file, line)

#define test_compare_uu(got, op, exp) \
  test_compare_x_uu(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

#define test_compare_x_dd(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(&((double){got}), &((double){exp}),                      \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_dd_cmp, test_compare_d_prn, \
                      test_compare_d_prn, NULL))

#define test_compare_l_dd(got, op, exp, file, line) \
  test_compare_x_dd(got, op, exp, #got " " #op " " #exp, file, line)

#define test_compare_dd(got, op, exp) \
  test_compare_x_dd(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

struct test_compare_eps_d
{
  double eps;
  double delta;
};

#define test_compare_x_eps(got, exp, eps, desc, file, line)             \
  test_compare_typed(&((double){got}), &((double){exp}),                \
                     &((struct test_compare_eps_d){eps}), desc, (file), \
                     (line), test_compare_eps_cmp, test_compare_d_prn,  \
                     test_compare_d_prn, test_compare_eps_ext_prn)

#define test_compare_l_eps(got, exp, eps, file, line)                          \
  test_compare_x_eps(got, exp, eps, "fabs(" #got " - " #exp ") < " #eps, file, \
                     line)

#define test_compare_eps(got, exp, eps)                                  \
  test_compare_x_eps(got, exp, eps, "fabs(" #got " - " #exp ") < " #eps, \
                     __FILE__, __LINE__)

#define test_compare_x_ss(got, exp, desc, file, line)                      \
  test_compare_typed(&((const char *){got}), &((const char *){exp}), NULL, \
                     desc, (file), (line), test_compare_ss_cmp,            \
                     test_compare_s_prn, test_compare_s_prn, NULL)

#define test_compare_l_ss(got, exp, file, line) \
  test_compare_x_ss(got, exp, "strcmp(" #got ", " #exp ") == 0", file, line))

#define test_compare_ss(got, exp)                                          \
  test_compare_x_ss(got, exp, "strcmp(" #got ", " #exp ") == 0", __FILE__, \
                    __LINE__)

struct test_compare_ssn_d
{
  size_t n;
};

#define test_compare_x_ssn(got, exp, n, desc, file, line)                     \
  test_compare_typed(&((const char *){got}), &((const char *){exp}),          \
                     &((struct test_compare_ssn_d){n}), desc, (file), (line), \
                     test_compare_ssn_cmp, test_compare_sn_prn,               \
                     test_compare_sn_prn, NULL)

#define test_compare_l_ssn(got, exp, n, file, line)                           \
  test_compare_x_ssn(got, exp, n, "strncmp(" #got ", " #exp ", " #n ") == 0", \
                     file, line)

#define test_compare_ssn(got, exp, n)                                         \
  test_compare_x_ssn(got, exp, n, "strncmp(" #got ", " #exp ", " #n ") == 0", \
                     __FILE__, __LINE__)
/**
 * Compare pointers with given operator @p op.
 *
 * @note Both @p got and @p exp must not be expressions with side effects since
 *       they are evaluated more than once.
 */
#define test_compare_x_pp(got, op, exp, desc, file, line)                      \
  (check_op_str((#op "\0\0\0")) && ((void)sizeof((got)op(exp)), 1) &&   \
   test_compare_typed(&((uintmax_t){(uintptr_t)(got)}),                        \
                      &((uintmax_t){(uintptr_t)(exp)}),                        \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_uu_cmp, test_compare_p_prn, \
                      test_compare_p_prn, NULL))

#define test_compare_l_pp(got, op, exp, file, line) \
  test_compare_x_pp(got, op, exp, #got " " #op " " #exp, file, line)

#define test_compare_pp(got, op, exp) \
  test_compare_x_pp(got, op, exp, #got " " #op " " #exp, __FILE__, __LINE__)

struct test_compare_pd_d_inf
{
  uintptr_t value;
  const char *label;
  size_t label_size;
};

struct test_compare_pd_d
{
  struct test_compare_pd_d_inf a, b;
  size_t sz;
};

/**
 * Tests whether a - b == exp for pointers a and b.
 *
 * `a` and `b` must not be `void *`. Since GCC (and replicated compilers like
 * clang and Intel compiler on Linux, etc) allows arithmetic on `void *`, this
 * macro uses `*a` to check this.
 *
 * @note Both @p a and @p b must not be expressions with side effects since they
 *       are evaluated more than once.
 */
#define test_compare_x_poff(a, b, exp, desc, file, line)            \
  test_compare_typed(                                               \
    &((ptrdiff_t){(a) - (b)}), &((ptrdiff_t){exp}),                 \
    &((struct test_compare_pd_d){{(uintptr_t)(a), #a, sizeof(#a)},  \
                                 {(uintptr_t)(b), #b, sizeof(#b)},  \
                                 sizeof(*(a))}),                    \
    desc, (file), (line), test_compare_pd_cmp, test_compare_pd_prn, \
    test_compare_pd_prn, test_compare_pd_ext_prn)

#define test_compare_l_poff(a, b, exp, file, line) \
  test_compare_x_poff(a, b, exp, #a " - " #b " == " #exp, file, line)

#define test_compare_poff(a, b, exp) \
  test_compare_x_poff(a, b, exp, #a " - " #b " == " #exp, __FILE__, __LINE__)

/* main function */
struct test_run_func
{
  const char *name;
  int (*func)(void);
};

static inline int test_util_mpi_rank(void)
{
#ifdef JUPITER_MPI
  int rank = 0;
  int has_mpi;
  MPI_Initialized(&has_mpi);
  if (has_mpi)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
#else
  return 0;
#endif
}

static inline int test_util_mpi_nproc(void)
{
#ifdef JUPITER_MPI
  int nproc = 1;
  int has_mpi;
  MPI_Initialized(&has_mpi);
  if (has_mpi)
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  return nproc;
#else
  return 1;
#endif
}

static inline int run_test_func_if(const char *name, struct test_run_func *f)
{
  return strcmp(name, f->name) == 0;
}

static inline int run_test_func_run(struct test_run_func *f)
{
  int rank = test_util_mpi_rank();

  if (rank == 0)
    fprintf(stderr, "\n\n==== Testing %s\n", f->name);

  return f->func();
}

static inline int run_test_main_impl(int argc, char **argv, int ntests,
                                     struct test_run_func *tests)
{
  int nf = 0;
  int fail[ntests];
  int run[ntests];
  int irun = 0;
  int rank = test_util_mpi_rank();
  int nproc = test_util_mpi_nproc();

  memset(run, 0, sizeof(run));
  memset(fail, 0, sizeof(fail));

  if (argc <= 1) {
    irun = ntests;
    for (int ti = 0; ti < ntests; ++ti)
      run[ti] = ti;
  } else {
    if (rank == 0) {
      for (int i = 1; i < argc; ++i) {
        int ti;
        for (ti = 0; ti < ntests; ++ti) {
          if (run_test_func_if(argv[i], &tests[ti])) {
            int j;
            for (j = 0; j < irun; ++j) {
              if (run[j] == ti)
                break;
            }
            if (j == irun)
              run[irun++] = ti;
            break;
          }
        }
        if (ti == ntests) {
          nf = 1;
          fprintf(stderr, "!!!! No tests matched on \"%s\"\n", argv[i]);
        }
      }
    }

#ifdef JUPITER_MPI
    if (nproc > 1) {
      MPI_Bcast(&nf, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast(&irun, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast(&run, irun, MPI_INT, 0, MPI_COMM_WORLD);
    }
#endif
  }

  if (nf) {
    fprintf(stderr, "\n**** Available tests:\n");
    for (int ti = 0; ti < ntests; ++ti)
      fprintf(stderr, "     - %s\n", tests[ti].name);
  } else if (irun == 0) {
    fprintf(stderr, "!!!! No tests has been run!\n");
  }

  if (nf || irun == 0)
    return EXIT_FAILURE;

  for (int i = 0; i < irun; ++i) {
    fail[i] = run_test_func_run(&tests[run[i]]);
    if (fail[i])
      nf = 1;
  }

#ifdef JUPITER_MPI
  if (nproc > 1)
    MPI_Allreduce(MPI_IN_PLACE, fail, irun, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
#endif

  if (!nf) {
    if (rank == 0)
      fprintf(stderr, "\n==== All tests passed!\n");
    return EXIT_SUCCESS;
  }

  if (rank == 0) {
    fprintf(stderr, "\n!!!! Failed tests:\n");
    for (int ti = 0; ti < ntests; ++ti) {
      int i;
      for (i = 0; i < irun; ++i) {
        if (run[i] == ti)
          break;
      }
      if (i < irun && fail[i])
        fprintf(stderr, "     - %s\n", tests[ti].name);
    }
  }
  return EXIT_FAILURE;
}

static inline int run_test_main_mpi(int *argc, char ***argv, int ntests,
                                    struct test_run_func *tests)
{
  int r;

#ifdef JUPITER_MPI
  MPI_Init(argc, argv);
#endif

  r = run_test_main_impl(*argc, *argv, ntests, tests);

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif

  return r;
}

#define run_test_main(argc, argv, ...)                               \
  run_test_main_mpi(&(argc), &(argv),                                \
                    (sizeof((struct test_run_func[]){__VA_ARGS__}) / \
                     sizeof(struct test_run_func)),                  \
                    ((struct test_run_func[]){__VA_ARGS__}))

#define test_entry(test_name) {.name = #test_name, .func = test_name##_test}

#endif
