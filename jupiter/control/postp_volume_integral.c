#include "postp_volume_integral.h"
#include "abstract_stats.h"
#include "abstract_stats_data.h"
#include "cell.h"
#include "cell_data.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "extent.h"
#include "grid_data.h"
#include "information.h"
#include "overflow.h"
#include "para_util.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"

struct jcntrl_postp_volume_integral
{
  jcntrl_abstract_stat_all stat;
  jcntrl_char_array *volume_name;
  jcntrl_char_array *cardinality_name;
};
#define jcntrl_postp_volume_integral__ancestor jcntrl_abstract_stat_all
#define jcntrl_postp_volume_integral__dnmem stat.jcntrl_abstract_stat_all__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_volume_integral);

static jcntrl_postp_volume_integral *
jcntrl_postp_volume_integral_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_volume_integral, obj);
}

static void *jcntrl_postp_volume_integral_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_volume_integral_downcast_impl(obj);
}

static int jcntrl_postp_volume_integral_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_volume_integral *v;
  v = jcntrl_postp_volume_integral_downcast_impl(obj);
  v->volume_name = NULL;
  v->cardinality_name = NULL;
  return 1;
}

static void jcntrl_postp_volume_integral_destructor(jcntrl_shared_object *obj)
{
  jcntrl_postp_volume_integral *v;
  v = jcntrl_postp_volume_integral_downcast_impl(obj);
  if (v->volume_name)
    jcntrl_char_array_delete(v->volume_name);
  if (v->cardinality_name)
    jcntrl_char_array_delete(v->cardinality_name);
}

static jcntrl_shared_object *jcntrl_postp_volume_integral_allocator(void)
{
  jcntrl_postp_volume_integral *v;
  v = jcntrl_shared_object_default_allocator(jcntrl_postp_volume_integral);
  return v ? jcntrl_postp_volume_integral_object(v) : NULL;
}

static void jcntrl_postp_volume_integral_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static jcntrl_size_type jcntrl_postp_volume_integral_number_of_work_arrays_impl(
  jcntrl_shared_object *obj)
{
  return 3;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                    number_of_work_arrays)

static int jcntrl_postp_volume_integral_work_array_types_impl(
  jcntrl_shared_object *obj, const jcntrl_shared_object_data **types[])
{
  static const jcntrl_shared_object_data *t[3];
  static int t_inited = 0;

  if (!t_inited) {
    t[0] = jcntrl_double_array_metadata_init(); // for sum(phi * dV)
    t[1] = jcntrl_double_array_metadata_init(); // for sum(dV)
    t[2] = jcntrl_size_array_metadata_init();   // for N
    t_inited = 1;
  }
  *types = t;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                    work_array_types)

struct calc_local_data
{
  const char *mask;
  jcntrl_grid_data *grid;
  jcntrl_data_array *input;
  jcntrl_extent pext;
  jcntrl_extent dext;
  double sum_dV;
  double sum_phidV;
  jcntrl_size_type N;
};

static int jcntrl_postp_volume_integral_calc_local(jcntrl_size_type jj,
                                                   void *arg)
{
  struct calc_local_data *p = arg;
  int i, j, k;
  jcntrl_size_type ii;
  jcntrl_cell_hex h;
  double phi, dV;

  if (!jcntrl_extent_index(p->pext, jj, &i, &j, &k))
    return 0;

  ii = jcntrl_extent_addr(p->dext, i, j, k);
  if (ii < 0)
    return 0;

  if (p->mask && p->mask[ii])
    return 1;

  jcntrl_cell_hex_init_g(&h, p->grid, i, j, k, NULL);
  dV = jcntrl_cell_volume(jcntrl_cell_hex_cell(&h));
  phi = jcntrl_data_array_get_value(p->input, ii);

  p->sum_dV += dV;
  p->sum_phidV += phi * dV;
  p->N += 1;

  return 1;
}

static int jcntrl_postp_volume_integral_calc_local_stat_impl(
  jcntrl_shared_object *obj, jcntrl_grid_data *grid, const int piece_extent[6],
  const int data_extent[6], jcntrl_data_array *input, jcntrl_bool_array *mask,
  jcntrl_data_array **work_arrays)
{
  jcntrl_postp_volume_integral *v;
  jcntrl_extent pext = jcntrl_extent_c(piece_extent);
  jcntrl_extent dext = jcntrl_extent_c(data_extent);
  jcntrl_size_type n;
  double sum_dV, sum_phidV;
  jcntrl_size_type card;
  int r;
  double *sum_phidVa;
  double *sum_dVa;
  jcntrl_size_type *Na;

  v = jcntrl_postp_volume_integral_downcast_impl(obj);
  n = jcntrl_extent_size(pext);

  sum_phidVa = jcntrl_data_array_get_writable_double(work_arrays[0]);
  sum_dVa = jcntrl_data_array_get_writable_double(work_arrays[1]);
  Na = jcntrl_data_array_get_writable_sizes(work_arrays[2]);

  card = 0;
  sum_dV = 0.0;
  sum_phidV = 0.0;

  r = 1;
  if (n > 0 && input) {
    const char *m = NULL;
    if (mask)
      m = jcntrl_bool_array_get(mask);

    jcntrl_cell_hex_metadata_init();
#ifdef _OPENMP
#pragma omp parallel
#endif
    {
      struct calc_local_data d = {
        .mask = m,
        .grid = grid,
        .input = input,
        .pext = pext,
        .dext = dext,
        .sum_dV = 0.0,
        .sum_phidV = 0.0,
        .N = 0,
      };
      if (jcntrl_para_loop(0, n, jcntrl_postp_volume_integral_calc_local, &d,
                           &r)) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        sum_dV += d.sum_dV;
#ifdef _OPENMP
#pragma omp atomic update
#endif
        sum_phidV += d.sum_phidV;
#ifdef _OPENMP
#pragma omp atomic update
#endif
        card += d.N;
      }
    }
  }

  sum_phidVa[0] = sum_phidV;
  sum_dVa[0] = sum_dV;
  Na[0] = card;
  return r;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                    calc_local_stat)

static int jcntrl_postp_volume_integral_calc_global_stat_impl(
  jcntrl_shared_object *obj, jcntrl_data_array **work_arrays,
  jcntrl_data_array *output)
{
  jcntrl_size_type n;
  const double *phidVa;
  double phidV;

  n = jcntrl_data_array_get_ntuple(work_arrays[0]);
  phidVa = jcntrl_data_array_get_double(work_arrays[0]);

  phidV = 0.0;
  for (jcntrl_size_type i = 0; i < n; ++i)
    phidV += phidVa[i];

  jcntrl_data_array_set_value(output, 0, phidV);
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                    calc_global_stat)

static int jcntrl_postp_volume_integral_array_set_name(jcntrl_data_array *d,
                                                       jcntrl_cell_data *cdata,
                                                       jcntrl_char_array *name)
{
  const char *nm;
  jcntrl_size_type nlen;

  if (!jcntrl_data_array_get_writable(d)) {
    jcntrl_data_array_delete(d);
    return 0;
  }

  nlen = jcntrl_char_array_get_ntuple(name);
  nm = jcntrl_char_array_get(name);

  if (!jcntrl_data_array_set_name(d, nm, nlen)) {
    jcntrl_data_array_delete(d);
    return 0;
  }

  if (!jcntrl_cell_data_add_array(cdata, d)) {
    jcntrl_data_array_delete(d);
    return 0;
  }

  jcntrl_shared_object_release_ownership(jcntrl_data_array_object(d));
  return 1;
}

static int jcntrl_postp_volume_integral_add_double(double value,
                                                   jcntrl_cell_data *cdata,
                                                   jcntrl_char_array *name)
{
  jcntrl_double_array *a;
  jcntrl_data_array *d;

  a = jcntrl_double_array_for(&value, 1);
  if (!a)
    return 0;

  d = jcntrl_double_array_data(a);
  return jcntrl_postp_volume_integral_array_set_name(d, cdata, name);
}

static int jcntrl_postp_volume_integral_add_size(jcntrl_size_type value,
                                                 jcntrl_cell_data *cdata,
                                                 jcntrl_char_array *name)
{
  jcntrl_size_array *a;
  jcntrl_data_array *d;

  a = jcntrl_size_array_for(&value, 1);
  if (!a)
    return 0;

  d = jcntrl_size_array_data(a);
  return jcntrl_postp_volume_integral_array_set_name(d, cdata, name);
}

static int jcntrl_postp_volume_integral_postprocess_impl(
  jcntrl_shared_object *obj, jcntrl_information *output_info,
  jcntrl_size_type result_size, jcntrl_data_array **work_arrays)
{
  jcntrl_postp_volume_integral *v;
  jcntrl_shared_object *oobj;
  jcntrl_grid_data *grid;
  jcntrl_cell_data *cdata;
  jcntrl_size_type nlen, n;

  v = jcntrl_postp_volume_integral_downcast_impl(obj);

  oobj = jcntrl_information_get_object(output_info, JCNTRL_INFO_DATA_OBJECT);
  if (!oobj)
    return 0;

  grid = jcntrl_grid_data_downcast(oobj);
  if (!grid)
    return 0;

  cdata = jcntrl_grid_data_cell_data(grid);

  n = jcntrl_extent_size(jcntrl_extent_c(jcntrl_grid_data_extent(grid)));
  if (n <= 0)
    return 1;

  JCNTRL_ASSERT(n == 1);

  nlen = 0;
  if (v->volume_name)
    nlen = jcntrl_char_array_get_ntuple(v->volume_name);
  if (nlen > 0) {
    double V;
    const double *dV;

    n = jcntrl_data_array_get_ntuple(work_arrays[1]);
    dV = jcntrl_data_array_get_double(work_arrays[1]);

    V = 0.0;
    for (jcntrl_size_type i = 0; i < n; ++i)
      V += dV[i];

    if (!jcntrl_postp_volume_integral_add_double(V, cdata, v->volume_name))
      return 0;
  }

  nlen = 0;
  if (v->cardinality_name)
    nlen = jcntrl_char_array_get_ntuple(v->cardinality_name);
  if (nlen > 0) {
    jcntrl_size_type N;
    const jcntrl_size_type *dN;

    n = jcntrl_data_array_get_ntuple(work_arrays[2]);
    dN = jcntrl_data_array_get_sizes(work_arrays[2]);

    N = 0;
    for (jcntrl_size_type i = 0; i < n; ++i) {
      if (jcntrl_s_add_overflow(N, dN[i], &N))
        return 0;
    }

    if (!jcntrl_postp_volume_integral_add_size(N, cdata, v->cardinality_name))
      return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                    postprocess)

static void
jcntrl_postp_volume_integral_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_volume_integral_downcast_v;
  p->initializer = jcntrl_postp_volume_integral_initializer;
  p->destructor = jcntrl_postp_volume_integral_destructor;
  p->allocator = jcntrl_postp_volume_integral_allocator;
  p->deleter = jcntrl_postp_volume_integral_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                          number_of_work_arrays);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                          work_array_types);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                          calc_local_stat);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                          calc_global_stat);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_volume_integral, jcntrl_abstract_stat,
                          postprocess);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_volume_integral,
                                   jcntrl_postp_volume_integral_init_func)

jcntrl_postp_volume_integral *jcntrl_postp_volume_integral_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_volume_integral);
}

void jcntrl_postp_volume_integral_delete(jcntrl_postp_volume_integral *v)
{
  jcntrl_shared_object_delete(jcntrl_postp_volume_integral_object(v));
}

jcntrl_shared_object *
jcntrl_postp_volume_integral_object(jcntrl_postp_volume_integral *v)
{
  return jcntrl_abstract_stat_all_object(&v->stat);
}

jcntrl_executive *
jcntrl_postp_volume_integral_executive(jcntrl_postp_volume_integral *v)
{
  return jcntrl_abstract_stat_all_executive(&v->stat);
}

jcntrl_postp_volume_integral *
jcntrl_postp_volume_integral_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_postp_volume_integral, obj);
}

void jcntrl_postp_volume_integral_set_controller(
  jcntrl_postp_volume_integral *v, const jcntrl_mpi_controller *controller)
{
  jcntrl_abstract_stat_all_set_controller(&v->stat, controller);
}

void jcntrl_postp_volume_integral_set_root_rank(jcntrl_postp_volume_integral *v,
                                                int root)
{
  jcntrl_abstract_stat_set_root_rank(&v->stat.stat, root);
}

int jcntrl_postp_volume_integral_root_rank(jcntrl_postp_volume_integral *v)
{
  return jcntrl_abstract_stat_root_rank(&v->stat.stat);
}

const jcntrl_mpi_controller *
jcntrl_postp_volume_integral_controller(jcntrl_postp_volume_integral *v)
{
  return jcntrl_abstract_stat_all_controller(&v->stat);
}

jcntrl_char_array *
jcntrl_postp_volume_integral_volume_varname(jcntrl_postp_volume_integral *v)
{
  return v->volume_name;
}

int jcntrl_postp_volume_integral_set_volume_varname(
  jcntrl_postp_volume_integral *v, jcntrl_data_array *d)
{
  return jcntrl_set_string(&v->volume_name, d);
}

int jcntrl_postp_volume_integral_set_volume_varname_c(
  jcntrl_postp_volume_integral *v, const char *name, jcntrl_size_type clen)
{
  return jcntrl_set_string_c(&v->volume_name, name, clen);
}

jcntrl_char_array *jcntrl_postp_volume_integral_cardinality_varname(
  jcntrl_postp_volume_integral *v)
{
  return v->cardinality_name;
}

int jcntrl_postp_volume_integral_set_cardinality_varname(
  jcntrl_postp_volume_integral *v, jcntrl_data_array *d)
{
  return jcntrl_set_string(&v->cardinality_name, d);
}

int jcntrl_postp_volume_integral_set_cardinality_varname_c(
  jcntrl_postp_volume_integral *v, const char *name, jcntrl_size_type clen)
{
  return jcntrl_set_string_c(&v->cardinality_name, name, clen);
}
