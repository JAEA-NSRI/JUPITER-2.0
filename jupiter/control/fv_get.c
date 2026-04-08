#include "fv_get.h"
#include "cell_data.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "executive.h"
#include "executive_data.h"
#include "field_variable.h"
#include "global.h"
#include "grid_data.h"
#include "information.h"
#include "input.h"
#include "mask_data.h"
#include "mpi_controller.h"
#include "output.h"
#include "overflow.h"
#include "para_util.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"
#include "extent.h"
#include "subarray.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

struct jcntrl_fv_get
{
  jcntrl_executive exec;
  const jcntrl_mpi_controller *controller;
  jcntrl_char_array *varname;
  int exclude_masked;
  int extract_extent;
  jcntrl_extent extent;
};
#define jcntrl_fv_get__ancestor jcntrl_executive
#define jcntrl_fv_get__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_fv_get);

static jcntrl_fv_get *jcntrl_fv_get_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_fv_get, obj);
}

static void *jcntrl_fv_get_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_fv_get_downcast_impl(obj);
}

static int jcntrl_fv_get_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;
  jcntrl_fv_get *p;

  p = jcntrl_fv_get_downcast_impl(obj);
  p->controller = NULL;
  p->varname = jcntrl_char_array_new();
  p->exclude_masked = 0;
  p->extract_extent = 0;
  p->extent = jcntrl_extent_empty();

  if (!p->varname)
    return 0;

  exe = jcntrl_fv_get_executive(p);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);

  if (!jcntrl_input_set_number_of_ports(input, 1))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;

  return 1;
}

static void jcntrl_fv_get_destructor(jcntrl_shared_object *obj)
{
  jcntrl_fv_get *p = jcntrl_fv_get_downcast_impl(obj);
  if (p->varname)
    jcntrl_char_array_delete(p->varname);
}

static jcntrl_shared_object *jcntrl_fv_get_allocator(void)
{
  jcntrl_fv_get *p = jcntrl_shared_object_default_allocator(jcntrl_fv_get);
  return p ? jcntrl_fv_get_object(p) : NULL;
}

static void jcntrl_fv_get_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int
jcntrl_fv_get_fill_input_port_information_impl(jcntrl_shared_object *obj,
                                               int index, jcntrl_input *input)
{
  jcntrl_information *info;
  info = jcntrl_input_information(input);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_GRID))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_get, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_fv_get_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_field_variable))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_get, jcntrl_executive,
                    fill_output_port_information)

static jcntrl_size_type jcntrl_fv_get_extent_size(jcntrl_fv_get *p,
                                                  jcntrl_extent *overlap_extent,
                                                  const int *src_extent,
                                                  jcntrl_extent data_extent)
{
  jcntrl_extent e = jcntrl_extent_c(src_extent);

  if (p->extract_extent)
    e = jcntrl_extent_overlap(e, p->extent);

  e = jcntrl_extent_overlap(e, data_extent);

  *overlap_extent = e;
  return jcntrl_extent_size(e);
}

struct count_loop_ldata
{
  const jcntrl_extent *extent;
  const jcntrl_extent *data_extent;
  const char *mask;
  jcntrl_size_type *indices;
  jcntrl_size_type ln;
};

static int jcntrl_fv_get_idx_count_loop(jcntrl_size_type jj, void *arg)
{
  jcntrl_size_type js;
  int i, j, k;
  struct count_loop_ldata *p;
  p = (struct count_loop_ldata *)arg;

  if (!jcntrl_extent_index(*p->extent, jj, &i, &j, &k))
    return 0;

  js = jcntrl_extent_addr(*p->data_extent, i, j, k);
  if (js < 0)
    return 0;

  if (!p->mask || !p->mask[js]) {
    if (jcntrl_s_add_overflow(p->ln, 1, &p->ln))
      return 0;

    p->indices[jj] = js;
  } else {
    p->indices[jj] = -1;
  }
  return 1;
}

struct reindexer_ldata
{
  const int *extents;
  const jcntrl_size_type *indices;
  const int *ranks;
  jcntrl_size_type *work;
  jcntrl_size_type *outindices;
};

static int jcntrl_fv_get_build_indices(jcntrl_fv_get *p, jcntrl_grid_data *grid,
                                       const int *src_extent,
                                       jcntrl_size_array *indices)
{
  jcntrl_extent extent;
  jcntrl_extent data_extent;
  jcntrl_size_type n, ntot;
  jcntrl_bool_array *mask_array;
  const char *mask;
  const jcntrl_extent *extentp, *data_extentp;
  jcntrl_size_type *indicesp;
  int ret;

  data_extent = jcntrl_extent_c(jcntrl_grid_data_extent(grid));
  ntot = jcntrl_fv_get_extent_size(p, &extent, src_extent, data_extent);
  if (ntot < 0)
    return 0;

  jcntrl_size_array_resize(indices, ntot);
  if (ntot == 0)
    return 1;

  mask_array = NULL;
  mask = NULL;
  if (p->exclude_masked) {
    jcntrl_mask_data *m;
    m = jcntrl_grid_data_get_mask(grid);
    if (m)
      mask_array = jcntrl_mask_data_array(m);
  }

  if (mask_array && jcntrl_bool_array_get_ntuple(mask_array) > 0) {
    mask = jcntrl_bool_array_get(mask_array);
  }

  n = 0;
  extentp = &extent;
  data_extentp = &data_extent;
  indicesp = jcntrl_size_array_get_writable(indices);
  ret = 1;

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    struct count_loop_ldata l = {
      .data_extent = data_extentp,
      .extent = extentp,
      .indices = indicesp,
      .mask = mask,
      .ln = 0,
    };
    if (jcntrl_para_loop(0, ntot, jcntrl_fv_get_idx_count_loop, &l, &ret)) {
#ifdef _OPENMP
#pragma omp critical
      {
        jcntrl_size_type lt;
        lt = n;
        if (!jcntrl_s_add_overflow(lt, l.ln, &lt)) {
          n = lt;
        } else {
          ret = 0;
        }
      }
#else
      n = l.ln;
#endif
    }
  }
  if (!ret)
    return 0;

  {
    jcntrl_size_type js = 0;
    for (jcntrl_size_type jj = 0; jj < ntot; ++jj) {
      if (indicesp[jj] == -1)
        continue;
      if (jj != js)
        indicesp[js] = indicesp[jj];
      ++js;
    }
    JCNTRL_ASSERT(js == n);
  }

  if (!jcntrl_size_array_resize(indices, n))
    return 0;

  return 1;
}

static int jcntrl_fv_get_indices_sorter(const void *a, const void *b)
{
  const jcntrl_size_type *pa = a;
  const jcntrl_size_type *pb = b;
  jcntrl_size_type ii, jj;

  ii = *(pa + 1);
  jj = *(pb + 1);

  if (ii >= 0 && jj >= 0)
    return ii - jj;
  if (jj >= 0)
    return 1;
  if (ii >= 0)
    return -1;
  return 0;
}

static int jcntrl_fv_get_calc_recv_indices(int nproc, jcntrl_int_array *extents,
                                           jcntrl_size_array *recv_pnts,
                                           jcntrl_size_array *recv_indices,
                                           jcntrl_size_array *work)
{
  jcntrl_extent global_extent;
  jcntrl_size_type nindices;
  jcntrl_size_type szw;
  const int *iexts;
  const jcntrl_size_type *ipnts;
  jcntrl_size_type *indices;
  jcntrl_size_type *wrk;
  jcntrl_size_type *ranktab;
  jcntrl_size_type jx;
  int irank;

  JCNTRL_ASSERT(extents);
  JCNTRL_ASSERT(recv_pnts);
  JCNTRL_ASSERT(recv_indices);
  JCNTRL_ASSERT(work);

  JCNTRL_ASSERT(jcntrl_size_array_get_ntuple(recv_pnts) == nproc);
  JCNTRL_ASSERT(jcntrl_int_array_get_ntuple(extents) == nproc * 6);

  nindices = jcntrl_size_array_get_ntuple(recv_indices);
  if (nindices <= 0)
    return 1;

  if (jcntrl_s_mul_overflow(nindices, 2, &szw))
    return 0;

  if (jcntrl_s_add_overflow(szw, nproc, &szw))
    return 0;

  if (!jcntrl_size_array_resize(work, szw))
    return 0;

  iexts = jcntrl_int_array_get(extents);
  ipnts = jcntrl_size_array_get(recv_pnts);
  indices = jcntrl_size_array_get_writable(recv_indices);
  wrk = jcntrl_size_array_get_writable(work);

  JCNTRL_ASSERT(iexts);
  JCNTRL_ASSERT(ipnts);
  JCNTRL_ASSERT(indices);
  JCNTRL_ASSERT(wrk);

  /* Calculate last index for each ranks */
  ranktab = wrk + 2 * nindices;
  jx = 0;
  for (int i = 0; i < nproc; ++i) {
    if (jcntrl_s_add_overflow(jx, ipnts[i], &jx))
      return 0;
    ranktab[i] = jx;
  }

  /* Calculate covering extent */
  global_extent = jcntrl_extent_empty();

  for (int i = 0; i < nproc; ++i) {
    jcntrl_size_type jj;
    jcntrl_extent p;

    if (jcntrl_s_mul_overflow(i, 6, &jj))
      return 0;

    p = jcntrl_extent_c(&iexts[jj]);
    global_extent = jcntrl_extent_cover(global_extent, p);
  }

  /* Calculate address in covering extent */
  irank = 0;
  for (jcntrl_size_type ii = 0; ii < nindices; ++ii) {
    int i, j, k;
    jcntrl_size_type jj, jt;
    jcntrl_extent p;

    while (irank < nproc && ii >= ranktab[irank])
      irank++;

    if (irank == nproc)
      return 0;

    jj = (jcntrl_size_type)irank * 6;
    p = jcntrl_extent_c(&iexts[jj]);

    if (!jcntrl_extent_index(p, indices[ii], &i, &j, &k))
      return 0;

    jj = jcntrl_extent_addr(global_extent, i, j, k);

    if (jcntrl_s_mul_overflow(ii, 2, &jt))
      return 0;

    wrk[jt] = ii;

    if (jcntrl_s_add_overflow(jt, 1, &jt))
      return 0;

    wrk[jt] = jj;
  }

  szw = sizeof(jcntrl_size_type);
  if (jcntrl_s_mul_overflow(szw, 2, &szw))
    return 0;

  /* Sort array by address of covering extent */
  qsort(wrk, nindices, szw, jcntrl_fv_get_indices_sorter);

  /* Fill recv_indices with indices of sorted array to pack */
  for (jcntrl_size_type ii = 0; ii < nindices; ++ii)
    indices[wrk[ii * 2]] = ii;

  return 1;
}

static int jcntrl_fv_get_process_update_data_impl(jcntrl_information *request,
                                                  jcntrl_input *input,
                                                  jcntrl_output *output,
                                                  jcntrl_shared_object *obj)
{
  jcntrl_fv_get *p;
  jcntrl_information *iinfo, *oinfo;
  jcntrl_shared_object *iobj, *oobj;
  jcntrl_grid_data *grid_in;
  jcntrl_field_variable *fv_out;
  jcntrl_size_array *send_indices, *recv_indices, *recv_indices_work;
  jcntrl_size_array *recv_pnts;
  jcntrl_int_array *extents;
  jcntrl_data_array **recv_idxs;
  jcntrl_data_array *send_data;
  jcntrl_data_array *recv_data;
  const int *src_extent;
  int r;
  int nproc;

  p = jcntrl_fv_get_downcast_impl(obj);
  input = jcntrl_input_next_port(input);
  if (!input)
    return 0;

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  iinfo = jcntrl_input_upstream_information(input);
  oinfo = jcntrl_output_information(output);

  JCNTRL_ASSERT(iinfo);
  JCNTRL_ASSERT(oinfo);

  iobj = jcntrl_information_get_object(iinfo, JCNTRL_INFO_DATA_OBJECT);
  oobj = jcntrl_information_get_object(oinfo, JCNTRL_INFO_DATA_OBJECT);

  JCNTRL_ASSERT(iobj);
  JCNTRL_ASSERT(oobj);

  grid_in = jcntrl_grid_data_downcast(iobj);
  fv_out = jcntrl_field_variable_downcast(oobj);

  if (!grid_in)
    return 0;
  if (!fv_out)
    return 0;

  src_extent = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_PIECE_EXTENT);
  if (!src_extent)
    src_extent = jcntrl_grid_data_extent(grid_in);

  nproc = p->controller ? jcntrl_mpi_controller_nproc(p->controller) : 1;

  send_indices = jcntrl_size_array_new();
  recv_indices = jcntrl_size_array_new();
  recv_indices_work = jcntrl_size_array_new();
  extents = jcntrl_int_array_new();
  recv_pnts = jcntrl_size_array_new();
  recv_idxs = calloc(nproc, sizeof(jcntrl_data_array *));

  r = 1;
  do {
    jcntrl_size_type nidx;
    jcntrl_size_type nextents;
    jcntrl_size_type *send_pntsp;
    int rank;

    if (!send_indices || !recv_indices || !extents || !recv_pnts ||
        !recv_indices_work || !recv_idxs) {
      r = 0;
      break;
    }

    if (!jcntrl_fv_get_build_indices(p, grid_in, src_extent, send_indices)) {
      r = 0;
      break;
    }

    if (jcntrl_s_mul_overflow(nproc, 6, &nextents)) {
      r = 0;
      break;
    }

    if (!jcntrl_int_array_resize(extents, nextents)) {
      r = 0;
      break;
    }

    nidx = jcntrl_size_array_get_ntuple(send_indices);
    if (!jcntrl_size_array_resize(recv_pnts, nproc)) {
      r = 0;
      break;
    }

    send_pntsp = jcntrl_size_array_get_writable(recv_pnts);

    if (send_pntsp) {
      int rank;
      rank = p->controller ? jcntrl_mpi_controller_rank(p->controller) : 0;
      send_pntsp[rank] = nidx;
    }
  } while (0);

  if (p->controller) {
    if (!jcntrl_mpi_controller_all(p->controller, r))
      r = 0;
  }

  do {
    jcntrl_static_int_array lextent;
    jcntrl_data_array *send, *recv;
    jcntrl_extent dextent;

    if (!r)
      break;

    dextent = jcntrl_extent_c(jcntrl_grid_data_extent(grid_in));
    jcntrl_static_int_array_init_base(&lextent, dextent.extent, 6);

    send = jcntrl_static_int_array_data(&lextent);
    recv = jcntrl_int_array_data(extents);
    if (p->controller) {
      if (!jcntrl_mpi_controller_allgather(p->controller, recv, send, 6)) {
        r = 0;
        break;
      }
    } else {
      if (!jcntrl_data_array_resize(recv, 6)) {
        r = 0;
        break;
      }

      if (!jcntrl_data_array_copy(recv, send, 6, 0, 0)) {
        r = 0;
        break;
      }
    }

    if (p->controller) {
      send = jcntrl_size_array_data(recv_pnts);
      recv = send;
      if (!jcntrl_mpi_controller_allgather(p->controller, recv, send, 1)) {
        r = 0;
        break;
      }
    }

    send = jcntrl_size_array_data(send_indices);
    recv = jcntrl_size_array_data(recv_indices);
    if (p->controller) {
      if (!jcntrl_mpi_controller_allgatherva(p->controller, recv, send)) {
        r = 0;
        break;
      }
    } else {
      jcntrl_size_type ntuple = jcntrl_data_array_get_ntuple(send);

      if (!jcntrl_data_array_resize(recv, ntuple)) {
        r = 0;
        break;
      }

      if (!jcntrl_data_array_copy(recv, send, ntuple, 0, 0)) {
        r = 0;
        break;
      }
    }

    if (!jcntrl_fv_get_calc_recv_indices(nproc, extents, recv_pnts,
                                         recv_indices, recv_indices_work)) {
      r = 0;
      break;
    }

    {
      jcntrl_data_array *src;
      const jcntrl_size_type *pntsp;
      jcntrl_size_type js;
      jcntrl_size_type jn;

      src = jcntrl_size_array_data(recv_indices);
      pntsp = jcntrl_size_array_get(recv_pnts);

      js = 0;
      for (int i = 0; i < nproc; ++i) {
        jcntrl_data_subarray *sp;

        jn = pntsp[i];
        if (jn <= 0)
          continue;

        sp = jcntrl_data_subarray_new(src, js, jn);
        if (!sp) {
          r = 0;
          break;
        }

        recv_idxs[i] = jcntrl_data_subarray_data(sp);
      }
      if (!r)
        break;
    }

    {
      jcntrl_cell_data *cdata;
      const char *name;
      jcntrl_size_type namelen;

      send_data = NULL;
      name = jcntrl_char_array_get(p->varname);
      namelen = jcntrl_char_array_get_ntuple(p->varname);

      cdata = jcntrl_grid_data_cell_data(grid_in);
      if (cdata)
        send_data = jcntrl_cell_data_get_array_by_name(cdata, name, namelen);

      if (!send_data) {
        jcntrl_raise_argument_error(__FILE__, __LINE__,
                                    "Required variable is not defined");
        r = 0;
        break;
      }
    }

    recv_data = jcntrl_field_variable_array(fv_out);
    if (!recv_data) {
      jcntrl_double_array *p;
      p = jcntrl_double_array_new();
      if (!p) {
        r = 0;
        break;
      }

      recv_data = jcntrl_double_array_data(p);
      r = jcntrl_field_variable_set_array(fv_out, recv_data);

      /* disown the array, but delete if failed to transfer ownership */
      if (!jcntrl_shared_object_delete(jcntrl_data_array_object(recv_data)))
        recv_data = NULL;

      if (!r)
        break;
    }
    if (!jcntrl_data_array_resize(recv_data,
                                  jcntrl_size_array_get_ntuple(recv_indices))) {
      r = 0;
      break;
    }
  } while (0);

  if (p->controller) {
    if (!jcntrl_mpi_controller_all(p->controller, r))
      r = 0;
  }

  do {
    jcntrl_data_array *send_idxs;

    if (!r)
      break;

    send_idxs = jcntrl_size_array_data(send_indices);
    if (p->controller) {
      r = jcntrl_mpi_controller_allgathervi(p->controller, recv_data, recv_idxs,
                                            send_data, send_idxs);
    } else {
      r = jcntrl_data_array_copyidx(recv_data, send_data, recv_idxs[0],
                                    send_idxs);
    }
  } while (0);

  if (send_indices)
    jcntrl_size_array_delete(send_indices);
  if (recv_indices)
    jcntrl_size_array_delete(recv_indices);
  if (recv_indices_work)
    jcntrl_size_array_delete(recv_indices_work);
  if (recv_pnts)
    jcntrl_size_array_delete(recv_pnts);
  if (extents)
    jcntrl_int_array_delete(extents);
  if (recv_idxs) {
    for (int i = 0; i < nproc; ++i) {
      if (recv_idxs[i])
        jcntrl_data_array_delete(recv_idxs[i]);
    }
  }
  return r;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_get, jcntrl_executive, process_update_data)

static void jcntrl_fv_get_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_fv_get_downcast_v;
  p->initializer = jcntrl_fv_get_initializer;
  p->destructor = jcntrl_fv_get_destructor;
  p->allocator = jcntrl_fv_get_allocator;
  p->deleter = jcntrl_fv_get_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_get, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_get, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_get, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_fv_get, jcntrl_fv_get_init_func)

jcntrl_fv_get *jcntrl_fv_get_new(void)
{
  return jcntrl_shared_object_new(jcntrl_fv_get);
}

void jcntrl_fv_get_delete(jcntrl_fv_get *p)
{
  jcntrl_shared_object_delete(jcntrl_fv_get_object(p));
}

jcntrl_executive *jcntrl_fv_get_executive(jcntrl_fv_get *p) { return &p->exec; }

jcntrl_shared_object *jcntrl_fv_get_object(jcntrl_fv_get *p)
{
  return jcntrl_executive_object(jcntrl_fv_get_executive(p));
}

jcntrl_fv_get *jcntrl_fv_get_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_fv_get, obj);
}

void jcntrl_fv_get_set_controller(jcntrl_fv_get *p,
                                  const jcntrl_mpi_controller *controller)
{
  JCNTRL_ASSERT(p);
  p->controller = controller;
}

const jcntrl_mpi_controller *jcntrl_fv_get_get_controller(jcntrl_fv_get *p)
{
  JCNTRL_ASSERT(p);
  return p->controller;
}

int jcntrl_fv_get_set_varname_c(jcntrl_fv_get *p, const char *name)
{
  jcntrl_data_array *a;
  jcntrl_static_cstr_array ch;
  int len;
  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(p->varname);

  len = strlen(name);
  if (len < 0)
    len = 0;

  jcntrl_static_cstr_array_init_base(&ch, name, len);
  return jcntrl_fv_get_set_varname(p, jcntrl_static_cstr_array_data(&ch));
}

int jcntrl_fv_get_set_varname(jcntrl_fv_get *p, jcntrl_data_array *array)
{
  jcntrl_size_type len;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(p->varname);
  JCNTRL_ASSERT(array);

  len = jcntrl_data_array_get_ntuple(array);

  if (!jcntrl_char_array_resize(p->varname, len))
    return 0;

  if (len > 0) {
    if (!jcntrl_char_array_copy(p->varname, array, len, 0, 0))
      return 0;
  }
  return 1;
}

jcntrl_char_array *jcntrl_fv_get_get_varname(jcntrl_fv_get *p)
{
  return p->varname;
}

void jcntrl_fv_get_set_exclude_masked(jcntrl_fv_get *p, int exclude_masked)
{
  JCNTRL_ASSERT(p);

  p->exclude_masked = !!exclude_masked;
}

int jcntrl_fv_get_get_exclude_masked(jcntrl_fv_get *p)
{
  JCNTRL_ASSERT(p);

  return p->exclude_masked;
}

void jcntrl_fv_get_set_extract_extent(jcntrl_fv_get *p, int extract_extent)
{
  JCNTRL_ASSERT(p);

  p->extract_extent = !!extract_extent;
}

int jcntrl_fv_get_get_extract_extent(jcntrl_fv_get *p)
{
  JCNTRL_ASSERT(p);

  return p->extract_extent;
}

void jcntrl_fv_get_set_extent(jcntrl_fv_get *p, const int extent[6])
{
  JCNTRL_ASSERT(p);

  p->extent = jcntrl_extent_c(extent);
}

const int *jcntrl_fv_get_get_extent(jcntrl_fv_get *p)
{
  JCNTRL_ASSERT(p);

  return p->extent.extent;
}

int jcntrl_install_fv_get(void)
{
  return jcntrl_executive_install(JCNTRL_FV_GET, jcntrl_fv_get);
}
