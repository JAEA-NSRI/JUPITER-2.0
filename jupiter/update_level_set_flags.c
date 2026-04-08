
#include "update_level_set_flags.h"
#include "boundary_util.h"
#include "component_data.h"
#include "csv.h"
#include "csvutil.h"
#include "func.h"
#include "serializer/defs.h"
#include "serializer/msgpackx.h"
#include "serializer/util.h"
#include "struct.h"
#include "common_util.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

void update_level_set_flags_share_flag(update_level_set_flags *flags,
                                       mpi_param *mpi)
{
  CSVASSERT(flags);
  CSVASSERT(mpi);

#ifdef JUPITER_MPI
  if (mpi->npe > 1) {
    int f = update_level_set_flags_wants_update(flags);
    if (for_any_rank(mpi, f)) {
      int dist_rank = mpi->npe;
      update_level_set_reason res = flags->reason;

      if (f)
        dist_rank = mpi->rank;

      MPI_Allreduce(MPI_IN_PLACE, &dist_rank, 1, MPI_INT, MPI_MIN,
                    mpi->CommJUPITER);
      CSVASSERT(dist_rank < mpi->npe && dist_rank >= 0);

      MPI_Bcast(&res, sizeof(update_level_set_reason), MPI_CHAR, dist_rank,
                mpi->CommJUPITER);

      if (for_any_rank(mpi, res != flags->reason))
        res = UPDATE_LEVEL_SET_BY_NONE;

      if (!f)
        update_level_set_flags_mark_update(flags, res);
    }
  }
#endif
}

const char *
update_level_set_flags_get_reason_str(update_level_set_reason reason)
{
  switch(reason)
  {
  case UPDATE_LEVEL_SET_BY_INITIAL_VOF:
    return "there are non-0 initial VOF value";
  case UPDATE_LEVEL_SET_BY_PHASE_CHANGE:
    return "phase change model is enabled";
  case UPDATE_LEVEL_SET_BY_RESTART:
    return "restart data requires update";
  case UPDATE_LEVEL_SET_BY_INPUT:
    return "it's set to be updated forcibly";
  case UPDATE_LEVEL_SET_BY_CONTROLLED_VOF:
    return "there are non-0 VOF value after controlled change";
  case UPDATE_LEVEL_SET_BY_LIQUID_INLET:
    return "there are inlet condition(s) of liquids";
  case UPDATE_LEVEL_SET_BY_NONE:
    return "of unknown reason";
  }

  CSVUNREACHABLE();
  return NULL;
}

static void *update_level_set_flags_describer_data = NULL;
static update_level_set_flags_describer_func_type
  *update_level_set_flags_describer_func = NULL;

void update_level_set_flags_set_describe_reason_func(
  update_level_set_flags_describer_func_type *const func, void *args)
{
  update_level_set_flags_describer_data = args;
  update_level_set_flags_describer_func = func;
}

void update_level_set_flags_describe_reason(
  const update_level_set_flags *flags)
{
  if (update_level_set_flags_describer_func) {
    update_level_set_flags_describer_func(
      update_level_set_flags_describer_data, flags, flags->reason);
  }
}

struct fl_finder_data
{
  type *fl;
};

static int fl_finder(ptrdiff_t jj, void *arg)
{
  struct fl_finder_data *d = (struct fl_finder_data *)arg;
  return d->fl[jj] != 0.0;
}

int update_level_set_flags_if_fl_exists(type *fl, domain *cdo, mpi_param *mpi)
{
  ptrdiff_t jj;
  struct fl_finder_data data = {.fl = fl};

  CSVASSERT(fl);
  CSVASSERT(cdo);
  CSVASSERT(mpi);

  jj = struct_domain_find_if(cdo->mx, cdo->my, cdo->mz,    //
                             cdo->stm, cdo->stm, cdo->stm, //
                             cdo->stp, cdo->stp, cdo->stp, fl_finder, &data);

  if (for_any_rank(mpi, jj >= 0)) {
    return 1;
  }
  return 0;
}

int update_level_set_flags_if_liquid_inlet_exists(
  fluid_boundary_data *boundary_list_head)
{
  fluid_boundary_data *fb = fluid_boundary_data_next(boundary_list_head);
  for (; fb != boundary_list_head; fb = fluid_boundary_data_next(fb)) {
    int ncomp;
    if (fb->cond != INLET)
      continue;

    ncomp = 0;
    if (fb->comps)
      ncomp = inlet_component_data_ncomp(fb->comps);

    if (ncomp <= 0)
      continue;

    for (int i = 0; i < ncomp; ++i) {
      struct inlet_component_element *e;
      e = inlet_component_data_get(fb->comps, i);
      if (!component_phases_has_liquid(e->comp.d->phases))
        continue;

      return 1;
    }
  }
  return 0;
}

#define UPLSFLG_KEYNAME "update-level-set-flags"

static msgpackx_data *update_level_set_flags_readc(const char *file_name,
                                                   msgpackx_map_node **map_node,
                                                   msgpackx_error *err,
                                                   ptrdiff_t *eloc)
{
  msgpackx_error xerr = MSGPACKX_SUCCESS;
  msgpackx_data *data;
  msgpackx_map_node *mnode = NULL;

  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    if (err)
      *err = MSGPACKX_ERR_SYS;
    return NULL;
  }

  data = msgpackx_data_read(fp, err, eloc);
  fclose(fp);

  if (!data)
    return data;

  do {
    msgpackx_node *node;
    msgpackx_array_node *anode;

    anode = msgpackx_read_header_data(data, UPLSFLG_KEYNAME, NULL, &xerr);
    if (!anode) {
      if (eloc)
        *eloc = 0;
      break;
    }

    node = msgpackx_array_node_get_child_node(anode);
    mnode = msgpackx_node_get_map(node);
    if (!mnode) {
      if (eloc) {
        const void *p1, *p2;
        p1 = msgpackx_node_get_data_pointer(msgpackx_data_root_node(data),
                                            NULL);
        if (node) {
          p2 = msgpackx_node_get_data_pointer(node, NULL);
        } else {
          p2 = msgpackx_node_get_data_pointer(msgpackx_array_node_upcast(anode),
                                              NULL);
        }
        *eloc = (const char *)p2 - (const char*)p1;
      }
      break;
    }
  } while (0);

  if (!mnode) {
    if (err)
      *err = xerr;
    msgpackx_data_delete(data);
    return NULL;
  }

  if (map_node)
    *map_node = mnode;

  return data;
}

static int update_level_set_flags_seth(msgpackx_map_node *map_head,
                                       const char *keyname,
                                       update_level_set_flags *flags)
{
  msgpackx_error err;
  int flg;

  flg = !!update_level_set_flags_wants_update(flags);
  err = msgpackx_map_skey_set_bool(map_head, keyname, strlen(keyname), flg);

  if (err != MSGPACKX_SUCCESS) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return 1;
  }
  return 0;
}

int update_level_set_flags_write(const char *file_name, const char *keyname,
                                 update_level_set_flags *flags)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_data *data = NULL;
  msgpackx_node *node;
  msgpackx_array_node *ahead;
  msgpackx_array_node *anode;
  msgpackx_map_node *mnode = NULL;
  FILE *fp;

  data = update_level_set_flags_readc(file_name, &mnode, NULL, NULL);
  if (!data) {
    msgpackx_array_node *an;

    data = msgpackx_data_new();
    if (!data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return 1;
    }

    anode = msgpackx_make_header_data(data, UPLSFLG_KEYNAME, &ahead, &err);
    if (err != MSGPACKX_SUCCESS)
      goto error;

    do {
      an = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      if (!an) {
        err = MSGPACKX_ERR_NOMEM;
        break;
      }

      mnode = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
      if (!mnode) {
        err = MSGPACKX_ERR_NOMEM;
        break;
      }
    } while (0);
    if (err != MSGPACKX_SUCCESS)
      goto error;

    msgpackx_array_node_set_child_node(an, msgpackx_map_node_upcast(mnode));
    msgpackx_array_node_insert_prev(anode, an);
  }

  CSVASSERT(data);
  if (update_level_set_flags_seth(mnode, keyname, flags)) {
    msgpackx_data_delete(data);
    return 1;
  }

  errno = 0;
  fp = fopen(file_name, "wb");
  if (!fp) {
    if (errno != 0) {
      csvperror(file_name, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0,
                NULL);
    } else {
      csvperror(file_name, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
    }
    msgpackx_data_delete(data);
    return 1;
  }

  msgpackx_data_write(data, fp);
  fclose(fp);

  msgpackx_data_delete(data);
  return 0;

error:
  csvperror(file_name, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0, err,
            NULL);
  if (data)
    msgpackx_data_delete(data);
  return 1;
}

static int update_level_set_flags_geth(msgpackx_map_node *map_head,
                                       const char *keyname,
                                       update_level_set_flags *flags,
                                       msgpackx_error *err)
{
  msgpackx_error xerr = MSGPACKX_SUCCESS;
  int f;

  f = msgpackx_map_skey_get_bool(map_head, keyname, strlen(keyname), &xerr);
  if (xerr != MSGPACKX_SUCCESS) {
    if (err)
      *err = xerr;
    return 1;
  }

  /* If false, flags will not be changed (not set to false) */
  if (f)
    update_level_set_flags_mark_update(flags, UPDATE_LEVEL_SET_BY_RESTART);
  return 0;
}

int update_level_set_flags_read(const char *file_name, const char *keyname,
                                update_level_set_flags *flags,
                                msgpackx_error *err, ptrdiff_t *eloc)
{
  msgpackx_map_node *mnode;
  msgpackx_data *data;
  int r;

  data = update_level_set_flags_readc(file_name, &mnode, err, eloc);
  if (!data) {
    return 1;
  }

  r = update_level_set_flags_geth(mnode, keyname, flags, err);
  msgpackx_data_delete(data);
  return r;
}
