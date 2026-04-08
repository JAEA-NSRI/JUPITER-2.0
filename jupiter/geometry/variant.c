
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "defs.h"
#include "variant.h"
#include "enumutil.h"
#include "list.h"
#include "error.h"
#include "geom_assert.h"
#include "common.h"
#include "vector.h"
#include "svector.h"
#include "matrix.h"
#include "quat.h"

struct geom_variant_ptr_data
{
  void *p;
  geom_variant_ext_deallocator *dealloc;
  size_t ref;
};

union geom_variant_value
{
  char ch;
  unsigned char uch;
  double dble;
  int i;
  long int li;
  geom_size_type sz;
  intmax_t mxi;
  geom_svec3 sv3;
  geom_vec2 v2;
  geom_vec3 v3;
  geom_vec4 v4;
  geom_quat qu;
  geom_mat22 m22;
  geom_mat33 m33;
  geom_mat43 m43;
  struct geom_variant_ptr_data *ext;
  char fstr[1];
};

enum {
  GEOM_SHORT_STRING_MAX_SIZE =
  sizeof(union geom_variant_value) - offsetof(union geom_variant_value, fstr)
};

struct geom_variant
{
  int type;
  union geom_variant_value value;
};

struct geom_variant_list
{
  struct geom_list list;
  struct geom_variant data;
};

#define geom_variant_list_entry(ptr) \
  geom_list_entry(ptr, struct geom_variant_list, list)

int geom_variant_is_exttype(int t)
{
  switch(t) {
  case GEOM_VARTYPE_STRING:
  case GEOM_VARTYPE_INFO_MAP:
    return 1;
  default:
    if (t >= GEOM_VARTYPE_EXTTYPE_MIN &&
        t <= GEOM_VARTYPE_EXTTYPE_MAX) return 1;
    break;
  }
  return 0;
}

int geom_variant_has_exttype(const geom_variant *v)
{
  GEOM_ASSERT(v);
  return geom_variant_is_exttype(v->type);
}

int geom_variant_is_enumtype(int t)
{
  switch(t) {
  case GEOM_VARTYPE_DATA_OPERATOR:
  case GEOM_VARTYPE_SHAPE_OPERATOR:
  case GEOM_VARTYPE_SHAPE:
  case GEOM_VARTYPE_PHASE:
  case GEOM_VARTYPE_INIT_FUNC:
  case GEOM_VARTYPE_ERROR:
  case GEOM_VARTYPE_SURFACE_SHAPE:
    return 1;
  default:
    if (t >= GEOM_VARTYPE_ENUMTYPE_MIN &&
        t <= GEOM_VARTYPE_ENUMTYPE_MAX) return 1;
    break;
  }
  return 0;
}

int geom_variant_has_enumtype(const geom_variant *v)
{
  GEOM_ASSERT(v);
  return geom_variant_is_enumtype(v->type);
}

static
geom_error geom_variant_init(geom_variant *v)
{
  v->type = GEOM_VARTYPE_NULL;
  return GEOM_SUCCESS;
}

geom_variant *geom_variant_new(geom_error *e)
{
  geom_variant *m;
  geom_error ee;
  m = (geom_variant *)malloc(sizeof(geom_variant));
  if (!m) {
    if (e) {
      *e = GEOM_ERR_NOMEM;
    }
    return NULL;
  }
  ee = geom_variant_init(m);
  if (e) *e = ee;
  return m;
}

static
void geom_variant_ext_delink(geom_variant *v)
{
  GEOM_ASSERT(v && geom_variant_has_exttype(v));

#ifdef JUPITER_GEOMETRY_USE_OPENMP
#pragma omp critical (geom_variant_shared_ptr)
#endif
  {
    struct geom_variant_ptr_data *p;
    p = v->value.ext;
    if (p) {
      size_t refc;
      refc = p->ref;
      refc--;
      if (refc == 0) {
        if (p->dealloc) {
          p->dealloc(p->p);
        }
        free(p);
      } else {
        p->ref = refc;
      }
    }
  }
}

void geom_variant_delete(geom_variant *v)
{
  if (v) {
    geom_variant_nullify(v);
    free(v);
  }
}

void geom_variant_copy(geom_variant *to, const geom_variant *from)
{
  GEOM_ASSERT(from);
  GEOM_ASSERT(to);

  geom_variant_nullify(to);

#ifdef JUPITER_GEOMETRY_USE_OPENMP
#pragma omp critical (geom_variant_shared_ptr)
#endif
  {
    to->type = from->type;
    if (geom_variant_has_exttype(from)) {
      struct geom_variant_ptr_data *p;
      p = from->value.ext;
      if (p) {
        p->ref += 1;
      }
    }
    to->value = from->value;
  }
}

geom_variant *geom_variant_dup(const geom_variant *from, geom_error *e)
{
  geom_variant *t;

  GEOM_ASSERT(from);

  t = geom_variant_new(e);
  if (!t) return NULL;

  geom_variant_copy(t, from);
  return t;
}

void geom_variant_nullify(geom_variant *v)
{
  GEOM_ASSERT(v);

  if (geom_variant_has_exttype(v)) {
    geom_variant_ext_delink(v);
  }
  v->type = GEOM_VARTYPE_NULL;
}

int geom_variant_get_type(const geom_variant *v)
{
  GEOM_ASSERT(v);
  return v->type;
}

int geom_variant_is_null(const geom_variant *v)
{
  return (geom_variant_get_type(v) == GEOM_VARTYPE_NULL);
}

static
geom_error geom_variant_set(geom_variant *v, int type, ptrdiff_t offs,
                            void *buf, size_t m)
{
  geom_variant_nullify(v);
  v->type = type;
  memcpy((char *)(&v->value) + offs, buf, m);
  return GEOM_SUCCESS;
}

geom_error geom_variant_set_enum(geom_variant *v, int type, int val)
{
  GEOM_ASSERT(v);
  GEOM_ASSERT(geom_variant_is_enumtype(type));

  return geom_variant_set(v, type, offsetof(union geom_variant_value, i),
                          &val, sizeof(val));
}

geom_error geom_variant_set_ext(geom_variant *v, int type, void *p,
                                geom_variant_ext_deallocator *dealloc)
{
  geom_error e;
  struct geom_variant_ptr_data *ptr;

  GEOM_ASSERT(v);
  GEOM_ASSERT(geom_variant_is_exttype(type));

  ptr = (struct geom_variant_ptr_data *)
    malloc(sizeof(struct geom_variant_ptr_data));
  if (!ptr) return GEOM_ERR_NOMEM;

  ptr->p = p;
  ptr->dealloc = dealloc;
  ptr->ref = 1;

  e = geom_variant_set(v, type, offsetof(union geom_variant_value, ext),
                       &ptr, sizeof(struct geom_variant_ptr_data *));
  if (e != GEOM_SUCCESS) {
    free(ptr);
  }
  return e;
}

geom_error geom_variant_set_char(geom_variant *v, char ch)
{
  return geom_variant_set(v, GEOM_VARTYPE_CHAR,
                          offsetof(union geom_variant_value, ch),
                          &ch, sizeof(ch));
}

geom_error geom_variant_set_uchar(geom_variant *v, unsigned char uch)
{
  return geom_variant_set(v, GEOM_VARTYPE_UCHAR,
                          offsetof(union geom_variant_value, uch),
                          &uch, sizeof(uch));
}

geom_error geom_variant_set_int(geom_variant *v, int ival)
{
  return geom_variant_set(v, GEOM_VARTYPE_INT,
                          offsetof(union geom_variant_value, i),
                          &ival, sizeof(ival));
}

geom_error geom_variant_set_long_int(geom_variant *v, long int lval)
{
  return geom_variant_set(v, GEOM_VARTYPE_LONG_INT,
                          offsetof(union geom_variant_value, li),
                          &lval, sizeof(lval));
}

geom_error geom_variant_set_double(geom_variant *v, double dval)
{
  return geom_variant_set(v, GEOM_VARTYPE_DOUBLE,
                          offsetof(union geom_variant_value, dble),
                          &dval, sizeof(dval));
}

geom_error geom_variant_set_size_value(geom_variant *v, geom_size_type sval)
{
  return geom_variant_set(v, GEOM_VARTYPE_SIZE,
                          offsetof(union geom_variant_value, sz),
                          &sval, sizeof(sval));
}

geom_error geom_variant_set_vec2(geom_variant *v, geom_vec2 vec)
{
  return geom_variant_set(v, GEOM_VARTYPE_VECTOR2,
                          offsetof(union geom_variant_value, v2),
                          &vec, sizeof(vec));
}

geom_error geom_variant_set_vec3(geom_variant *v, geom_vec3 vec)
{
  return geom_variant_set(v, GEOM_VARTYPE_VECTOR3,
                          offsetof(union geom_variant_value, v3),
                          &vec, sizeof(vec));
}

geom_error geom_variant_set_vec4(geom_variant *v, geom_vec4 vec)
{
  return geom_variant_set(v, GEOM_VARTYPE_VECTOR4,
                          offsetof(union geom_variant_value, v4),
                          &vec, sizeof(vec));
}

geom_error geom_variant_set_svec3(geom_variant *v, geom_svec3 vec)
{
  return geom_variant_set(v, GEOM_VARTYPE_SIZE_VECTOR3,
                          offsetof(union geom_variant_value, sv3),
                          &vec, sizeof(vec));
}

geom_error geom_variant_set_quat(geom_variant *v, geom_quat qu)
{
  return geom_variant_set(v, GEOM_VARTYPE_QUATERNION,
                          offsetof(union geom_variant_value, qu),
                          &qu, sizeof(qu));
}

geom_error geom_variant_set_mat22(geom_variant *v, geom_mat22 m22)
{
  return geom_variant_set(v, GEOM_VARTYPE_MATRIX22,
                          offsetof(union geom_variant_value, m22),
                          &m22, sizeof(m22));
}

geom_error geom_variant_set_mat33(geom_variant *v, geom_mat33 m33)
{
  return geom_variant_set(v, GEOM_VARTYPE_MATRIX33,
                          offsetof(union geom_variant_value, m33),
                          &m33, sizeof(m33));
}

geom_error geom_variant_set_mat43(geom_variant *v, geom_mat43 m43)
{
  return geom_variant_set(v, GEOM_VARTYPE_MATRIX43,
                          offsetof(union geom_variant_value, m43),
                          &m43, sizeof(m43));
}

geom_error geom_variant_set_string(geom_variant *v, const char *str, size_t n)
{
  if (n == 0) {
    n = strlen(str);
  }
  if (n + 1 < GEOM_SHORT_STRING_MAX_SIZE) {
    char tmp[GEOM_SHORT_STRING_MAX_SIZE];

    strncpy(tmp, str, n);
    for (; n < GEOM_SHORT_STRING_MAX_SIZE; ++n) {
      tmp[n] = '\0';
    }

    return geom_variant_set(v, GEOM_VARTYPE_STRING_SHORT,
                            offsetof(union geom_variant_value, fstr),
                            tmp, GEOM_SHORT_STRING_MAX_SIZE);
  } else {
    geom_error e;
    char *buf;

    buf = (char *)malloc(sizeof(char) * (n + 1));
    if (!buf) return GEOM_ERR_NOMEM;

    strncpy(buf, str, n);
    buf[n] = '\0';

    e = geom_variant_set_ext(v, GEOM_VARTYPE_STRING, buf, free);
    if (e != GEOM_SUCCESS) {
      free(buf);
    }
    return e;
  }
}

geom_error geom_variant_set_data_op(geom_variant *v, geom_data_operator op)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_DATA_OPERATOR, op);
}

geom_error geom_variant_set_shape_op(geom_variant *v, geom_shape_operator op)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_SHAPE_OPERATOR, op);
}

geom_error geom_variant_set_init_func(geom_variant *v, geom_init_func f)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_INIT_FUNC, f);
}

geom_error geom_variant_set_shape(geom_variant *v, geom_shape s)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_SHAPE, s);
}

geom_error geom_variant_set_phase(geom_variant *v, geom_vof_phase vf)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_PHASE, vf);
}

geom_error geom_variant_set_error(geom_variant *v, geom_error error)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_ERROR, error);
}

geom_error geom_variant_set_surface_shape(geom_variant *v,
                                          geom_surface_shape s)
{
  return geom_variant_set_enum(v, GEOM_VARTYPE_SURFACE_SHAPE, s);
}

/**
 * @memberof geom_variant
 * @brief Common code to get value
 * @param v variant to get from
 * @param t requested type to get
 * @param offs offset in `union geom_variant_value`
 * @param loc Location to store the result
 * @param sz size of data
 * @param e if not NULL, set errors
 *
 * If e is not NULL, this function writes GEOM_SUCCESS even if no error.
 *
 * If errors occured (the possible error is only type mismatch,
 * ::GEOM_ERR_VARIANT_TYPE), content of `loc` is unchanged.
 *
 * For extended type, use geom_variant_get_ext().
 */
static void
geom_variant_get(const geom_variant *v, int type,
                 ptrdiff_t offs, void *loc, size_t sz, geom_error *e)
{
  GEOM_ASSERT(v);
  GEOM_ASSERT(loc);

  if (v->type != type) {
    if (e) *e = GEOM_ERR_VARIANT_TYPE;
    return;
  }
  memcpy(loc, (const char *)(&v->value) + offs, sz);
  if (e) {
    *e = GEOM_SUCCESS;
  }
}

int geom_variant_get_enum(const geom_variant *v, int type, int invalid_val,
                          geom_error *e)
{
  int ival;
  ival = invalid_val;

  geom_variant_get(v, type, offsetof(union geom_variant_value, i),
                   &ival, sizeof(int), e);
  return ival;
}

void *geom_variant_get_ext(const geom_variant *v, int type, geom_error *e)
{
  GEOM_ASSERT(v);

  if (v->type != type) {
    if (e) *e = GEOM_ERR_VARIANT_TYPE;
    return NULL;
  }
  if (v->value.ext) {
    if (e) *e = GEOM_SUCCESS;
    return v->value.ext->p;
  } else {
    if (e) *e = GEOM_SUCCESS;
    return NULL;
  }
}

const char *geom_variant_get_string(const geom_variant *v, geom_error *err)
{
  geom_error e;
  const char *r;
  geom_variant_type t;

  GEOM_ASSERT(v);

  r = NULL;
  t = geom_variant_get_type(v);
  if (t == GEOM_VARTYPE_STRING) {
    r = (char *)geom_variant_get_ext(v, GEOM_VARTYPE_STRING, &e);
  } else if (t == GEOM_VARTYPE_STRING_SHORT) {
    r = v->value.fstr;
    e = GEOM_SUCCESS;
  } else {
    e = GEOM_ERR_VARIANT_TYPE;
  }
  if (err) *err = e;
  return r;
}

char geom_variant_get_char(const geom_variant *v, geom_error *err)
{
  char ch;

  ch = 0;
  geom_variant_get(v, GEOM_VARTYPE_CHAR,
                   offsetof(union geom_variant_value, ch),
                   &ch, sizeof(char), err);
  return ch;
}

unsigned char geom_variant_get_uchar(const geom_variant *v, geom_error *err)
{
  unsigned char uch;
  uch = 0;
  geom_variant_get(v, GEOM_VARTYPE_UCHAR,
                   offsetof(union geom_variant_value, uch),
                   &uch, sizeof(unsigned char), err);
  return uch;
}

int geom_variant_get_int(const geom_variant *v, geom_error *err)
{
  geom_error e;
  int ival;

  ival = 0;
  geom_variant_get(v, GEOM_VARTYPE_INT,
                   offsetof(union geom_variant_value, i),
                   &ival, sizeof(int), &e);
  if (e == GEOM_ERR_VARIANT_TYPE) {
    switch(geom_variant_get_type(v)) {
    case GEOM_VARTYPE_CHAR:
      ival = geom_variant_get_char(v, &e);
      break;
    case GEOM_VARTYPE_UCHAR:
      ival = geom_variant_get_uchar(v, &e);
      break;
    default:
      /* NOP */
      break;
    }
  }
  if (err) *err = e;
  return ival;
}

long int geom_variant_get_long_int(const geom_variant *v, geom_error *err)
{
  geom_error e;
  long int ival;
  ival = 0;
  geom_variant_get(v, GEOM_VARTYPE_LONG_INT,
                   offsetof(union geom_variant_value, li),
                   &ival, sizeof(long int), &e);
  if (e == GEOM_ERR_VARIANT_TYPE) {
    switch(geom_variant_get_type(v)) {
    case GEOM_VARTYPE_CHAR:
    case GEOM_VARTYPE_UCHAR:
    case GEOM_VARTYPE_INT:
      ival = geom_variant_get_int(v, &e);
      break;
    default:
      /* NOP */
      break;
    }
  }
  if (err) *err = e;
  return ival;
}

geom_size_type geom_variant_get_size_value(const geom_variant *v, geom_error *err)
{
  geom_size_type sz;
  sz = 0;
  geom_variant_get(v, GEOM_VARTYPE_SIZE,
                   offsetof(union geom_variant_value, sz),
                   &sz, sizeof(geom_size_type), err);
  return sz;
}

double geom_variant_get_double(const geom_variant *v, geom_error *err)
{
  double dbl;
  dbl = 0.0;
  geom_variant_get(v, GEOM_VARTYPE_DOUBLE,
                   offsetof(union geom_variant_value, dble),
                   &dbl, sizeof(dbl), err);
  return dbl;
}

geom_vec2 geom_variant_get_vec2(const geom_variant *v, geom_error *err)
{
  geom_vec2 v2;
  v2 = geom_vec2_c(0.0, 0.0);
  geom_variant_get(v, GEOM_VARTYPE_VECTOR2,
                   offsetof(union geom_variant_value, v2),
                   &v2, sizeof(v2), err);
  return v2;
}

geom_vec3 geom_variant_get_vec3(const geom_variant *v, geom_error *err)
{
  geom_vec3 v3;
  v3 = geom_vec3_c(0.0, 0.0, 0.0);
  geom_variant_get(v, GEOM_VARTYPE_VECTOR3,
                   offsetof(union geom_variant_value, v3),
                   &v3, sizeof(v3), err);
  return v3;
}

geom_vec4 geom_variant_get_vec4(const geom_variant *v, geom_error *err)
{
  geom_vec4 v4;
  v4 = geom_vec4_c(0.0, 0.0, 0.0, 0.0);
  geom_variant_get(v, GEOM_VARTYPE_VECTOR4,
                   offsetof(union geom_variant_value, v4),
                   &v4, sizeof(v4), err);
  return v4;
}

geom_svec3 geom_variant_get_svec3(const geom_variant *v, geom_error *err)
{
  geom_svec3 sv3;
  sv3 = geom_svec3_c(0, 0, 0);
  geom_variant_get(v, GEOM_VARTYPE_SIZE_VECTOR3,
                   offsetof(union geom_variant_value, sv3),
                   &sv3, sizeof(sv3), err);
  return sv3;
}

geom_quat geom_variant_get_quat(const geom_variant *v, geom_error *err)
{
  geom_quat qu;
  qu = geom_quat_c(0.0, 0.0, 0.0, 0.0);
  geom_variant_get(v, GEOM_VARTYPE_QUATERNION,
                   offsetof(union geom_variant_value, qu),
                   &qu, sizeof(qu), err);
  return qu;
}

geom_mat22 geom_variant_get_mat22(const geom_variant *v, geom_error *err)
{
  geom_mat22 m22;
  m22 = geom_mat22_E();
  geom_variant_get(v, GEOM_VARTYPE_MATRIX22,
                   offsetof(union geom_variant_value, m22),
                   &m22, sizeof(m22), err);
  return m22;
}

geom_mat33 geom_variant_get_mat33(const geom_variant *v, geom_error *err)
{
  geom_mat33 m33;
  m33 = geom_mat33_E();
  geom_variant_get(v, GEOM_VARTYPE_MATRIX33,
                   offsetof(union geom_variant_value, m33),
                   &m33, sizeof(m33), err);
  return m33;
}

geom_mat43 geom_variant_get_mat43(const geom_variant *v, geom_error *err)
{
  geom_mat43 m43;
  m43 = geom_mat43_E();
  geom_variant_get(v, GEOM_VARTYPE_MATRIX43,
                   offsetof(union geom_variant_value, m43),
                   &m43, sizeof(m43), err);
  return m43;
}

geom_data_operator
geom_variant_get_data_op(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_DATA_OPERATOR,
                               GEOM_OP_INVALID, err);
}

geom_shape_operator
geom_variant_get_shape_op(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_SHAPE_OPERATOR,
                               GEOM_OP_INVALID, err);
}

geom_init_func
geom_variant_get_init_func(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_INIT_FUNC,
                               GEOM_INIT_FUNC_INVALID, err);
}

geom_vof_phase
geom_variant_get_phase(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_PHASE,
                               GEOM_PHASE_INVALID, err);
}

geom_shape
geom_variant_get_shape(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_SHAPE,
                               GEOM_SHAPE_INVALID, err);
}

geom_error
geom_variant_get_error(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_ERROR,
                               GEOM_SUCCESS, err);
}

geom_surface_shape
geom_variant_get_surface_shape(const geom_variant *v, geom_error *err)
{
  return geom_variant_get_enum(v, GEOM_VARTYPE_SURFACE_SHAPE,
                               GEOM_SURFACE_SHAPE_INVALID, err);
}

static int geom_variant_get_builtin_enum_to_str(
  char **buf, const geom_variant *v, geom_error *e, const char *tname,
  int getfunc(const geom_variant *v, geom_error *e),
  const char *convfunc(int value))
{
  geom_error ex;
  const char *p;
  int value;

  ex = GEOM_SUCCESS;
  value = getfunc(v, &ex);
  if (e)
    *e = ex;
  if (ex != GEOM_SUCCESS)
    return -1;

  p = convfunc(value);
  if (!p)
    return geom_asprintf(buf, "((%s) %d)", tname, value);
  return geom_asprintf(buf, "%s", p);
}

#define ENUM_VAR_TO_TEXT(typename, getfunc, convfunc)                         \
  static const char *geom_variant_##typename##_to_strp(int value)             \
  {                                                                           \
    return convfunc((geom_##typename)value);                                  \
  }                                                                           \
                                                                              \
  static int geom_variant_get_##                                              \
    typename##_i(const geom_variant *v, geom_error *e)                        \
  {                                                                           \
    return (int)getfunc(v, e);                                                \
  }                                                                           \
                                                                              \
  static int geom_variant_##                                                  \
    typename##_to_str(char **buf, const geom_variant *v, geom_error *e)       \
  {                                                                           \
    return geom_variant_get_builtin_enum_to_str(buf, v, e, "geom_" #typename, \
                                                geom_variant_get_##           \
                                                typename##_i,                 \
                                                geom_variant_##               \
                                                typename##_to_strp);          \
  }

ENUM_VAR_TO_TEXT(init_func, geom_variant_get_init_func, geom_init_func_to_str)
ENUM_VAR_TO_TEXT(data_operator, geom_variant_get_data_op,
                 geom_data_operator_to_str)
ENUM_VAR_TO_TEXT(shape_operator, geom_variant_get_shape_op,
                 geom_shape_operator_to_str)
ENUM_VAR_TO_TEXT(vof_phase, geom_variant_get_phase, geom_vof_phase_to_str)
ENUM_VAR_TO_TEXT(shape, geom_variant_get_shape, geom_shape_to_str)
ENUM_VAR_TO_TEXT(surface_shape, geom_variant_get_surface_shape,
                 geom_surface_shape_to_str)

geom_error
geom_variant_to_string(char **buf, const geom_variant *v)
{
  geom_error e;
  int r;
  geom_variant_type t;
  char *mattmp[7];
  int   matrow;

  t = geom_variant_get_type(v);
  switch(t) {
  case GEOM_VARTYPE_CHAR:
    {
      char ch;
      ch = geom_variant_get_char(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%c", ch);
    }
    break;
  case GEOM_VARTYPE_UCHAR:
    {
      unsigned char ch;
      ch = geom_variant_get_uchar(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%c", ch);
    }
    break;
  case GEOM_VARTYPE_INT:
  case GEOM_VARTYPE_LONG_INT:
    {
      long int l;
      l = geom_variant_get_long_int(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%ld", l);
    }
    break;
  case GEOM_VARTYPE_SIZE:
    {
      geom_size_type sz;
      sz = geom_variant_get_size_value(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%" PRIdMAX, (intmax_t)sz);
    }
    break;
  case GEOM_VARTYPE_DOUBLE:
    {
      double d;
      d = geom_variant_get_double(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%g", d);
    }
    break;
  case GEOM_VARTYPE_LIST_HEAD:
    e = GEOM_SUCCESS;
    r = geom_asprintf(buf, "(head)");
    break;
  case GEOM_VARTYPE_NULL:
    e = GEOM_SUCCESS;
    r = geom_asprintf(buf, "(null)");
    break;
  case GEOM_VARTYPE_STRING:
  case GEOM_VARTYPE_STRING_SHORT:
    {
      const char *s;
      s = geom_variant_get_string(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "%s", s);
    }
    break;

  case GEOM_VARTYPE_INIT_FUNC:
    r = geom_variant_init_func_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_DATA_OPERATOR:
    r = geom_variant_data_operator_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_SHAPE_OPERATOR:
    r = geom_variant_shape_operator_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_PHASE:
    r = geom_variant_vof_phase_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_SHAPE:
    r = geom_variant_shape_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_ERROR:
    {
      geom_error errval;
      errval = geom_variant_get_error(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "(error \"%s\")", geom_strerror(errval));
    }
    break;

  case GEOM_VARTYPE_SURFACE_SHAPE:
    r = geom_variant_surface_shape_to_str(buf, v, &e);
    if (e != GEOM_SUCCESS) return e;
    break;

  case GEOM_VARTYPE_MATRIX22:
    {
      geom_mat22 m22;

      m22 = geom_variant_get_mat22(v, &e);
      if (e != GEOM_SUCCESS) return e;
      matrow = 2;
      r = geom_mat22_to_str(mattmp, m22, 0, ",", "[", "]", "%g", -1, -1);
    }
    goto matrix;

  case GEOM_VARTYPE_MATRIX33:
    {
      geom_mat33 m33;

      m33 = geom_variant_get_mat33(v, &e);
      if (e != GEOM_SUCCESS) return e;
      matrow = 3;
      r = geom_mat33_to_str(mattmp, m33, 0, ",", "[", "]", "%g", -1, -1);
    }
    goto matrix;

  case GEOM_VARTYPE_MATRIX43:
    {
      geom_mat43 m43;

      m43 = geom_variant_get_mat43(v, &e);
      if (e != GEOM_SUCCESS) return e;
      matrow = 4;
      r = geom_mat43_to_str(mattmp, m43, 0, ",", "[", "]", "%g", -1, -1);
    }
    goto matrix;

  matrix:
    {
      if (r < 0) break;
      switch(matrow) {
      case 2:
        r = geom_asprintf(buf, "[%s,%s]", mattmp[0], mattmp[1]);
        break;
      case 3:
        r = geom_asprintf(buf, "[%s,%s,%s]",
                          mattmp[0], mattmp[1], mattmp[2]);
        break;
      case 4:
        r = geom_asprintf(buf, "[%s,%s,%s,%s]",
                          mattmp[0], mattmp[1], mattmp[2], mattmp[3]);
        break;
      default:
        GEOM_UNREACHABLE();
      }
      free(mattmp[0]);
    }
    break;

  case GEOM_VARTYPE_VECTOR2:
    {
      geom_vec2 v2;
      v2 = geom_variant_get_vec2(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_vec2_to_str(buf, v2, "%g", -1, -1);
    }
    break;

  case GEOM_VARTYPE_VECTOR3:
    {
      geom_vec3 v3;
      v3 = geom_variant_get_vec3(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_vec3_to_str(buf, v3, "%g", -1, -1);
    }
    break;

  case GEOM_VARTYPE_VECTOR4:
    {
      geom_vec4 v4;
      v4 = geom_variant_get_vec4(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_vec4_to_str(buf, v4, "%g", -1, -1);
    }
    break;

  case GEOM_VARTYPE_QUATERNION:
    {
      geom_quat qu;
      qu = geom_variant_get_quat(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_quat_to_str(buf, qu, "%g", -1, -1);
    }
    break;

  case GEOM_VARTYPE_SIZE_VECTOR3:
    {
      geom_svec3 sv;
      sv = geom_variant_get_svec3(v, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_svec3_to_str(buf, sv, -1, -1);
    }
    break;

  default:
    if (geom_variant_is_exttype(t)) {
      void *p;
      p = geom_variant_get_ext(v, t, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "((extended type %04x) %p)", (int)t, p);
    } else if (geom_variant_is_enumtype(t)) {
      int ev;
      ev = geom_variant_get_enum(v, t, 0, &e);
      if (e != GEOM_SUCCESS) return e;
      r = geom_asprintf(buf, "((enum type %04x) %d)", (int)t, ev);
    } else {
      e = GEOM_SUCCESS;
      r = geom_asprintf(buf, "(unknown type)");
    }
    break;
  }

  /*
   * With this statement, a clever compiler can check whether no path
   * exist that 'e' is left uninitialized, but note that any function
   * call with '&e' will be assumed that `e` is initialized.
   */
  GEOM_ASSERT(e == GEOM_SUCCESS);
  if (r < 0) {
    e = GEOM_ERR_NOMEM;
  }
  return e;
}

geom_variant_list *geom_variant_list_new(geom_error *e)
{
  geom_variant_list *l;
  geom_error er;

  er = GEOM_SUCCESS;
  l = (geom_variant_list *)malloc(sizeof(geom_variant_list));
  if (!l) {
    if (e) {
      *e = GEOM_ERR_NOMEM;
    }
    return NULL;
  }

  geom_variant_init(&l->data);
  l->data.type = GEOM_VARTYPE_LIST_HEAD;

  geom_list_init(&l->list);
  if (e) *e = er;
  return l;
}

static geom_variant_list *
geom_variant_list_new_set(const geom_variant *v, geom_error *e)
{
  geom_variant_list *l;

  l = (geom_variant_list *)malloc(sizeof(geom_variant_list));
  if (!l) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  geom_variant_init(&l->data);
  if (v) {
    geom_variant_list_set(l, v);
  }
  return l;
}

geom_variant_list *
geom_variant_list_insert_next(geom_variant_list *p, const geom_variant *v,
                              geom_error *e)
{
  geom_variant_list *l;

  GEOM_ASSERT(p);

  l = geom_variant_list_new_set(v, e);
  if (!l) return NULL;

  geom_list_insert_next(&p->list, &l->list);
  return l;
}

geom_variant_list *
geom_variant_list_insert_prev(geom_variant_list *p, const geom_variant *v,
                              geom_error *e)
{
  geom_variant_list *l;

  GEOM_ASSERT(p);

  l = geom_variant_list_new_set(v, e);
  if (!l) return NULL;

  geom_list_insert_prev(&p->list, &l->list);
  return l;
}

void geom_variant_list_delete(geom_variant_list *p)
{
  if (p) {
    geom_list_delete(&p->list);
    geom_variant_nullify(&p->data);
  }
  free(p);
}

void geom_variant_list_delete_all(geom_variant_list *p)
{
  geom_variant_list *l;
  struct geom_list *pp, *n, *head;

  if (!p) return;

  head = &p->list;
  geom_list_foreach_safe(pp, n, head) {
    l = geom_variant_list_entry(pp);
    geom_variant_nullify(&l->data);
    free(l);
  }
  free(p);
}

void geom_variant_list_clear(geom_variant_list *p)
{
  geom_variant_list *n;

  GEOM_ASSERT(p);
  GEOM_ASSERT(geom_variant_list_is_head(p));

  n = geom_variant_list_next(p);
  /* already empty list */
  if (n == p) return;

  /* Remove head from the list. */
  geom_list_delete(&p->list);
  geom_variant_list_delete_all(n);
}

int geom_variant_list_is_empty(geom_variant_list *p)
{
  GEOM_ASSERT(p);

  if (geom_variant_list_is_head(p)) {
    return geom_list_empty(&p->list);
  }
  return 0;
}

int geom_variant_list_is_head(geom_variant_list *p)
{
  GEOM_ASSERT(p);

  return p->data.type == GEOM_VARTYPE_LIST_HEAD;
}

geom_variant_list *geom_variant_list_next(geom_variant_list *p)
{
  struct geom_list *l;
  GEOM_ASSERT(p);

  l = geom_list_next(&p->list);
  if (l) {
    return geom_variant_list_entry(l);
  } else {
    return NULL;
  }
}

geom_variant_list *geom_variant_list_prev(geom_variant_list *p)
{
  struct geom_list *l;
  GEOM_ASSERT(p);

  l = geom_list_prev(&p->list);
  if (l) {
    return geom_variant_list_entry(l);
  } else {
    return NULL;
  }
}

const geom_variant *geom_variant_list_get(geom_variant_list *p)
{
  GEOM_ASSERT(p);

  return &p->data;
}

geom_error geom_variant_list_get_copy(geom_variant *dest, geom_variant_list *p)
{
  GEOM_ASSERT(dest);
  GEOM_ASSERT(p);

  if (geom_variant_list_is_head(p)) return GEOM_ERR_LIST_HEAD;

  geom_variant_copy(dest, geom_variant_list_get(p));
  return GEOM_SUCCESS;
}

geom_error geom_variant_list_set(geom_variant_list *p, const geom_variant *v)
{
  GEOM_ASSERT(p);
  GEOM_ASSERT(v);

  if (geom_variant_list_is_head(p)) return GEOM_ERR_LIST_HEAD;

  geom_variant_copy(&p->data, v);
  return GEOM_SUCCESS;
}

geom_variant_list *geom_variant_list_copy(geom_variant_list *p, geom_error *e)
{
  geom_variant_list *nhead;
  geom_variant_list *ncur, *ocur;
  const geom_variant *cp;
  struct geom_list *ohead, *op;

  GEOM_ASSERT(p);

  nhead = geom_variant_list_new(e);
  if (!nhead) return NULL;

  if (!geom_variant_list_is_head(p)) {
    while (!geom_variant_list_is_head(p)) {
      p = geom_variant_list_prev(p);
    }
  }

  ohead = &p->list;

  geom_list_foreach(op, ohead) {
    ocur = geom_variant_list_entry(op);
    cp = geom_variant_list_get(ocur);
    ncur = geom_variant_list_insert_prev(nhead, cp, e);
    if (!ncur) {
      geom_variant_list_delete_all(nhead);
      return NULL;
    }
  }
  if (e) *e = GEOM_SUCCESS;
  return nhead;
}
