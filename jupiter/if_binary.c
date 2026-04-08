
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include "common_util.h"
#include "geometry/data.h"
#include "geometry/func_defs.h"
#include "geometry/vector.h"
#include "struct.h"
#include "csvutil.h"
#include "csvutil_extra.h"
#include "if_binary.h"
#include "func.h"
#include "os/asprintf.h"

#include "geometry/error.h"
#include "geometry/defs.h"
#include "geometry/global.h"
#include "geometry/func_data.h"
#include "geometry/init.h"
#include "geometry/abuilder.h"
#include "geometry/variant.h"
#include "geometry/infomap.h"
#include "geometry/svector.h"

struct jupiter_init_func_binary_data
{
  char *fname;
  int use_float;
  int unified;
  type *data;
  double value_for_unspecified;
  binary_output_mode outmode;
  geom_svec3 ncells;
  geom_svec3 origin;
  geom_svec3 ncopy;
  geom_svec3 copy_offset;
  domain *cdo;
  int uptodate;
};

enum jupite_init_func_binary_args_loc
{
  ARG_LOC_FNAME = 0,
  ARG_LOC_PRECISION,
  ARG_LOC_OUTMODE,
  ARG_LOC_UNSPEC,
  ARG_LOC_CELLS,
  ARG_LOC_ORIGIN,
  ARG_LOC_NCOPY,
  ARG_LOC_COPY_OFFSET,
};

static int
jupiter_init_func_binary_is_unified(geom_variant_list *args, geom_error *e)
{
  geom_error ee;
  geom_variant_list *lf;
  const char *f;
  int r;

  lf = geom_variant_list_next(args);
  if (lf == args) {
    if (e) *e = GEOM_ERR_DEPENDENCY;
    return -1;
  }

  ee = GEOM_SUCCESS;
  f = geom_variant_get_string(geom_variant_list_get(lf), &ee);
  if (ee != GEOM_SUCCESS) {
    if (e) *e = ee;
    return -1;
  }

  if (strlen(f) <= 0) {
    if (e) *e = GEOM_ERR_RANGE;
    return -1;
  }

  r = format_has_mpi_rank(f);
  if (r < 0) {
    if (e) *e = GEOM_ERR_RANGE;
    return r;
  }
  return !r;
}

static geom_variant_type
jupiter_init_func_binary_args_next(geom_args_builder *b,
                                   geom_variant *description, int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  static const char * descriptions[] = {
    "File Name", "Precision", "Output mode", "Value for unspecified region",
    "Number of cells", "Origin point (in cell)", "Number of copies",
    "Copy offsets (in cell)", "Ovarlap operation method"
  };

  if (description &&
      l >= 0 && (size_t)l < sizeof(descriptions) / sizeof(char *)) {
    geom_variant_set_string(description, descriptions[l], 0);
  }

  switch (l) {
  case ARG_LOC_FNAME:
    return GEOM_VARTYPE_STRING;
  case ARG_LOC_PRECISION:
    return GEOM_VARTYPE_INT;
  case ARG_LOC_OUTMODE:
    *optional = 1;
    return (geom_variant_type)JUPITER_VARTYPE_OUTPUT_MODE;
  case ARG_LOC_UNSPEC:
    *optional = 1;
    return GEOM_VARTYPE_DOUBLE;
  case ARG_LOC_CELLS:
  case ARG_LOC_ORIGIN:
  case ARG_LOC_NCOPY:
  case ARG_LOC_COPY_OFFSET:
    *optional = 1;
    return GEOM_VARTYPE_SIZE_VECTOR3;
  default:
    return GEOM_VARTYPE_NULL;
  }
}

static geom_error
jupiter_init_func_binary_args_check(void *p, geom_args_builder *b,
                                    geom_size_type index, const geom_variant *v,
                                    geom_variant *errinfo)
{
  geom_error e;
  double cval;

  e = GEOM_SUCCESS;

  if (index == ARG_LOC_FNAME) {
    const char *p;
    p = geom_variant_get_string(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (!p || strlen(p) == 0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "File name must not be empty", 0);
      } else {
        geom_warn("File name must not be empty");
      }
      return GEOM_ERR_RANGE;
    }
    return e;
  }
  if (index == ARG_LOC_PRECISION) {
    int n;
    n = geom_variant_get_int(v, &e);
    if (e != GEOM_SUCCESS) return e;
#ifdef JUPITER_DOUBLE
    if (n != 4 && n != 8) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Precision must be 4 or 8", 0);
      } else {
        geom_warn("%d: Precision must be 4 or 8", n);
      }
      return GEOM_ERR_RANGE;
    }
#else
#define SINGLE_WARN " (note: single-precision JUPITER does not support " \
        "reading double-precision binary)"
    if (n != 4) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Precision must be 4" SINGLE_WARN,
                                0);
      } else {
        geom_warn("%d: Precision must be 4" SINGLE_WARN, n);
      }
      return GEOM_ERR_RANGE;
    }
#endif
    return e;
  }
  if (index == ARG_LOC_OUTMODE) {
    binary_output_mode om;
    int uni;

    om = geom_variant_get_enum(v, JUPITER_VARTYPE_OUTPUT_MODE,
                               BINARY_OUTPUT_INVALID, &e);
    if (e != GEOM_SUCCESS) {
      if (!geom_variant_is_null(v))
        return e;
      e = GEOM_SUCCESS;
    }

    uni = 0;
    if (p) {
      struct jupiter_init_func_binary_data *pp;

      pp = (struct jupiter_init_func_binary_data *)p;
      uni = pp->unified;
    } else if (b) {
      geom_variant_list *ll;
      ll = geom_args_builder_get_list(b);
      uni = jupiter_init_func_binary_is_unified(ll, &e);
      if (e != GEOM_SUCCESS) {
        return GEOM_ERR_DEPENDENCY;
      }
    } else {
      return GEOM_ERR_DEPENDENCY;
    }
    if (om == BINARY_OUTPUT_INVALID) {
      if (uni) {
        om = BINARY_OUTPUT_UNIFY_MPI;
      } else {
        om = BINARY_OUTPUT_BYPROCESS;
      }
    }
    if (uni && om == BINARY_OUTPUT_BYPROCESS) {
      const char msg[] =
        "File name indicates unified data. In this case, input mode should "
        "be one of UNIFY (eqv. UNIFY_MPI), UNIFY_MPI or UNIFY_GATHER. "
        "Assumed UNIFY_MPI.";
      if (errinfo) {
        geom_variant_set_string(errinfo, msg, 0);
      } else {
        geom_warn(msg);
      }
    }
    if (!uni && om != BINARY_OUTPUT_BYPROCESS) {
      const char msg[] = "File name indicates rank-splitted data. In this "
                         "case, input mode should be BYPROCESS.";
      if (errinfo) {
        geom_variant_set_string(errinfo, msg, 0);
      } else {
        geom_warn(msg);
      }
    }
    return e;
  }
  if (index == ARG_LOC_UNSPEC) {
    double t;
    t = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) {
      if (!geom_variant_is_null(v)) return e;
      e = GEOM_SUCCESS;
      t = 0.0;
    }
    if (!isfinite(t)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Value must be finite", 0);
      } else {
        geom_warn("%g: Value must be finite", t);
      }
      return GEOM_ERR_RANGE;
    }
    return e;
  }
  if (index == ARG_LOC_CELLS || index == ARG_LOC_ORIGIN ||
      index == ARG_LOC_NCOPY || index == ARG_LOC_COPY_OFFSET) {
    geom_svec3 sv;
    int uni;

    if (p) {
      struct jupiter_init_func_binary_data *pp;
      pp = (struct jupiter_init_func_binary_data *)p;
      uni = pp->unified;
    } else if (b) {
      geom_variant_list *ll;
      ll = geom_args_builder_get_list(b);
      uni = jupiter_init_func_binary_is_unified(ll, &e);
      if (e != GEOM_SUCCESS) return GEOM_ERR_DEPENDENCY;
    } else {
      return GEOM_ERR_DEPENDENCY;
    }

    sv = geom_variant_get_svec3(v, &e);
    if (e != GEOM_SUCCESS) {
      if (!geom_variant_is_null(v)) return e;
      sv = geom_svec3_c(1, 1, 1);
      e = GEOM_SUCCESS;
    } else if (!uni) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Number of cells, origin, repeats cannot be set for splitted-by-rank data", 0);
      } else {
        geom_warn("Number of cells, origin, repeats cannot be set for splitted-by-rank data", 0);
      }
      return GEOM_ERR_RANGE;
    }

#define BE_POSITIVE " must be positive"
#define PSVEC "(%" PRIdMAX ", %" PRIdMAX ", %" PRIdMAX ")"
    if (index == ARG_LOC_CELLS || index == ARG_LOC_NCOPY) {
      geom_size_type sx, sy, sz;
      sx = geom_svec3_x(sv);
      sy = geom_svec3_y(sv);
      sz = geom_svec3_z(sv);
      if (sx <= 0 || sy <= 0 || sz <= 0) {
        if (index == ARG_LOC_CELLS) {
          if (errinfo) {
            geom_variant_set_string(errinfo, "Number of cells" BE_POSITIVE, 0);
          } else {
            geom_warn(PSVEC ": Number of cells" BE_POSITIVE,
                      (intmax_t)sx, (intmax_t)sy, (intmax_t)sz);
          }
        } else {
          if (errinfo) {
            geom_variant_set_string(errinfo, "Number of copies" BE_POSITIVE, 0);
          } else {
            geom_warn(PSVEC ": Number of copies" BE_POSITIVE,
                      (intmax_t)sx, (intmax_t)sy, (intmax_t)sz);
          }
        }
        e = GEOM_ERR_RANGE;
        return e;
      }
    }
    return e;
  }
  return GEOM_ERR_RANGE;
}

static void
jupiter_init_func_binary_init(struct jupiter_init_func_binary_data *d)
{
  d->cdo = NULL;
  d->fname = NULL;
  d->outmode = BINARY_OUTPUT_INVALID;
  d->data = NULL;
  d->unified = 0;
  d->use_float = -1;
  d->value_for_unspecified = 0.0;
  d->ncells = geom_svec3_c(0, 0, 0);
  d->origin = geom_svec3_c(0, 0, 0);
  d->ncopy = geom_svec3_c(1, 1, 1);
  d->copy_offset = geom_svec3_c(0, 0, 0);
  d->uptodate = 0;
}

static geom_error
jupiter_init_func_binary_set_value(void *p, geom_size_type index,
                                   const geom_variant *value)
{
  geom_error ee;
  struct jupiter_init_func_binary_data *pp;

  pp = (struct jupiter_init_func_binary_data *)p;

  if (index == ARG_LOC_FNAME) {
    const char *fn;
    char *fnn;
    int r;
    int unified;

    fn = geom_variant_get_string(value, &ee);
    if (ee != GEOM_SUCCESS) {
      return ee;
    }

    unified = 1;
    r = format_without_mpi_rank(&fnn, fn);
    if (r < 0) {
      r = format_mpi_rank(&fnn, fn);
      unified = 0;
      if (r < 0) {
        return GEOM_ERR_NOMEM;
      }
    }
    if (!pp->fname || strcmp(pp->fname, fnn) != 0) {
      pp->fname = fnn;
      pp->unified = unified;
      pp->uptodate = 0;
      if (unified) {
        if (pp->outmode != BINARY_OUTPUT_UNIFY_GATHER) {
          pp->outmode = BINARY_OUTPUT_UNIFY_MPI;
        }
      } else {
        pp->outmode = BINARY_OUTPUT_BYPROCESS;
      }
    } else {
      free(fnn);
    }
    return GEOM_SUCCESS;
  }
  if (index == ARG_LOC_PRECISION) {
    int prec;
    prec = geom_variant_get_int(value, &ee);
    if (ee != GEOM_SUCCESS) {
      return ee;
    }
    if (pp->use_float != prec) {
      pp->uptodate = 0;
      pp->use_float = prec;
    }
    return GEOM_SUCCESS;
  }
  if (index == ARG_LOC_OUTMODE) {
    binary_output_mode omode;
    omode = geom_variant_get_enum(value, JUPITER_VARTYPE_OUTPUT_MODE,
                                  BINARY_OUTPUT_INVALID, &ee);
    if (ee != GEOM_SUCCESS) {
      if (!geom_variant_is_null(value))
        return ee;
      if (pp->unified) {
        omode = BINARY_OUTPUT_UNIFY_MPI;
      } else {
        omode = BINARY_OUTPUT_BYPROCESS;
      }
    }
    pp->outmode = omode;
    return GEOM_SUCCESS;
  }
  if (index == ARG_LOC_UNSPEC) {
    double val;
    val = geom_variant_get_double(value, &ee);
    if (ee != GEOM_SUCCESS) {
      return ee;
    }
    if (pp->value_for_unspecified != val) {
      pp->value_for_unspecified = val;
      pp->uptodate = 0;
    }
    return GEOM_SUCCESS;
  }
  if (index == ARG_LOC_CELLS || index == ARG_LOC_ORIGIN ||
      index == ARG_LOC_NCOPY || index == ARG_LOC_COPY_OFFSET) {
    geom_svec3 sv;
    geom_svec3 *dest;
    sv = geom_variant_get_svec3(value, &ee);
    if (ee != GEOM_SUCCESS) {
      return ee;
    }
    switch (index) {
    case ARG_LOC_CELLS:
      dest = &pp->ncells;
      break;
    case ARG_LOC_ORIGIN:
      dest = &pp->origin;
      break;
    case ARG_LOC_NCOPY:
      dest = &pp->ncopy;
      break;
    case ARG_LOC_COPY_OFFSET:
      dest = &pp->copy_offset;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    if (!geom_svec3_eql(*dest, sv)) {
      *dest = sv;
      pp->uptodate = 0;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error
jupiter_init_func_binary_get_value(void *p, geom_size_type index,
                                   geom_variant *out_variable)
{
  struct jupiter_init_func_binary_data *pp;

  pp = (struct jupiter_init_func_binary_data *)p;

  switch (index) {
  case ARG_LOC_FNAME:
    if (pp->fname) {
      return geom_variant_set_string(out_variable, pp->fname, 0);
    }
    geom_variant_nullify(out_variable);
    return GEOM_SUCCESS;
  case ARG_LOC_PRECISION:
    return geom_variant_set_int(out_variable, pp->use_float);
  case ARG_LOC_OUTMODE:
    return geom_variant_set_enum(out_variable,
                                 JUPITER_VARTYPE_OUTPUT_MODE, pp->outmode);
  case ARG_LOC_UNSPEC:
    return geom_variant_set_double(out_variable, pp->value_for_unspecified);
  case ARG_LOC_CELLS:
    return geom_variant_set_svec3(out_variable, pp->ncells);
  case ARG_LOC_ORIGIN:
    return geom_variant_set_svec3(out_variable, pp->origin);
  case ARG_LOC_NCOPY:
    return geom_variant_set_svec3(out_variable, pp->ncopy);
  case ARG_LOC_COPY_OFFSET:
    return geom_variant_set_svec3(out_variable, pp->copy_offset);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
jupiter_init_func_binary_n_params(void *p, geom_args_builder *b)
{
  if (!b) {
    return ARG_LOC_COPY_OFFSET + 1;
  }
  if (geom_args_builder_value_at(b, ARG_LOC_UNSPEC)) {
    return ARG_LOC_COPY_OFFSET + 1;
  }
  if (geom_args_builder_value_at(b, ARG_LOC_OUTMODE)) {
    return ARG_LOC_OUTMODE + 1;
  }
  return ARG_LOC_PRECISION + 1;
}


static void *
jupiter_init_func_binary_allocator(void)
{
  struct jupiter_init_func_binary_data *d;
  d = (struct jupiter_init_func_binary_data *)
    malloc(sizeof(struct jupiter_init_func_binary_data));
  if (!d) return NULL;

  jupiter_init_func_binary_init(d);
  return d;
}

static void
jupiter_init_func_binary_deallocator(void *p)
{
  struct jupiter_init_func_binary_data *d;
  d = (struct jupiter_init_func_binary_data *)p;

  free(d->fname);
  free(d->data);
  free(d);
}

static double
jupiter_init_func_binary_initf(void *p, double x, double y, double z, void *a)
{
  struct jupiter_init_func_binary_data *d;
  struct jupiter_init_func_args *aa;
  domain *cdo;
  type *data;
  int jx, jy, jz;
  geom_range3 rng;
  geom_svec3 pnt;
  ptrdiff_t off;

  d = (struct jupiter_init_func_binary_data *)p;
  aa = (struct jupiter_init_func_args *)a;

  CSVASSERT(d->data);
  CSVASSERT(d->cdo);
  CSVASSERT(aa);

  cdo = d->cdo;
  data = d->data;

  rng = geom_range3_c_range(geom_range_c(cdo->stm, cdo->stm + cdo->nx, 0),
                            geom_range_c(cdo->stm, cdo->stm + cdo->ny, 0),
                            geom_range_c(cdo->stm, cdo->stm + cdo->nz, 0));

  jz = aa->cell / cdo->mxy;
  jx = aa->cell % cdo->mxy;
  jy = jx / cdo->mx;
  jx = jx % cdo->mx;
  pnt = geom_svec3_c(jx, jy, jz);

  if (!geom_range3_include(rng, pnt)) {
    return d->value_for_unspecified;
  }

  off = calc_address(jx - cdo->stm, jy - cdo->stm, jz - cdo->stm,
                     cdo->nx, cdo->ny, cdo->nz);
  return data[off];
}

static geom_error
jupiter_init_func_binary_info_map(void *p, geom_info_map *list)
{
  struct jupiter_init_func_binary_data *sf;
  geom_variant *v;
  geom_error e;

  e = GEOM_SUCCESS;
  sf = (struct jupiter_init_func_binary_data *)p;

  v = geom_variant_new(&e);
  if (e != GEOM_SUCCESS) return e;

  if (!sf) {
    geom_variant_nullify(v);
    geom_info_map_append(list, v, "(Data could not be allocated)", "", &e);
  } else {
    e = geom_variant_set_string(v, sf->fname, 0);
    if (e != GEOM_SUCCESS) goto clean;
    geom_info_map_append(list, v, "File name", "", &e);
    if (e != GEOM_SUCCESS) goto clean;

    geom_variant_set_int(v, sf->use_float);
    if (e != GEOM_SUCCESS) goto clean;
    geom_info_map_append(list, v, "Floating point precision for file", "", &e);
    if (e != GEOM_SUCCESS) goto clean;

    if (sf->unified) {
      geom_variant_set_enum(v, JUPITER_VARTYPE_OUTPUT_MODE, sf->outmode);
    } else {
      geom_variant_set_enum(v, JUPITER_VARTYPE_OUTPUT_MODE,
                            BINARY_OUTPUT_BYPROCESS);
    }
    if (e != GEOM_SUCCESS) goto clean;
    geom_info_map_append(list, v, "Read mode", "", &e);
    if (e != GEOM_SUCCESS) goto clean;

    if (sf->unified) {
      int use_cdo_value =
        sf->cdo && geom_svec3_eql(sf->ncells, geom_svec3_c(0, 0, 0));

      e = geom_variant_set_double(v, sf->value_for_unspecified);
      if (e != GEOM_SUCCESS) goto clean;
      geom_info_map_append(list, v, "Value for out-of-range", "I", &e);
      if (e != GEOM_SUCCESS) goto clean;

      if (use_cdo_value) {
        geom_svec3 sv = geom_svec3_c(sf->cdo->gnx, sf->cdo->gny, sf->cdo->gnz);
        e = geom_variant_set_svec3(v, sv);
      } else {
        e = geom_variant_set_svec3(v, sf->ncells);
      }
      if (e != GEOM_SUCCESS) goto clean;
      geom_info_map_append(list, v, "Data size", "cell(s)", &e);
      if (e != GEOM_SUCCESS) goto clean;

      e = geom_variant_set_svec3(v, sf->origin);
      if (e != GEOM_SUCCESS) goto clean;
      geom_info_map_append(list, v, "Origin point", "cell(s)", &e);
      if (e != GEOM_SUCCESS) goto clean;

      e = geom_variant_set_svec3(v, sf->ncopy);
      if (e != GEOM_SUCCESS) goto clean;
      geom_info_map_append(list, v, "Number of repeat", "", &e);
      if (e != GEOM_SUCCESS) goto clean;

      e = geom_variant_set_svec3(v, sf->copy_offset);
      if (e != GEOM_SUCCESS) goto clean;
      geom_info_map_append(list, v, "Repeat offset", "cell(s)", &e);
      if (e != GEOM_SUCCESS) goto clean;
    }
  }

clean:
  geom_variant_delete(v);
  return e;
}

static void *
jupiter_init_func_binary_copy(void *p)
{
  struct jupiter_init_func_binary_data *pp, *np;

  np = (struct jupiter_init_func_binary_data *)
    malloc(sizeof(struct jupiter_init_func_binary_data));
  if (!np) return NULL;

  pp = (struct jupiter_init_func_binary_data *)p;
  *np = *pp;
  if (pp->fname) {
    np->fname = jupiter_strdup(pp->fname);
  }
  np->uptodate = 0;
  np->data = NULL;
  return np;
}

static
geom_init_funcs jupiter_init_func_binary = {
  .enum_val = GEOM_INIT_FUNC_USER,
  .c = {
    .allocator = jupiter_init_func_binary_allocator,
    .deallocator = jupiter_init_func_binary_deallocator,
    .set_value = jupiter_init_func_binary_set_value,
    .get_value = jupiter_init_func_binary_get_value,
    .n_params = jupiter_init_func_binary_n_params,
    .args_next = jupiter_init_func_binary_args_next,
    .args_check = jupiter_init_func_binary_args_check,
    .infomap_gen = jupiter_init_func_binary_info_map,
    .copy = jupiter_init_func_binary_copy,
  },
  .func = jupiter_init_func_binary_initf,
};

geom_error jupiter_install_init_func_binary(void)
{
  return geom_install_init_func(&jupiter_init_func_binary);
}

int jupiter_init_func_binary_data_id(void)
{
  if (jupiter_init_func_binary.enum_val == GEOM_INIT_FUNC_USER) {
    return GEOM_INIT_FUNC_INVALID;
  }
  return jupiter_init_func_binary.enum_val;
}

void jupiter_init_func_binary_set_cdo_to_all_data(geom_data *data, domain *cdo)
{
  geom_data_element *data_el;
  geom_init_data *init_data;
  geom_init_element *el;

  CSVASSERT(data);

  for (data_el = geom_data_get_element(data); data_el;
       data_el = geom_data_element_next(data_el)) {
    init_data = geom_data_element_get_init(data_el);
    if (!init_data)
      continue;
    for (el = geom_init_data_get_element(init_data); el;
         el = geom_init_element_next(el)) {
      jupiter_init_func_binary_set_cdo(el, cdo);
    }
  }
}

void jupiter_init_func_binary_set_cdo(geom_init_element *el, domain *cdo)
{
  void *pd;
  struct jupiter_init_func_binary_data *d;

  CSVASSERT(cdo);
  CSVASSERT(el);

  pd = geom_init_element_get_func_data(el, jupiter_init_func_binary_data_id());
  if (!pd) return;

  d = (struct jupiter_init_func_binary_data *)pd;
  d->cdo = cdo;
}

int jupiter_init_func_binary_read_data(domain *cdo, mpi_param *mpi,
                                       geom_init_element *el, int force)
{
  void *pd;
  struct jupiter_init_func_binary_data *d;
  size_t nxyz;
  geom_svec3 gn;
  binary_output_mode outmode;
  int use_float;
  int r;

  CSVASSERT(cdo);
  CSVASSERT(mpi);
  CSVASSERT(el);

  pd = geom_init_element_get_func_data(el, jupiter_init_func_binary_data_id());
  if (!pd) return 0;
  d = (struct jupiter_init_func_binary_data *)pd;

  if (!force && d->uptodate) return 0;

  d->cdo = cdo;
  if (d->data) free(d->data);

  outmode = d->outmode;
  switch(outmode) {
  case BINARY_OUTPUT_BYPROCESS:
    if (d->unified) {
      outmode = BINARY_OUTPUT_UNIFY_MPI;
    }
    break;
  case BINARY_OUTPUT_UNIFY_GATHER:
  case BINARY_OUTPUT_UNIFY_MPI:
    if (!d->unified) {
      outmode = BINARY_OUTPUT_BYPROCESS;
    }
    break;
  default:
    outmode = BINARY_OUTPUT_INVALID;
    return -1;
  }

  nxyz  = cdo->nx;
  nxyz *= cdo->ny;
  nxyz *= cdo->nz;
  gn = geom_svec3_c(cdo->gnx, cdo->gny, cdo->gnz);

  d->data = (type *)malloc(sizeof(type) * nxyz);
  if (!d->data) return -1;

  use_float = 0;
  if (d->use_float == 4) {
    use_float = 1;
  }

  if (d->unified) {
    if ((geom_svec3_eql(d->ncells, geom_svec3_c(0, 0, 0)) ||
         geom_svec3_eql(d->ncells, gn)) &&
        geom_svec3_eql(d->origin, geom_svec3_c(0, 0, 0)) &&
        geom_svec3_eql(d->ncopy, geom_svec3_c(1, 1, 1))) {

      r = input_binary(mpi, d->data, NULL, 0, 0, 0, 0, 0, 0,
                       cdo->nx, cdo->ny, cdo->nz, 1, d->fname, d->outmode,
                       use_float);
    } else {
      size_t i;

#pragma omp parallel for
      for (i = 0; i < nxyz; ++i) {
        d->data[i] = d->value_for_unspecified;
      }

      r = input_geometry_binary_unified(mpi, d->data, 0, 0, 0, 0, 0, 0,
                                        cdo->nx, cdo->ny, cdo->nz, 1,
                                        d->fname, use_float,
                                        d->origin, d->ncells,
                                        d->ncopy, d->copy_offset);
    }
  } else {
    r = input_binary(mpi, d->data, NULL, 0, 0, 0, 0, 0, 0,
                     cdo->nx, cdo->ny, cdo->nz, 1, d->fname,
                     BINARY_OUTPUT_BYPROCESS, use_float);
  }

  if (r == 0) d->uptodate = 1;

  return r;
}
