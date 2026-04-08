
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <jupiter/table/table.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/fv_table.h>
#include <jupiter/control/global.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/input.h>
#include <jupiter/control/information.h>
#include <jupiter/control/output.h>
#include <jupiter/control/field_variable.h>
#include <jupiter/control/data_array.h>

#include <jupiter/control/struct_data.h>
#include <time.h>

#include "jupiter/control/static_array.h"
#include "test-util.h"
#include "control_test.h"
#include "control_test_field_value_feeder.h"

static double x1[] = {0.0, 1.0, 2.0, 4.0, 5.0, 6.0, 10.0, 12.0, 22.0, 24.0};
static double v1[] = {11.0, 2.0, 1.0, -4.0, 2.0, 5.0, 1.0, -1.0, 3.0, -11.0};

int test_control_fv_table(void)
{
  int ret;
  jcntrl_information *info;
  jcntrl_shared_object *obj;
  jcntrl_executive *exe;
  jcntrl_fv_table *fv;
  jcntrl_field_variable *out_var;
  jcntrl_input *input;
  jcntrl_output *output;
  double xvalue, yvalue;
  struct field_value_feeder x_feeder, y_feeder;

  if (!test_compare_ii(jcntrl_install_fv_table(), ==, 1))
    return 1;

  exe = jcntrl_executive_new(JCNTRL_FV_TABLE);
  if (!test_compare_pp(exe, !=, NULL))
    return 1;

  fv = jcntrl_fv_table_downcast(jcntrl_executive_object(exe));
  if (!test_compare_pp(fv, !=, NULL)) {
    fprintf(stderr, "..... Expected jcntrl_fv_table executive but got %s\n",
            jcntrl_executive_get_class_name(exe));
    jcntrl_executive_delete(exe);
    return 1;
  }

  ret = 0;
  if (!test_compare_ii(field_value_feeder_init(&x_feeder), ==, 1))
    ret = 1;

  if (!test_compare_ii(field_value_feeder_init(&y_feeder), ==, 1))
    ret = 1;

  if (ret)
    goto clean;

  jcntrl_static_double_array_init_base(&x_feeder.array, &xvalue, 1);
  jcntrl_static_double_array_init_base(&y_feeder.array, &yvalue, 1);

  if (!test_compare_ii(jcntrl_fv_table_set_table_1d(fv, 10, x1, v1), ==, 1))
    ret = 1;

  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  if (!test_compare_pp(input, !=, NULL)) {
    ret = 1;
    goto clean;
  }

  if (!test_compare_pp(output, !=, NULL)) {
    ret = 1;
    goto clean;
  }

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 1), ==, input)) {
    ret = 1;
    goto clean;
  }

  if (!test_compare_ii(jcntrl_executive_update_information(exe), ==, 1))
    ret = 1;

  output = jcntrl_output_next_port(output);
  if (!test_compare_pp(output, !=, NULL))
    ret = 1;

  info = jcntrl_output_information(output);
  if (!test_compare_pp(info, !=, NULL))
    ret = 1;

  obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!test_compare_pp(obj, !=, NULL))
    ret = 1;

  out_var = jcntrl_field_variable_downcast(obj);
  if (!test_compare_pp(out_var, !=, NULL))
    ret = 1;

  input = jcntrl_input_next_port(input);
  if (!test_compare_pp(input, !=, NULL))
    ret = 1;

  if (ret)
    goto clean;

  if (!test_compare_ii(jcntrl_input_connect(input, jcntrl_executive_output_port(
                                                     &x_feeder.executive, 0)),
                       ==, 1)) {
    ret = 1;
  }

  xvalue = 0.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 11.0))
    ret = 1;

  xvalue = 7.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 4.0))
    ret = 1;

  xvalue = -1.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 11.0))
    ret = 1;

  xvalue = -2.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 11.0))
    ret = 1;

  xvalue = 25.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -11.0))
    ret = 1;

  xvalue = 26.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -11.0))
    ret = 1;

  /* circular */
  jcntrl_fv_table_set_extend_mode(fv, JCNTRL_FV_TABLE_EXTEND_CIRCULAR);

  xvalue = -1.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -4.0))
    ret = 1;

  xvalue = -2.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 3.0))
    ret = 1;

  xvalue = 25.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 2.0))
    ret = 1;

  xvalue = 26.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 1.0))
    ret = 1;

  /* extrapolate */
  jcntrl_fv_table_set_extend_mode(fv, JCNTRL_FV_TABLE_EXTEND_EXTRAPOLATE);

  xvalue = -1.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 20.0))
    ret = 1;

  xvalue = -2.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 29.0))
    ret = 1;

  xvalue = 25.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -18.0))
    ret = 1;

  xvalue = 26.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -25.0))
    ret = 1;

  /* mirror */
  jcntrl_fv_table_set_extend_mode(fv, JCNTRL_FV_TABLE_EXTEND_MIRROR);

  xvalue = -1.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 2.0))
    ret = 1;

  xvalue = -2.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 1.0))
    ret = 1;

  xvalue = 25.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, -4.0))
    ret = 1;

  xvalue = 26.0;
  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_dd(jcntrl_field_variable_value(out_var, NULL), ==, 3.0))
    ret = 1;

clean:
  field_value_feeder_clean(&x_feeder);
  field_value_feeder_clean(&y_feeder);
  jcntrl_fv_table_delete(fv);
  return ret;
}
