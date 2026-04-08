#include "jupiter/random/random.h"
#include "jupiter/general_ivector.h"
#include "jupiter/general_dvector.h"
#include "jupiter/general_vector_node.h"
#include "test-util.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

static int ptrsource[100] = {0};
static int *ptr = ptrsource;

struct general_vector_tester
{
  void *assign;
  void *copy;
  void *alloc;
  int alloc_n;
  int copy_n;
  void *nullify;
  void *del;
  int *ptr;
  struct general_vector_node node;
};

static struct general_vector_tester *
general_vector_tester__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct general_vector_tester, node);
}

static void *general_vector_tester__alloc(struct general_vector_node *a, int n)
{
  struct general_vector_tester *v;
  v = general_vector_tester__getter(a);

  v->alloc = ptr++;
  v->alloc_n = n;
  v->ptr = v->alloc;
  fprintf(stderr, "[alloc  ] a: %p, p: %p, n: %d\n", (void *)a, (void *)v->ptr,
          n);
  return v->alloc;
}

static void general_vector_tester__delete(struct general_vector_node *a)
{
  struct general_vector_tester *v;
  v = general_vector_tester__getter(a);

  fprintf(stderr, "[delete ] a: %p\n", (void *)a);
  v->del = v->ptr;
  v->ptr = NULL;
}

static void general_vector_tester__copy(struct general_vector_node *to, int ts,
                                        struct general_vector_node *from,
                                        int fs, int n)
{
  struct general_vector_tester *vf, *vt;
  vf = general_vector_tester__getter(from);
  vt = general_vector_tester__getter(to);

  vt->copy = vf;
  vt->copy_n = n;
  vt->ptr = vt->ptr;
  fprintf(stderr, "[copy   ] t: %p, ts: %d, f: %p, fs: %d, p: %p, n: %d\n",
          (void *)to, ts, (void *)from, fs, (void *)vt->ptr, n);
}

static void general_vector_tester__assign(struct general_vector_node *to,
                                          struct general_vector_node *from)
{
  struct general_vector_tester *vf, *vt;
  vf = general_vector_tester__getter(from);
  vt = general_vector_tester__getter(to);

  vt->assign = vf;
  vt->ptr = vf->ptr;
  fprintf(stderr, "[assign ] t: %p, f: %p, p: %p\n", (void *)to, (void *)from,
          (void *)vt->ptr);
}

static void general_vector_tester__nullify(struct general_vector_node *a)
{
  struct general_vector_tester *v;
  v = general_vector_tester__getter(a);

  v->nullify = v->ptr;
  v->ptr = NULL;
  fprintf(stderr, "[nullify] a: %p, p: %p\n", (void *)a, (void *)v->ptr);
}

static const struct general_vector_callbacks *
general_vector_tester_funcs_set(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = general_vector_tester__alloc,
    .copy = general_vector_tester__copy,
    .assign = general_vector_tester__assign,
    .deleter = general_vector_tester__delete,
    .nullify = general_vector_tester__nullify,
  };
  return f;
}

#define general_vector_tester_funcs() \
  general_vector_funcs(general_vector_tester_funcs_set)

static void general_vector_tester_init(struct general_vector_tester *t)
{
  memset(t, 0, sizeof(struct general_vector_tester));
  general_vector_node_init(&t->node, general_vector_tester_funcs());
}

static int iv_sort(int i, int j, void *a)
{
  if (i < j)
    return -1;
  if (i == j)
    return 0;
  return 1;
}

static void test_bsearch(int *r, int n, int idx, int exp, const char *f, int l)
{
  printf("[bsearch: %10d]: %d", n, idx);
  if (idx != exp) {
    printf(" !!\n ... expected %d at %s(%d)", exp, f, l);
    *r = 0;
  }
  printf("\n");
}

#define test_bsearch(r, n, idx, exp) \
  test_bsearch((r), (n), (idx), (exp), __FILE__, __LINE__)

int main(int argc, char **argv)
{
  struct general_vector_tester t1, t2, t3, t4;
  general_dvector dv1, dv2;
  general_ivector iv, iv2, iv3;
  jupiter_random_seed sd;
  int n, idx;
  int r = 1;
  jupiter_random_seed_fill_random(&sd);

  general_vector_tester_init(&t1);
  general_vector_tester_init(&t2);
  general_vector_tester_init(&t3);
  general_vector_tester_init(&t4);

  general_vector_node_resize(&t1.node, &t2.node, 100, 0,
                             general_vector_tester_funcs());
  general_vector_node_resize(&t1.node, &t2.node, 10, 1,
                             general_vector_tester_funcs());
  general_vector_node_resize(&t3.node, &t2.node, 10, 0,
                             general_vector_tester_funcs());
  general_vector_node_share(&t1.node, &t3.node, general_vector_tester_funcs());
  general_vector_node_reserve(&t3.node, &t2.node, 5,
                              general_vector_tester_funcs());
  general_vector_node_reserve(&t3.node, &t2.node, 20,
                              general_vector_tester_funcs());
  general_vector_node_reserve(&t3.node, &t2.node, 100,
                              general_vector_tester_funcs());

  general_vector_node_alloc(&t4.node, 1000, general_vector_tester_funcs());

  general_vector_node_clear(&t1.node, general_vector_tester_funcs());
  general_vector_node_clear(&t2.node, general_vector_tester_funcs());
  general_vector_node_clear(&t3.node, general_vector_tester_funcs());
  general_vector_node_clear(&t4.node, general_vector_tester_funcs());
  general_vector_node_clear(&t1.node, general_vector_tester_funcs());

  general_ivector_init(&iv);
  general_ivector_resize(&iv, 100, 0);
  for (int i = 0; i < 100; ++i) {
    iv.d[i] = jupiter_random_nextn(&sd, INT_MAX);
    printf("%5d: %10d\n", i, iv.d[i]);
  }

  general_ivector_sort(&iv, iv_sort, NULL);
  printf("--- sort\n");
  for (int i = 0; i < 100; ++i) {
    printf("%5d: %10d%s\n", i, iv.d[i],
           (i == 0 || iv.d[i - 1] <= iv.d[i]) ? "" : (r = 0, " !!"));
  }

  n = 109035056;
  idx = general_ivector_bsearch_range(&iv, 0, 5, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 5);

  n = 109035056;
  idx = general_ivector_bsearch_range(&iv, 0, 4, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  n = 109035056;
  idx = general_ivector_bsearch_range(&iv, 5, 10, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 5);

  n = 109035056;
  idx = general_ivector_bsearch_range(&iv, 6, 10, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  n = 810153988;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 41);

  n = 810153987;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  n = 810153989;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  n = 12950312;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 0);

  n = 12950311;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  n = 2129797626;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 99);

  n = 2129797627;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, -1);

  {
    int binf[10], bint[10];

    for (int i = 0; i < 10; ++i) {
      binf[i] = 0;
      bint[i] = 0;
    }

    for (int i = 0; i < 100; ++i) {
      iv.d[i] = jupiter_random_nextn(&sd, 10);
      printf("%5d: %10d\n", i, iv.d[i]);
      binf[iv.d[i]] += 1;
    }

    general_ivector_sort(&iv, iv_sort, NULL);
    printf("--- sort\n");
    for (int i = 0; i < 100; ++i) {
      printf("%5d: %10d%s\n", i, iv.d[i],
             (i == 0 || iv.d[i - 1] <= iv.d[i]) ? "" : (r = 0, " !!"));
      bint[iv.d[i]] += 1;
    }

    for (int i = 0; i < 10; ++i) {
      printf("n[%d] = %3d -> %3d\n", i, binf[i], bint[i]);
    }
  }

  n = 3;
  idx = general_ivector_bsearch(&iv, iv_sort, n, NULL);
  test_bsearch(&r, n, idx, 30);

  if (!test_compare_ii(general_ivector_uniq(&iv, iv_sort, NULL), ==, 1))
    r = 0;
  if (!test_compare_ii(iv.node.n, ==, 10))
    r = 0;
  for (int i = 0; i < 10; ++i) {
    printf("%5d: %10d\n", i, iv.d[i]);
    if (iv.d[i] != i)
      r = 0;
  }

  printf("--- Unique unsorted\n");
  general_ivector_resize(&iv, 100, 0);
  for (int i = 0; i < 100; ++i) {
    iv.d[i] = jupiter_random_nextn(&sd, 40);
    printf("%5d: %10d\n", i, iv.d[i]);
  }

  if (!test_compare_ii(general_ivector_uniq(&iv, iv_sort, NULL), ==, 1))
    r = 0;
  if (!test_compare_ii(iv.node.n, ==, 35))
    r = 0;
  if (!test_compare_ii(iv.d[0], ==, 11))
    r = 0;
  if (!test_compare_ii(iv.d[1], ==, 13))
    r = 0;
  if (!test_compare_ii(iv.d[2], ==, 19))
    r = 0;
  if (!test_compare_ii(iv.d[3], ==, 2))
    r = 0;
  if (!test_compare_ii(iv.d[4], ==, 30))
    r = 0;
  if (!test_compare_ii(iv.d[5], ==, 24))
    r = 0;
  if (!test_compare_ii(iv.d[6], ==, 23))
    r = 0;
  if (!test_compare_ii(iv.d[7], ==, 28))
    r = 0;
  if (!test_compare_ii(iv.d[8], ==, 22))
    r = 0;
  if (!test_compare_ii(iv.d[9], ==, 38))
    r = 0;
  if (!test_compare_ii(iv.d[10], ==, 14))
    r = 0;
  if (!test_compare_ii(iv.d[11], ==, 37))
    r = 0;
  if (!test_compare_ii(iv.d[12], ==, 31))
    r = 0;
  if (!test_compare_ii(iv.d[13], ==, 3))
    r = 0;
  if (!test_compare_ii(iv.d[14], ==, 12))
    r = 0;
  if (!test_compare_ii(iv.d[15], ==, 27))
    r = 0;
  if (!test_compare_ii(iv.d[16], ==, 39))
    r = 0;
  if (!test_compare_ii(iv.d[17], ==, 0))
    r = 0;
  if (!test_compare_ii(iv.d[18], ==, 35))
    r = 0;
  if (!test_compare_ii(iv.d[19], ==, 6))
    r = 0;
  if (!test_compare_ii(iv.d[20], ==, 16))
    r = 0;
  if (!test_compare_ii(iv.d[21], ==, 4))
    r = 0;
  if (!test_compare_ii(iv.d[22], ==, 17))
    r = 0;
  if (!test_compare_ii(iv.d[23], ==, 34))
    r = 0;
  if (!test_compare_ii(iv.d[24], ==, 18))
    r = 0;
  if (!test_compare_ii(iv.d[25], ==, 8))
    r = 0;
  if (!test_compare_ii(iv.d[26], ==, 36))
    r = 0;
  if (!test_compare_ii(iv.d[27], ==, 10))
    r = 0;
  if (!test_compare_ii(iv.d[28], ==, 21))
    r = 0;
  if (!test_compare_ii(iv.d[29], ==, 9))
    r = 0;
  if (!test_compare_ii(iv.d[30], ==, 26))
    r = 0;
  if (!test_compare_ii(iv.d[31], ==, 1))
    r = 0;
  if (!test_compare_ii(iv.d[32], ==, 29))
    r = 0;
  if (!test_compare_ii(iv.d[33], ==, 5))
    r = 0;
  if (!test_compare_ii(iv.d[34], ==, 15))
    r = 0;

  general_ivector_init(&iv2);
  general_ivector_init(&iv3);

  printf("--- Merge unsorted\n");
  general_ivector_resize(&iv2, 20, 0);
  for (int i = 0; i < 20; ++i) {
    iv2.d[i] = jupiter_random_nextn(&sd, 40);
    printf("%5d: %10d\n", i, iv2.d[i]);
  }

  general_ivector_merge(&iv3, &iv, &iv2, iv_sort, NULL);

  if (!test_compare_ii(iv3.node.n, ==, 36))
    r = 0;
  if (!test_compare_ii(iv3.d[0], ==, 11))
    r = 0;
  if (!test_compare_ii(iv3.d[1], ==, 13))
    r = 0;
  if (!test_compare_ii(iv3.d[2], ==, 19))
    r = 0;
  if (!test_compare_ii(iv3.d[3], ==, 2))
    r = 0;
  if (!test_compare_ii(iv3.d[4], ==, 30))
    r = 0;
  if (!test_compare_ii(iv3.d[5], ==, 24))
    r = 0;
  if (!test_compare_ii(iv3.d[6], ==, 23))
    r = 0;
  if (!test_compare_ii(iv3.d[7], ==, 28))
    r = 0;
  if (!test_compare_ii(iv3.d[8], ==, 22))
    r = 0;
  if (!test_compare_ii(iv3.d[9], ==, 38))
    r = 0;
  if (!test_compare_ii(iv3.d[10], ==, 14))
    r = 0;
  if (!test_compare_ii(iv3.d[11], ==, 37))
    r = 0;
  if (!test_compare_ii(iv3.d[12], ==, 31))
    r = 0;
  if (!test_compare_ii(iv3.d[13], ==, 3))
    r = 0;
  if (!test_compare_ii(iv3.d[14], ==, 12))
    r = 0;
  if (!test_compare_ii(iv3.d[15], ==, 27))
    r = 0;
  if (!test_compare_ii(iv3.d[16], ==, 39))
    r = 0;
  if (!test_compare_ii(iv3.d[17], ==, 0))
    r = 0;
  if (!test_compare_ii(iv3.d[18], ==, 35))
    r = 0;
  if (!test_compare_ii(iv3.d[19], ==, 6))
    r = 0;
  if (!test_compare_ii(iv3.d[20], ==, 16))
    r = 0;
  if (!test_compare_ii(iv3.d[21], ==, 4))
    r = 0;
  if (!test_compare_ii(iv3.d[22], ==, 17))
    r = 0;
  if (!test_compare_ii(iv3.d[23], ==, 34))
    r = 0;
  if (!test_compare_ii(iv3.d[24], ==, 18))
    r = 0;
  if (!test_compare_ii(iv3.d[25], ==, 8))
    r = 0;
  if (!test_compare_ii(iv3.d[26], ==, 36))
    r = 0;
  if (!test_compare_ii(iv3.d[27], ==, 10))
    r = 0;
  if (!test_compare_ii(iv3.d[28], ==, 21))
    r = 0;
  if (!test_compare_ii(iv3.d[29], ==, 9))
    r = 0;
  if (!test_compare_ii(iv3.d[30], ==, 26))
    r = 0;
  if (!test_compare_ii(iv3.d[31], ==, 1))
    r = 0;
  if (!test_compare_ii(iv3.d[32], ==, 29))
    r = 0;
  if (!test_compare_ii(iv3.d[33], ==, 5))
    r = 0;
  if (!test_compare_ii(iv3.d[34], ==, 15))
    r = 0;
  if (!test_compare_ii(iv3.d[35], ==, 7))
    r = 0;

  general_ivector_clear(&iv);
  general_ivector_clear(&iv2);
  general_ivector_clear(&iv3);

  //---

  general_dvector_init(&dv1);
  general_dvector_init(&dv2);
  general_dvector_resize(&dv1, 100, 0);
  general_dvector_share(&dv2, &dv1);

  if (!dv2.d || dv1.d != dv2.d) {
    printf("failed to share\n");
    r = 0;
  }

  dv2.d[0] = 1.0;
  if (!test_compare_dd(dv1.d[0], ==, 1.0))
    r = 0;

  general_dvector_clear(&dv1);
  general_dvector_clear(&dv2);

  general_dvector_resize(&dv1, 100, 0);
  general_dvector_share(&dv2, &dv1);

  if (!dv2.d || dv1.d != dv2.d) {
    printf("failed to share\n");
    r = 0;
  }

  dv1.d[0] = 0.0;
  dv2.d[0] = 1.0;

  general_dvector_resize(&dv1, 101, 1);

  if (!test_compare_dd(dv1.d[0], ==, 1.0))
    r = 0;
  if (!test_compare_pp(dv1.d, !=, dv2.d))
    r = 0;

  general_dvector_clear(&dv1);
  general_dvector_clear(&dv2);

  return r ? EXIT_SUCCESS : EXIT_FAILURE;
}
