#include <stdio.h>
#include <inttypes.h>
#include "test-util.h"
#include <jupiter/csvutil.h>
#include <jupiter/os/asprintf.h>
#include <jupiter/dccalc.h>

int main(int argc, char **argv)
{
  int r = 0;
  int i, icomm;
  int diff, base;
  ptrdiff_t addr;

  if (test_compare(dc_calc_binary_size(-1),    0)) r = 1;
  if (test_compare(dc_calc_binary_size( 0),    0)) r = 1;
  if (test_compare(dc_calc_binary_size( 1),    0)) r = 1;
  if (test_compare(dc_calc_binary_size( 2),    2)) r = 1;
  if (test_compare(dc_calc_binary_size( 5),   20)) r = 1;
  if (test_compare(dc_calc_binary_size(10),   90)) r = 1;
  if (test_compare(dc_calc_binary_size(50), 2450)) r = 1;

  if (test_compare(dc_calc_binary_size_commutative(-1),    0)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative( 0),    0)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative( 1),    0)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative( 2),    1)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative( 5),   10)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative(10),   45)) r = 1;
  if (test_compare(dc_calc_binary_size_commutative(50), 1225)) r = 1;

  if (test_compare(dc_calc_binary_address(0, 0, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 0,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 0,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 0, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 0,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 0,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 1, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 1,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 1,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 1, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 1,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 1,  1), -1)) r = 1;

  if (test_compare(dc_calc_binary_address_commutative(0, 0, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 0,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 0,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 0, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 0,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 0,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 1, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 1,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 1,  1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 1, -1), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 1,  0), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 1,  1), -1)) r = 1;

  if (test_compare(dc_calc_binary_address(0, 0, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 1, 2),  0)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 0, 2),  1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 1, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 1, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 2, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(-1, 0, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, -1, 2), -1)) r = 1;

  if (test_compare(dc_calc_binary_address(0, 1, 5),  0)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 2, 5),  1)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 3, 5),  2)) r = 1;
  if (test_compare(dc_calc_binary_address(0, 4, 5),  3)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 0, 5),  4)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 2, 5),  5)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 3, 5),  6)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 4, 5),  7)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 0, 5),  8)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 1, 5),  9)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 3, 5), 10)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 4, 5), 11)) r = 1;
  if (test_compare(dc_calc_binary_address(3, 0, 5), 12)) r = 1;
  if (test_compare(dc_calc_binary_address(3, 1, 5), 13)) r = 1;
  if (test_compare(dc_calc_binary_address(3, 2, 5), 14)) r = 1;
  if (test_compare(dc_calc_binary_address(3, 4, 5), 15)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 0, 5), 16)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 1, 5), 17)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 2, 5), 18)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 3, 5), 19)) r = 1;

  if (test_compare(dc_calc_binary_address(0, 0, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(1, 1, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(2, 2, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(3, 3, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 4, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(5, 5, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(4, 5, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address(5, 4, 5), -1)) r = 1;

  if (test_compare(dc_calc_binary_address_commutative(0, 0, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 1, 2),  0)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 0, 2),  0)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 1, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 1, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 2, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(-1, 0, 2), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, -1, 2), -1)) r = 1;

  if (test_compare(dc_calc_binary_address_commutative(0, 1, 5),  0)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 2, 5),  1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 3, 5),  2)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(0, 4, 5),  3)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 0, 5),  0)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 0, 5),  1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(3, 0, 5),  2)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 0, 5),  3)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 2, 5),  4)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 3, 5),  5)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 4, 5),  6)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 1, 5),  4)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(3, 1, 5),  5)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 1, 5),  6)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 3, 5),  7)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 4, 5),  8)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(3, 2, 5),  7)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 2, 5),  8)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(3, 4, 5),  9)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 3, 5),  9)) r = 1;

  if (test_compare(dc_calc_binary_address_commutative(0, 0, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(1, 1, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(2, 2, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(3, 3, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 4, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(5, 5, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(4, 5, 5), -1)) r = 1;
  if (test_compare(dc_calc_binary_address_commutative(5, 4, 5), -1)) r = 1;

  if (test_compare_c(dc_calc_binary_ids(0, -1, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids(0,  0, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids(0,  1, &diff, &base) == 0)) r = 1;

  if (test_compare_c(dc_calc_binary_ids_commutative(0, -1, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids_commutative(0,  0, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids_commutative(0,  1, &diff, &base) == 0)) r = 1;

  if (test_compare_c(dc_calc_binary_ids(-1, 2, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids( 2, 2, &diff, &base) == 0)) r = 1;

  if (test_compare_c(dc_calc_binary_ids_commutative(-1, 2, &diff, &base) == 0)) r = 1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 1, 2, &diff, &base) == 0)) r = 1;

  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 0, 2, &diff, &base) == 1 && diff == 0 && base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 1, 2, &diff, &base) == 1 && diff == 1 && base == 0)) r = 1;

  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 0, 2, &diff, &base) == 1 && diff == 0 && base == 1)) r = 1;

  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(-1, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 0, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 1, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 2, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 3, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 4, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 5, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 6, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 7, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 8, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids( 9, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(10, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(11, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(12, 5, &diff, &base) == 1 &&
                     diff == 3 &&
                     base == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(13, 5, &diff, &base) == 1 &&
                     diff == 3 &&
                     base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(14, 5, &diff, &base) == 1 &&
                     diff == 3 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(15, 5, &diff, &base) == 1 &&
                     diff == 3 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(16, 5, &diff, &base) == 1 &&
                     diff == 4 &&
                     base == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(17, 5, &diff, &base) == 1 &&
                     diff == 4 &&
                     base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(18, 5, &diff, &base) == 1 &&
                     diff == 4 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(19, 5, &diff, &base) == 1 &&
                     diff == 4 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids(20, 5, &diff, &base) == 0)) r = 1;

  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(-1, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 0, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 1)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 1, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 2, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 3, 5, &diff, &base) == 1 &&
                     diff == 0 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 4, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 2)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 5, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 6, 5, &diff, &base) == 1 &&
                     diff == 1 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 7, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 3)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 8, 5, &diff, &base) == 1 &&
                     diff == 2 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative( 9, 5, &diff, &base) == 1 &&
                     diff == 3 &&
                     base == 4)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(10, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(11, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(12, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(13, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(14, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(15, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(16, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(17, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(18, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(19, 5, &diff, &base) == 0)) r = 1;
  diff = base = -1;
  if (test_compare_c(dc_calc_binary_ids_commutative(20, 5, &diff, &base) == 0)) r = 1;

  /* Bulk reverse function check */
  for (icomm = 0; icomm < 2; ++icomm) {
    const char *ccomm;

    if (icomm) {
      ccomm = "_commutative";
    } else {
      ccomm = "";
    }
    for (i = 2; i < 51; ++i) {
      ptrdiff_t ns;
      ptrdiff_t ptr;

      if (icomm) {
        ns = dc_calc_binary_size_commutative(i);
      } else {
        ns = dc_calc_binary_size(i);
      }
      for (ptr = 0; ptr < ns; ++ptr) {
        int stat;
        int m;
        ptrdiff_t got;
        char *txt;
        const char *ctxt;

        diff = -1;
        base = -1;
        got = -1;
        if (icomm) {
          stat = (dc_calc_binary_ids_commutative(ptr, i, &diff, &base) == 1);
        } else {
          stat = (dc_calc_binary_ids(ptr, i, &diff, &base) == 1);
        }
        if (stat) {
          if (icomm) {
            stat = ((got = dc_calc_binary_address_commutative(diff, base, i)) == ptr);
          } else {
            stat = ((got = dc_calc_binary_address(diff, base, i)) == ptr);
          }
        }
        m = jupiter_asprintf(&txt, "dc_calc_binary_ids%s(%" PRIdMAX ", %d, "
                             "&diff, &base) == 1 && "
                             "dc_calc_binary_address%s(%d, %d, %d) == "
                             "%" PRIdMAX "", ccomm,
                             (intmax_t)ptr, i, ccomm, diff, base, i,
                             (intmax_t)ptr);
        if (m < 0) {
          ctxt = "(error formatting)";
          txt = NULL;
        } else {
          ctxt = txt;
        }
        if (test_compare_fail(!stat, __FILE__, __LINE__, txt,
                              "Got %" PRIdMAX "", (intmax_t)got)) {
          r = 1;
        }
        free(txt);
      }
    }
  }

  i = INT_MAX - 1;
  base = i - 1;
  diff = base - 1;
  for (icomm = 0; icomm < 2; ++icomm) {
    const char *ccomm;
    if (icomm) {
      ccomm = "_commutative";
    } else {
      ccomm = "";
    }

    if (icomm) {
      addr = dc_calc_binary_address_commutative(diff, base, i);
    } else {
      addr = dc_calc_binary_address(diff, base, i);
    }
    if (addr == -1) {
      fprintf(stderr, "..... dc_calc_binary_address%s(%d, %d, %d) failed\n", ccomm, diff, base, i);
    } else if (addr < 0) {
      fprintf(stderr, "..... dc_calc_binary_address%s(%d, %d, %d) overflowed\n", ccomm, diff, base, i);
      r = 1;
    } else {
      int ndiff, nbase, res;
      const char *rev;

      fprintf(stderr, "..... dc_calc_binary_address%s(%d, %d, %d) => %" PRIdMAX "\n", ccomm, diff, base, i, (intmax_t)addr);
      if (icomm) {
        res = dc_calc_binary_ids_commutative(addr, i, &ndiff, &nbase);
      } else {
        res = dc_calc_binary_ids(addr, i, &ndiff, &nbase);
      }
      if (res != 1 || ndiff != diff || nbase != base) {
        rev = "FAIL";
      } else {
        rev = "PASS";
      }
      fprintf(stderr, "%s: dc_calc_binary_ids%s(%" PRIdMAX ", %d, &ndiff, &nbase) => %d, ndiff => %d, nbase => %d\n", rev, ccomm, (intmax_t)addr, i, res, ndiff, nbase);
    }
  }

  if (r) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
