
#include <stdio.h>
#include <string.h>

#include "test-util.h"
#include "geometry_test.h"

#include <jupiter/geometry/error.h>
#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/abuilder.h>
#include <jupiter/geometry/variant.h>
#include <jupiter/geometry/global.h>
#include <jupiter/geometry/data.h>
#include <jupiter/geometry/init.h>
#include <jupiter/geometry/shape.h>
#include <jupiter/geometry/surface_shape.h>
#include <jupiter/geometry/vector.h>

int trip_test(void)
{
  int ecnt;
  geom_error e;
  geom_data *data;
  geom_data_element *el;
  geom_init_data *init_d;
  geom_init_element *init_el;
  geom_init_args_builder *init_ab;
  geom_shape_data *shape_d;
  geom_shape_element *shape_el;
  geom_shape_args_builder *shape_ab;
  geom_surface_shape_data *surf_shape_d;
  geom_surface_shape_element *surf_shape_el;
  geom_surface_shape_args_builder *surf_shape_ab;
  geom_args_builder *ab;
  geom_variant_list *vl;
  geom_variant *v;

  init_ab = NULL;
  shape_ab = NULL;
  surf_shape_ab = NULL;
  v = NULL;
  vl = NULL;

  ecnt = 0;
  data = geom_data_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create new data")) {
    return 1;
  }

  el = geom_data_element_new(data, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create new data element")) {
    ecnt = 1;
    goto error;
  }

  e = geom_data_add_element(el);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to add element")) ecnt++;

  geom_data_element_set_name(el, "Test geometry");

  fprintf(stderr, "\n---- Test initialization definitions\n");
  init_d = geom_init_data_new(el, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create new init data")) {
    ecnt = 1;
    goto error;
  }

  init_ab = geom_init_args_builder_new(GEOM_INIT_FUNC_CONST, &e);
  if (test_compare_f(e, GEOM_SUCCESS,
                     "Failed to create new init argument builder data")) {
    ecnt = 1;
    goto error;
  }

  init_el = geom_init_element_new(init_d, &e);
  if (test_compare_f(e, GEOM_SUCCESS,
                     "Could not create new init_element")) {
    ecnt = 1;
    goto error;
  }

  v = geom_variant_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS,
                     "Failed to create new variant data")) {
    ecnt = 1;
    goto error;
  }

  vl = geom_variant_list_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS,
                     "Failed to create new variant list")) {
    ecnt = 1;
    goto error;
  }

  e = geom_variant_set_double(v, 1.0);
  if (test_compare_f(e, GEOM_SUCCESS,
                     "Could not set value to variant")) ecnt++;

  geom_variant_list_insert_prev(vl, v, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Could not add variant to list")) ecnt++;

  ab = geom_init_args_get_builder(init_ab);
  e = geom_args_builder_vlist(ab, vl, NULL, NULL);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to set init_func data")) ecnt++;

  geom_variant_list_delete_all(vl);
  vl = NULL;

  e = geom_init_element_set_func(init_el, init_ab);
  if (test_compare_f(e, GEOM_SUCCESS, "Could not set function")) ecnt++;

  e = geom_init_element_set_component(init_el, 1, NULL, NULL, "Component I");
  if (test_compare_f(e, GEOM_SUCCESS, "Could not set component")) ecnt++;

  geom_init_args_builder_delete(init_ab);
  init_ab = NULL;

  geom_init_data_add_element(init_el);

  init_el = geom_init_element_new(init_d, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Could not create init element")) ecnt++;

  do {
    geom_variant_list *retp;

    if (test_compare_xf(!init_el, "init_el", "Element is not set")) ecnt++;
    if (!init_el) break;

    vl = geom_variant_list_new(&e);
    if (e != GEOM_SUCCESS) break;

    geom_variant_set_int(v, 100);
    geom_variant_list_insert_prev(vl, v, &e);
    if (e != GEOM_SUCCESS) break;

    e = geom_init_element_set_func_vlist(init_el, NULL,
                                         GEOM_INIT_FUNC_CONST, vl, &retp);
    if (test_compare_f(e, GEOM_ERR_VARIANT_TYPE, "Unexpected error")) ecnt++;

    geom_variant_set_double(v, 100.0);
    geom_variant_list_insert_prev(retp, v, &e);
    geom_variant_list_delete(retp);

    e = geom_init_element_set_func_vlist(init_el, NULL,
                                         GEOM_INIT_FUNC_CONST, vl, &retp);
    if (test_compare_f(e, GEOM_SUCCESS, "Could not set value")) ecnt++;

    geom_init_data_add_element(init_el);

    geom_variant_list_delete_all(vl);
    vl = NULL;
  } while(0);

  do {
    double retv;

    init_el = geom_init_data_get_element(init_d);
    if (test_compare_xf(!init_el, "init_el", "Element is not set")) ecnt++;
    if (!init_el) break;

    retv = geom_init_element_calc_func_at(init_el, 0.0, 0.0, 0.0, NULL, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to calc func")) ecnt++;
    if (test_compare_f(retv, 1.0, "Unexpected function result")) ecnt++;
    init_el = geom_init_element_next(init_el);
    if (test_compare_xf(!init_el, "init_el", "Element is not set")) ecnt++;
    if (!init_el) break;

    retv = geom_init_element_calc_func_at(init_el, 0.0, 0.0, 0.0, NULL, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to calc func")) ecnt++;
    if (test_compare_f(retv, 100.0, "Unexpected function result")) ecnt++;

    init_el = geom_init_element_next(init_el);
    if (test_compare_xf(!!init_el, "!init_el", "Extra init element found")) ecnt++;
  } while(0);

  fprintf(stderr, "\n---- Test Shapes\n");
  e = GEOM_SUCCESS;
  shape_d = geom_shape_data_new(el, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create shape data")) {
    ecnt++;
    goto error;
  }

  shape_el = geom_shape_element_new(shape_d, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create shape element")) {
    ecnt++;
    goto error;
  }

  shape_ab = geom_shape_args_builder_new(GEOM_SHAPE_BOX, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create builder")) {
    ecnt++;
    goto error;
  }

  ab = geom_shape_args_get_builder(shape_ab);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to get builder")) {
    ecnt++;
    goto error;
  }

  if (!vl) {
    vl = geom_variant_list_new(&e);
    if (test_compare_f(e, GEOM_SUCCESS,
                       "Failed to create new variant list")) {
      ecnt++;
      goto error;
    }
  }

  do {
    geom_variant_set_vec3(v, geom_vec3_c(0.0, 0.0, 0.0));
    geom_variant_list_insert_prev(vl, v, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to add vector")) ecnt++;

    geom_variant_set_vec3(v, geom_vec3_c(10.0, 10.0, 10.0));
    geom_variant_list_insert_prev(vl, v, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to add vector")) ecnt++;
  } while(0);

  e = geom_args_builder_vlist(ab, vl, NULL, NULL);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to set builder to list")) ecnt++;

  e = geom_shape_element_set_shape(shape_el, GEOM_SOP_ADD, shape_ab);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to set shape")) ecnt++;

  geom_shape_args_builder_delete(shape_ab);
  shape_ab = NULL;

  fprintf(stderr, "\n---- Test Surface Shapes\n");
  e = GEOM_SUCCESS;
  surf_shape_d = geom_surface_shape_data_new(el, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create surface shape data")) {
    ecnt++;
    goto error;
  }

  surf_shape_el = geom_surface_shape_element_new(surf_shape_d, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Falied to create shape element")) {
    ecnt++;
    goto error;
  }

  surf_shape_ab =
    geom_surface_shape_args_builder_new(GEOM_SURFACE_SHAPE_PARALLELOGRAM, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to create builder")) {
    ecnt++;
    goto error;
  }

  ab = geom_surface_shape_args_get_builder(surf_shape_ab);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to get builder")) {
    ecnt++;
    goto error;
  }

  geom_variant_list_clear(vl);
  do {
    geom_variant_set_vec2(v, geom_vec2_c(0.0, 0.0));
    geom_variant_list_insert_prev(vl, v, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to add vector")) ecnt++;

    geom_variant_set_vec2(v, geom_vec2_c(1.0, 3.0));
    geom_variant_list_insert_prev(vl, v, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to add vector")) ecnt++;

    geom_variant_set_vec2(v, geom_vec2_c(3.0, 1.0));
    geom_variant_list_insert_prev(vl, v, &e);
    if (test_compare_f(e, GEOM_SUCCESS, "Failed to add vector")) ecnt++;
  } while(0);

  e = geom_args_builder_vlist(ab, vl, NULL, NULL);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to set builder to list")) ecnt++;

  e = geom_surface_shape_element_set_shape(surf_shape_el, GEOM_SOP_ADD,
                                           surf_shape_ab);
  if (test_compare_f(e, GEOM_SUCCESS, "Failed to set shape")) ecnt++;

  geom_surface_shape_args_builder_delete(surf_shape_ab);
  surf_shape_ab = NULL;

 error:
  if (e != GEOM_SUCCESS) {
    fprintf(stderr, "Error: %s\n", geom_strerror(e));
  }
  geom_variant_delete(v);
  geom_variant_list_delete_all(vl);
  geom_init_args_builder_delete(init_ab);
  geom_shape_args_builder_delete(shape_ab);
  geom_surface_shape_args_builder_delete(surf_shape_ab);
  geom_data_delete(data);
  return ecnt;
}
