
#include <stdlib.h>

#include "defs.h"
#include "func_defs.h"
#include "variant.h"
#include "geom_assert.h"

#include "abuilder.h"
#include "abuilder-priv.h"

struct geom_args_builder
{
  geom_args_nextf *next;
  geom_args_checkf *checker;
  geom_size_type icnt;
  geom_variant *cur_param_desc;
  geom_variant_type required_type;
  int optional;
  geom_variant_list *list;
  geom_variant_list *curs;
  geom_size_type ccnt;
};

geom_args_builder *
geom_args_builder_new(geom_args_nextf *nextf,
                      geom_args_checkf *checker, geom_error *e)
{
  geom_args_builder *b;

  GEOM_ASSERT(nextf);

  b = (geom_args_builder *)malloc(sizeof(geom_args_builder));
  if (!b) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  b->list = geom_variant_list_new(e);
  if (!b->list) {
    geom_args_builder_free(b);
    return NULL;
  }

  b->cur_param_desc = geom_variant_new(e);
  if (!b->cur_param_desc) {
    geom_args_builder_free(b);
    return NULL;
  }

  b->next = nextf;
  b->checker = checker;
  b->icnt = -1;
  b->required_type = GEOM_VARTYPE_NULL;
  b->curs = NULL;
  b->ccnt = 0;
  if (e) *e = GEOM_SUCCESS;
  return b;
}

geom_variant_type
geom_args_builder_next(geom_args_builder *b)
{
  GEOM_ASSERT(b);
  GEOM_ASSERT(b->next);
  GEOM_ASSERT(b->cur_param_desc);

  ++b->icnt;
  b->optional = 0;
  b->required_type = b->next(b, b->cur_param_desc, &b->optional);

  return b->required_type;
}

geom_variant_type geom_args_builder_get_type(geom_args_builder *b)
{
  GEOM_ASSERT(b);

  if (b->icnt < 0) {
    return GEOM_VARTYPE_NULL;
  }
  return b->required_type;
}

int geom_args_builder_is_optional(geom_args_builder *b)
{
  GEOM_ASSERT(b);

  if (b->icnt < 0) {
    return 0;
  }
  return b->optional;
}

const geom_variant *
geom_args_builder_get_description(geom_args_builder *b)
{
  GEOM_ASSERT(b);

  if (b->icnt < 0) {
    return NULL;
  }
  return b->cur_param_desc;
}

geom_size_type geom_args_builder_get_loc(geom_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->icnt;
}

geom_error
geom_args_builder_check(geom_args_builder *b, const geom_variant *v,
                        geom_variant *errinfo)
{
  geom_variant_type t;

  GEOM_ASSERT(b);
  GEOM_ASSERT(v);

  if (b->icnt < 0) {
    return GEOM_ERR_RANGE;
  }
  t = geom_variant_get_type(v);
  if (t == GEOM_VARTYPE_STRING_SHORT) {
    t = GEOM_VARTYPE_STRING;
  }
  if (t != GEOM_VARTYPE_NULL) {
    switch (b->required_type) {
    case GEOM_VARTYPE_INT_OR_SVEC3:
      if (t != GEOM_VARTYPE_INT && t != GEOM_VARTYPE_SIZE_VECTOR3) {
        return GEOM_ERR_VARIANT_TYPE;
      }
      break;
    default:
      if (t != b->required_type) {
        return GEOM_ERR_VARIANT_TYPE;
      }
      break;
    }
  }
  if (b->checker) {
    return b->checker(NULL, b, geom_args_builder_get_loc(b), v, errinfo);
  }
  return GEOM_SUCCESS;
}

geom_error
geom_args_builder_set_value(geom_args_builder *b, const geom_variant *v)
{
  geom_error e;
  geom_variant_list *bp, *lp;
  geom_size_type bc, ic;

  GEOM_ASSERT(b);
  GEOM_ASSERT(v);

  if (b->icnt < 0) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  bc = b->ccnt;
  ic = b->icnt;
  bp = b->curs;
  lp = b->list;

  if (!bp || bc < ic) {
    if (!bp) {
      bp = geom_variant_list_next(lp);
      bc = 0;
    }
    for (; bp != lp && bc < ic; ++bc) {
      bp = geom_variant_list_next(bp);
    }
    if (bp == lp && bc < ic) {
      for (; bc < ic; ++bc) {
        geom_variant_list_insert_prev(lp, NULL, &e);
        if (e != GEOM_SUCCESS) return e;
      }
    }
  } else {
    GEOM_ASSERT(ic >= 0);
    if (ic == 0) {
      bp = geom_variant_list_next(lp);
    } else {
      if (bc - ic < ic) {
        for (; bc > ic; --bc) {
          bp = geom_variant_list_prev(bp);
        }
      } else {
        bc = 0;
        bp = geom_variant_list_next(lp);
        for (; bc < ic; ++bc) {
          bp = geom_variant_list_next(bp);
        }
      }
    }
  }

  if (bp == lp) {
    bp = geom_variant_list_insert_prev(lp, v, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
  } else {
    e = geom_variant_list_set(bp, v);
  }

  b->ccnt = ic;
  b->list = lp;
  b->curs = bp;
  return e;
}

void geom_args_builder_free(geom_args_builder *b)
{
  if (b) {
    if (b->list) {
      geom_variant_list_delete_all(b->list);
    }
    if (b->cur_param_desc) {
      geom_variant_delete(b->cur_param_desc);
    }
  }
  free(b);
}

geom_variant_list *geom_args_builder_get_list(geom_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->list;
}

const geom_variant *
geom_args_builder_value_at(geom_args_builder *b, geom_size_type l)
{
  geom_variant_list *lp;

  GEOM_ASSERT(b);
  GEOM_ASSERT(l >= 0);

  lp = geom_variant_list_next(b->list);
  for (; l > 0; --l) {
    lp = geom_variant_list_next(lp);
    if (lp == b->list) break;
  }
  if (lp == b->list) return NULL;
  return geom_variant_list_get(lp);
}

geom_error geom_args_builder_vargs(geom_args_builder *bb, geom_variant *einfo,
                                   va_list ap)
{
  geom_error e;
  geom_variant_type t;
  geom_variant *var;
  int i;
  char ch;
  double dbl;
  long int li;
  const char *cp;
  size_t sz;

  GEOM_ASSERT(bb);

  e = GEOM_SUCCESS;

  var = geom_variant_new(&e);
  if (!var) {
    return e;
  }

  bb->icnt = -1;
  while ((t = geom_args_builder_next(bb)) != GEOM_VARTYPE_NULL) {
    switch (t) {
    case GEOM_VARTYPE_INT:
      i = va_arg(ap, int);
      e = geom_variant_set_int(var, i);
      break;
    case GEOM_VARTYPE_CHAR:
      ch = va_arg(ap, int); /* char will be promoted to int */
      e = geom_variant_set_char(var, ch);
      break;
    case GEOM_VARTYPE_DOUBLE:
      dbl = va_arg(ap, double);
      e = geom_variant_set_double(var, dbl);
      break;
    case GEOM_VARTYPE_LONG_INT:
      li = va_arg(ap, long int);
      e = geom_variant_set_long_int(var, li);
      break;
    case GEOM_VARTYPE_STRING:
      cp = va_arg(ap, const char *);
      e = geom_variant_set_string(var, cp, 0);
      break;
    case GEOM_VARTYPE_SIZE:
      sz = va_arg(ap, size_t);
      e = geom_variant_set_size_value(var, sz);
      break;
    default:
      if (geom_variant_is_enumtype(t)) {
        i = va_arg(ap, int);
        e = geom_variant_set_enum(var, t, i);
      } else {
        e = GEOM_ERR_VARIANT_TYPE;
      }
      break;
    }
    if (einfo) {
      geom_variant_set_string(einfo, "Type conversion failed", 0);
    }
    if (e != GEOM_SUCCESS) break;

    e = geom_args_builder_check(bb, var, einfo);
    if (e != GEOM_SUCCESS) break;

    e = geom_args_builder_set_value(bb, var);
    if (e != GEOM_SUCCESS) break;
  }

  geom_variant_delete(var);
  return e;
}

geom_error geom_args_builder_vlist(geom_args_builder *b, geom_variant_list *l,
                                   geom_variant_list **retcur,
                                   geom_variant *einfo)
{
  geom_error e;
  geom_variant_list *cur;
  geom_variant_type t;
  const geom_variant *cv;

  GEOM_ASSERT(b);
  GEOM_ASSERT(l);

  if (!geom_variant_list_is_head(l)) {
    return GEOM_ERR_NOT_LIST_HEAD;
  }

  e = GEOM_SUCCESS;
  cur = geom_variant_list_next(l);

  b->icnt = -1;
  while ((t = geom_args_builder_next(b)) != GEOM_VARTYPE_NULL) {
    if (geom_variant_list_is_head(cur)) {
      e = GEOM_ERR_SHORT_LIST;
      break;
    }
    cv = geom_variant_list_get(cur);
    e = geom_args_builder_check(b, cv, einfo);
    if (e != GEOM_SUCCESS) {
      break;
    }

    e = geom_args_builder_set_value(b, cv);
    if (e != GEOM_SUCCESS) {
      break;
    }

    cur = geom_variant_list_next(cur);
  }
  if (retcur) *retcur = cur;
  return e;
}
