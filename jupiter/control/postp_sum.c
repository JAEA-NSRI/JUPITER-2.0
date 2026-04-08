#include "postp_sum.h"
#include "cell_data.h"
#include "data_array.h"
#include "error.h"
#include "executive.h"
#include "extent.h"
#include "grid_data.h"
#include "information.h"
#include "para_util.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "abstract_stats.h"
#include "abstract_stats_data.h"
#include "defs.h"

struct jcntrl_postp_sum
{
  jcntrl_abstract_stat_all stat;
  jcntrl_char_array *cardinality_name;
};
#define jcntrl_postp_sum__ancestor jcntrl_abstract_stat_all
#define jcntrl_postp_sum__dnmem stat.jcntrl_abstract_stat_all__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_sum);

static jcntrl_postp_sum *
jcntrl_postp_sum_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_sum, obj);
}

static void *jcntrl_postp_sum_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_sum_downcast_impl(obj);
}

static int jcntrl_postp_sum_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_sum *s;
  s = jcntrl_postp_sum_downcast_impl(obj);
  s->cardinality_name = NULL;
  return 1;
}

static void jcntrl_postp_sum_desctructor(jcntrl_shared_object *obj)
{
  jcntrl_postp_sum *s;
  s = jcntrl_postp_sum_downcast_impl(obj);
  if (s->cardinality_name)
    jcntrl_char_array_delete(s->cardinality_name);
}

static jcntrl_shared_object *jcntrl_postp_sum_allocator(void)
{
  jcntrl_postp_sum *s;
  s = jcntrl_shared_object_default_allocator(jcntrl_postp_sum);
  return s ? jcntrl_postp_sum_object(s) : NULL;
}

static void jcntrl_postp_sum_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static jcntrl_size_type
jcntrl_postp_sum_number_of_work_arrays_impl(jcntrl_shared_object *obj)
{
  return 2;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_sum, jcntrl_abstract_stat,
                    number_of_work_arrays)

static int jcntrl_postp_sum_work_array_types_impl(
  jcntrl_shared_object *obj, const jcntrl_shared_object_data **types[])
{
  static const jcntrl_shared_object_data *t[2];
  static int t_inited = 0;

  if (!t_inited) {
    t[0] = jcntrl_double_array_metadata_init(); // for sum(phi)
    t[1] = jcntrl_size_array_metadata_init();   // for N
    t_inited = 1;
  }
  *types = t;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_sum, jcntrl_abstract_stat, work_array_types)

struct calc_local_data
{
  const char *mask;
  jcntrl_extent pext;
  jcntrl_extent dext;
  jcntrl_data_array *input;
  double sum_phi;
  jcntrl_size_type N;
};

static int jcntrl_postp_sum_calc_local_stat(jcntrl_size_type jj, void *arg)
{
  int i, j, k;
  jcntrl_size_type ii;
  struct calc_local_data *p = arg;

  if (!jcntrl_extent_index(p->pext, jj, &i, &j, &k))
    return 0;

  ii = jcntrl_extent_addr(p->dext, i, j, k);
  if (ii < 0)
    return 0;

  if (p->mask && p->mask[ii])
    return 1;

  p->sum_phi += jcntrl_data_array_get_value(p->input, ii);
  p->N += 1;
  return 1;
}

static int jcntrl_postp_sum_calc_local_stat_impl(
  jcntrl_shared_object *obj, jcntrl_grid_data *grid, const int piece_extent[6],
  const int data_extent[6], jcntrl_data_array *input, jcntrl_bool_array *mask,
  jcntrl_data_array **work_arrays)
{
  jcntrl_postp_sum *s;
  jcntrl_extent pext, dext;
  jcntrl_size_type n;
  double *suma;
  jcntrl_size_type *na;
  const char *m;
  int r;

  s = jcntrl_postp_sum_downcast_impl(obj);
  pext = jcntrl_extent_c(piece_extent);
  dext = jcntrl_extent_c(data_extent);
  n = jcntrl_extent_size(pext);
  suma = jcntrl_data_array_get_writable_double(work_arrays[0]);
  na = jcntrl_data_array_get_writable_sizes(work_arrays[1]);
  JCNTRL_ASSERT(suma);
  JCNTRL_ASSERT(na);

  r = 1;
  if (n > 0) {
    double sum;
    jcntrl_size_type nc;

    sum = 0.0;
    nc = 0;

    m = NULL;
    if (mask)
      m = jcntrl_bool_array_get(mask);

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
      struct calc_local_data d = {
        .mask = m,
        .pext = pext,
        .dext = dext,
        .input = input,
        .sum_phi = 0.0,
        .N = 0,
      };
      if (jcntrl_para_loop(0, n, jcntrl_postp_sum_calc_local_stat, &d, &r)) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        sum += d.sum_phi;
#ifdef _OPENMP
#pragma omp atomic update
#endif
        nc += d.N;
      }
    }
    *suma = sum;
    *na = nc;
  } else {
    *suma = 0.0;
    *na = 0;
  }

  return r;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_sum, jcntrl_abstract_stat, calc_local_stat)

static int
jcntrl_postp_sum_calc_global_stat_impl(jcntrl_shared_object *obj,
                                       jcntrl_data_array **work_arrays,
                                       jcntrl_data_array *output)
{
  jcntrl_size_type n;
  const double *phia;
  double phi;

  n = jcntrl_data_array_get_ntuple(work_arrays[0]);
  phia = jcntrl_data_array_get_double(work_arrays[0]);

  phi = 0.0;
  for (jcntrl_size_type i = 0; i < n; ++i)
    phi += phia[i];

  jcntrl_data_array_set_value(output, 0, phi);
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_sum, jcntrl_abstract_stat, calc_global_stat)

static int jcntrl_postp_sum_postprocess_impl(jcntrl_shared_object *obj,
                                             jcntrl_information *output_info,
                                             jcntrl_size_type result_size,
                                             jcntrl_data_array **work_arrays)
{
  jcntrl_postp_sum *s;
  jcntrl_shared_object *oobj;
  jcntrl_grid_data *grid;
  jcntrl_cell_data *cdata;
  jcntrl_size_type nlen, n;

  s = jcntrl_postp_sum_downcast_impl(obj);

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
  if (s->cardinality_name)
    nlen = jcntrl_char_array_get_ntuple(s->cardinality_name);
  if (nlen > 0) {
    jcntrl_size_type N;
    jcntrl_size_type *dN;
    jcntrl_data_array *nary;

    n = jcntrl_data_array_get_ntuple(work_arrays[1]);
    dN = jcntrl_data_array_get_writable_sizes(work_arrays[1]);

    N = 0;
    for (jcntrl_size_type i = 0; i < n; ++i)
      N += dN[i];

    dN[0] = N;
    if (!jcntrl_data_array_resize(work_arrays[1], 1))
      return 0;

    nary = jcntrl_char_array_data(s->cardinality_name);
    if (!jcntrl_data_array_set_name_d(work_arrays[1], nary))
      return 0;

    jcntrl_cell_data_add_array(cdata, work_arrays[1]);
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_sum, jcntrl_abstract_stat, postprocess)

static void jcntrl_postp_sum_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_sum_downcast_v;
  p->initializer = jcntrl_postp_sum_initializer;
  p->destructor = jcntrl_postp_sum_desctructor;
  p->allocator = jcntrl_postp_sum_allocator;
  p->deleter = jcntrl_postp_sum_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_sum, jcntrl_abstract_stat,
                          number_of_work_arrays);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_sum, jcntrl_abstract_stat,
                          work_array_types);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_sum, jcntrl_abstract_stat,
                          calc_local_stat);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_sum, jcntrl_abstract_stat,
                          calc_global_stat);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_sum, jcntrl_abstract_stat,
                          postprocess);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_sum, jcntrl_postp_sum_init_func)

jcntrl_postp_sum *jcntrl_postp_sum_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_sum);
}

void jcntrl_postp_sum_delete(jcntrl_postp_sum *s)
{
  jcntrl_shared_object_delete(jcntrl_postp_sum_object(s));
}

jcntrl_shared_object *jcntrl_postp_sum_object(jcntrl_postp_sum *s)
{
  JCNTRL_ASSERT(s);
  return jcntrl_executive_object(jcntrl_postp_sum_executive(s));
}

jcntrl_executive *jcntrl_postp_sum_executive(jcntrl_postp_sum *s)
{
  JCNTRL_ASSERT(s);
  return jcntrl_abstract_stat_all_executive(&s->stat);
}

jcntrl_postp_sum *jcntrl_postp_sum_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_postp_sum, obj);
}

void jcntrl_postp_sum_set_controller(jcntrl_postp_sum *s,
                                     const jcntrl_mpi_controller *controller)
{
  JCNTRL_ASSERT(s);
  jcntrl_abstract_stat_all_set_controller(&s->stat, controller);
}

const jcntrl_mpi_controller *jcntrl_postp_sum_controller(jcntrl_postp_sum *s)
{
  JCNTRL_ASSERT(s);
  return jcntrl_abstract_stat_all_controller(&s->stat);
}

void jcntrl_postp_sum_set_root_rank(jcntrl_postp_sum *s, int root)
{
  JCNTRL_ASSERT(s);
  jcntrl_abstract_stat_set_root_rank(&s->stat.stat, root);
}

int jcntrl_postp_sum_root_rank(jcntrl_postp_sum *s)
{
  JCNTRL_ASSERT(s);
  return jcntrl_abstract_stat_root_rank(&s->stat.stat);
}

jcntrl_char_array *jcntrl_postp_sum_cardinality_varname(jcntrl_postp_sum *s)
{
  JCNTRL_ASSERT(s);
  return s->cardinality_name;
}

int jcntrl_postp_sum_set_cardinality_varname(jcntrl_postp_sum *s,
                                             jcntrl_data_array *d)
{
  JCNTRL_ASSERT(s);
  return jcntrl_set_string(&s->cardinality_name, d);
}

int jcntrl_postp_sum_set_cardinality_varname_c(jcntrl_postp_sum *s,
                                               const char *name,
                                               jcntrl_size_type clen)
{
  JCNTRL_ASSERT(s);
  return jcntrl_set_string_c(&s->cardinality_name, name, clen);
}
