
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "serializer_test.h"

#include <jupiter/serializer/buffer.h>

/*
 * This test contains large size allocation for test allocation
 * failure.
 *
 * But, AddressSanitizer will abort when allocating large size of
 * memory, instead of returning NULL. So this function tells
 * AddressSanitizer to return NULL for allocation error.
 *
 * See https://github.com/google/sanitizers/wiki/AddressSanitizerFlags
 * for details.
 *
 * Use `env ASAN_OPTIONS=help=1 [executable]` to see list of options.
 * (Available options are depedent to the compiler and its version)
 *
 * Since AddressSanitizer can only be usable on GCC or Clang, this
 * function might conflicts to an internal functions in other
 * compilers (and violates DCL37-C[1]). If so, define
 * SKIP_ASAN_FUNCTIONS to avoid definition of ASAN functions.
 *
 * [1]: https://wiki.sei.cmu.edu/confluence/display/c/DCL37-C.+Do+not+declare+or+define+a+reserved+identifier
 *
 * Settings in this function can be overridden by the environment
 * variable, ASAN_OPTIONS. This has priority.
 */
#ifndef SKIP_ASAN_FUNCTIONS
const char *__asan_default_options(void)
{
  return "allocator_may_return_null=1";
}
#endif

void msgpackx_buffer_tree_print(msgpackx_buffer *ptr)
{
  char *dump;
  dump = msgpackx_buffer_tree_dump(ptr, 1, "..... ");
  if (dump) {
    fprintf(stderr, "%s", dump); /* dump includes '\n' */
    free(dump);
  } else {
    fprintf(stderr, "ERROR Could not print tree\n");
  }
}

int buffer_test(void)
{
  int ecnt;
  msgpackx_buffer *buf;
  msgpackx_buffer *bp, *bq, *br, *be;
  char *p;
  ptrdiff_t lp;

  ecnt = 0;

  buf = msgpackx_buffer_new();
  if (!buf) {
    fprintf(stderr, "ERROR: Could not allocate buffer pointer");
    return 1;
  }

  /*
   * In 32 bit enviroment (in other words, `sizeof(ptrdiff_t) == 4`,
   * or `PTRDIFF_MAX == (2^31 - 1)`), this test may not pass in case
   * you have memory of 2 GiB or more.
   *
   * In 64 bit enviroment (in other words, `sizeof(ptrdiff_t) == 8`,
   * or `PTRDIFF_MAX == (2^63 - 1)`), this test may not pass in case
   * you have memory of 8 EiB (1 EiB = 1024 TiB = 1024^2 GiB) or more.
   */
  if (test_compare(msgpackx_buffer_reserve(buf, PTRDIFF_MAX), NULL)) {
    fprintf(stderr, "..... Wow, you have %" PRIdMAX
            " bytes (or more) of memory!\n",
            (intmax_t)PTRDIFF_MAX);
  }

  if (test_compare(msgpackx_buffer_resize(buf, PTRDIFF_MAX), NULL)) {
    fprintf(stderr, "..... Wow, you have %" PRIdMAX
            " bytes (or more) of memory!\n",
            (intmax_t)PTRDIFF_MAX);
  }

  msgpackx_buffer_delete(buf);

  buf = msgpackx_buffer_new();
  if (!buf) {
    fprintf(stderr, "ERROR: Could not allocate buffer pointer");
    return 1;
  }

  if (test_compare(msgpackx_buffer_reserve(buf, -1), NULL)) {
    ecnt++;
  }

  if (test_compare(msgpackx_buffer_resize(buf, -1), NULL)) {
    ecnt++;
  }

  msgpackx_buffer_delete(buf);

  buf = msgpackx_buffer_new();
  if (!buf) {
    fprintf(stderr, "ERROR: Could not allocate buffer pointer");
    return 1;
  }

  if (test_compare(msgpackx_buffer_size(buf), (ptrdiff_t)0)) {
    ecnt++;
  }

  msgpackx_buffer_delete_all_referenced(buf);

  buf = msgpackx_buffer_new();
  if (!buf) {
    fprintf(stderr, "ERROR: Could not allocate buffer pointer");
    return 1;
  }

  if (!msgpackx_buffer_resize(buf, 256)) {
    ecnt++;
    goto error;
  }

  p = msgpackx_buffer_pointer(buf);
  for (lp = 0; lp < 256; ++lp) {
    p[lp] = (char)lp;
  }

  bp = msgpackx_buffer_substr(buf, 6, 3, MSGPACKX_SEEK_SET);
  p = msgpackx_buffer_make_cstr(bp);
  if (test_compare_bytes(bytes(0x06,0x07,0x08), p)) {
    test_print_bytes(p, 3);
    ecnt++;
  }
  free(p);

  msgpackx_buffer_tree_print(bp);
  msgpackx_buffer_delete(bp);
  fprintf(stderr, "..... ---------\n");
  msgpackx_buffer_tree_print(buf);

  bp = msgpackx_buffer_substr(buf,  7, strlen("libra") + 2, MSGPACKX_SEEK_SET);
  bq = msgpackx_buffer_substr(buf, 64, strlen("libra"), MSGPACKX_SEEK_SET);

  msgpackx_buffer_delete(buf);
  buf = msgpackx_buffer_substr(bp, 0, 0, MSGPACKX_SEEK_SET);
  if (!buf) {
    fprintf(stderr, "ERROR Could not make a substr");
    ecnt++;
    goto error;
  }

  msgpackx_buffer_raw_copy(bq, "libra", strlen("libra"), 0);
  fprintf(stderr, "..... ---------\n");
  msgpackx_buffer_tree_print(bp);

  be = msgpackx_buffer_substr(buf, 255, 1, MSGPACKX_SEEK_SET);
  if (test_compare(memcmp(msgpackx_buffer_pointer(be),
                          "\xff", 1), 0)) {
    ecnt++;
  }
  msgpackx_buffer_tree_print(be);

  br = msgpackx_buffer_substr(buf, 222, 1, MSGPACKX_SEEK_SET);
  msgpackx_buffer_delete(br);

  msgpackx_buffer_copy(bp, bq, 0);
  if (test_compare_buffer_sz(bytes('l','i','b','r','a',0x0c,0x0d), bp)) {
    test_print_bytes(msgpackx_buffer_pointer(bp),
                     msgpackx_buffer_size(bp));
    ecnt++;
  }

  msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_ZERO);
  if (test_compare(msgpackx_buffer_size(bp), 7)) {
    ecnt++;
  }
  if (test_compare_buffer(bytes('l','i','b','r','a',0x00,0x00,0x0e), bp)) {
    test_print_bytes(msgpackx_buffer_pointer(bp),
                     msgpackx_buffer_size(bp) + 1);
    ecnt++;
  }

  br = msgpackx_buffer_substr(buf, 10, 5, MSGPACKX_SEEK_SET);
  if (test_compare_buffer_sz(bytes('r','a',0x00,0x00,0x0e), br)) {
    test_print_bytes(msgpackx_buffer_pointer(br),
                     msgpackx_buffer_size(br));
    ecnt++;
  }

  if (test_compare(msgpackx_buffer_is_overlapped(br, bp), 1))
    ecnt++;

  if (test_compare(msgpackx_buffer_is_overlapped(br, be), 0))
    ecnt++;

  if (test_compare(msgpackx_buffer_is_overlapped(buf, be), 0))
    ecnt++;

  if (test_compare(msgpackx_buffer_any_overlapped(br), 1))
    ecnt++;

  if (test_compare(msgpackx_buffer_any_overlapped(be), 0))
    ecnt++;

  if (test_compare(msgpackx_buffer_any_overlapped(buf), 0))
    ecnt++;

  msgpackx_buffer_tree_print(bp);
  msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_SHRINK);
  if (test_compare_buffer(bytes('l','i','b','r','a',0x0e,0x0f,0x10), bp)) {
    test_print_bytes(msgpackx_buffer_pointer(bp),
                     msgpackx_buffer_size(bp) + 1);
    ecnt++;
  }
  msgpackx_buffer_tree_print(bp);

  if (test_compare(msgpackx_buffer_size(bp), 5)) {
    ecnt++;
  }
  if (test_compare_buffer_sz(bytes('i','b','r','a',0x0e), br)) {
    test_print_bytes(msgpackx_buffer_pointer(br),
                     msgpackx_buffer_size(br) + 1);
    ecnt++;
  }

  msgpackx_buffer_goto(bq, 0, strlen("sagittarius"), MSGPACKX_SEEK_CUR);
  msgpackx_buffer_raw_copy(bq, "sagittarius", strlen("sagittarius"), 0);

  msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_TRUNC);
  if (test_compare_buffer(bytes('s','a','g','i','t',0x0e,0x0f), bp)) {
    test_print_bytes(msgpackx_buffer_pointer(bp),
                     msgpackx_buffer_size(bp) + 2);
    ecnt++;
  }
  msgpackx_buffer_tree_print(bp);

  msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_EXPAND);

  if (test_compare(msgpackx_buffer_size(bp), strlen("sagittarius"))) {
    ecnt++;
  }
  if (test_compare_buffer(bytes('s','a','g','i','t','t','a','r','i','u','s',
                                0x0e,0x0f),bp)) {
    test_print_bytes(msgpackx_buffer_pointer(bp),
                     msgpackx_buffer_size(bp) + 2);
    ecnt++;
  }

  if (test_compare_buffer(bytes('s','a','g','i','t','t','a','r','i','u','s',
                                0x4b,0x4c),bq)) {
    test_print_bytes(msgpackx_buffer_pointer(bq),
                     msgpackx_buffer_size(bq) + 2);
    ecnt++;
  }

  msgpackx_buffer_increment(bq);
  if (test_compare_buffer(bytes('a','g','i','t','t','a','r','i','u','s',
                                0x4b,0x4c,0x4d),bq)) {
    test_print_bytes(msgpackx_buffer_pointer(bq),
                     msgpackx_buffer_size(bq) + 2);
    ecnt++;
  }

  msgpackx_buffer_decrement(bq);
  if (test_compare_buffer(bytes('s','a','g','i','t','t','a','r','i','u','s',
                                0x4b,0x4c),bq)) {
    test_print_bytes(msgpackx_buffer_pointer(bq),
                     msgpackx_buffer_size(bq) + 2);
    ecnt++;
  }

  if (test_compare(msgpackx_buffer_is_substr(bq), 1)) {
    ecnt++;
  }

  msgpackx_buffer_goto(bq, 0, -1, MSGPACKX_SEEK_CUR);
  if (test_compare(msgpackx_buffer_is_substr(bq), 0)) {
    ecnt++;
  }
  msgpackx_buffer_tree_print(bq);

  /*
   * Shrinked 2 bytes on copying "libra",
   * Extended 6 bytes on copying "sagittarius".
   * Resulting offset of bq will be 68 (+4).
   */
  lp = (char *)msgpackx_buffer_pointer(bq) - (char *)msgpackx_buffer_pointer_root(bq);
  fprintf(stderr, "..... offset of bq: %" PRIdMAX "\n", (intmax_t)lp);
  if (test_compare(((void)"offset of bq", lp), 68)) {
    ecnt++;
  }

  fprintf(stderr, "..... sizeof bq: %" PRIdMAX "\n",
          (intmax_t)msgpackx_buffer_size(bq));

  br = msgpackx_buffer_substr(buf, 255 + 4, 1, MSGPACKX_SEEK_SET);
  fprintf(stderr, "..... addr br %p\n", msgpackx_buffer_pointer(br));
  if (test_compare_buffer(bytes(0xff), br)) {
    test_print_bytes(msgpackx_buffer_pointer(br), 1);
    ecnt++;
  }

  if (test_compare(msgpackx_buffer_pointer(br), msgpackx_buffer_pointer(be))) {
    ecnt++;
  }
  msgpackx_buffer_tree_print(br);

  msgpackx_buffer_goto(bq, 0, strlen("sagittarius"), MSGPACKX_SEEK_CUR);
  msgpackx_buffer_goto(br, 0, 0, MSGPACKX_SEEK_SET);
  msgpackx_buffer_copy(br, bq, MSGPACKX_COPY_EXPAND);
  fprintf(stderr, "..... addr br %p\n", msgpackx_buffer_pointer(br));
  if (test_compare_buffer_sz(bytes('s','a','g','i','t','t','a','r','i','u','s'),
                             br)) {
    test_print_bytes(msgpackx_buffer_pointer(br),
                     msgpackx_buffer_size(br));
    ecnt++;
  }
  msgpackx_buffer_tree_print(br);

  lp = (char *)msgpackx_buffer_pointer(bq) - (char *)msgpackx_buffer_pointer_root(bq);
  msgpackx_buffer_goto(bq, 121, strlen("sagittarius"), MSGPACKX_SEEK_SET);
  msgpackx_buffer_relocate(br, bq, strlen("sagittarius"), MSGPACKX_SEEK_CUR);
  fprintf(stderr, "..... -------------\n");
  msgpackx_buffer_tree_print(br);
  msgpackx_buffer_goto(br, 0, 1, MSGPACKX_SEEK_CUR);
  fprintf(stderr, "..... -------------\n");
  msgpackx_buffer_tree_print(br);

  msgpackx_buffer_raw_copy(br, "capricorn", strlen("capricorn"),
                           MSGPACKX_COPY_EXPAND);
  if (test_compare_buffer_sz(bytes('c','a','p','r','i','c','o','r','n'), br)) {
    test_print_bytes(msgpackx_buffer_pointer(br),
                     msgpackx_buffer_size(br));
    ecnt++;
  }

  if (test_compare(msgpackx_buffer_size(bq), strlen("sagittarius"))) {
    ecnt++;
  }
  msgpackx_buffer_tree_print(bq);

  msgpackx_buffer_copy(bq, br, MSGPACKX_COPY_SHRINK);
  if (test_compare_buffer_sz(bytes('c','a','p','r','i','c','o','r','n'), bq)) {
    test_print_bytes(msgpackx_buffer_pointer(bq),
                     msgpackx_buffer_size(bq));
    ecnt++;
  }
  msgpackx_buffer_tree_print(bq);

  msgpackx_buffer_goto(bq, lp, -1, MSGPACKX_SEEK_SET);

  p = msgpackx_buffer_pointer(buf);
  if (!msgpackx_buffer_resize(buf, 1048576)) {
    ecnt++;
    fprintf(stderr, "..... Failed to expand size.\n");
    goto error;
  }

  if (p == msgpackx_buffer_pointer(buf)) {
    fprintf(stderr, "..... Expansion did not cause memory move. Skip.\n");
  } else {
    fprintf(stderr, "..... Base pointer moved to %p from %p\n",
            msgpackx_buffer_pointer(buf), (void *)p);

    if (test_compare(msgpackx_buffer_size(bp), strlen("sagittarius"))) {
      ecnt++;
    }
    if (test_compare_buffer(bytes('s','a','g','i','t','t','a','r','i','u','s',0x0e,0x0f), bp)) {
      test_print_bytes(msgpackx_buffer_pointer(bp),
                       msgpackx_buffer_size(bp) + 2);
      ecnt++;
    }

    msgpackx_buffer_tree_print(bq);
    msgpackx_buffer_goto(bq, 0, strlen("sagittarius"), MSGPACKX_SEEK_CUR);
    if (test_compare_buffer(bytes('s','a','g','i','t','t','a','r','i','u','s',0x4b,0x4c), bq)) {
      test_print_bytes(msgpackx_buffer_pointer(bq),
                       msgpackx_buffer_size(bq) + 2);
      ecnt++;
    }

    msgpackx_buffer_goto(bq, 0, 0, MSGPACKX_SEEK_END);

    msgpackx_buffer_delete(buf);
    buf = msgpackx_buffer_substr(bp, 1024, 11, MSGPACKX_SEEK_CUR);

    msgpackx_buffer_raw_copy(bp, "virgo", strlen("virgo"), 0);
    bq = msgpackx_buffer_substr(bp, 0, strlen("virgo"),
                                MSGPACKX_SEEK_CUR);
    msgpackx_buffer_tree_print(bp);
    msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_SHRINK);

    if (test_compare_buffer(bytes('v','i','r','g','o',0x0e,0x0f), bp)) {
      test_print_bytes(msgpackx_buffer_pointer(bp),
                       msgpackx_buffer_size(bp) + 2);
      ecnt++;
    }

    msgpackx_buffer_tree_print(bp);
    fprintf(stderr, "..... -------\n");
    msgpackx_buffer_decrement(bq);
    msgpackx_buffer_tree_print(bq);
    fprintf(stderr, "..... -------\n");
    msgpackx_buffer_goto(be, -255, 1, MSGPACKX_SEEK_CUR);
    msgpackx_buffer_tree_print(be);
    fprintf(stderr, "..... -------\n");

    {
      msgpackx_buffer *bx[300];
      ptrdiff_t m;
      memset(bx, 0, sizeof(msgpackx_buffer*) * 300);

      m = 1;
      for (lp = 0; lp < 300; ++lp) {
        m  = (m * 48271) % ((1 << 30) - 1);
        bp = msgpackx_buffer_substr(buf,
                                    m % 1048011,
                                    m % 271, MSGPACKX_SEEK_SET);
        bx[lp] = bp;
      }

      for (lp = 0; lp < 225; ++lp) {
        msgpackx_buffer_delete(bx[lp]);
      }

      msgpackx_buffer_tree_print(bp);

      bp = bx[225];
      bq = bx[226];
      msgpackx_buffer_goto(bp, 0, msgpackx_buffer_size(bq),
                           MSGPACKX_SEEK_CUR);
      msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_FIXED);
      msgpackx_buffer_copy(bp, bq, MSGPACKX_COPY_CREATE);

      fprintf(stderr, "..... --------------\n");
      msgpackx_buffer_tree_print(bp);
      msgpackx_buffer_delete(bp);
    }
  }

 error:
  msgpackx_buffer_delete_all_referenced(buf);
  return ecnt;
}
