
/*
 * Do not use this source as reference of shape implementation.
 * See shp_box.c (for body) or shp_tra.c (for transformation).
 */

#include "shp_specials.h"
#include "abuilder.h"
#include "defs.h"
#include "func_data.h"
#include "global.h"

static geom_variant_type
geom_shape_specials_args_next(geom_args_builder *b, geom_variant *description,
                              int *optional)
{
  /* No extra arguments required. */
  return GEOM_VARTYPE_NULL;
}

static geom_shape_funcs geom_shape_comb_set = {
  .enum_val = GEOM_SHAPE_COMB,
  .shape_type = GEOM_SHPT_SPECIAL,
  .c =
    {
      .allocator = NULL,
      .deallocator = NULL,
      .set_value = NULL,
      .get_value = NULL,
      .args_next = geom_shape_specials_args_next,
      .args_check = NULL,
      .infomap_gen = NULL,
    },
  .body_testf = NULL,
  .body_bboxf = NULL,
  .transform_func = NULL,
};

static geom_shape_funcs geom_shape_gst_set = {
  .enum_val = GEOM_SHAPE_GST,
  .shape_type = GEOM_SHPT_SPECIAL,
  .c =
    {
      .allocator = NULL,
      .deallocator = NULL,
      .set_value = NULL,
      .get_value = NULL,
      .args_next = geom_shape_specials_args_next,
      .args_check = NULL,
      .infomap_gen = NULL,
    },
  .body_testf = NULL,
  .body_bboxf = NULL,
  .transform_func = NULL,
};

static geom_shape_funcs geom_shape_ged_set = {
  .enum_val = GEOM_SHAPE_GED,
  .shape_type = GEOM_SHPT_SPECIAL,
  .c =
    {
      .allocator = NULL,
      .deallocator = NULL,
      .set_value = NULL,
      .get_value = NULL,
      .args_next = geom_shape_specials_args_next,
      .args_check = NULL,
      .infomap_gen = NULL,
    },
  .body_testf = NULL,
  .body_bboxf = NULL,
  .transform_func = NULL,
};

geom_error geom_install_shape_specials(void)
{
  geom_error e;
  e = geom_install_shape_func(&geom_shape_comb_set);
  e = geom_install_shape_func(&geom_shape_gst_set);
  e = geom_install_shape_func(&geom_shape_ged_set);
  return e;
}

const geom_shape_funcs *geom_shape_gst = &geom_shape_gst_set;
const geom_shape_funcs *geom_shape_ged = &geom_shape_ged_set;
const geom_shape_funcs *geom_shape_comb = &geom_shape_comb_set;
