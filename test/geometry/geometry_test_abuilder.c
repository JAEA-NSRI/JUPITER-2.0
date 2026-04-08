
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "geometry_test.h"

#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/error.h>
#include <jupiter/geometry/abuilder.h>
#include <jupiter/geometry/abuilder-priv.h>
#include <jupiter/geometry/variant.h>
#include <jupiter/geometry/global.h>

static geom_variant_type abuilder_test_types[] = {
  GEOM_VARTYPE_DOUBLE,
  GEOM_VARTYPE_DOUBLE,
  GEOM_VARTYPE_STRING,
  GEOM_VARTYPE_INT,
};

static int abuilder_test_optional[] = {
  0, 0, 1, 0,
};

static const char *
abuilder_test_description[] = {
  "Arg 1", NULL, "Text Argument", "Int argument",
};

static geom_variant_type
abuilder_test_next(geom_args_builder *b, geom_variant *p, int *optional)
{
  geom_size_type l = geom_args_builder_get_loc(b);

  if (l < 0 ||
      (size_t)l >= sizeof(abuilder_test_types) / sizeof(geom_variant_type)) {
    return GEOM_VARTYPE_NULL;
  }
  if (abuilder_test_description[l]) {
    geom_variant_set_string(p, abuilder_test_description[l], 0);
  } else {
    geom_variant_nullify(p);
  }

  *optional = abuilder_test_optional[l];
  return abuilder_test_types[l];
}

static
void abuilder_test_print_list(geom_args_builder *b)
{
  geom_error e;
  geom_size_type l;
  geom_variant_list *lp, *cp;
  const geom_variant *cv;
  char *strp;

  lp = geom_args_builder_get_list(b);
  if (lp) {
    cp = lp;
    l = 0;
    do {
      cv = geom_variant_list_get(cp);
      e = geom_variant_to_string(&strp, cv);
      if (e != GEOM_ERR_NOMEM) {
        if (e == GEOM_SUCCESS) {
          fprintf(stderr, "%4" PRIdMAX ". %s\n", (intmax_t)l, strp);
        } else {
          fprintf(stderr, "%4" PRIdMAX ". (Error: %s)\n",
                  (intmax_t)l, geom_strerror(e));
        }
        free(strp);
      } else {
        fprintf(stderr, "%4" PRIdMAX ". (Error: %s)\n",
                (intmax_t)l, geom_strerror(e));
      }
      l++;
      cp = geom_variant_list_next(cp);
    } while (cp != lp);
  } else {
    fprintf(stderr, "* (list is empty)\n");
  }
}

static
geom_error abuilder_test_check(void *n, geom_args_builder *b,
                               geom_size_type index, const geom_variant *p,
                               geom_variant *errinfo)
{
  switch(index) {
  case 0:
    return GEOM_SUCCESS; /* accept all values */
  case 1:
    {
      double t;
      t = geom_variant_get_double(p, NULL);
      if (t >= 0.0 && t <= 1.0) return GEOM_SUCCESS;
      geom_warn("Value must be 0 <= v <= 1: %g", t);
      if (errinfo) {
        geom_variant_set_string(errinfo, "Value must be 0 <= v <= 1", 0);
      }
    }
    return GEOM_ERR_RANGE;
  case 2:
    {
      const char *str;
      str = geom_variant_get_string(p, NULL);
      if (str) {
        if (strncmp(str, "geom_", 5) != 0) return GEOM_ERR_RANGE;
        return GEOM_SUCCESS;
      }
    }
    return GEOM_ERR_RANGE;
  default:
    return GEOM_ERR_VARIANT_TYPE;
  }
}

int abuilder_test(void)
{
  geom_variant_type t;
  geom_error e;
  geom_args_builder *builder;
  geom_variant *v;
  geom_variant *emsg;
  const geom_variant *descp;
  geom_size_type l;
  int ecnt;

  emsg = NULL;
  descp = NULL;

  builder = geom_args_builder_new(abuilder_test_next, abuilder_test_check, &e);
  if (test_compare_f(e, GEOM_SUCCESS, "Argument builder creation failed")) {
    return 1;
  }
  ecnt = 0;

  v = geom_variant_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS, "Variant value creation failed")) {
    ecnt = 1;
    goto error;
  }

  emsg = geom_variant_new(&e);
  if (test_compare_f(e, GEOM_SUCCESS, "Error info value creation failed")) {
    ecnt = 1;
  }

  while ((t = geom_args_builder_next(builder)) != GEOM_VARTYPE_NULL) {
    descp = geom_args_builder_get_description(builder);
    switch(t) {
    case GEOM_VARTYPE_DOUBLE:
      geom_variant_set_double(v, 99.9);
      break;
    case GEOM_VARTYPE_STRING:
      geom_variant_set_string(v, "geom_string", 0);
      break;
    default:
      geom_variant_nullify(v);
      break;
    }
    e = geom_args_builder_check(builder, v, emsg);
    l = geom_args_builder_get_loc(builder);
    switch(l) {
    case 0:
      if (test_compare(geom_args_builder_is_optional(builder), 0)) ecnt++;
      if (test_compare(e, GEOM_SUCCESS)) ecnt++;
      break;
    case 1:
      if (test_compare(geom_args_builder_is_optional(builder), 0)) ecnt++;
      if (test_compare(e, GEOM_ERR_RANGE)) ecnt++;
      if (emsg) {
        if (test_compare(strcmp(geom_variant_get_string(emsg, &e),
                                "Value must be 0 <= v <= 1"), 0)) {
          ecnt++;
        }
        if (test_compare(e, GEOM_SUCCESS)) ecnt++;
      }
      break;
    case 2:
      if (test_compare(geom_args_builder_is_optional(builder), 1)) ecnt++;
      if (test_compare(e, GEOM_SUCCESS)) ecnt++;
      break;
    case 3:
      if (test_compare(geom_args_builder_is_optional(builder), 0)) ecnt++;
      if (test_compare(e, GEOM_ERR_VARIANT_TYPE)) ecnt++;
      break;
    default:
      if (test_compare(geom_args_builder_is_optional(builder), 0)) ecnt++;
      if (test_compare(e, GEOM_ERR_RANGE)) ecnt++;
      break;
    }
    if (descp) {
      const char *descs;
      descs = geom_variant_get_string(descp, NULL);
      fprintf(stderr, "What: %s\n", descs);
      if (descs) {
        if (test_compare(strcmp(descs, abuilder_test_description[l]),
                         0)) {
          ecnt++;
        }
      } else {
        if (test_compare(descs, abuilder_test_description[l])) {
          ecnt++;
        }
      }
    }
    e = geom_args_builder_set_value(builder, v);
    if (test_compare(e, GEOM_SUCCESS)) ecnt++;

    fprintf(stderr, "---- icnt %td\n", l);
    abuilder_test_print_list(builder);
  }

  e = GEOM_SUCCESS;

 error:
  if (e != GEOM_SUCCESS) {
    ecnt = 1;
  }
  geom_variant_delete(v);
  geom_variant_delete(emsg);
  geom_args_builder_free(builder);
  return ecnt;
}
