
/*
 * Do not use this source as reference of surface shape implementation.
 */

#include "surfshp_specials.h"
#include "defs.h"
#include "func_defs.h"
#include "global.h"

static geom_variant_type
geom_surface_shape_specials_args_next(geom_args_builder *b,
                                      geom_variant *description, int *optional)
{
  /* No extra argument required */
  return GEOM_VARTYPE_NULL;
}

static geom_surface_shape_funcs geom_surface_shape_comb_set = {
  .enum_val = GEOM_SURFACE_SHAPE_COMB,
  .shape_type = GEOM_SHPT_SPECIAL,
  .c = {
    .allocator = NULL,
    .deallocator = NULL,
    .set_value = NULL,
    .get_value = NULL,
    .args_next = geom_surface_shape_specials_args_next,
    .args_check = NULL,
    .infomap_gen = NULL,
  },
  .body_bboxf = NULL,
  .body_testf = NULL,
  .to_pathf = NULL
};

geom_error geom_install_surface_shape_specials(void)
{
  return geom_install_surface_shape_func(&geom_surface_shape_comb_set);
}

const geom_surface_shape_funcs *geom_surface_shape_comb =
  &geom_surface_shape_comb_set;
