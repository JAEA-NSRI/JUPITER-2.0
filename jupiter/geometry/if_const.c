
#include <stdlib.h>
#include <math.h>

#include "geom_assert.h"
#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "infomap.h"

#include "if_const.h"

/*
 * function specific data.
 *
 * recommend to use struct even if the data has only one item.
 *
 * VLA is allowed.
 */
struct geom_init_func_const_data
{
  double cval;
};

static geom_variant_type
geom_init_func_const_args_next(geom_args_builder *b,
                               geom_variant *description, int *optional)
{
  geom_size_type l;

  /*
   * Use geom_args_builder_get_loc function to obtain
   * the index of argument to requested for.
   */
  l = geom_args_builder_get_loc(b);

  /*
   * We need a double for the first argument.
   */
  if (l == 0) {
    /*
     * Give description of the value.
     */
    geom_variant_set_string(description, "constant value", 0);

    /*
     * If the parameter is optional (not mandatory), set *optional to 1.
     */
    /* *optional = 1; */

    /*
     * Return the required data type.
     */
    return GEOM_VARTYPE_DOUBLE;
  }

  /*
   * Others are NULL.
   */
  return GEOM_VARTYPE_NULL;
}

static geom_error
geom_init_func_const_args_check(void *p, geom_args_builder *b,
                                geom_size_type index, const geom_variant *v,
                                geom_variant *errinfo)
{
  geom_error e;
  double cval;

  e = GEOM_SUCCESS;

  /*
   * One of `p` (the type of your extra data) or `b` is available for
   * referring other arguments.
   *
   * When `p` is used, it indicates the caller wants to modify one of
   * parameters and `p` has been always fully initialized.
   *
   * When `b` is used, it indicates the caller wants to create a new
   * one and some parameters can be missing.
   */

  if (index == 0) {
    cval = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) return e;

    if (!isfinite(cval)) {
      if (errinfo) {
        /*
         * You can pass extra error information via setting errinfo with
         * a text.
         *
         * If you allcated the memory to format the string here, you can
         * free immediately after setting the text.
         *
         * errinfo can be NULL (do not confuse with nullified variant)
         * when the caller does not want error info.
         */
        geom_variant_set_string(errinfo, "Value must be finite", 0);

        /*
         * If you need formatting here, yon can use `geom_asprintf`,
         * set it, and then `free`. Note that do not forget `free` the
         * memory allocated by `geom_asprintf`
         *
         * And, do not forget to handle errors.
         */
        /*
         * int r;
         * char *b;
         * r = geom_asprintf(&b, "Value must be finite: %g", cval);
         * if (r < 0) { // fallback
         *   geom_variant_set_string(errinfo, "Value must be finite", 0);
         * } else {
         *   geom_variant_set_string(errinfo, b, 0);
         *   free(b);
         * }
         */

      } else {
        /*
         * To notifying the diagnostic message but not indicating error,
         * use geom_warn.
         *
         * You should not include newline here.
         *
         * You may not concern about printing the information of the
         * location of input set. The library's user has responsibility
         * of printing those message.  (see the documentation of
         * `geom_set_warning_function`)
         */
        geom_warn("%g: Value should be finite", cval);
      }

      /*
       * Returns error
       */
      return GEOM_ERR_RANGE;
    }

    /*
     * Returns ok
     */
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error
geom_init_func_const_get_value(void *p, geom_size_type index,
                               geom_variant *v)
{
  struct geom_init_func_const_data *pp;

  /*
   * If allocator is defined, p is guaranteed to be available.
   */
  /* GEOM_ASSERT(p); */

  /*
   * Explicit cast does not have any effect at runtime (in C),
   * just be textually meaningful.
   */
  pp = (struct geom_init_func_const_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_double(v, pp->cval);
  }
  return GEOM_ERR_RANGE;
}

static geom_error
geom_init_func_const_set_value(void *p, geom_size_type index,
                               const geom_variant *v)
{
  geom_error ee;
  struct geom_init_func_const_data *pp;

  /*
   * If allocator is defined, p is guaranteed to be available.
   */
  /* GEOM_ASSERT(p); */

  /*
   * Explicit cast does not have any effect at runtime (in C),
   * just be textually meaningful.
   */
  pp = (struct geom_init_func_const_data *)p;

  ee = GEOM_SUCCESS;

  if (index == 0) {
    double cval;

    /*
     * Get the double data from the variant.
     *
     * You can reject assigning value by returning GEOM_ERR_RANGE.
     */
    cval = geom_variant_get_double(v, &ee);
    if (ee == GEOM_SUCCESS) {
      pp->cval = cval;
    }
    return ee;
  }

  /* Return GEOM_ERR_RANGE for invalid index value */
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_init_func_const_get_n_params(void *p, geom_args_builder *b)
{
  /*
   * Return number of parameters. You can use -1 if no information available.
   *
   * See geom_init_func_args_checkf() for the usage of arguments.
   */
  return 1;
}

static void *
geom_init_func_const_allocator(void)
{
  struct geom_init_func_const_data *p;
  p = (struct geom_init_func_const_data*)
    malloc(sizeof(struct geom_init_func_const_data));

  if (!p) return NULL; /* If could not allocate, return NULL */

  p->cval = 0.0;

  return p;
}

static void
geom_init_func_const_deallocator(void *p)
{
  free(p);
}

static double
geom_init_func_const_initf(void *p, double x, double y, double z, void *a)
{
  struct geom_init_func_const_data *pp;
  /*
   * If allocator is defined, p is guaranteed to be available.
   *
   * To assert, use `GEOM_ASSERT()` macro (which is declared in error.h)
   */
  /* if (!p) return 0.0; */

  /*
   * Explicit cast does not have any effect at runtime (in C),
   * just be textually meaningful.
   */
  pp = (struct geom_init_func_const_data *)p;

  /*
   * For defining library-defined initializations, you must not use
   * the last argument.
   */

  return pp->cval;
}

static geom_error
geom_init_func_const_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_init_func_const_data *pp;

  pp = (struct geom_init_func_const_data *)p;

  /*
   * Clear geom_error with GEOM_SUCCESS.
   */
  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  /*
   * Set up the variant with the value.
   */
  geom_variant_set_double(v, pp->cval);

  /*
   * Give variant with description.
   *
   * Use 'I' for unit of initialization target (such as K for
   * temperature, and m/s for velocity) and 'L' for length (such as m)
   */
  geom_info_map_append(list, v, "Constant value", "I", &e);

  /*
   * Release the variant data.
   */
  geom_variant_delete(v);

  return e;
}

static void *
geom_init_func_const_copy(void *p)
{
  /*
   * This function description is basic copy.
   *
   * You are allowed to make a shallow copy of the data. There is one
   * restriction that is the returning pointer must not equal to `p`.
   */
  struct geom_init_func_const_data *np, *ori;

  /*
   * Again, `p` is guaranteed to be available.
   */
  ori = (struct geom_init_func_const_data *)p;

  /*
   * To reduce code, using allocator function is one of choices.
   */
  np = (struct geom_init_func_const_data *)geom_init_func_const_allocator();

  /*
   * If allocation fails, return `NULL`
   */
  if (!np) return NULL;

  /*
   * Copy data.
   */
  np->cval = ori->cval;

  return np;
}

static
geom_init_funcs geom_init_func_const = {
  .enum_val = GEOM_INIT_FUNC_CONST,
  .c = {
    .allocator = geom_init_func_const_allocator,
    .deallocator = geom_init_func_const_deallocator,
    .set_value = geom_init_func_const_set_value,
    .get_value = geom_init_func_const_get_value,
    .n_params = geom_init_func_const_get_n_params,
    .args_next = geom_init_func_const_args_next,
    .args_check = geom_init_func_const_args_check,
    .infomap_gen = geom_init_func_const_info_map,
    .copy = geom_init_func_const_copy,
  },
  .func = geom_init_func_const_initf,
};

geom_error geom_install_init_func_const(void)
{
  return geom_install_init_func(&geom_init_func_const);
}
