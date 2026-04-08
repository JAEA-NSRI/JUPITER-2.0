
#include "jupiter/control/csvparser.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/string_array.h"
#include "test-util.h"
#include "test/control/control_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef CSVPARSER_TEST_FILE
#define CSVPARSER_TEST_FILE "__tmp.txt"
#endif

int test_control_csvparser(void)
{
  FILE *fp;
  jcntrl_char_array *d;
  jcntrl_string_array *s;
  jcntrl_csvparser *p;
  int ret;
  ret = 0;

  if (!test_compare_pp((p = jcntrl_csvparser_new()), !=, NULL))
    return 1;

  do {
    const char *cp;
    jcntrl_size_type l;

    cp = NULL;
    fp = NULL;
    if (!test_compare_pp((fp = fopen(CSVPARSER_TEST_FILE, "w+b")), !=, NULL))
      ret = 1;
    if (ret)
      break;

    fprintf(fp, "abcd,efgh,  i,j,, kl\nmno,pqr,\r\n,stu,vwxyz\n"
                "112233,\"45678\",90101,\"\",\"\"\"\",\"nl>\n<nl\"\n"
                "\",\"\n");
    if (!test_compare_ii(fseek(fp, 0, SEEK_SET), ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_csvparser_set_input_file(p,
                                                         CSVPARSER_TEST_FILE),
                         ==, 1))
      ret = 1;

    jcntrl_csvparser_set_stream(p, fp);

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "abcd", 4))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "efgh", 4))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 3))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "  i", 3))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 1))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "j", 1))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 0))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 3))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, " kl", 3))
        ret = 1;
    }

    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 3))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "mno", 3))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 3))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "pqr", 3))
        ret = 1;
    }

    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 0))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 3))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "stu", 3))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 5))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "vwxyz", 5))
        ret = 1;
    }

    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 6))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "112233", 6))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 5))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "45678", 5))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 5))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "90101", 5))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 0))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 1))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "\"", 1))
        ret = 1;
    }

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 7))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, "nl>\n<nl", 7))
        ret = 1;
    }

    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;

    cp = NULL;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), !=, NULL))
      ret = 1;
    if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 1))
      ret = 1;
    if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
      ret = 1;
    if (cp) {
      if (!test_compare_ssn(cp, ",", 1))
        ret = 1;
    }

    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;

    fclose(fp);
    if (!test_compare_pp((fp = fopen(CSVPARSER_TEST_FILE, "w+b")), !=, NULL))
      ret = 1;
    if (ret)
      break;

    fprintf(fp, "0011,2233,4455,\"6789\",1011,1024\naabb,ccdd,");
    if (!test_compare_ii(fseek(fp, 0, SEEK_SET), ==, 0))
      ret = 1;

    jcntrl_csvparser_set_stream(p, fp);

    s = NULL;
    if (!test_compare_pp((s = jcntrl_csvparser_read_row(p)), !=, NULL))
      ret = 1;

    if (s) {
      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 6))
        ret = 1;

      if (!test_compare_pp((d = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;
      if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
        ret = 1;
      if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
        ret = 1;
      if (cp) {
        if (!test_compare_ssn(cp, "0011", 4))
          ret = 1;
      }

      if (!test_compare_pp((d = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;
      if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
        ret = 1;
      if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
        ret = 1;
      if (cp) {
        if (!test_compare_ssn(cp, "2233", 4))
          ret = 1;
      }

      if (!test_compare_pp((d = jcntrl_string_array_get(s, 2)), !=, NULL))
        ret = 1;
      if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
        ret = 1;
      if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
        ret = 1;
      if (cp) {
        if (!test_compare_ssn(cp, "4455", 4))
          ret = 1;
      }

      if (!test_compare_pp((d = jcntrl_string_array_get(s, 3)), !=, NULL))
        ret = 1;
      if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
        ret = 1;
      if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
        ret = 1;
      if (cp) {
        if (!test_compare_ssn(cp, "6789", 4))
          ret = 1;
      }
    }
    if (!test_compare_pp((s = jcntrl_csvparser_read_row(p)), !=, NULL))
      ret = 1;

    if (s) {
      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      if (!test_compare_pp((d = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;
      if (!test_compare_ii((l = jcntrl_char_array_get_ntuple(d)), ==, 4))
        ret = 1;
      if (!test_compare_pp((cp = jcntrl_char_array_get(d)), !=, NULL))
        ret = 1;
      if (cp) {
        if (!test_compare_ssn(cp, "aabb", 4))
          ret = 1;
      }
    }

    fclose(fp);
    if (!test_compare_pp((fp = fopen(CSVPARSER_TEST_FILE, "w+b")), !=, NULL))
      ret = 1;
    if (ret)
      break;

    jcntrl_csvparser_set_stream(p, fp);

    if (!test_compare_ii(jcntrl_csvparser_eof(p), ==, 0))
      ret = 1;

    if (!test_compare_pp((s = jcntrl_csvparser_read_row(p)), ==, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_csvparser_eof(p), ==, 1))
      ret = 1;

    fclose(fp);
    if (!test_compare_pp((fp = fopen(CSVPARSER_TEST_FILE, "w+b")), !=, NULL))
      ret = 1;
    if (ret)
      break;

    fprintf(fp, "abcd\"efg\"\n");
    if (!test_compare_ii(fseek(fp, 0, SEEK_SET), ==, 0))
      ret = 1;

    jcntrl_csvparser_set_stream(p, fp);

    if (!test_compare_ii(jcntrl_csvparser_error(p), ==, 0))
      ret = 1;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_csvparser_error(p), ==, 1))
      ret = 1;

    fclose(fp);
    if (!test_compare_pp((fp = fopen(CSVPARSER_TEST_FILE, "w+b")), !=, NULL))
      ret = 1;
    if (ret)
      break;

    fprintf(fp, "\"efg\n");
    if (!test_compare_ii(fseek(fp, 0, SEEK_SET), ==, 0))
      ret = 1;

    jcntrl_csvparser_set_stream(p, fp);

    if (!test_compare_ii(jcntrl_csvparser_error(p), ==, 0))
      ret = 1;
    if (!test_compare_pp((d = jcntrl_csvparser_read_cell(p)), ==, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_csvparser_error(p), ==, 1))
      ret = 1;
  } while (0);

  if (fp)
    fclose(fp);
  jcntrl_csvparser_delete(p);
  return ret;
}
