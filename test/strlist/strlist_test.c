
#include "test/strlist/strlist_test_util.h"
#include <test/test-util/test-util.h>

#include <jupiter/csvutil.h>
#include <jupiter/geometry/list.h>
#include <jupiter/optparse.h>
#include <jupiter/os/os.h>
#include <jupiter/strlist.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int plen(jupiter_strlist *l, void *arg)
{
#ifdef _OPENMP
  int ti = omp_get_thread_num();
  fprintf(stderr, "* this is thread %d\n", ti);
  jupiter_sleep((ti + 1) * 100);
#endif

#ifdef _OPENMP
#pragma omp atomic update
#endif
  *(size_t *)arg += jupiter_strlist_length(l);
  return 0;
}

int main(int argc, char **argv)
{
  int r = EXIT_SUCCESS;
  jupiter_strlist_head llh;
  jupiter_strlist_head_init(&llh);

  do {
    jupiter_strlist *l;

    l = jupiter_strlist_dup_s("abc");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    jupiter_strlist_append(&llh, l);


    if (!test_compare_ii(l->node.len, ==, 4)) {
      r = EXIT_FAILURE;
      break;
    }

    if (!test_compare_ii(l->buf[0], ==, 'a'))
      r = EXIT_FAILURE;
    if (!test_compare_ii(l->buf[1], ==, 'b'))
      r = EXIT_FAILURE;
    if (!test_compare_ii(l->buf[2], ==, 'c'))
      r = EXIT_FAILURE;
    if (!test_compare_ii(l->buf[3], ==, '\0'))
      r = EXIT_FAILURE;
    if (r != EXIT_SUCCESS)
      break;

    fprintf(stderr, "....: Test of test_compare_sls()\n");
    if (!test_compare_ii(test_compare_sls(l, "abc"), ==, 1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(l, "abcd"), ==, 0))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(l, "ab"), ==, 0))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(l, "azc"), ==, 0))
      r = EXIT_FAILURE;

    if (!test_compare_ii((l->buf[3] = 'z'), ==, 'z'))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(l, "abcz"), ==, 1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(l, "abc"), ==, 0))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(NULL, NULL), ==, 1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(test_compare_sls(NULL, "xxx"), ==, 0))
      r = EXIT_FAILURE;

    fprintf(stderr, "....: End test of test_compare_sls()\n");
  } while (0);

  jupiter_strlist_free_all(&llh);
  if (!test_compare_ii(geom_list_empty(&llh.list), ==, 1))
    r = EXIT_FAILURE;

  do {
    jupiter_strlist *l, *l2;
    jupiter_strlist_head lxh;

    if (r != EXIT_SUCCESS)
      break;

    jupiter_strlist_head_init(&lxh);
    do {
      if (!test_compare_pp((l = jupiter_strlist_dup_s("a")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&llh, l);

      if (!test_compare_pp((l = jupiter_strlist_dup_s("b")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&llh, l);

      if (!test_compare_pp((l = jupiter_strlist_dup_s("c")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&llh, l);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_sls((l = jupiter_strlist_last(&llh)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_prev(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_prev(l)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_prev(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_dup_s("d")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_insert_next(jupiter_strlist_first(&llh), l);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_dup_s("e")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_insert_prev(jupiter_strlist_last(&llh), l);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_dup_s("x")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&lxh, l);

      if (!test_compare_pp((l = jupiter_strlist_dup_s("y")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&lxh, l);

      if (!test_compare_pp((l = jupiter_strlist_dup_s("z")), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_append(&lxh, l);

      if (!test_compare_sls((l = jupiter_strlist_first(&lxh)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      jupiter_strlist_append_list(&llh, &lxh);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp(jupiter_strlist_first(&lxh), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_last(&llh)), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_delete(l);
      jupiter_strlist_append(&lxh, l);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_sls((l = jupiter_strlist_first(&lxh)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      jupiter_strlist_prepend_list(&llh, &lxh);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp(jupiter_strlist_first(&lxh), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_last(&llh)), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_delete(l);
      if (!test_compare_pp(jupiter_strlist_get_head(l), ==, NULL))
        r = EXIT_FAILURE;

      jupiter_strlist_append(&lxh, l);
      if (!test_compare_pp(jupiter_strlist_get_head(l), ==, &lxh))
        r = EXIT_FAILURE;

      if (!test_compare_pp((l = jupiter_strlist_last(&llh)), !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      jupiter_strlist_delete(l);
      jupiter_strlist_append(&lxh, l);

      l2 = jupiter_strlist_first(&llh);
      for (int i = 0; i < 3 && l; ++i)
        l2 = jupiter_strlist_next(l2);

      if (!test_compare_pp(l2, !=, NULL)) {
        r = EXIT_FAILURE;
        break;
      }

      if (!test_compare_pp(jupiter_strlist_get_head(l2), ==, &llh))
        r = EXIT_FAILURE;

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(l2, ==, l))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_sls((l = jupiter_strlist_first(&lxh)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      jupiter_strlist_insert_list_prev(l2, &lxh);

      if (!test_compare_sls((l = jupiter_strlist_first(&llh)), "z"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "a"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "d"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "y"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "x"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "b"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(l2, ==, l))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "e"))
        r = EXIT_FAILURE;
      if (!test_compare_sls((l = jupiter_strlist_next(l)), "c"))
        r = EXIT_FAILURE;
      if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
        r = EXIT_FAILURE;

      if (!test_compare_pp(jupiter_strlist_first(&lxh), ==, NULL))
        r = EXIT_FAILURE;
    } while (0);

    jupiter_strlist_free_all(&lxh);
    jupiter_strlist_free_all(&llh);
  } while (0);

  do {
    int i;
    jupiter_strlist *l;

    if (r != EXIT_SUCCESS)
      break;

    l = jupiter_strlist_dup_s("Rigeligelelgelileliielaalee");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    if (!test_compare_uu(jupiter_strlist_size(l), ==, 28))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_length(l), ==, 27))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_size_list(l, l), ==, 28))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_length_list(l, l), ==, 27))
      r = EXIT_FAILURE;

    jupiter_strlist_append(&llh, l);
    if (!test_compare_uu(jupiter_strlist_size_list(l, l), ==, 28))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_length_list(l, l), ==, 27))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_size_all(&llh), ==, 28))
      r = EXIT_FAILURE;
    if (!test_compare_uu(jupiter_strlist_length_all(&llh), ==, 27))
      r = EXIT_FAILURE;

    l = jupiter_strlist_split_s(l, "li");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    if (!test_compare_sls(l, "Rige"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "gelelge"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "le"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "ielaalee"))
      r = EXIT_FAILURE;
    if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
      r = EXIT_FAILURE;

    {
      size_t sz = 0;

#ifdef _OPENMP
#pragma omp parallel num_threads(4)
#pragma omp single
#endif
      {
        jupiter_strlist_foreach_all_p(&llh, plen, &sz);
      }

      if (!test_compare_uu(sz, ==, 4 + 7 + 2 + 8))
        r = EXIT_FAILURE;
    }

    jupiter_strlist_free_all(&llh);

    l = jupiter_strlist_dup_s("AlnilamAlnairAlnitakAliothAlkaidAlhenaAl");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }
    jupiter_strlist_append(&llh, l);

    l = jupiter_strlist_split_ch(l, 'l');
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    if (!test_compare_sls(l, "A"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "ni"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "amA"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "nairA"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "nitakA"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "iothA"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "kaidA"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l), "henaA"))
      r = EXIT_FAILURE;
    if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
      r = EXIT_FAILURE;

    l = jupiter_strlist_join_all(&llh, NULL);
    jupiter_strlist_free_all(&llh);
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    jupiter_strlist_append(&llh, l);

    if (!test_compare_sls(l, "AniamAnairAnitakAiothAkaidAhenaA"))
      r = EXIT_FAILURE;
    if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
      r = EXIT_FAILURE;

    l = jupiter_strlist_asprintf(":%s:", "Shaula");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    if (!test_compare_sls(l, ":Shaula:")) {
      r = EXIT_FAILURE;
      break;
    }

    jupiter_strlist_prepend(&llh, l);

    if (!test_compare_sls(l = jupiter_strlist_first(&llh), ":Shaula:"))
      r = EXIT_FAILURE;
    if (!test_compare_sls(l = jupiter_strlist_next(l),
                          "AniamAnairAnitakAiothAkaidAhenaA"))
      r = EXIT_FAILURE;
    if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
      r = EXIT_FAILURE;

    l = jupiter_strlist_join_all(&llh, "---");
    if (!test_compare_pp(l, !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }
    jupiter_strlist_free_all(&llh);
    jupiter_strlist_append(&llh, l);

    if (!test_compare_sls(l, ":Shaula:" "---"
                          "AniamAnairAnitakAiothAkaidAhenaA"))
      r = EXIT_FAILURE;
    if (!test_compare_pp(jupiter_strlist_next(l), ==, NULL))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strstr(l, ":Shaula:"), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "Shaula:"), ==, 1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "Ani"), ==, 11))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "Ana"), ==, 16))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "zzz"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, ""), ==, -1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strchr(l, ':'), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'z'), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'S'), ==, 1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'A'), ==, 11))
      r = EXIT_FAILURE;

    if (!test_compare_pp((l = jupiter_strlist_dup_s("ab")), !=, NULL)) {
      r = EXIT_FAILURE;
      break;
    }

    jupiter_strlist_append(&llh, l);

    if (!test_compare_ii(jupiter_strlist_strstr(l, "ab"), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "abc"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "zzz"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, ""), ==, -1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strchr(l, 'a'), ==, 0))
      r = EXIT_FAILURE;

    l->buf[2] = 'c';
    if (!test_compare_sls(l, "abc"))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strstr(l, "ab"), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "abc"), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "bc"), ==, 1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "abcd"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "zzz"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, ""), ==, -1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strchr(l, 'a'), ==, 0))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'c'), ==, 2))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'd'), ==, -1))
      r = EXIT_FAILURE;

    l->buf[0] = '\0';
    if (!test_compare_sls(l, ""))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strstr(l, "ab"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, "abc"), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strstr(l, ""), ==, -1))
      r = EXIT_FAILURE;

    if (!test_compare_ii(jupiter_strlist_strchr(l, 'a'), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'c'), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, 'd'), ==, -1))
      r = EXIT_FAILURE;
    if (!test_compare_ii(jupiter_strlist_strchr(l, '\0'), ==, -1))
      r = EXIT_FAILURE;

  } while (0);
  jupiter_strlist_free_all(&llh);

  csvperrorf("<file>", 0, 0, CSV_EL_ERROR, NULL,
             "This\nis\n\nmessage\nwith\nnl");
  csvperrorf("<file>", 99, 100, CSV_EL_INFO, NULL,
             "This is message\nwith\nnewline\n");
  csvperrorf("<file>", 0, 0, CSV_EL_INFO, "value",
             "This is\nmessage with newline\n");
  csvperrorf(NULL, 0, 0, CSV_EL_INFO, NULL,
             "This is\nmessage with extra newline\n\n");
  csvperrorf(NULL, 0, 0, CSV_EL_WARN, NULL, "");
  csvperrorf("<file>", 1, 0, CSV_EL_WARN, NULL,
             "This is message without newline\n");
  csvperrorf("<file>", 1, 0, CSV_EL_FATAL, "value",
             "This is\n\nmessage\nwith newline");
  csvperrorf("<file>", 1, 2, CSV_EL_DEBUG, "value",
             "This is\nmessage\nwith newline\n");
  csvperrorf("<file>", 1, 2, CSV_EL_DEBUG, "value",
             "This is\nmessage\nwith newline\n");
  csvperrorf("<file>", 1, 2, CSV_EL_INFO, "value with\nnl",
             "This is\nmessage\nwith format: %d\n%s!\n", 100, "text with\nnl");

  set_jupiter_print_levels(jupiter_print_levels_all());

#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif
  csvperrorf("<file>", 1, 2, CSV_EL_DEBUG, "retry",
             "This is\nmessage\nwith newline\n");
#ifdef JUPITER_MPI
  MPI_Finalize();
#endif

  return r;
}
