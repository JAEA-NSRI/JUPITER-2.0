
#include "jupiter/control/csvparser.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/input.h"
#include "jupiter/control/static_array.h"
#include "jupiter/control/string_array.h"
#include "jupiter/control/write_fv_csv.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_expect_raise.h"
#include "test/control/control_test_field_value_feeder.h"
#include "test/control/control_test_util.h"

#include <stdio.h>
#include <stddef.h>

#ifndef CSVWRITER_TEST_FILE
#define CSVWRITER_TEST_FILE "__tmp.txt"
#endif

int test_control_write_fv_csv(void)
{
  struct field_value_feeder f1, f2, f3;
  jcntrl_write_fv_csv *writer;
  jcntrl_csvparser *parser;
  int ret;
  double inputs[10] = {0.0};
  ret = 0;

  field_value_feeder_init(&f1);
  field_value_feeder_init(&f2);
  field_value_feeder_init(&f3);
  writer = NULL;
  parser = NULL;

  jcntrl_static_double_array_init_base(&f1.array, &inputs[0], 1);
  jcntrl_static_double_array_init_base(&f2.array, &inputs[3], 1);
  jcntrl_static_double_array_init_base(&f3.array, &inputs[7], 3);
  inputs[0] = 1.5;
  inputs[3] = 3.5;
  inputs[7] = -33.5;

  control_test_use_expect_raise();

  do {
    FILE *fp;
    jcntrl_input *input;
    jcntrl_output *output;

    fp = fopen(CSVWRITER_TEST_FILE, "r");
    if (fp) {
      fclose(fp);
      if (!test_compare_ii(remove(CSVWRITER_TEST_FILE), ==, 0))
        ret = 1;
    }

    if (!test_compare_pp(jcntrl_executive_set_name(&f1.executive, "f1"), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_executive_set_name(&f2.executive, "f2"), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_executive_set_name(&f3.executive, "f\"3\""), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((writer = jcntrl_write_fv_csv_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((parser = jcntrl_csvparser_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(
          jcntrl_write_fv_csv_set_output_file_c(writer, CSVWRITER_TEST_FILE,
                                                strlen(CSVWRITER_TEST_FILE)),
          ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_set_number_of_inputs(writer, 2),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 3)), ==,
                         NULL))
      ret = 1;

    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 0)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&f1.executive, 0)),
                         !=, NULL))
      ret = 1;

    if (!input || !output)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 1)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&f2.executive, 0)),
                         !=, NULL))
      ret = 1;

    if (!input || !output)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 0, 0.0), ==, 1))
      ret = 1;

    inputs[0] = 3.0;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 1, 0.0), ==, 1))
      ret = 1;

    inputs[0] = 33333.3359375;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 2, 0.0), ==, 1))
      ret = 1;

    jcntrl_write_fv_csv_set_precision(writer, 11);

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 3, 0.0), ==, 1))
      ret = 1;

    jcntrl_write_fv_csv_set_use_scientific(writer, 1);

    inputs[3] = 6.103515625e-5;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 4, 0.0), ==, 1))
      ret = 1;

    jcntrl_write_fv_csv_set_precision(writer, -1);

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 5, 0.0), ==, 1))
      ret = 1;

    inputs[3] = -10.0;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 6, 0.0), ==, 1))
      ret = 1;

    inputs[3] = 1.0;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    /* This will not be written */
    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 0, 0.0), ==, 1))
      ret = 1;

    jcntrl_csvparser_set_input_file(parser, CSVWRITER_TEST_FILE);
    jcntrl_csvparser_open_file(parser);

    do {
      jcntrl_data_array *d;
      jcntrl_char_array *c;

      jcntrl_string_array *s;
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chstr(d, "f1"))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chstr(d, "f2"))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 1.5))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 3.5))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 3.0))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 3.5))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 33333.3359375, 0.1))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 3.5))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 33333.3359375, 0.00001))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 3.5))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 33333.3359375, 0.00001))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, 6.103515625e-5))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 33333.3359375, 0.01))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 6.103515625e-5, 1.0e-10))
          ret = 1;

      //
      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_string_array_get_ntuple(s), ==, 2))
        ret = 1;

      c = NULL;
      d = NULL;
      if (!test_compare_pp((c = jcntrl_string_array_get(s, 0)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_cheps(d, 33333.3359375, 0.01))
          ret = 1;

      if (!test_compare_pp((c = jcntrl_string_array_get(s, 1)), !=, NULL))
        ret = 1;

      if (c)
        if (!test_compare_pp((d = jcntrl_char_array_data(c)), !=, NULL))
          ret = 1;

      if (d)
        if (!test_compare_jcntrl_chdd(d, ==, -10.0))
          ret = 1;

      if (!test_compare_pp((s = jcntrl_csvparser_read_row(parser)), ==, NULL))
        ret = 1;

      if (!test_compare_ii(jcntrl_csvparser_eof(parser), ==, 1))
        ret = 1;
    } while (0);

    jcntrl_csvparser_close_stream(parser);


    // swap input
    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 0)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&f2.executive, 0)),
                         !=, NULL))
      ret = 1;

    if (!input || !output)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 1)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&f1.executive, 0)),
                         !=, NULL))
      ret = 1;

    if (!input || !output)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 99, 0.0), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_CSV_HEADER_MISMATCH))
      ret = 1;

    // Add port
    if (!test_compare_ii(jcntrl_write_fv_csv_set_number_of_inputs(writer, 3),
                         ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((input = jcntrl_write_fv_csv_get_input(writer, 2)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&f3.executive, 0)),
                         !=, NULL))
      ret = 1;

    if (!input || !output)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(
                           jcntrl_write_fv_csv_executive(writer)),
                         ==, 1))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_write_fv_csv_write(writer, 100, 0.0), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_CSV_HEADER_MISMATCH))
      ret = 1;

  } while (0);

  field_value_feeder_clean(&f1);
  field_value_feeder_clean(&f2);
  field_value_feeder_clean(&f3);
  if (writer)
    jcntrl_write_fv_csv_delete(writer);
  if (parser)
    jcntrl_csvparser_delete(parser);

  return ret;
}
