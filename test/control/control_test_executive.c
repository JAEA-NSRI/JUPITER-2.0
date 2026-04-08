
#include <stdlib.h>
#include <string.h>

#include <jupiter/control/defs.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/input.h>
#include <jupiter/control/output.h>
#include <jupiter/control/field_variable.h>
#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/executive_data.h>
#include <jupiter/control/data_array.h>

#include "control_test.h"
#include "jupiter/control/shared_object_priv.h"
#include "test-util.h"

static int fill_input(jcntrl_shared_object *data, int index,
                      jcntrl_input *input);
static int fill_output(jcntrl_shared_object *data, int index,
                       jcntrl_output *output);
static int upd_info(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data);
static int upd_extent(jcntrl_information *request, jcntrl_input *input,
                      jcntrl_output *output, jcntrl_shared_object *data);
static int upd_data(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data);

struct executive_test_shared_data
{
  enum test_case
  {
    TEST_CASE_CALLINGS,
    TEST_CASE_ERROR,
    TEST_CASE_DATA,
  } case_number;
};

struct executive_test_data
{
  jcntrl_executive executive;
  struct executive_test_shared_data *shared_data;
  int call_counter;
  int upd_info_called;
  int upd_extent_called;
  int upd_data_called;
};
#define executive_test_data__ancestor jcntrl_executive
#define executive_test_data__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(executive_test_data);

JCNTRL_VIRTUAL_WRAP_FN(executive_test_data, fill_input, jcntrl_executive,
                       fill_input_port_information)
JCNTRL_VIRTUAL_WRAP_FN(executive_test_data, fill_output, jcntrl_executive,
                       fill_output_port_information)
JCNTRL_VIRTUAL_WRAP_FN(executive_test_data, upd_info, jcntrl_executive,
                       process_update_information)
JCNTRL_VIRTUAL_WRAP_FN(executive_test_data, upd_extent, jcntrl_executive,
                       process_update_extent)
JCNTRL_VIRTUAL_WRAP_FN(executive_test_data, upd_data, jcntrl_executive,
                       process_update_data)

static struct executive_test_data *test_data_dn(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(executive_test_data, struct executive_test_data,
                                obj);
}

static void *test_data_dnv(jcntrl_shared_object *obj)
{
  return test_data_dn(obj);
}

static int test_data_ini(jcntrl_shared_object *obj)
{
  struct executive_test_data *data;
  data = test_data_dn(obj);

  data->shared_data = NULL;
  data->call_counter = 0;
  data->upd_info_called = 0;
  data->upd_extent_called = 0;
  data->upd_data_called = 0;
  return 1;
}

static void test_data_destr(jcntrl_shared_object *obj) { /* nop */ }

static void test_data_initf(jcntrl_shared_object_funcs *f)
{
  f->initializer = test_data_ini;
  f->destructor = test_data_destr;
  f->allocator = NULL;
  f->deleter = NULL;
  f->downcast = test_data_dnv;
  JCNTRL_VIRTUAL_WRAP_SET(f, executive_test_data, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(f, executive_test_data, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(f, executive_test_data, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(f, executive_test_data, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(f, executive_test_data, jcntrl_executive,
                          process_update_data);
}

static void *other_data_dnv(jcntrl_shared_object *obj) { return obj; }

static void other_data_initf(jcntrl_shared_object_funcs *f)
{
  f->downcast = other_data_dnv;
}

#define other_class__ancestor jcntrl_executive
JCNTRL_VTABLE_NONE(other_class);

JCNTRL_SHARED_METADATA_INIT_DEFINE(executive_test_data, test_data_initf)
JCNTRL_SHARED_METADATA_INIT_DEFINE(other_class, other_data_initf)

static struct executive_test_data exe_data, exe_down_data;

static void reset_test_data(struct executive_test_data *data)
{
  JCNTRL_ASSERT(data);
  data->call_counter = 0;
  data->upd_info_called = 0;
  data->upd_extent_called = 0;
  data->upd_data_called = 0;
}

static void init_test_data(struct executive_test_data *data,
                           struct executive_test_shared_data *sdata)
{
  JCNTRL_ASSERT(data);
  JCNTRL_ASSERT(sdata);

  jcntrl_executive_init(&data->executive, executive_test_data_metadata_init());
  data->shared_data = sdata;
  reset_test_data(data);
}

int test_control_executive(void)
{
  int ret;
  jcntrl_input *input;
  jcntrl_output *output;
  static struct jcntrl_executive *exe, *exe_down;
  static struct executive_test_shared_data shared_data;

  other_class_metadata_init();

  init_test_data(&exe_data, &shared_data);
  init_test_data(&exe_down_data, &shared_data);

  exe = &exe_data.executive;
  exe_down = &exe_down_data.executive;

  ret = 0;

  if (!test_compare_pp(jcntrl_executive_set_name(exe, "test object"), !=, NULL))
    ret = 1;
  if (!test_compare_ss(jcntrl_executive_get_name(exe), "test object"))
    ret = 1;

  if (!test_compare_ii(
        jcntrl_executive_is_a(exe, executive_test_data_metadata_init()), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_is_a(exe, other_class_metadata_init()),
                       ==, 0))
    ret = 1;

  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  if (!test_compare_pp(input, !=, NULL))
    ret = 1;
  if (!test_compare_pp(output, !=, NULL))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 1), !=, NULL))
    ret = 1;
  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 1), !=, NULL))
    ret = 1;

  {
    jcntrl_input *ip;
    jcntrl_output *op;

    ip = jcntrl_input_next_port(input);
    op = jcntrl_output_next_port(output);

    if (!test_compare_ii(jcntrl_input_connect(ip, op), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_executive_check_loop(exe), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_input_disconnect(ip), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_executive_check_loop(exe), ==, 1))
      ret = 1;
  }

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 0), !=, NULL))
    ret = 1;
  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 1), !=, NULL))
    ret = 1;

  input = jcntrl_executive_get_input(exe_down);
  output = jcntrl_executive_get_output(exe_down);

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 1), !=, NULL))
    ret = 1;
  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 1), !=, NULL))
    ret = 1;

  output = jcntrl_executive_get_output(exe);
  input = jcntrl_executive_get_input(exe_down);

  output = jcntrl_output_next_port(output);
  input = jcntrl_input_next_port(input);
  if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
    ret = 1;

  shared_data.case_number = TEST_CASE_CALLINGS;
  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_information(exe_down), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_information(exe), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_extent(exe_down), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_extent(exe), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_data(exe_down), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 1))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_data(exe), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe_down), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 3))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 3))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 3))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  shared_data.case_number = TEST_CASE_ERROR;
  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_information(exe_down), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_information(exe), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_extent(exe_down), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_extent(exe), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_data(exe_down), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update_data(exe), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe_down), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  shared_data.case_number = TEST_CASE_DATA;
  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe_down), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 3))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 3))
    ret = 1;

  reset_test_data(&exe_data);
  reset_test_data(&exe_down_data);

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_info_called, ==, 1))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_extent_called, ==, 2))
    ret = 1;
  if (!test_compare_ii(exe_data.upd_data_called, ==, 3))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_info_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_extent_called, ==, 0))
    ret = 1;
  if (!test_compare_ii(exe_down_data.upd_data_called, ==, 0))
    ret = 1;

  jcntrl_executive_delete(exe);
  jcntrl_executive_delete(exe_down);
  return ret;
}

static int fill_input(jcntrl_shared_object *data, int index,
                      jcntrl_input *input)
{
  jcntrl_information *info;
  jcntrl_input *p;

  if (!test_compare_ii(jcntrl_input_is_head(input), ==, 0))
    return 0;

  info = jcntrl_input_information(input);
  if (!test_compare_pp(info, !=, NULL))
    return 0;
  if (!test_compare_ii(
        jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                        JCNTRL_DATATYPE_FIELD_VAR),
        ==, 1))
    return 0;

  return 1;
}

static int fill_output(jcntrl_shared_object *data, int index,
                       jcntrl_output *output)
{
  jcntrl_information *info;
  jcntrl_output *p;

  if (!test_compare_ii(jcntrl_output_is_head(output), ==, 0))
    return 0;

  info = jcntrl_output_information(output);
  if (!test_compare_pp(info, !=, NULL))
    return 0;
  if (!test_compare_ii(jcntrl_information_set_objecttype(info,
                                                         JCNTRL_INFO_DATATYPE,
                                                         jcntrl_field_variable),
                       ==, 1))
    return 0;

  return 1;
}

static int upd_info(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  jcntrl_input *in1;
  jcntrl_output *out1;
  jcntrl_information *ininfo;
  jcntrl_information *outinfo;
  jcntrl_shared_object *obj;
  jcntrl_field_variable *fv;

  struct executive_test_data *edata;
  edata = test_data_dn(data);
  edata->upd_info_called = ++edata->call_counter;

  in1 = jcntrl_input_next_port(input);
  ininfo = NULL;
  if (in1) {
    ininfo = jcntrl_input_upstream_information(in1);
  }

  out1 = jcntrl_output_next_port(output);
  outinfo = NULL;
  if (out1) {
    outinfo = jcntrl_output_information(out1);
  }

  switch (edata->shared_data->case_number) {
  case TEST_CASE_CALLINGS:
    break;
  case TEST_CASE_ERROR:
    return 0;
  case TEST_CASE_DATA:
    if (in1) {
      /* Assumes connected */
      if (!test_compare_pp(ininfo, !=, NULL))
        return 0;

      if (!test_compare_ii(jcntrl_information_has(ininfo,
                                                  JCNTRL_INFO_DATA_OBJECT),
                           ==, 1))
        return 0;

      obj = jcntrl_information_get_object(ininfo, JCNTRL_INFO_DATA_OBJECT);
      if (!test_compare_pp(obj, !=, NULL))
        return 0;

      fv = jcntrl_field_variable_downcast(obj);
      if (!test_compare_pp(fv, !=, NULL))
        return 0;
    }

    if (!test_compare_pp(outinfo, !=, NULL))
      return 0;

    if (!test_compare_ii(jcntrl_information_has(outinfo,
                                                JCNTRL_INFO_DATA_OBJECT),
                         ==, 1))
      return 0;

    obj = jcntrl_information_get_object(outinfo, JCNTRL_INFO_DATA_OBJECT);
    if (!test_compare_pp(obj, !=, NULL))
      return 0;

    fv = jcntrl_field_variable_downcast(obj);
    if (!test_compare_pp(fv, !=, NULL))
      return 0;

    break;
  }

  return 1;
}

static int upd_extent(jcntrl_information *request, jcntrl_input *input,
                      jcntrl_output *output, jcntrl_shared_object *data)
{
  struct executive_test_data *edata;
  edata = test_data_dn(data);
  edata->upd_extent_called = ++edata->call_counter;

  switch (edata->shared_data->case_number) {
  case TEST_CASE_CALLINGS:
    break;
  case TEST_CASE_ERROR:
    return 0;
  case TEST_CASE_DATA:
    break;
  }

  return 1;
}

static int upd_data(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  jcntrl_input *in1;
  jcntrl_output *out1;
  jcntrl_information *ininfo;
  jcntrl_information *outinfo;
  jcntrl_shared_object *obj;
  jcntrl_field_variable *fv;
  jcntrl_data_array *da;
  double fval;

  struct executive_test_data *edata;
  edata = test_data_dn(data);
  edata->upd_data_called = ++edata->call_counter;

  in1 = jcntrl_input_next_port(input);
  ininfo = NULL;
  if (in1) {
    ininfo = jcntrl_input_upstream_information(in1);
  }

  out1 = jcntrl_output_next_port(output);
  outinfo = NULL;
  if (out1) {
    outinfo = jcntrl_output_information(out1);
  }

  switch (edata->shared_data->case_number) {
  case TEST_CASE_CALLINGS:
    break;
  case TEST_CASE_ERROR:
    return 0;
  case TEST_CASE_DATA:
    if (!ininfo) { /* Feed a value to downstream */
      obj = jcntrl_information_get_object(outinfo, JCNTRL_INFO_DATA_OBJECT);
      fv = jcntrl_field_variable_downcast(obj);
      if (!test_compare_ii(jcntrl_field_variable_set_value(fv, 1245.0), ==, 1))
        return 0;

      if (!test_compare_pp(jcntrl_executive_get_output_data_object_as(
                             &edata->executive, 0, jcntrl_field_variable),
                           ==, fv))
        return 0;
    } else {
      int ret;

      ret = 0;
      obj = jcntrl_information_get_object(ininfo, JCNTRL_INFO_DATA_OBJECT);
      fv = jcntrl_field_variable_downcast(obj);
      if (!test_compare_dd((fval = jcntrl_field_variable_value(fv, &ret)), ==,
                           1245.0)) {
        return 0;
      }
      if (!test_compare_ii(ret, ==, 0)) {
        return 0;
      }

      if (!test_compare_pp(jcntrl_executive_get_input_data_object_as(
                             &edata->executive, 0, jcntrl_field_variable),
                           ==, fv))
        return 0;

      obj = jcntrl_information_get_object(outinfo, JCNTRL_INFO_DATA_OBJECT);
      fv = jcntrl_field_variable_downcast(obj);

      fval += 1000.0;
      if (!test_compare_ii(jcntrl_field_variable_set_value(fv, fval), ==, 1))
        return 0;
    }
    break;
  }

  return 1;
}
