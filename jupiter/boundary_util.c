
#include "component_data.h"
#include "component_data_defs.h"
#include "component_info.h"

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "csv.h"
#include "control/defs.h"
#include "serializer/defs.h"
#include "serializer/util.h"
#include "boundary_util.h"
#include "common_util.h"
#include "struct.h"
#include "csvutil.h"
#include "csvutil_extra.h"
#include "field_control.h"
#include "geometry/common.h"
#include "geometry/list.h"
#include "serializer/buffer.h"
#include "serializer/msgpackx.h"
#include "serializer/byteswap.h"

#define fluid_boundary_data_from_meta(ptr)                                     \
  geom_container_of(ptr, struct fluid_boundary_data, meta)

#define thermal_boundary_data_from_meta(ptr)                                   \
  geom_container_of(ptr, struct thermal_boundary_data, meta)

struct inlet_component_data *inlet_component_data_new(int ncomp)
{
  struct inlet_component_data *ptr;
  size_t sz;

  sz = sizeof(struct inlet_component_data);
  if (ncomp > 0) {
    sz += sizeof(struct inlet_component_element) * ncomp;
  }

  ptr = (struct inlet_component_data *)malloc(sz);
  if (!ptr)
    return NULL;

  ptr->ncomp = ncomp;
  if (ncomp > 0) {
    for (int i = 0; i < ncomp; ++i) {
      component_info_data_init(&ptr->array[i].comp);
      controllable_type_init(&ptr->array[i].ratio);
    }
  }
  return ptr;
}

void inlet_component_data_delete(struct inlet_component_data *data)
{
  if (!data)
    return;

  if (data->ncomp > 0) {
    for (int i = 0; i < data->ncomp; ++i) {
      controllable_type_remove_from_list(&data->array[i].ratio);
    }
  }
  free(data);
}

struct inlet_component_data *
inlet_component_data_dup(struct inlet_component_data *src)
{
  struct inlet_component_data *dest;
  int ncomp;

  CSVASSERT(src);

  ncomp = inlet_component_data_ncomp(src);
  dest = inlet_component_data_new(ncomp);
  if (!dest)
    return NULL;

  for (int i = 0; i < ncomp; ++i) {
    dest->array[i].comp = src->array[i].comp;
    controllable_type_copy(&dest->array[i].ratio, &src->array[i].ratio);
  }
  return dest;
}

void inlet_component_data_remove_from_list(struct inlet_component_data *list)
{
  int ncomp;
  CSVASSERT(list);

  ncomp = inlet_component_data_ncomp(list);
  for (int i = 0; i < ncomp; ++i) {
    controllable_type_remove_from_list(&list->array[i].ratio);
  }
}

void inlet_component_data_add_to_list(struct inlet_component_data *list,
                                      controllable_type *control_head)
{
  int ncomp;
  CSVASSERT(list);

  ncomp = inlet_component_data_ncomp(list);
  for (int i = 0; i < ncomp; ++i) {
    controllable_type_remove_from_list(&list->array[i].ratio);
    controllable_type_add_to_list_if_needed(control_head,
                                            &list->array[i].ratio);
  }
}

boundary_direction boundary_direction_normalize(boundary_direction dir)
{
  if (dir & BOUNDARY_DIR_ALL || dir & BOUNDARY_DIR_X) {
    dir |= BOUNDARY_DIR_WEST;
    dir |= BOUNDARY_DIR_EAST;
  }
  if (dir & BOUNDARY_DIR_ALL || dir & BOUNDARY_DIR_Y) {
    dir |= BOUNDARY_DIR_SOUTH;
    dir |= BOUNDARY_DIR_NORTH;
  }
  if (dir & BOUNDARY_DIR_ALL || dir & BOUNDARY_DIR_Z) {
    dir |= BOUNDARY_DIR_BOTTOM;
    dir |= BOUNDARY_DIR_TOP;
  }
  return dir;
}

static void boundary_meta_data_init(struct boundary_meta_data *meta)
{
  meta->id = -1;
  meta->used = 0;
}

void fluid_boundary_data_init(fluid_boundary_data *data)
{
  CSVASSERT(data);

  data->comps = NULL;
  data->cond = BOUNDARY_MPI;
  data->control = TRIP_CONTROL_INVALID;
  data->out_p_cond = OUT_P_COND_INVALID;
  controllable_type_init(&data->inlet_vel_u);
  controllable_type_init(&data->inlet_vel_v);
  controllable_type_init(&data->inlet_vel_w);
  controllable_type_init(&data->const_p);
  boundary_meta_data_init(&data->meta);
  geom_list_init(&data->list);
}

fluid_boundary_data *fluid_boundary_data_new(fluid_boundary_data *head)
{
  fluid_boundary_data *n;

  CSVASSERT(head);

  n = (fluid_boundary_data *)malloc(sizeof(fluid_boundary_data));
  if (!n)
    return NULL;

  fluid_boundary_data_init(n);
  geom_list_insert_prev(&head->list, &n->list);

  return n;
}

void fluid_boundary_data_delete(fluid_boundary_data *data)
{
  if (!data)
    return;

  geom_list_delete(&data->list);
  controllable_type_remove_from_list(&data->inlet_vel_u);
  controllable_type_remove_from_list(&data->inlet_vel_v);
  controllable_type_remove_from_list(&data->inlet_vel_w);
  controllable_type_remove_from_list(&data->const_p);
  inlet_component_data_delete(data->comps);
  free(data);
}

void fluid_boundary_data_delete_all(fluid_boundary_data *head)
{
  struct geom_list *p, *n, *h;
  fluid_boundary_data *fp;

  CSVASSERT(head);

  h = &head->list;
  geom_list_foreach_safe (p, n, h) {
    fp = fluid_boundary_entry(p);
    fluid_boundary_data_delete(fp);
  }
}

fluid_boundary_data *fluid_boundary_data_next(fluid_boundary_data *data)
{
  return fluid_boundary_entry(geom_list_next(&data->list));
}

fluid_boundary_data *fluid_boundary_data_prev(fluid_boundary_data *data)
{
  return fluid_boundary_entry(geom_list_next(&data->list));
}

void fluid_boundary_data_move(fluid_boundary_data *data,
                              fluid_boundary_data *new_head)
{
  CSVASSERT(data);
  CSVASSERT(new_head);
  geom_list_delete(&data->list);
  geom_list_insert_prev(&new_head->list, &data->list);
}

fluid_boundary_data *
fluid_boundary_set_by_vof(int nx, int ny, int nz,
                          fluid_boundary_data *fl_data,
                          type threshold, fluid_boundary_data **dest, type *vof)
{
  int jx, jy, jz;

  CSVASSERT(dest);
  CSVASSERT(vof);
  CSVASSERT(nx > 0);
  CSVASSERT(ny > 0);
  CSVASSERT(nz > 0);
  CSVASSERT(fl_data);

#pragma omp parallel for collapse(3)
  for (jz = 0; jz < nz; ++jz) {
    for (jy = 0; jy < ny; ++jy) {
      for (jx = 0; jx < nx; ++jx) {
        ptrdiff_t jj;
        type v;
        jj = calc_address(jx, jy, jz, nx, ny, nz);
        v = vof[jj];
        if (threshold < 0.0) {
          if (v >= -threshold) {
            continue;
          }
        } else if (threshold == 0.0) {
          if (v <= 0.0) {
            continue;
          }
        } else {
          if (v < threshold) {
            continue;
          }
        }

        dest[jj] = fl_data;
      }
    }
  }

  return fl_data;
}

void thermal_boundary_data_init(thermal_boundary_data *data)
{
  CSVASSERT(data);

  data->cond = BOUNDARY_MPI;
  data->control = TRIP_CONTROL_INVALID;
  controllable_type_init(&data->temperature);
  data->diffusion_limit = 0.0;
  boundary_meta_data_init(&data->meta);
  geom_list_init(&data->list);
}

thermal_boundary_data *thermal_boundary_data_new(thermal_boundary_data *head)
{
  thermal_boundary_data *n;

  CSVASSERT(head);

  n = (thermal_boundary_data *)malloc(sizeof(thermal_boundary_data));
  if (!n)
    return NULL;

  thermal_boundary_data_init(n);
  geom_list_insert_prev(&head->list, &n->list);

  return n;
}

static void thermal_boundary_data_free(thermal_boundary_data *data)
{
  CSVASSERT(data);

  free(data);
}

void thermal_boundary_data_delete(thermal_boundary_data *data)
{
  if (data) {
    geom_list_delete(&data->list);
    thermal_boundary_data_free(data);
  }
}

void thermal_boundary_data_delete_all(thermal_boundary_data *head)
{
  struct geom_list *p, *n, *h;
  thermal_boundary_data *fp;

  CSVASSERT(head);

  h = &head->list;
  geom_list_foreach_safe (p, n, h) {
    fp = thermal_boundary_entry(p);
    thermal_boundary_data_free(fp);
  }
}

thermal_boundary_data *thermal_boundary_data_next(thermal_boundary_data *data)
{
  return thermal_boundary_entry(geom_list_next(&data->list));
}

thermal_boundary_data *thermal_boundary_data_prev(thermal_boundary_data *data)
{
  return thermal_boundary_entry(geom_list_prev(&data->list));
}

void thermal_boundary_data_move(thermal_boundary_data *data,
                                thermal_boundary_data *new_head)
{
  geom_list_delete(&data->list);
  geom_list_insert_prev(&new_head->list, &data->list);
}

thermal_boundary_data *
thermal_boundary_set_by_vof(int nx, int ny, int nz,
                            thermal_boundary_data *th_data, type threshold,
                            thermal_boundary_data **dest, type *vof)
{
  int jx, jy, jz;

  CSVASSERT(dest);
  CSVASSERT(vof);
  CSVASSERT(nx > 0);
  CSVASSERT(ny > 0);
  CSVASSERT(nz > 0);
  CSVASSERT(th_data);

#pragma omp parallel for collapse(3)
  for (jz = 0; jz < nz; ++jz) {
    for (jy = 0; jy < ny; ++jy) {
      for (jx = 0; jx < nx; ++jx) {
        ptrdiff_t jj;
        type v;
        jj = calc_address(jx, jy, jz, nx, ny, nz);
        v = vof[jj];
        if (threshold < 0.0) {
          if (v >= -threshold) {
            continue;
          }
        } else if (threshold == 0.0) {
          if (v <= 0.0) {
            continue;
          }
        } else {
          if (v < threshold) {
            continue;
          }
        }

        dest[jj] = th_data;
      }
    }
  }

  return th_data;
}

void surface_boundary_data_init(surface_boundary_data *data)
{
  CSVASSERT(data);

  data->cond = BOUNDARY_MPI;
  data->comps = NULL;
  data->control = TRIP_CONTROL_INVALID;
  data->inlet_dir = SURFACE_INLET_DIR_INVALID;
  boundary_meta_data_init(&data->meta);
  controllable_type_init(&data->normal_inlet_vel);
  geom_list_init(&data->list);
}

surface_boundary_data *surface_boundary_data_new(surface_boundary_data *head)
{
  surface_boundary_data *p;

  p = (surface_boundary_data *)malloc(sizeof(surface_boundary_data));
  if (!p)
    return NULL;

  surface_boundary_data_init(p);
  geom_list_insert_prev(&head->list, &p->list);
  return p;
}

void surface_boundary_data_delete(surface_boundary_data *data)
{
  if (!data)
    return;

  geom_list_delete(&data->list);
  controllable_type_remove_from_list(&data->normal_inlet_vel);
  inlet_component_data_delete(data->comps);
  free(data);
}

void surface_boundary_data_delete_all(surface_boundary_data *head)
{
  struct geom_list *p, *n, *h;
  surface_boundary_data *sp;

  CSVASSERT(head);

  h = &head->list;
  geom_list_foreach_safe(p, n, h) {
    sp = surface_boundary_entry(p);
    surface_boundary_data_delete(sp);
  }
}

surface_boundary_data *surface_boundary_data_next(surface_boundary_data *p)
{
  CSVASSERT(p);
  return surface_boundary_entry(geom_list_next(&p->list));
}

surface_boundary_data *surface_boundary_data_prev(surface_boundary_data *p)
{
  CSVASSERT(p);
  return surface_boundary_entry(geom_list_prev(&p->list));
}

void surface_boundary_data_move(surface_boundary_data *data,
                                surface_boundary_data *new_head)
{
  CSVASSERT(data);
  CSVASSERT(new_head);
  geom_list_delete(&data->list);
  geom_list_insert_prev(&new_head->list, &data->list);
}

/* Dumping functions */
static ptrdiff_t boundary_data_bin_unit_size(ptrdiff_t ncond)
{
  uintmax_t um;
  if (ncond <= 0)
    return -1;

  um = ncond;
  if (um <= UINT8_MAX)
    return sizeof(uint8_t);
  if (um <= UINT16_MAX)
    return sizeof(uint16_t);
  if (um <= UINT32_MAX)
    return sizeof(uint32_t);
  if (um <= UINT64_MAX)
    return sizeof(uint64_t);
  return -1;
}

static msgpackx_map_node *
boundary_data_make_map_node_of_keystr(const char *string)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  if (!msgpackx_node_set_str(node, string, strlen(string), &err)) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  if (!mnode) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  if (!msgpackx_map_node_set_key(mnode, node, &err)) {
    msgpackx_node_delete(node);
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  return mnode;
}

static msgpackx_map_node *boundary_data_make_ss_pair(const char *key,
                                                     const char *value)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  if (!msgpackx_node_set_str(node, value, strlen(value), &err)) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = boundary_data_make_map_node_of_keystr(key);
  if (!mnode) {
    msgpackx_node_delete(node);
    return NULL;
  }

  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_map_node *boundary_data_make_si_pair(const char *key,
                                                     intmax_t value)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  if (!msgpackx_node_set_int(node, value, &err)) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = boundary_data_make_map_node_of_keystr(key);
  if (!mnode) {
    msgpackx_node_delete(node);
    return NULL;
  }

  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_map_node *boundary_data_make_sd_pair(const char *key,
                                                     double value)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  if (!msgpackx_node_set_double(node, value, &err)) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = boundary_data_make_map_node_of_keystr(key);
  if (!mnode) {
    msgpackx_node_delete(node);
    return NULL;
  }

  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_map_node *boundary_data_dump(
  void *data, const char *type, int cond,
  msgpackx_error (*delegator)(void *data, msgpackx_map_node *mhead))
{
  msgpackx_error err;
  msgpackx_map_node *mhead, *mnode;
  const char *str;

  switch (cond) {
  case WALL:
    str = "WALL";
    break;
  case SLIP:
    str = "SLIP";
    break;
  case OUT:
    str = "OUT";
    break;
  case INLET:
    str = "IN";
    break;
  case INSULATION:
    str = "INSULATION";
    break;
  case ISOTHERMAL:
    str = "ISOTHERMAL";
    break;
  case DIFFUSION:
    str = "DIFFUSION";
    break;
  case BOUNDARY_MPI:
    str = "MPI";
    break;
  default:
    str = "(unknown condition)";
    break;
  }

  mnode = boundary_data_make_ss_pair(type, str);
  if (!mnode) {
    return NULL;
  }

  mhead = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  if (!mhead) {
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
    msgpackx_map_node_delete_all(mhead);
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  if (data) {
    err = delegator(data, mhead);
    if (err != MSGPACKX_SUCCESS) {
      msgpackx_map_node_delete_all(mhead);
      return NULL;
    }
  }

  return mhead;
}

static void boundary_array_set_used(struct boundary_array *arr, int nbx,
                                    int nby)
{
  int i, j;

#pragma omp for collapse(2)
  for (j = 0; j < nby; ++j) {
    for (i = 0; i < nbx; ++i) {
      ptrdiff_t jj;
      fluid_boundary_data *f;
      thermal_boundary_data *t;

      jj = calc_address(i, j, 0, nbx, nby, 1);
      f = arr->fl[jj];
      if (f) {
#pragma omp atomic write
        f->meta.id = -1;
#pragma omp atomic write
        f->meta.used = 1;
      }
      t = arr->th[jj];
      if (t) {
#pragma omp atomic write
        t->meta.id = -1;
#pragma omp atomic write
        t->meta.used = 1;
      }
    }
  }
}

static msgpackx_array_node *
boundary_data_make_id_bin_array(void *buf, struct boundary_array *arr, int nbx,
                                int nby, ptrdiff_t usz, int th)
{
  msgpackx_array_node *anode;
  msgpackx_node *node;
  uint8_t *u8p;
  uint16_t *u16p;
  uint32_t *u32p;
  uint64_t *u64p;
  ptrdiff_t msz;
  msgpackx_error err;
  ptrdiff_t ii, jj;

  u8p = buf;
  u16p = buf;
  u32p = buf;
  u64p = buf;

  if (arr) {
#pragma omp parallel for collapse(2)
    for (jj = 0; jj < nby; ++jj) {
      for (ii = 0; ii < nbx; ++ii) {
        ptrdiff_t ij;
        uintmax_t val;

        ij = calc_address(ii, jj, 0, nbx, nby, 0);
        if (!th) {
          val = arr->fl[ij]->meta.id;
        } else {
          val = arr->th[ij]->meta.id;
        }

        switch (usz) {
        case sizeof(uint8_t):
          u8p[ij] = val;
          break;
        case sizeof(uint16_t):
          u16p[ij] = val;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
          u16p[ij] = msgpackx_byteswap_2(u16p[ij]);
#endif
          break;
        case sizeof(uint32_t):
          u32p[ij] = val;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
          u32p[ij] = msgpackx_byteswap_4(u32p[ij]);
#endif
          break;
        case sizeof(uint64_t):
          u64p[ij] = val;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
          u64p[ij] = msgpackx_byteswap_8(u64p[ij]);
#endif
          break;
        }
      }
    }
  }

  err = MSGPACKX_SUCCESS;
  anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  node = msgpackx_node_new();
  if (!anode || !node) {
    if (anode)
      msgpackx_array_node_delete(anode);
    if (node)
      msgpackx_node_delete(node);
    free(buf);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    goto error;
  }

  msgpackx_array_node_set_child_node(anode, node);

  if (arr) {
    msz = usz * nbx * nby;
    msgpackx_node_set_bin(node, buf, msz, &err);
  } else {
    msgpackx_node_set_nil(node, &err);
  }
  if (err != MSGPACKX_SUCCESS) {
    free(buf);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    goto error;
  }

  return anode;

error:
  msgpackx_array_node_delete(anode);
  msgpackx_node_delete(node);
  return NULL;
}

static msgpackx_map_node *
fluid_boundary_make_vin_node(fluid_boundary_data *data)
{
  int i;
  controllable_type *const_vel[3];
  msgpackx_node *node;
  msgpackx_map_node *mnode;
  msgpackx_array_node *ahead;
  msgpackx_array_node *anode;

  ahead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  if (!ahead) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  mnode = boundary_data_make_map_node_of_keystr("velocity-in");
  if (!mnode) {
    msgpackx_array_node_delete(ahead);
    return NULL;
  }

  node = msgpackx_array_node_upcast(ahead);
  msgpackx_map_node_set_value(mnode, node);

  const_vel[0] = &data->inlet_vel_u;
  const_vel[1] = &data->inlet_vel_v;
  const_vel[2] = &data->inlet_vel_w;

  for (i = 0; i < 3; ++i) {
    msgpackx_map_node *vmhead;

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!anode) {
      msgpackx_map_node_delete(mnode);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return NULL;
    }

    vmhead = controllable_type_to_msgpackx(const_vel[i]);
    if (!vmhead) {
      msgpackx_array_node_delete(anode);
      msgpackx_map_node_delete(mnode);
      return NULL;
    }

    node = msgpackx_map_node_upcast(vmhead);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);
  }

  return mnode;
}

msgpackx_map_node *
inlet_component_element_to_msgpackx(struct inlet_component_element *element)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mhead;
  msgpackx_map_node *mnode;
  msgpackx_map_node *ratio_head;

  CSVASSERT(element);

  mhead = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  if (!mhead) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  mnode = boundary_data_make_si_pair("matid", element->comp.d->comp_index);
  if (!mnode) {
    msgpackx_map_node_delete_all(mhead);
    return NULL;
  }

  if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
    msgpackx_map_node_delete(mnode);
    msgpackx_map_node_delete_all(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = boundary_data_make_map_node_of_keystr("ratio");
  ratio_head = controllable_type_to_msgpackx(&element->ratio);
  if (!mnode || !ratio_head) {
    msgpackx_map_node_delete_all(mhead);
    if (mnode)
      msgpackx_map_node_delete(mnode);
    if (ratio_head)
      msgpackx_map_node_delete_all(ratio_head);
    return NULL;
  }

  node = msgpackx_map_node_upcast(ratio_head);
  msgpackx_map_node_set_value(mnode, node);

  if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
    msgpackx_map_node_delete_all(mhead);
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  return mhead;
}

msgpackx_array_node *
inlet_component_data_to_msgpackx(struct inlet_component_data *data)
{
  int ncomp;
  msgpackx_node *node;
  msgpackx_array_node *ahead;
  msgpackx_array_node *anode;

  ahead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  if (!ahead) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  ncomp = inlet_component_data_ncomp(data);
  if (ncomp <= 0)
    return ahead;

  for (int i = 0; i < ncomp; ++i) {
    msgpackx_map_node *melement;

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!anode) {
      msgpackx_array_node_delete_all(ahead);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return NULL;
    }

    melement = inlet_component_element_to_msgpackx(&data->array[i]);
    if (!melement) {
      msgpackx_array_node_delete_all(anode);
      return NULL;
    }

    node = msgpackx_map_node_upcast(melement);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);
  }

  return ahead;
}

static msgpackx_map_node *
fluid_boundary_make_comps_node(fluid_boundary_data *data)
{
  int i;
  msgpackx_node *node;
  msgpackx_map_node *mnode;
  msgpackx_array_node *ahead;
  msgpackx_array_node *anode;

  mnode = boundary_data_make_map_node_of_keystr("inlet-vof");
  if (!mnode) {
    return NULL;
  }

  ahead = inlet_component_data_to_msgpackx(data->comps);
  if (!ahead) {
    msgpackx_map_node_delete(mnode);
    return NULL;
  }

  node = msgpackx_array_node_upcast(ahead);
  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_error fluid_boundary_dump_core(void *data,
                                               msgpackx_map_node *mhead)
{
  long line;
  msgpackx_error err;
  msgpackx_map_node *mnode;

  fluid_boundary_data *fdata;
  fdata = (fluid_boundary_data *)data;

  err = MSGPACKX_SUCCESS;

  if (fdata->cond == INLET) {
    const char *ctrl_str;
    ptrdiff_t i;

    switch (fdata->control) {
    case TRIP_CONTROL_CONST:
      ctrl_str = "CONST";
      break;
    case TRIP_CONTROL_CONTROL:
      ctrl_str = "CONTROL";
      break;
    case TRIP_CONTROL_PULSE:
      ctrl_str = "PULSE";
      break;
    case TRIP_CONTROL_INVALID:
      ctrl_str = "(invalid)";
      break;
    }

    mnode = boundary_data_make_ss_pair("inlet-control", ctrl_str);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      return err;
    }

    mnode = fluid_boundary_make_vin_node(fdata);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      msgpackx_map_node_delete(mnode);
      return err;
    }

    mnode = fluid_boundary_make_comps_node(fdata);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      msgpackx_map_node_delete(mnode);
      return err;
    }
  } else {
    /* No additional data */
  }
  return MSGPACKX_SUCCESS;
}

static msgpackx_map_node *fluid_boundary_dump(fluid_boundary_data *fdata,
                                              msgpackx_error *err)
{
  return boundary_data_dump(fdata, "fluid", fdata->cond,
                            fluid_boundary_dump_core);
}

static msgpackx_map_node *
thermal_boundary_make_wall_temp(thermal_boundary_data *tdata)
{
  msgpackx_node *node;
  msgpackx_map_node *mnode;
  msgpackx_map_node *wallt_head;

  mnode = boundary_data_make_map_node_of_keystr("wall-t");
  if (!mnode) {
    return NULL;
  }

  wallt_head = controllable_type_to_msgpackx(&tdata->temperature);
  if (!wallt_head) {
    msgpackx_map_node_delete(mnode);
    return NULL;
  }

  node = msgpackx_map_node_upcast(wallt_head);
  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_map_node *
thermal_boundary_make_diff_limit(thermal_boundary_data *tdata)
{
  return boundary_data_make_sd_pair("diffusion-min-temp",
                                    tdata->diffusion_limit);
}

static msgpackx_error thermal_boundary_dump_core(void *data,
                                                 msgpackx_map_node *mhead)
{
  long line;
  msgpackx_error err;
  msgpackx_map_node *mnode;
  msgpackx_node *node;

  thermal_boundary_data *tdata;
  tdata = (thermal_boundary_data *)data;

  err = MSGPACKX_SUCCESS;
  mnode = NULL;
  node = NULL;

  if (tdata->cond == ISOTHERMAL) {
    const char *ctrl_str;

    switch (tdata->control) {
    case TRIP_CONTROL_CONST:
      ctrl_str = "CONST";
      break;
    case TRIP_CONTROL_CONTROL:
      ctrl_str = "CONTROL";
      break;
    case TRIP_CONTROL_PULSE:
      ctrl_str = "PULSE";
      break;
    case TRIP_CONTROL_INVALID:
      ctrl_str = "(invalid)";
      break;
    }

    mnode = boundary_data_make_ss_pair("wall-t-control", ctrl_str);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      msgpackx_map_node_delete(mnode);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      return err;
    }

    mnode = thermal_boundary_make_wall_temp(tdata);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      msgpackx_map_node_delete(mnode);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      return err;
    }

  } else if (tdata->cond == DIFFUSION) {
    mnode = thermal_boundary_make_diff_limit(tdata);
    if (!mnode) {
      return MSGPACKX_ERR_NOMEM;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, &err)) {
      msgpackx_map_node_delete(mnode);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      return err;
    }
  } else {
    /* No additional data */
  }

  return MSGPACKX_SUCCESS;
}

static msgpackx_map_node *thermal_boundary_dump(thermal_boundary_data *tdata,
                                                msgpackx_error *err)
{
  return boundary_data_dump(tdata, "thermal", tdata->cond,
                            thermal_boundary_dump_core);
}

msgpackx_data *boundary_data_array_dump(fluid_boundary_data *fl_head,
                                        thermal_boundary_data *th_head, int nbx,
                                        int nby, int nbz,
                                        struct boundary_array bnd_W[nby * nbz],
                                        struct boundary_array bnd_E[nby * nbz],
                                        struct boundary_array bnd_S[nbx * nbz],
                                        struct boundary_array bnd_N[nbx * nbz],
                                        struct boundary_array bnd_B[nbx * nby],
                                        struct boundary_array bnd_T[nbx * nby])
{
  msgpackx_array_node *ahead, *anode;
  struct geom_list *lp;
  fluid_boundary_data *fdata;
  thermal_boundary_data *tdata;
  ptrdiff_t np;
  int ret;
  msgpackx_data *data;

  CSVASSERT(fl_head);
  CSVASSERT(th_head);
  CSVASSERT(nbx > 0);
  CSVASSERT(nby > 0);
  CSVASSERT(nbz > 0);

  data = msgpackx_data_new();
  if (!data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  fl_head->meta.used = 0;
  geom_list_foreach (lp, &fl_head->list) {
    fdata = fluid_boundary_entry(lp);
    fdata->meta.used = 0;
  }

  th_head->meta.used = 0;
  geom_list_foreach (lp, &th_head->list) {
    tdata = thermal_boundary_entry(lp);
    tdata->meta.used = 0;
  }

#pragma omp parallel
  {
    if (bnd_W)
      boundary_array_set_used(bnd_W, nby, nbz);
    if (bnd_E)
      boundary_array_set_used(bnd_E, nby, nbz);
    if (bnd_S)
      boundary_array_set_used(bnd_S, nbx, nbz);
    if (bnd_N)
      boundary_array_set_used(bnd_N, nbx, nbz);
    if (bnd_B)
      boundary_array_set_used(bnd_B, nbx, nby);
    if (bnd_T)
      boundary_array_set_used(bnd_T, nbx, nby);
  }

  np = 0;
  geom_list_foreach (lp, &fl_head->list) {
    fdata = fluid_boundary_entry(lp);
    if (fdata->meta.used) {
      fdata->meta.id = np;
      ++np;
    }
  }

  geom_list_foreach (lp, &th_head->list) {
    tdata = thermal_boundary_entry(lp);
    if (tdata->meta.used) {
      tdata->meta.id = np;
      ++np;
    }
  }
  if (fl_head->meta.used) {
    fl_head->meta.id = np;
    np++;
  }
  if (th_head->meta.used) {
    th_head->meta.id = np;
    np++;
  }

  {
    msgpackx_array_node *asubhead;
    msgpackx_node *node;
    msgpackx_error err;
    int8_t bom[3];
    int i;

    err = MSGPACKX_SUCCESS;

    ahead = msgpackx_make_header_data(data, NULL, NULL, &err);
    if (!ahead) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      goto error;
    }

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    asubhead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
    if (!anode || !asubhead) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (asubhead)
        msgpackx_array_node_delete(asubhead);
      if (anode)
        msgpackx_array_node_delete(anode);
      goto error;
    }

    node = msgpackx_array_node_upcast(asubhead);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);

    /* Add to sub array */
    ahead = asubhead;

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    asubhead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
    if (!anode || !asubhead) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (asubhead)
        msgpackx_array_node_delete(asubhead);
      if (anode)
        msgpackx_array_node_delete(anode);
      goto error;
    }

    node = msgpackx_array_node_upcast(asubhead);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);

    geom_list_foreach (lp, &fl_head->list) {
      msgpackx_map_node *mhead;

      fdata = fluid_boundary_entry(lp);
      if (!fdata->meta.used)
        continue;

      anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      if (!anode) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        goto error;
      }

      mhead = fluid_boundary_dump(fdata, &err);
      if (err != MSGPACKX_SUCCESS)
        goto error;

      node = msgpackx_map_node_upcast(mhead);
      msgpackx_array_node_set_child_node(anode, node);
      msgpackx_array_node_insert_prev(asubhead, anode);
    }

    geom_list_foreach (lp, &th_head->list) {
      msgpackx_map_node *mhead;

      tdata = thermal_boundary_entry(lp);
      if (!tdata->meta.used)
        continue;

      anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      if (!anode) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        goto error;
      }

      mhead = thermal_boundary_dump(tdata, &err);
      if (err != MSGPACKX_SUCCESS)
        goto error;

      node = msgpackx_map_node_upcast(mhead);
      msgpackx_array_node_set_child_node(anode, node);
      msgpackx_array_node_insert_prev(asubhead, anode);
    }

    if (fl_head->meta.used) {
      msgpackx_map_node *mhead;

      anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      if (!anode) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        goto error;
      }

      mhead = fluid_boundary_dump(fl_head, &err);
      if (err != MSGPACKX_SUCCESS)
        goto error;

      node = msgpackx_map_node_upcast(mhead);
      msgpackx_array_node_set_child_node(anode, node);
      msgpackx_array_node_insert_prev(asubhead, anode);
    }

    if (th_head->meta.used) {
      msgpackx_map_node *mhead;

      anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      if (!anode) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        goto error;
      }

      mhead = thermal_boundary_dump(th_head, &err);
      if (err != MSGPACKX_SUCCESS)
        goto error;

      node = msgpackx_map_node_upcast(mhead);
      msgpackx_array_node_set_child_node(anode, node);
      msgpackx_array_node_insert_prev(asubhead, anode);
    }
  }

  {
    msgpackx_array_node *asubhead;
    msgpackx_node *node;
    msgpackx_error err;
    int nbs[3];
    ptrdiff_t i;

    err = MSGPACKX_SUCCESS;

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    asubhead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
    if (!anode || !asubhead) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (anode)
        msgpackx_array_node_delete(anode);
      if (asubhead)
        msgpackx_array_node_delete_all(asubhead);
      goto error;
    }
    node = msgpackx_array_node_upcast(asubhead);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);

    nbs[0] = nbx;
    nbs[1] = nby;
    nbs[2] = nbz;

    for (i = 0; i < 3; ++i) {
      anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
      node = msgpackx_node_new();
      if (!node || !anode) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        if (node)
          msgpackx_node_delete(node);
        if (anode)
          msgpackx_array_node_delete(anode);
        goto error;
      }

      msgpackx_array_node_set_child_node(anode, node);
      msgpackx_array_node_insert_prev(asubhead, anode);

      msgpackx_node_set_uint(node, nbs[i], &err);
      if (err != MSGPACKX_SUCCESS) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_SERIALIZE,
                  0, 0, NULL);
        goto error;
      }
    }
  }

  {
    void *buf;
    ptrdiff_t sz;
    ptrdiff_t usz;
    msgpackx_array_node *asubhead, *anode;
    msgpackx_node *node;

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    asubhead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
    if (!anode || !asubhead) {
      if (anode)
        msgpackx_array_node_delete(anode);
      if (asubhead)
        msgpackx_array_node_delete_all(asubhead);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      goto error;
    }

    node = msgpackx_array_node_upcast(asubhead);
    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);

    usz = boundary_data_bin_unit_size(np);
    if (usz < 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "Too many boundary conditions defined");
      goto error;
    }

    sz = (nbx < nby) ? nby : nbx;
    sz *= (nby < nbz) ? nbz : nby;
    sz *= usz;
    if (sz < 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Too big data for the serializer.");
      goto error;
    }

    buf = malloc((sz > 0) ? sz : 1);
    if (buf) {
      int j;
      for (j = 0; j < 2; ++j) {
        anode = boundary_data_make_id_bin_array(buf, bnd_W, nby, nbz, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);

        anode = boundary_data_make_id_bin_array(buf, bnd_E, nby, nbz, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);

        anode = boundary_data_make_id_bin_array(buf, bnd_S, nbx, nbz, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);

        anode = boundary_data_make_id_bin_array(buf, bnd_N, nbx, nbz, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);

        anode = boundary_data_make_id_bin_array(buf, bnd_B, nbx, nby, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);

        anode = boundary_data_make_id_bin_array(buf, bnd_T, nbx, nby, usz, j);
        if (!anode)
          break;
        msgpackx_array_node_insert_prev(asubhead, anode);
      }
      free(buf);

      if (!anode) {
        goto error;
      }

    } else {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      goto error;
    }
  }

  goto clean;

error:
  msgpackx_data_delete(data);
  data = NULL;
  ret = 1;
  goto clean;

clean:
  return data;
}

static type boundary_msgpackx_node_get_ftype(msgpackx_node *node,
                                             msgpackx_error *err)
{
  msgpackx_error exerr;
  double dt;
  float ft;
  type tt;

  exerr = MSGPACKX_SUCCESS;

  dt = msgpackx_node_get_double(node, &exerr);
  ft = dt;
  if (exerr == MSGPACKX_ERR_MSG_TYPE) {
    exerr = MSGPACKX_SUCCESS;
    ft = msgpackx_node_get_float(node, &exerr);
    dt = ft;
  }
  if (err && exerr != MSGPACKX_SUCCESS) {
    *err = exerr;
  }
#ifdef JUPITER_DOUBLE
  tt = dt;
#else
  tt = ft;
#endif
  return tt;
}

/**
 * This function is for functions which requires NUL-terminated string.
 *
 * Returning string is allocated.
 *
 * Please deallocate by `free()`.
 */
static char *boundary_msgpackx_node_get_str(msgpackx_node *node,
                                            msgpackx_error *err)
{
  msgpackx_error exerr;
  char *astr;
  const char *cstr;
  ptrdiff_t len;

  cstr = msgpackx_node_get_str(node, &len, &exerr);
  if (!cstr) {
    if (err)
      *err = exerr;
    return NULL;
  }

  if (len + 1 < len) {
    if (err)
      *err = MSGPACKX_ERR_RANGE;
    return NULL;
  }

  astr = (char *)malloc(sizeof(char) * (len + 1));
  if (!astr) {
    if (err)
      *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }

  astr[len] = '\0';
  strncpy(astr, cstr, len);

  return astr;
}

static msgpackx_map_node *boundary_data_check_type(msgpackx_map_node *head,
                                                   const char *type_name,
                                                   int type_name_len)
{
  return msgpackx_map_node_find_by_str(head, type_name, type_name_len, NULL);
}

static int fluid_boundary_data_get_cond(msgpackx_map_node *mhead)
{
  msgpackx_error err;
  msgpackx_node *node_r;
  const char *cstr;
  char *abuf;
  ptrdiff_t cstrlen;
  msgpackx_map_node *mnode;
  int cond;

  mnode = boundary_data_check_type(mhead, "fluid", strlen("fluid"));
  if (!mnode || mnode == mhead) {
    return -1;
  }

  err = MSGPACKX_SUCCESS;
  node_r = msgpackx_map_node_get_value(mnode);
  cstr = msgpackx_node_get_str(node_r, &cstrlen, &err);
  if (!cstr) {
    return -1;
  }

  if (cstrlen == strlen("MPI") && strncmp(cstr, "MPI", cstrlen) == 0) {
    return BOUNDARY_MPI;
  }

  abuf = (char *)malloc(sizeof(char) * (cstrlen + 1));
  if (!abuf) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  memcpy(abuf, cstr, cstrlen);
  abuf[cstrlen] = '\0';

  cond = str_to_boundary(abuf);
  free(abuf);

  if (cond < 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Invalid condition used for fluid boundary");
  }
  return cond;
}

static trip_control boundary_data_get_control(msgpackx_map_node *mhead,
                                              const char *keyname)
{
  trip_control control;
  msgpackx_error err;
  char *str;
  msgpackx_node *node_r;
  msgpackx_map_node *mnode;

  err = MSGPACKX_SUCCESS;
  mnode = msgpackx_map_node_find_by_str(mhead, keyname, strlen(keyname), &err);
  if (!mnode || mnode == mhead) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return TRIP_CONTROL_INVALID;
  }

  node_r = msgpackx_map_node_get_value(mnode);
  str = boundary_msgpackx_node_get_str(node_r, &err);
  if (!str) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return TRIP_CONTROL_INVALID;
  }

  control = str_to_trip_control(str);
  free(str);

  return control;
}

int inlet_component_element_from_msgpackx(struct inlet_component_element *dest,
                                          msgpackx_map_node *mhead,
                                          component_data *comp_data_head,
                                          jcntrl_executive_manager *manager)
{
  intmax_t comp_id;
  msgpackx_error err;
  msgpackx_node *node_r;
  msgpackx_map_node *mnode, *ratio_head;

  CSVASSERT(dest);
  CSVASSERT(mhead);

  node_r = NULL;
  mnode = msgpackx_map_node_find_by_str(mhead, "matid", strlen("matid"), &err);
  if (mnode && mnode != mhead) {
    node_r = msgpackx_map_node_get_value(mnode);
  }
  if (!node_r) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Key \"matid\" not found for inlet component data");
    return 1;
  }

  err = MSGPACKX_SUCCESS;
  comp_id = msgpackx_node_get_int(node_r, &err);
  if (err != MSGPACKX_SUCCESS) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return 1;
  }

  dest->comp.d = component_data_find_by_comp_index(comp_data_head, comp_id);
  if (!dest->comp.d) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "No component found for index %"PRIdMAX, comp_id);
    return 1;
  }
  if (dest->comp.d->comp_index != comp_id) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, "Overflow Error");
    return 1;
  }

  ratio_head = NULL;
  mnode = msgpackx_map_node_find_by_str(mhead, "ratio", strlen("ratio"), &err);
  if (mnode && mnode != mhead) {
    node_r = msgpackx_map_node_get_value(mnode);
    if (node_r)
      ratio_head = msgpackx_node_get_map(node_r);
  }
  if (!ratio_head) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Component ratio data not found");
    return 1;
  }

  if (controllable_type_from_msgpackx(&dest->ratio, ratio_head, manager))
    return 1;
  return 0;
}

struct inlet_component_data *
inlet_component_data_from_msgpackx(msgpackx_array_node *ahead,
                                   component_data *comp_data_head,
                                   jcntrl_executive_manager *manager)
{
  struct inlet_component_data *data;
  int ncomp;
  msgpackx_array_node *anode;

  ncomp = 0;
  for (anode = msgpackx_array_node_next(ahead); anode != ahead;
       anode = msgpackx_array_node_next(anode)) {
    ncomp++;
  }

  data = inlet_component_data_new(ncomp);
  if (!data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  anode = msgpackx_array_node_next(ahead);
  for (int i = 0; anode != ahead;
       anode = msgpackx_array_node_next(anode), ++i) {
    msgpackx_map_node *mnode;
    msgpackx_node *node;
    struct inlet_component_element *e;

    mnode = NULL;
    node = msgpackx_array_node_get_child_node(anode);
    if (node)
      mnode = msgpackx_node_get_map(node);
    if (!mnode) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "Inlet component data map not found");
      inlet_component_data_delete(data);
      return NULL;
    }

    if (inlet_component_element_from_msgpackx(&data->array[i], mnode,
                                              comp_data_head, manager)) {
      inlet_component_data_delete(data);
      return NULL;
    }
  }
  return data;
}

static int boundary_data_get_inlet_vel(fluid_boundary_data *dest,
                                       msgpackx_map_node *mhead,
                                       jcntrl_executive_manager *manager,
                                       controllable_type *control_head)
{
  msgpackx_node *node_r;
  msgpackx_error err;
  msgpackx_array_node *inlet_head, *inlet_node;
  msgpackx_map_node *mnode;
  controllable_type *inlet_dest[3];
  int i;

  mnode = msgpackx_map_node_find_by_str(mhead, "velocity-in",
                                        strlen("velocity-in"), &err);
  if (!mnode || mnode == mhead) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return 1;
  }

  node_r = msgpackx_map_node_get_value(mnode);
  inlet_head = msgpackx_node_get_array(node_r);
  if (!inlet_head) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Constant inlet velocity \"velocity-in\" "
               "not found in the boundary data");
    return 1;
  }

  inlet_dest[0] = &dest->inlet_vel_u;
  inlet_dest[1] = &dest->inlet_vel_v;
  inlet_dest[2] = &dest->inlet_vel_w;

  inlet_node = msgpackx_array_node_next(inlet_head);
  for (i = 0; i < 3; ++i, inlet_node = msgpackx_array_node_next(inlet_node)) {
    if (inlet_node == inlet_head) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary data: 3 elements required for inlet velocity.");
      return 1;
    }

    if (dest->control == TRIP_CONTROL_CONST ||
        dest->control == TRIP_CONTROL_CONTROL) {
      msgpackx_map_node *ctrl_head;

      ctrl_head = NULL;
      node_r = msgpackx_array_node_get_child_node(inlet_node);
      if (node_r) {
        ctrl_head = msgpackx_node_get_map(node_r);
      }
      if (!ctrl_head) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Boundary data: No data found for CONST/CONTROL data");
        return 1;
      }

      if (controllable_type_from_msgpackx(inlet_dest[i], ctrl_head, manager)) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Boundary data: Failed to set control data");
        return 1;
      }

      if (controllable_type_add_to_list_if_needed(control_head, inlet_dest[i]))
        dest->control = TRIP_CONTROL_CONTROL;

    } else {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary data: Given control mode not supported");
      return 1;
    }
  }

  return 0;
}

static int boundary_data_get_inlet_vof(fluid_boundary_data *dest,
                                       msgpackx_map_node *mhead,
                                       component_data *comp_data_head,
                                       jcntrl_executive_manager *manager,
                                       controllable_type *control_head)
{
  struct inlet_component_data *data;
  msgpackx_error err;
  msgpackx_node *node_r;
  msgpackx_array_node *comp_list_head;
  msgpackx_map_node *mnode;

  err = MSGPACKX_SUCCESS;
  mnode = msgpackx_map_node_find_by_str(mhead, "inlet-vof", strlen("inlet-vof"),
                                        &err);
  comp_list_head = NULL;
  if (mnode && mnode != mhead) {
    node_r = msgpackx_map_node_get_value(mnode);
    comp_list_head = msgpackx_node_get_array(node_r);
  }
  if (!comp_list_head) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Constant inlet VOF \"inlet-vof\" "
               "not found in the boundary data");
    return 1;
  }

  data = inlet_component_data_from_msgpackx(comp_list_head, comp_data_head,
                                            manager);
  if (!data)
    return 1;

  if (dest->comps)
    inlet_component_data_delete(dest->comps);
  dest->comps = data;

  if (inlet_component_data_ncomp(data) <= 0) {
    csvperrorf(
      __FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
      "Boundary data: At least one component is required for \"inlet-vof\"");
    return 1;
  }

  inlet_component_data_add_to_list(data, control_head);
  return 0;
}

/**
 * @brief Deserialize fluid_boundary_data
 * @param head Head of fluid_boundary_data list
 * @param mhead Data to parse.
 * @param comp_data_head List of available components
 * @param manager executive manager to search field variables
 * @param control_head Head item of controlled parameter list
 * @return Deserialized data
 * @retval NULL Allocation failed, or invalid data
 * @retval head Condition is BOUNDARY_MPI.
 *
 * The argument @p head is used for returning to BOUNDARY_MPI data.
 *
 * If the condition is not BOUNDARY_MPI, nothing to do with @p head.
 * The returned pointer will not become a member of list @p head.
 */
static fluid_boundary_data *fluid_boundary_data_construct(
  fluid_boundary_data *head, msgpackx_map_node *mhead,
  component_data *comp_data_head, jcntrl_executive_manager *manager,
  controllable_type *control_head)
{
  msgpackx_map_node *mnode;
  msgpackx_node *node_r;
  const char *cstr;
  msgpackx_error err;
  int cond;
  fluid_boundary_data *fl_data;

  cond = fluid_boundary_data_get_cond(mhead);
  if (cond == BOUNDARY_MPI) {
    return head;
  }

  if (cond < 0) {
    return NULL;
  }

  fl_data = calloc(sizeof(struct fluid_boundary_data), 1);
  if (!fl_data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  fluid_boundary_data_init(fl_data);
  fl_data->cond = cond;

  if (cond == INLET) {
    trip_control control;

    control = boundary_data_get_control(mhead, "inlet-control");
    if (control == TRIP_CONTROL_INVALID) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Invalid inlet control is used");
      fluid_boundary_data_delete(fl_data);
      return NULL;
    }

    fl_data->control = control;

    if (boundary_data_get_inlet_vel(fl_data, mhead, manager, control_head)) {
      fluid_boundary_data_delete(fl_data);
      return NULL;
    }

    if (boundary_data_get_inlet_vof(fl_data, mhead, comp_data_head, manager,
                                    control_head)) {
      fluid_boundary_data_delete(fl_data);
      return NULL;
    }
  }
  return fl_data;
}

static int thermal_boundary_data_get_cond(msgpackx_map_node *mhead)
{
  msgpackx_error err;
  msgpackx_node *node_r;
  const char *cstr;
  char *abuf;
  ptrdiff_t cstrlen;
  msgpackx_map_node *mnode;
  int cond;

  mnode = boundary_data_check_type(mhead, "thermal", strlen("thermal"));
  if (!mnode || mnode == mhead) {
    return -1;
  }

  err = MSGPACKX_SUCCESS;
  node_r = msgpackx_map_node_get_value(mnode);
  cstr = msgpackx_node_get_str(node_r, &cstrlen, &err);
  if (!cstr) {
    return -1;
  }

  if (cstrlen == strlen("MPI") && strncmp(cstr, "MPI", cstrlen) == 0) {
    return BOUNDARY_MPI;
  }

  abuf = (char *)malloc(sizeof(char) * (cstrlen + 1));
  if (!abuf) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  memcpy(abuf, cstr, cstrlen);
  abuf[cstrlen] = '\0';

  cond = str_to_tboundary(abuf);
  free(abuf);

  if (cond < 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Invalid condition used for thermal boundary");
  }
  return cond;
}

static int thermal_boundary_get_wall_temp(thermal_boundary_data *tdata,
                                          msgpackx_map_node *mhead,
                                          jcntrl_executive_manager *manager,
                                          controllable_type *control_head)
{
  trip_control control;
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;
  msgpackx_map_node *wallt_head;

  if (tdata->control == TRIP_CONTROL_CONST ||
      tdata->control == TRIP_CONTROL_CONTROL) {
    mnode =
      msgpackx_map_node_find_by_str(mhead, "wall-t", strlen("wall-t"), &err);
    wallt_head = NULL;
    if (mnode && mnode != mhead) {
      node = msgpackx_map_node_get_value(mnode);
      if (node) {
        wallt_head = msgpackx_node_get_map(node);
      }
    }
    if (!wallt_head) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Wall temperature data not found");
      return 1;
    }

    if (controllable_type_from_msgpackx(&tdata->temperature, wallt_head,
                                        manager))
      return 1;

    if (controllable_type_add_to_list_if_needed(control_head,
                                                &tdata->temperature))
      tdata->control = TRIP_CONTROL_CONTROL;

  } else {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Unsupported wall temperature trip control used");
    return 1;
  }

  return 0;
}

/**
 * @brief Deserialize thermal_boundary_data
 * @param head Head of thermal_boundary_data list
 * @param mhead Data to parse.
 * @param comp_data_head List of available components
 * @return Deserialized data
 * @retval NULL Allocation failed, or invalid data
 * @retval head Condition is BOUNDARY_MPI.
 *
 * The argument @p head is used for returning to BOUNDARY_MPI data.
 *
 * If the condition is not BOUNDARY_MPI, nothing to do with @p head.
 * The returned pointer will not become a member of list @p head.
 */
static thermal_boundary_data *thermal_boundary_data_construct(
  thermal_boundary_data *head, msgpackx_map_node *mhead,
  component_data *comp_data_head,
  jcntrl_executive_manager *manager, controllable_type *control_head)
{
  msgpackx_map_node *mnode;
  msgpackx_node *node_r;
  const char *cstr;
  char *abuf;
  ptrdiff_t cstrlen;
  msgpackx_error err;
  int cond;
  thermal_boundary_data *th_data;

  cond = thermal_boundary_data_get_cond(mhead);
  if (cond == BOUNDARY_MPI) {
    return head;
  }
  if (cond < 0) {
    return NULL;
  }

  th_data = calloc(sizeof(struct thermal_boundary_data), 1);
  if (!th_data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  thermal_boundary_data_init(th_data);
  th_data->cond = cond;

  if (cond == ISOTHERMAL) {
    trip_control control;

    control = boundary_data_get_control(mhead, "wall-t-control");
    if (control == TRIP_CONTROL_INVALID) {
      thermal_boundary_data_delete(th_data);
      return NULL;
    }

    th_data->control = control;

    if (thermal_boundary_get_wall_temp(th_data, mhead, manager, control_head)) {
      thermal_boundary_data_delete(th_data);
      return NULL;
    }
  } else if (cond == DIFFUSION) {
    err = MSGPACKX_SUCCESS;
    mnode = msgpackx_map_node_find_by_str(mhead, "diffusion-min-temp",
                                          strlen("diffusion-min-temp"), NULL);
    node_r = NULL;
    if (mnode && mnode != mhead) {
      node_r = msgpackx_map_node_get_value(mnode);
    }
    if (node_r) {
      th_data->diffusion_limit = boundary_msgpackx_node_get_ftype(node_r, &err);
      if (err == MSGPACKX_ERR_MSG_TYPE) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                   "Boundary data: A float32 or float64 data"
                   " required for \"diffusion-min-temp\"");
        th_data->diffusion_limit = 0.0;
      }
    } else {
      th_data->diffusion_limit = 0.0;
    }
  }

  return th_data;
}

static const void *boundary_data_array_bin_get(msgpackx_array_node *anode,
                                               ptrdiff_t *size_out)
{
  const void *mb;
  msgpackx_node *node_r;

  mb = NULL;
  node_r = msgpackx_array_node_get_child_node(anode);
  if (node_r) {
    mb = msgpackx_node_get_bin(node_r, size_out, NULL);
  }
  return mb;
}

static ptrdiff_t boundary_data_array_bin_get_element(const void *bin, int swap,
                                                     ptrdiff_t unit_size,
                                                     ptrdiff_t off)
{
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;

  switch (unit_size) {
  case sizeof(uint8_t):
    return *((const uint8_t *)bin + off);
  case sizeof(uint16_t):
    u16 = *((const uint16_t *)bin + off);
    if (swap) {
      u16 = msgpackx_byteswap_2(u16);
    }
    return u16;
  case sizeof(uint32_t):
    u32 = *((const uint32_t *)bin + off);
    if (swap) {
      u32 = msgpackx_byteswap_4(u32);
    }
    return u32;
  case sizeof(uint64_t):
    u64 = *((const uint64_t *)bin + off);
    if (swap) {
      u64 = msgpackx_byteswap_8(u64);
    }
    return u64;
  default:
    break;
  }
  return -1;
}

static int boundary_data_array_bin_check(msgpackx_array_node *anode, int nbx,
                                         int nby, ptrdiff_t ncond,
                                         struct boundary_meta_data **map,
                                         const char *dir, int swap)
{
  ptrdiff_t usz;
  const char *mb;
  ptrdiff_t reqsz;
  ptrdiff_t gotsz;
  enum bin_check_flg
  {
    index_out_of_range,
    index_invalid_type,
  } flg;
  int i, j;

  usz = boundary_data_bin_unit_size(ncond);
  if (usz < 0)
    return 1;

  reqsz = usz;
  reqsz *= nbx;
  reqsz *= nby;

  mb = boundary_data_array_bin_get(anode, &gotsz);
  if (!mb) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Could not get %s bin data", dir);
  }
  if (gotsz != reqsz) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Required size (%" PRIdMAX " bytes)"
               " does not match to stored size (%" PRIdMAX " bytes)"
               " for direction %s",
               (intmax_t)reqsz, (intmax_t)gotsz, dir);
    return 1;
  }

  flg = 0;
#pragma omp parallel for collapse(2) reduction(| : flg)
  for (j = 0; j < nby; ++j) {
    for (i = 0; i < nbx; ++i) {
      ptrdiff_t ind;
      ptrdiff_t jj;
      enum bin_check_flg lflg;

      lflg = flg;

      if (lflg == (index_out_of_range | index_invalid_type)) {
        continue;
      }

      jj = calc_address(i, j, 0, nbx, nby, 1);
      ind = boundary_data_array_bin_get_element(mb, swap, usz, jj);

      if (ind < 0 || ind >= ncond) {
        lflg |= index_out_of_range;
      } else {
        if (!map[ind]) {
          lflg |= index_invalid_type;
        }
      }
      flg |= lflg;
    }
  }

  if (flg & index_out_of_range) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Condition index number out of range");
  }
  if (flg & index_invalid_type) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Condition index number indicates wrong type "
               "(fluid or thermal)");
  }
  return flg;
}

static void boundary_data_array_get_read(msgpackx_array_node *fl_anode,
                                         msgpackx_array_node *th_anode, int nbx,
                                         int nby, ptrdiff_t ncond,
                                         struct boundary_meta_data **fl_map,
                                         struct boundary_meta_data **th_map,
                                         struct boundary_array *bnd, int swap)
{
  ptrdiff_t usz;
  const void *flmb, *thmb;
  ptrdiff_t flsz, thsz, rsz;
  ptrdiff_t i, j;

  CSVASSERT(fl_anode);
  CSVASSERT(th_anode);
  CSVASSERT(th_map);
  CSVASSERT(fl_map);
  CSVASSERT(bnd);
  CSVASSERT(ncond > 0);

  flmb = boundary_data_array_bin_get(fl_anode, &flsz);
  thmb = boundary_data_array_bin_get(th_anode, &thsz);

  usz = boundary_data_bin_unit_size(ncond);
  CSVASSERT(usz > 0);

  rsz = usz;
  rsz *= nbx;
  rsz *= nby;

  CSVASSERT(rsz == flsz);
  CSVASSERT(rsz == thsz);

#pragma omp parallel for collapse(2)
  for (j = 0; j < nby; ++j) {
    for (i = 0; i < nbx; ++i) {
      ptrdiff_t flim, thim;
      ptrdiff_t jj;

      jj = calc_address(i, j, 0, nbx, nby, 1);
      flim = boundary_data_array_bin_get_element(flmb, swap, usz, jj);
      thim = boundary_data_array_bin_get_element(thmb, swap, usz, jj);
      bnd->fl[jj] = fluid_boundary_data_from_meta(fl_map[flim]);
      bnd->th[jj] = thermal_boundary_data_from_meta(th_map[thim]);
    }
  }
}

int boundary_data_array_construct(
  msgpackx_data *data, fluid_boundary_data *fl_head,
  thermal_boundary_data *th_head, component_data *comp_data_head,
  jcntrl_executive_manager *manager, controllable_type *control_head,
  int nbx, int nby, int nbz,
  struct boundary_array *bnd_W, struct boundary_array *bnd_E,
  struct boundary_array *bnd_S, struct boundary_array *bnd_N,
  struct boundary_array *bnd_B, struct boundary_array *bnd_T)
{
  msgpackx_node *node[4];
  msgpackx_array_node *ahead, *anode;
  msgpackx_map_node *mhead;
  msgpackx_error err;
  int swap;
  ptrdiff_t i;
  ptrdiff_t ncond;
  struct thermal_boundary_data tmp_th_head;
  struct fluid_boundary_data tmp_fl_head;
  struct thermal_boundary_data *tdata;
  struct fluid_boundary_data *fdata;

  struct boundary_meta_data **fl_map, **th_map;

  CSVASSERT(fl_head);
  CSVASSERT(th_head);
  CSVASSERT(nbx > 0);
  CSVASSERT(nby > 0);
  CSVASSERT(nbz > 0);

  fluid_boundary_data_init(&tmp_fl_head);
  thermal_boundary_data_init(&tmp_th_head);

  err = MSGPACKX_SUCCESS;
  anode = msgpackx_read_header_data(data, NULL, &swap, &err);
  if (!anode) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return 1;
  }

  ahead = NULL;
  node[0] = msgpackx_array_node_get_child_node(anode);
  if (node[0]) {
    ahead = msgpackx_node_get_array(node[0]);
  }
  if (!ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Main data is not an array");
    return 1;
  }

  anode = msgpackx_array_node_next(ahead);
  for (i = 0; i < 3; ++i) {
    if (anode == ahead) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary data: Data does not found");
      return 1;
    }
    node[i] = msgpackx_array_node_get_child_node(anode);
    anode = msgpackx_array_node_next(anode);
  }
  if (anode != ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Excess data found");
    return 1;
  }

  ahead = msgpackx_node_get_array(node[0]);
  if (!ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Condition data is not an array");
    return 1;
  }

  anode = msgpackx_array_node_next(ahead);
  i = 0;
  for (; anode != ahead; anode = msgpackx_array_node_next(anode)) {
    ++i;
    if (i < 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary data: Too many conditions defined");
      return 1;
    }
  }
  ncond = i;
  if (ncond == 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: No condition data found");
    return 1;
  }

  fl_map =
    (struct boundary_meta_data **)calloc(sizeof(struct boundary_meta_data *),
                                         ncond * 2);
  if (!fl_map) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }
  th_map = fl_map + ncond;

  anode = msgpackx_array_node_next(ahead);
  for (i = 0; i < ncond; ++i) {
    msgpackx_node *node_r;

    node_r = msgpackx_array_node_get_child_node(anode);
    mhead = msgpackx_node_get_map(node_r);
    if (!mhead) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary data: Condition data must be a map");
      goto error;
    }

    tdata = NULL;
    fdata = fluid_boundary_data_construct(fl_head, mhead, comp_data_head,
                                          manager, control_head);
    if (!fdata) {
      tdata = thermal_boundary_data_construct(th_head, mhead, comp_data_head,
                                              manager, control_head);
    }
    if (!fdata && !tdata) {
      goto error;
    }
    if (fdata) {
      if (fdata != fl_head) {
        geom_list_insert_prev(&tmp_fl_head.list, &fdata->list);
      }
      fl_map[i] = &fdata->meta;
    }

    if (tdata) {
      if (tdata != th_head) {
        geom_list_insert_prev(&tmp_th_head.list, &tdata->list);
      }
      th_map[i] = &tdata->meta;
    }

    anode = msgpackx_array_node_next(anode);
  }

  ahead = msgpackx_node_get_array(node[1]);
  if (!ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Size data must be an array");
    goto error;
  }

  anode = msgpackx_array_node_next(ahead);
  err = MSGPACKX_SUCCESS;
  for (i = 0; i < 3; ++i) {
    uintmax_t im;
    msgpackx_node *node_r;
    msgpackx_error ex;

    node_r = msgpackx_array_node_get_child_node(anode);
    if (node_r) {
      ex = MSGPACKX_SUCCESS;
      im = msgpackx_node_get_uint(node_r, &ex);
      if (err == MSGPACKX_SUCCESS) {
        switch (i) {
        case 0:
          if (nbx < 0 || im != (uintmax_t)nbx) {
            csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                       "Boundary error: X size does not match"
                       " (req: %" PRIuMAX ", got: %" PRIuMAX ")",
                       (uintmax_t)nbx, im);
            err = MSGPACKX_ERR_RANGE;
          }
          break;
        case 1:
          if (nby < 0 || im != (uintmax_t)nby) {
            csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                       "Boundary error: Y size does not match"
                       " (req: %" PRIuMAX ", got: %" PRIuMAX ")",
                       (uintmax_t)nby, im);
            err = MSGPACKX_ERR_RANGE;
          }
          break;
        case 2:
          if (nbz < 0 || im != (uintmax_t)nbz) {
            csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                       "Boundary error: Z size does not match"
                       " (req: %" PRIuMAX ", got: %" PRIuMAX ")",
                       (uintmax_t)nbz, im);
            err = MSGPACKX_ERR_RANGE;
          }
          break;
        default:
          CSVUNREACHABLE();
          break;
        }
      } else {
        err = ex;
      }
    }

    anode = msgpackx_array_node_next(anode);
    if (i < 2 && anode == ahead) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Boundary error: 3 elements required for Size array,"
                 " but found %" PRIdMAX " elements.",
                 (intmax_t)i);
      err = MSGPACKX_ERR_RANGE;
      break;
    }
  }
  if (err != MSGPACKX_SUCCESS)
    goto error;

  ahead = msgpackx_node_get_array(node[2]);
  if (!ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: Cell data must be an array");
    goto error;
  }

  anode = msgpackx_array_node_next(ahead);
  if (anode == ahead) {
cell_data_array_short:
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Boundary data: 12 elements required for Cell array");
    goto error;
  }

  i = 0;
  i += boundary_data_array_bin_check(anode, nby, nbz, ncond, fl_map, "W", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nby, nbz, ncond, fl_map, "E", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nbz, ncond, fl_map, "S", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nbz, ncond, fl_map, "N", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nby, ncond, fl_map, "B", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nby, ncond, fl_map, "T", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nby, nbz, ncond, th_map, "W", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nby, nbz, ncond, th_map, "E", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nbz, ncond, th_map, "S", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nbz, ncond, th_map, "N", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nby, ncond, th_map, "B", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    goto cell_data_array_short;

  i += boundary_data_array_bin_check(anode, nbx, nby, ncond, th_map, "T", swap);
  anode = msgpackx_array_node_next(anode);
  if (anode != ahead) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
               "Boundary data: Excess data found in the cell erray");
  }

  if (i != 0)
    goto error;

  {
    msgpackx_array_node *flnode, *thnode;

    flnode = msgpackx_array_node_next(ahead);
    thnode = flnode;
    for (i = 0; i < 6; ++i) {
      thnode = msgpackx_array_node_next(thnode);
    }
    boundary_data_array_get_read(flnode, thnode, nby, nbz, ncond, fl_map,
                                 th_map, bnd_W, swap);

    flnode = msgpackx_array_node_next(flnode);
    thnode = msgpackx_array_node_next(thnode);
    boundary_data_array_get_read(flnode, thnode, nby, nbz, ncond, fl_map,
                                 th_map, bnd_E, swap);

    flnode = msgpackx_array_node_next(flnode);
    thnode = msgpackx_array_node_next(thnode);
    boundary_data_array_get_read(flnode, thnode, nbx, nbz, ncond, fl_map,
                                 th_map, bnd_S, swap);

    flnode = msgpackx_array_node_next(flnode);
    thnode = msgpackx_array_node_next(thnode);
    boundary_data_array_get_read(flnode, thnode, nbx, nbz, ncond, fl_map,
                                 th_map, bnd_N, swap);

    flnode = msgpackx_array_node_next(flnode);
    thnode = msgpackx_array_node_next(thnode);
    boundary_data_array_get_read(flnode, thnode, nbx, nby, ncond, fl_map,
                                 th_map, bnd_B, swap);

    flnode = msgpackx_array_node_next(flnode);
    thnode = msgpackx_array_node_next(thnode);
    boundary_data_array_get_read(flnode, thnode, nbx, nby, ncond, fl_map,
                                 th_map, bnd_T, swap);
  }

  geom_list_insert_list_prev(&fl_head->list, &tmp_fl_head.list);
  geom_list_insert_list_prev(&th_head->list, &tmp_th_head.list);

  geom_list_delete(&tmp_fl_head.list);
  geom_list_delete(&tmp_th_head.list);

  free(fl_map);
  return 0;

error:
  fluid_boundary_data_delete_all(&tmp_fl_head);
  thermal_boundary_data_delete_all(&tmp_th_head);
  free(fl_map);
  return 1;
}

int boundary_data_array_read_from_file(
  FILE *fp, const char *file, fluid_boundary_data *fl_head,
  thermal_boundary_data *th_head, component_data *comp_data_head,
  jcntrl_executive_manager *manager,
  controllable_type *control_head, int nbx, int nby, int nbz,
  struct boundary_array *bnd_W, struct boundary_array *bnd_E,
  struct boundary_array *bnd_S, struct boundary_array *bnd_N,
  struct boundary_array *bnd_B, struct boundary_array *bnd_T, csv_error *cserr,
  msgpackx_error *merr)
{
  msgpackx_data *data;
  int ret;

  ret = 1;
  errno = 0;
  data = msgpackx_data_read(fp, merr, NULL);
  if (!data) {
    if (errno == 0) {
      if (cserr)
        *cserr = CSV_ERR_SERIALIZE;
    } else {
      if (cserr)
        *cserr = CSV_ERR_SYS;
    }
    goto clean;
  }

  ret = boundary_data_array_construct(data, fl_head, th_head, comp_data_head,
                                      manager, control_head, nbx, nby, nbz,
                                      bnd_W, bnd_E, bnd_S, bnd_N, bnd_B, bnd_T);

clean:
  if (data) {
    msgpackx_data_delete(data);
  }
  return ret;
}

#ifdef JUPITER_MPI
#endif
