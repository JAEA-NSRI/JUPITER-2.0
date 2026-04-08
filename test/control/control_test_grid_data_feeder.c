#include "control_test_grid_data_feeder.h"
#include "jupiter/control/cell_data.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/error.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/information.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mask_data.h"
#include "jupiter/control/output.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/control/struct_grid.h"
#include "jupiter/control/grid_data.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_util.h"

static struct grid_data_feeder *
grid_data_feeder_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(grid_data_feeder, struct grid_data_feeder, obj);
}

static void *grid_data_feeder_downcast_v(jcntrl_shared_object *obj)
{
  return grid_data_feeder_downcast_impl(obj);
}

static int grid_data_feeder_initializer(jcntrl_shared_object *obj)
{
  struct grid_data_feeder *g = grid_data_feeder_downcast_impl(obj);
  const jcntrl_size_type n_pindex =
    sizeof(g->piece_index) / sizeof(g->piece_index[0]);

  jcntrl_struct_grid_init(&g->struct_grid);
  g->local_data_extent = jcntrl_extent_empty();
  g->whole_extent = jcntrl_extent_empty();
  g->mask_array = NULL;
  for (int i = 0; i < n_pindex; ++i)
    g->piece_index[i] = 0;
  g->number_of_pieces = 0;
  g->ncell_arrays = 0;
  g->cell_arrays = NULL;

  {
    jcntrl_input *input;
    jcntrl_output *output;

    input = jcntrl_executive_get_input(&g->executive);
    output = jcntrl_executive_get_output(&g->executive);

    if (!jcntrl_input_set_number_of_ports(input, 0))
      return 0;

    if (!jcntrl_output_set_number_of_ports(output, 1))
      return 0;
  }

  return 1;
}

static void grid_data_feeder_clean_arrays(struct grid_data_feeder *g)
{
  if (g->mask_array)
    jcntrl_data_array_delete(g->mask_array);
  g->mask_array = NULL;
  for (jcntrl_size_type i = 0; i < g->ncell_arrays; ++i) {
    jcntrl_data_array_delete(g->cell_arrays[i]);
    g->cell_arrays[i] = NULL;
  }
  jcntrl_struct_grid_clear(&g->struct_grid);
}

static void grid_data_feeder_destructor(jcntrl_shared_object *obj)
{
  struct grid_data_feeder *g = grid_data_feeder_downcast_impl(obj);
  grid_data_feeder_clean_arrays(g);
}

static int fill_input(jcntrl_shared_object *data, int index,
                      jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(grid_data_feeder, fill_input, jcntrl_executive,
                       fill_input_port_information)

static int fill_output(jcntrl_shared_object *data, int index,
                       jcntrl_output *output)
{
  jcntrl_information *info;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_grid_data))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(grid_data_feeder, fill_output, jcntrl_executive,
                       fill_output_port_information)

static int upd_info(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  struct grid_data_feeder *feeder;
  jcntrl_information *info;

  feeder = grid_data_feeder_downcast_impl(data);

  // first port
  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(grid_data_feeder, upd_info, jcntrl_executive,
                       process_update_information)

static int upd_extent(jcntrl_information *request, jcntrl_input *input,
                      jcntrl_output *output, jcntrl_shared_object *data)
{
  struct grid_data_feeder *feeder;
  jcntrl_information *info;

  feeder = grid_data_feeder_downcast_impl(data);

  // first port
  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (!jcntrl_information_set_extent(info, JCNTRL_INFO_WHOLE_EXTENT,
                                     feeder->whole_extent.extent))
    return 0;

  if (!jcntrl_information_set_extent(info, JCNTRL_INFO_DATA_EXTENT,
                                     feeder->struct_grid.extent))
    return 0;

  if (!jcntrl_information_set_extent(info, JCNTRL_INFO_PIECE_EXTENT,
                                     feeder->local_data_extent.extent))
    return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(grid_data_feeder, upd_extent, jcntrl_executive,
                       process_update_extent)

static int upd_data(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  jcntrl_shared_object *outd;
  jcntrl_grid_data *gridd;
  jcntrl_cell_data *celld;
  struct grid_data_feeder *feeder;
  jcntrl_information *info;

  feeder = grid_data_feeder_downcast_impl(data);

  // first port
  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  outd = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!outd)
    return 0;

  gridd = jcntrl_grid_data_downcast(outd);
  if (!gridd)
    return 0;

  jcntrl_grid_data_set_struct_grid(gridd, &feeder->struct_grid);

  if (feeder->mask_array) {
    jcntrl_mask_data *m;
    jcntrl_bool_array *b;
    jcntrl_shared_object *o;

    o = jcntrl_data_array_object(feeder->mask_array);
    b = jcntrl_bool_array_downcast(o);
    if (b) {
      m = jcntrl_grid_data_get_mask(gridd);
      if (!m) {
        m = jcntrl_mask_data_new();
        if (!m)
          return 0;

        jcntrl_grid_data_set_mask(gridd, m);
        // Release ownership of mask data
        jcntrl_mask_data_delete(m);
      }

      jcntrl_mask_data_set_array(m, b);
    } else {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Given mask array is not jcntrl_bool_array");
      return 0;
    }
  } else {
    jcntrl_grid_data_set_mask(gridd, NULL);
  }

  celld = jcntrl_grid_data_cell_data(gridd);

  for (jcntrl_size_type nc = 0; nc < feeder->ncell_arrays; ++nc) {
    if (!jcntrl_cell_data_add_array(celld, feeder->cell_arrays[nc]))
      return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(grid_data_feeder, upd_data, jcntrl_executive,
                       process_update_data)

static void grid_data_feeder_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = grid_data_feeder_downcast_v;
  p->initializer = grid_data_feeder_initializer;
  p->destructor = grid_data_feeder_destructor;
  JCNTRL_VIRTUAL_WRAP_SET(p, grid_data_feeder, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, grid_data_feeder, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, grid_data_feeder, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, grid_data_feeder, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, grid_data_feeder, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(grid_data_feeder, grid_data_feeder_init_func)

int grid_data_feeder_init(struct grid_data_feeder *p)
{
  return jcntrl_shared_object_static_init(&p->executive.object,
                                          grid_data_feeder_metadata_init());
}

void grid_data_feeder_clean(struct grid_data_feeder *p)
{
  jcntrl_shared_object_delete(&p->executive.object);
}

static jcntrl_data_array *
grid_data_make_array(struct grid_data_feeder_set_ary_info *info)
{
  jcntrl_shared_object *o;
  jcntrl_generic_data_array *p;

  JCNTRL_ASSERT(info);
  JCNTRL_ASSERT(info->p);
  JCNTRL_ASSERT(info->n <= 0 || info->ary);

  o = jcntrl_shared_object_new_by_meta(info->p);
  if (!o)
    return NULL;

  p = jcntrl_generic_data_array_downcast(o);
  JCNTRL_ASSERT_X(p,
                  "Given type '%s' does not inherit jcntrl_generic_data_array",
                  info->p->name);
  if (!p) {
    jcntrl_shared_object_delete(o);
    jcntrl_raise_argument_error(
      __FILE__, __LINE__,
      "Given type does not inherit jcntrl_generic_data_array");
    return NULL;
  }

  if (!jcntrl_generic_data_array_bind(p, info->ary, info->p, info->n)) {
    jcntrl_shared_object_delete(o);
    return NULL;
  }

  if (info->name) {
    if (!jcntrl_data_array_set_name(jcntrl_generic_data_array_data(p),
                                    info->name, info->namelen)) {
      jcntrl_shared_object_delete(o);
      return NULL;
    }
  }

  return jcntrl_generic_data_array_data(p);
}

int grid_data_feeder_set_grid(
  struct grid_data_feeder *p, jcntrl_extent data_extent,
  struct grid_data_feeder_set_ary_info *xary,
  struct grid_data_feeder_set_ary_info *yary,
  struct grid_data_feeder_set_ary_info *zary,
  struct grid_data_feeder_set_ary_info *mask, //
  jcntrl_size_type ncell_arrays, jcntrl_data_array **cell_arrays,
  struct grid_data_feeder_set_ary_info *cell_arrays_info)
{
  jcntrl_size_type n;
  jcntrl_data_array *xa;
  jcntrl_data_array *ya;
  jcntrl_data_array *za;
  jcntrl_data_array *m;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(xary);
  JCNTRL_ASSERT(yary);
  JCNTRL_ASSERT(zary);
  JCNTRL_ASSERT(ncell_arrays >= 0);
  if (ncell_arrays > 0) {
    JCNTRL_ASSERT(cell_arrays);
    JCNTRL_ASSERT(cell_arrays_info);
  }
  JCNTRL_ASSERT(jcntrl_extent_nx(data_extent) == xary->n - 1);
  JCNTRL_ASSERT(jcntrl_extent_ny(data_extent) == yary->n - 1);
  JCNTRL_ASSERT(jcntrl_extent_nz(data_extent) == zary->n - 1);

  n = jcntrl_extent_size(data_extent);
  if (mask) {
    JCNTRL_ASSERT(n == mask->n);
  }

  m = NULL;
  xa = grid_data_make_array(xary);
  ya = grid_data_make_array(yary);
  za = grid_data_make_array(zary);
  if (mask)
    m = grid_data_make_array(mask);
  if (!xa || !ya || !za || (mask && !m)) {
    if (xa)
      jcntrl_data_array_delete(xa);
    if (ya)
      jcntrl_data_array_delete(ya);
    if (za)
      jcntrl_data_array_delete(za);
    if (m)
      jcntrl_data_array_delete(m);
    return 0;
  }

  for (jcntrl_size_type i = 0; i < ncell_arrays; ++i) {
    JCNTRL_ASSERT(cell_arrays_info[i].n == n);
    cell_arrays[i] = grid_data_make_array(&cell_arrays_info[i]);
    if (!cell_arrays[i]) {
      for (jcntrl_size_type j = 0; j < i; ++j) {
        jcntrl_data_array_delete(cell_arrays[j]);
        cell_arrays[j] = NULL;
      }
      jcntrl_data_array_delete(xa);
      jcntrl_data_array_delete(ya);
      jcntrl_data_array_delete(za);
      if (m)
        jcntrl_data_array_delete(m);
      return 0;
    }
  }

  grid_data_feeder_clean_arrays(p);

  jcntrl_struct_grid_set_extent(&p->struct_grid, data_extent.extent);
  jcntrl_struct_grid_set_x_coords(&p->struct_grid, xa);
  jcntrl_struct_grid_set_y_coords(&p->struct_grid, ya);
  jcntrl_struct_grid_set_z_coords(&p->struct_grid, za);

  // release ownership
  jcntrl_data_array_delete(xa);
  jcntrl_data_array_delete(ya);
  jcntrl_data_array_delete(za);

  p->mask_array = m;
  p->ncell_arrays = ncell_arrays;
  p->cell_arrays = cell_arrays;
  return 1;
}

int test_control_grid_data_feeder(void)
{
  int ret;
  struct grid_data_feeder f;
  jcntrl_shared_object *o;
  jcntrl_grid_data *g;
  jcntrl_output *output;
  jcntrl_information *info;
  jcntrl_executive *exe;
  ret = 0;

  grid_data_feeder_init(&f);
  exe = &f.executive;
  output = NULL;
  info = NULL;

  if (!test_compare_pp((output = jcntrl_executive_output_port(exe, 0)), !=,
                       NULL))
    ret = 1;

  if (output) {
    if (!test_compare_pp((info = jcntrl_output_information(output)), !=, NULL))
      ret = 1;
  }

  if (!test_compare_ii(grid_data_feeder_set(&f,
                                            ((jcntrl_extent){0, 1, 0, 1, 0, 1}),
                                            d(({0, 1}), "X"), d(({0, 1}), "Y"),
                                            d(({0, 1}))),
                       ==, 1))
    ret = 1;

  if (!test_compare_jcntrl_extent(jcntrl_extent_c(f.struct_grid.extent),
                                  ((jcntrl_extent){0, 1, 0, 1, 0, 1})))
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  o = NULL;
  g = NULL;
  if (!test_compare_pp(
        (o = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)), !=,
        NULL))
    ret = 1;

  if (o) {
    if (!test_compare_pp((g = jcntrl_grid_data_downcast(o)), !=, NULL))
      ret = 1;
  }

  if (g) {
    jcntrl_mask_data *m;
    jcntrl_cell_data *cdata;
    jcntrl_data_array *x, *y, *z;
    jcntrl_size_type l;

    cdata = NULL;
    x = y = z = NULL;

    if (!test_compare_pp((x = jcntrl_grid_data_x_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((y = jcntrl_grid_data_y_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((z = jcntrl_grid_data_z_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((m = jcntrl_grid_data_get_mask(g)), ==, NULL))
      ret = 1;

    if (x) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(x), ==, 2))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 0), ==, 0.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 1), ==, 1.0))
        ret = 1;
      if (!test_compare_ssn(jcntrl_data_array_name(x, &l), "X", 1))
        ret = 1;
      if (!test_compare_ii(l, ==, 1))
        ret = 1;
    }
    if (y) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(y), ==, 2))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(y, 0), ==, 0.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(y, 1), ==, 1.0))
        ret = 1;
      if (!test_compare_ssn(jcntrl_data_array_name(y, &l), "Y", 1))
        ret = 1;
      if (!test_compare_ii(l, ==, 1))
        ret = 1;
    }
    if (z) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(z), ==, 2))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(z, 0), ==, 0.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(z, 1), ==, 1.0))
        ret = 1;
      if (!test_compare_pp(jcntrl_data_array_name(z, &l), ==, NULL))
        ret = 1;
    }

    if (cdata) {
      if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 0))
        ret = 1;
    }
  }

  if (!test_compare_ii(grid_data_feeder_set(&f,
                                            ((jcntrl_extent){0, 3, 0, 2, 1, 2}),
                                            d(({0, 1, 2, 3})), d(({0, 1, 2})),
                                            d(({1, 2})), NOMASK,
                                            i(({1, 2, 3, 4, 5, 6}), "aaa"),
                                            c(({7, 8, 9, 10, 11, 12}), "bb"),
                                            d(({13, 14, 15, 16, 17, 18}), "d"),
                                            s(({19, 20, 21, 22, 23, 24}), "c")),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  o = NULL;
  g = NULL;
  if (!test_compare_pp(
        (o = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)), !=,
        NULL))
    ret = 1;

  if (o) {
    if (!test_compare_pp((g = jcntrl_grid_data_downcast(o)), !=, NULL))
      ret = 1;
  }

  if (g) {
    jcntrl_mask_data *m;
    jcntrl_cell_data *cdata;
    jcntrl_data_array *x, *y, *z;
    jcntrl_size_type l;

    cdata = NULL;
    x = y = z = NULL;

    if (!test_compare_pp((x = jcntrl_grid_data_x_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((y = jcntrl_grid_data_y_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((z = jcntrl_grid_data_z_coords(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(g)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((m = jcntrl_grid_data_get_mask(g)), ==, NULL))
      ret = 1;

    if (x) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(x), ==, 4))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 0), ==, 0.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 1), ==, 1.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 2), ==, 2.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(x, 3), ==, 3.0))
        ret = 1;
      if (!test_compare_pp(jcntrl_data_array_name(x, &l), ==, NULL))
        ret = 1;
    }
    if (y) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(y), ==, 3))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(y, 0), ==, 0.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(y, 1), ==, 1.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(y, 2), ==, 2.0))
        ret = 1;
      if (!test_compare_pp(jcntrl_data_array_name(y, &l), ==, NULL))
        ret = 1;
    }
    if (z) {
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(z), ==, 2))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(z, 0), ==, 1.0))
        ret = 1;
      if (!test_compare_dd(jcntrl_data_array_get_value(z, 1), ==, 2.0))
        ret = 1;
      if (!test_compare_pp(jcntrl_data_array_name(z, &l), ==, NULL))
        ret = 1;
    }

    if (cdata) {
      jcntrl_data_array *d1, *d2, *d3, *d4;

      if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 4))
        ret = 1;

      d1 = d2 = d3 = d4 = NULL;
      if (!test_compare_pp((d1 = jcntrl_cell_data_get_array(cdata, 0)), !=,
                           NULL))
        ret = 1;
      if (!test_compare_pp((d2 = jcntrl_cell_data_get_array(cdata, 1)), !=,
                           NULL))
        ret = 1;
      if (!test_compare_pp((d3 = jcntrl_cell_data_get_array(cdata, 2)), !=,
                           NULL))
        ret = 1;
      if (!test_compare_pp((d4 = jcntrl_cell_data_get_array(cdata, 3)), !=,
                           NULL))
        ret = 1;

      if (d1) {
        jcntrl_int_array *ip;
        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d1), ==, 6))
          ret = 1;

        if (!test_compare_pp((o = jcntrl_data_array_object(d1)), !=, NULL))
          ret = 1;

        if (!test_compare_ssn(jcntrl_data_array_name(d1, &l), "aaa", 3))
          ret = 1;

        if (!test_compare_ii(l, ==, 3))
          ret = 1;

        ip = NULL;
        if (o) {
          if (!test_compare_pp((ip = jcntrl_int_array_downcast(o)), !=, NULL))
            ret = 1;
        }

        if (ip) {
          const int *p;

          p = NULL;
          if (!test_compare_pp((p = jcntrl_int_array_get(ip)), !=, NULL))
            ret = 1;
          if (p) {
            if (!test_compare_ii(p[0], ==, 1))
              ret = 1;
            if (!test_compare_ii(p[1], ==, 2))
              ret = 1;
            if (!test_compare_ii(p[2], ==, 3))
              ret = 1;
            if (!test_compare_ii(p[3], ==, 4))
              ret = 1;
            if (!test_compare_ii(p[4], ==, 5))
              ret = 1;
            if (!test_compare_ii(p[5], ==, 6))
              ret = 1;
          }
        }
      }

      if (d2) {
        jcntrl_char_array *cp;

        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==, 6))
          ret = 1;

        if (!test_compare_pp((o = jcntrl_data_array_object(d2)), !=, NULL))
          ret = 1;

        if (!test_compare_ssn(jcntrl_data_array_name(d2, &l), "bb", 2))
          ret = 1;

        if (!test_compare_ii(l, ==, 2))
          ret = 1;

        cp = NULL;
        if (o) {
          if (!test_compare_pp((cp = jcntrl_char_array_downcast(o)), !=, NULL))
            ret = 1;
        }

        if (cp) {
          const char *p;

          p = NULL;
          if (!test_compare_pp((p = jcntrl_char_array_get(cp)), !=, NULL))
            ret = 1;
          if (p) {
            if (!test_compare_ii(p[0], ==, 7))
              ret = 1;
            if (!test_compare_ii(p[1], ==, 8))
              ret = 1;
            if (!test_compare_ii(p[2], ==, 9))
              ret = 1;
            if (!test_compare_ii(p[3], ==, 10))
              ret = 1;
            if (!test_compare_ii(p[4], ==, 11))
              ret = 1;
            if (!test_compare_ii(p[5], ==, 12))
              ret = 1;
          }
        }
      }

      if (d3) {
        jcntrl_double_array *dp;

        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d3), ==, 6))
          ret = 1;

        if (!test_compare_pp((o = jcntrl_data_array_object(d3)), !=, NULL))
          ret = 1;

        if (!test_compare_ssn(jcntrl_data_array_name(d3, &l), "d", 1))
          ret = 1;

        if (!test_compare_ii(l, ==, 1))
          ret = 1;

        dp = NULL;
        if (o) {
          if (!test_compare_pp((dp = jcntrl_double_array_downcast(o)), !=,
                               NULL))
            ret = 1;
        }

        if (dp) {
          const double *p;

          p = NULL;
          if (!test_compare_pp((p = jcntrl_double_array_get(dp)), !=, NULL))
            ret = 1;
          if (p) {
            if (!test_compare_dd(p[0], ==, 13))
              ret = 1;
            if (!test_compare_dd(p[1], ==, 14))
              ret = 1;
            if (!test_compare_dd(p[2], ==, 15))
              ret = 1;
            if (!test_compare_dd(p[3], ==, 16))
              ret = 1;
            if (!test_compare_dd(p[4], ==, 17))
              ret = 1;
            if (!test_compare_dd(p[5], ==, 18))
              ret = 1;
          }
        }
      }

      if (d4) {
        jcntrl_size_array *sp;

        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d4), ==, 6))
          ret = 1;

        if (!test_compare_pp((o = jcntrl_data_array_object(d4)), !=, NULL))
          ret = 1;

        if (!test_compare_ssn(jcntrl_data_array_name(d4, &l), "c", 1))
          ret = 1;

        if (!test_compare_ii(l, ==, 1))
          ret = 1;

        sp = NULL;
        if (o) {
          if (!test_compare_pp((sp = jcntrl_size_array_downcast(o)), !=, NULL))
            ret = 1;
        }

        if (sp) {
          const jcntrl_size_type *p;

          p = NULL;
          if (!test_compare_pp((p = jcntrl_size_array_get(sp)), !=, NULL))
            ret = 1;
          if (p) {
            if (!test_compare_ii(p[0], ==, 19))
              ret = 1;
            if (!test_compare_ii(p[1], ==, 20))
              ret = 1;
            if (!test_compare_ii(p[2], ==, 21))
              ret = 1;
            if (!test_compare_ii(p[3], ==, 22))
              ret = 1;
            if (!test_compare_ii(p[4], ==, 23))
              ret = 1;
            if (!test_compare_ii(p[5], ==, 24))
              ret = 1;
          }
        }
      }
    }
  }

  if (!test_compare_ii(grid_data_feeder_set(&f,
                                            ((jcntrl_extent){0, 1, 0, 1, 0, 1}),
                                            d(({0, 1})), d(({0, 1})),
                                            d(({1, 2})), b(({0}))),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
    ret = 1;

  if (!test_compare_ii(grid_data_feeder_set(&f,
                                            ((jcntrl_extent){0, 1, 0, 1, 0, 1}),
                                            d(({0, 1})), d(({0, 1})),
                                            d(({1, 2})), d(({0}))),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_update(exe), ==, 0))
    ret = 1;

  {
    char cary[] = {0, 1, 2, 3, 4};
    int iary[] = {0, 1, 2, 3, 4};
    double dary[5] = {0.0};
    jcntrl_size_type sary[5] = {0};

    if (!test_compare_ii(grid_data_feeder_set(
                           &f, ((jcntrl_extent){0, 5, 0, 1, 0, 1}),
                           d(({0, 1, 2, 3, 4, 5})), dp(2, dary), d(({1, 2})),
                           NOMASK, dp(5, dary, "a"), cp(5, cary, "m"),
                           sp(5, sary, "z"), ip(5, iary, "i")),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      ret = 1;
  }

  grid_data_feeder_clean(&f);
  return ret;
}
