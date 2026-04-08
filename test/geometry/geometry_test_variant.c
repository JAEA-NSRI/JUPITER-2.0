
#include <string.h>

#include "test-util.h"
#include "geometry_test.h"

#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/variant.h>

int variant_test(void)
{
  int ecnt;
  geom_error e;
  geom_variant *var;

  ecnt = 0;
  var = geom_variant_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS, "new variant")) {
    return 1;
  }

  e = geom_variant_set_char(var, 'a');
  if (test_compare_f(e, GEOM_SUCCESS, "set char")) ecnt++;

  test_compare(geom_variant_get_char(var, &e), 'a');
  if (test_compare_f(e, GEOM_SUCCESS, "get char")) ecnt++;

  geom_variant_get_string(var, &e);
  if (test_compare_f(e, GEOM_ERR_VARIANT_TYPE, "invalid type")) ecnt++;

  if (test_compare(geom_variant_get_char(var, &e), 'a')) ecnt++;

  geom_variant_get_double(var, &e);
  if (test_compare_f(e, GEOM_ERR_VARIANT_TYPE, "invalid type")) ecnt++;

  e = geom_variant_set_int(var, 5666);
  if (test_compare_f(e, GEOM_SUCCESS, "set int")) ecnt++;

  if (test_compare(geom_variant_get_int(var, &e), 5666)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get int")) ecnt++;

  e = geom_variant_set_string(var, "test string", 0);
  if (test_compare_f(e, GEOM_SUCCESS, "set string")) ecnt++;

  if (test_compare(strcmp(geom_variant_get_string(var, &e),
                          "test string"), 0)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get string")) ecnt++;

  e = geom_variant_set_string(var, "test string", 4);
  if (test_compare_f(e, GEOM_SUCCESS, "set string")) ecnt++;

  if (test_compare(strcmp(geom_variant_get_string(var, &e),
                          "test"), 0)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get string")) ecnt++;

  e = geom_variant_set_long_int(var, 9999);
  if (test_compare_f(e, GEOM_SUCCESS, "set long int")) ecnt++;

  if (test_compare(geom_variant_get_long_int(var, &e), 9999)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get long int")) ecnt++;

  e = geom_variant_set_double(var, 2.0);
  if (test_compare_f(e, GEOM_SUCCESS, "set double")) ecnt++;

  if (test_compare(geom_variant_get_double(var, &e), 2.0)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get double")) ecnt++;

  geom_variant_nullify(var);
  if (test_compare(geom_variant_is_null(var), 1)) ecnt++;

  e = geom_variant_set_data_op(var, GEOM_OP_ADD);
  if (test_compare_f(e, GEOM_SUCCESS, "set op")) ecnt++;

  if (test_compare(geom_variant_get_data_op(var, &e),
                   GEOM_OP_ADD)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get op")) ecnt++;

  e = geom_variant_set_data_op(var, GEOM_OP_INVALID);
  if (test_compare_f(e, GEOM_SUCCESS, "set op")) ecnt++;

  if (test_compare(geom_variant_get_data_op(var, &e),
                   GEOM_OP_INVALID)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get op")) ecnt++;

  e = geom_variant_set_shape_op(var, GEOM_SOP_ADD);
  if (test_compare_f(e, GEOM_SUCCESS, "set sop")) ecnt++;

  if (test_compare(geom_variant_get_shape_op(var, &e),
                   GEOM_SOP_ADD)) ecnt++;
  if (test_compare_f(e, GEOM_SUCCESS, "get sop")) ecnt++;

  e = geom_variant_set_int(var, 11);
  if (test_compare(geom_variant_get_data_op(var, &e),
                   GEOM_OP_INVALID)) ecnt++;
  if (test_compare_f(e, GEOM_ERR_VARIANT_TYPE, "get op")) ecnt++;

  e = GEOM_SUCCESS;

 error:
  geom_variant_delete(var);

  if (e != GEOM_SUCCESS || ecnt > 0) {
    return 1;
  } else {
    return 0;
  }
}
