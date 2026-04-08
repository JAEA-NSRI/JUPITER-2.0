
/*
 * All geometry definitions
 */
#include "defs.h"

/*
 * Include global.h to implement install function and
 * to call warning functions
 */
#include "global.h"

/*
 * Include func_data.h to set content of geom_init_funcs data.
 */
#include "func_data.h"

/*
 * Include abuilder.h to use geom_args_builder functions.
 */
#include "abuilder.h"

/*
 * Include variant.h to set/get geom_variant / geom_variant_list values.
 */
#include "variant.h"

#include "if_none.h"

/*
 * Mandatory functions are
 *  - Argument type request response function (.c.args_next)
 *  - Calculation function (.func)
 *
 * Optional but definitely required functions are
 *  - Allocator of function specific data (.c.allocator)
 *  - Deallocator of function specific data (.c.deallocator)
 *  - Value setter for function specific data (.c.set_value)
 *  - Value getter for function specific data (.c.get_value)
 *  - Number of parameters for function specific data (.c.n_params)
 *  - Copy function if function specific data is used (.c.copy)
 *    (Currently, the copy function is not used in the initialization.
 *    This may be used in future version)
 *
 * Optional functions which is used for improving the user experience only:
 *  - Argument value checker (.c.args_check)
 *  - Parameter description generator (.c.infomap_gen)
 */

/*
 * Argument type reqest response function
 *
 * If no arguments required, return GEOM_VARTYPE_NULL.
 *
 * All functions should be static.
 */
static
geom_variant_type geom_init_func_none_args_next(geom_args_builder *b,
                                                geom_variant *description,
                                                int *optional)
{
  return GEOM_VARTYPE_NULL;
}

/*
 * Implement your initialization function here.
 *
 * p is allocated data by allocator function.
 *
 * (x, y, z) is the axis value which will be calculated for.
 */
static double
geom_init_func_none_initf(void *p, double x, double y, double z, void *a)
{
  return 0.0;
}

/* Use C99 notation for setting functions, which is highly recommended */
static geom_init_funcs geom_init_func_none = {

  /* Set corresponding enum value of init_func to .enum_val */
  .enum_val = GEOM_INIT_FUNC_NONE,

  /* Set common functions to init and shape function are set to .c member */
  .c =
    {
      /* function specific data is not used here, setting NULL. */
      .allocator = NULL,
      .deallocator = NULL,

      /* Function specific deta is not present. set to NULL */
      .set_value = NULL,

      /* Function specific deta is not present. set to NULL */
      .get_value = NULL,

      /* Function specific deta is not present. set to NULL */
      .n_params = NULL,

      /* Argument type requester */
      .args_next = geom_init_func_none_args_next,

      /* If argument checking is not required, set to NULL */
      .args_check = NULL,

      /* Function specific deta is not present. set to NULL */
      .infomap_gen = NULL,

      /* Function specific data is not present. set to NULL */
      .copy = NULL,
    },

  /* Set initialization function to .func */
  .func = geom_init_func_none_initf,
};

geom_error geom_install_init_func_none(void)
{
  return geom_install_init_func(&geom_init_func_none);
}
