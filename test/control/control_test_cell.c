
#include "control_test.h"
#include "control_test_grid_data_feeder.h"
#include "jupiter/control/cell.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/grid_data.h"
#include "jupiter/control/information.h"
#include "jupiter/control/output.h"
#include "test-util.h"

#define XN(_1, _2, _3, _4, _5, _6, N, ...) N
#define XC(...) XN(__VA_ARGS__, 6, 5, 4, 3, 2, 1)
#define XT(x) JCNTRL_CELL_HEX_NEIGHBOR_##x
#define XV1(a) XT(a)
#define XV2(a, b) XV1(a) | XT(b)
#define XV3(a, b, c) XV2(a, b) | XT(c)
#define XV4(a, b, c, d) XV3(a, b, c) | XT(d)
#define XV5(a, b, c, d, e) XV4(a, b, c, d) | XT(e)
#define XV6(a, b, c, d, e, f) XV5(a, b, c, d, e) | XT(f)
#define XV(N, ...) (XV##N(__VA_ARGS__))
#define XE(N, ...) XV(N, __VA_ARGS__)

#define JCNTRL_CELL_HEX_NEIGHBOR_(...) XE(XC(__VA_ARGS__), __VA_ARGS__)

int test_control_cell(void)
{
  int ret;
  struct grid_data_feeder feeder;
  ret = 0;

  grid_data_feeder_init(&feeder);

  do {
    jcntrl_output *output;
    jcntrl_information *info;
    jcntrl_shared_object *obj;
    jcntrl_grid_data *grid;
    jcntrl_cell_hex h;
    double p[8][3];
    double center[3];

    if (!test_compare_ii(grid_data_feeder_set(&feeder,
                                              jcntrl_extent_c(
                                                (int[]){0, 4, 5, 8, 11, 14}),
                                              d(({0, 1, 2, 3, 4})),
                                              d(({10, 12, 14, 16})),
                                              d(({33, 36, 40, 45}))),
                         ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_executive_update(&feeder.executive), ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&feeder.executive, 0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((grid = jcntrl_grid_data_downcast(obj)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 5, 11, NULL), ==,
                         1))
      ret = 1;

    if (!test_compare_dd(h.p1[0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(h.p1[1], ==, 10.0))
      ret = 1;
    if (!test_compare_dd(h.p1[2], ==, 33.0))
      ret = 1;
    if (!test_compare_dd(h.p2[0], ==, 1.0))
      ret = 1;
    if (!test_compare_dd(h.p2[1], ==, 12.0))
      ret = 1;
    if (!test_compare_dd(h.p2[2], ==, 36.0))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==, JCNTRL_CELL_HEX_NEIGHBOR_(E, N, T)))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_number_of_points(&h), ==, 8))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 0, p[0]);
    if (!test_compare_dd(p[0][0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(p[0][1], ==, 10.0))
      ret = 1;
    if (!test_compare_dd(p[0][2], ==, 33.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 1, p[1]);
    if (!test_compare_dd(p[1][0], ==, 1.0))
      ret = 1;
    if (!test_compare_dd(p[1][1], ==, 10.0))
      ret = 1;
    if (!test_compare_dd(p[1][2], ==, 33.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 2, p[2]);
    if (!test_compare_dd(p[2][0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(p[2][1], ==, 12.0))
      ret = 1;
    if (!test_compare_dd(p[2][2], ==, 33.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 3, p[3]);
    if (!test_compare_dd(p[3][0], ==, 1.0))
      ret = 1;
    if (!test_compare_dd(p[3][1], ==, 12.0))
      ret = 1;
    if (!test_compare_dd(p[3][2], ==, 33.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 4, p[4]);
    if (!test_compare_dd(p[4][0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(p[4][1], ==, 10.0))
      ret = 1;
    if (!test_compare_dd(p[4][2], ==, 36.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 5, p[5]);
    if (!test_compare_dd(p[5][0], ==, 1.0))
      ret = 1;
    if (!test_compare_dd(p[5][1], ==, 10.0))
      ret = 1;
    if (!test_compare_dd(p[5][2], ==, 36.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 6, p[6]);
    if (!test_compare_dd(p[6][0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(p[6][1], ==, 12.0))
      ret = 1;
    if (!test_compare_dd(p[6][2], ==, 36.0))
      ret = 1;

    jcntrl_cell_hex_get_point(&h, 7, p[7]);
    if (!test_compare_dd(p[7][0], ==, 1.0))
      ret = 1;
    if (!test_compare_dd(p[7][1], ==, 12.0))
      ret = 1;
    if (!test_compare_dd(p[7][2], ==, 36.0))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 6.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 0.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 11.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 34.5))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 10.0, 33.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 10.0, 33.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 12.0, 33.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 10.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 12.0, 33.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 12.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 10.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 12.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.5, 10.0, 33.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.5, 12.0, 33.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.5, 10.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.5, 12.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 11.0, 33.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 11.0, 33.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 11.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 11.0, 36.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 10.0, 34.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 10.0, 34.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 12.0, 34.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 1.0, 12.0, 34.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.25, 10.75, 34.0), ==, 1))
      ret = 1;

    //----
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 7, 13, NULL), ==,
                         1))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==, JCNTRL_CELL_HEX_NEIGHBOR_(W, S, B)))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 10.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 3.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 15.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 42.5))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.25, 14.75, 44.0), ==, 1))
      ret = 1;

    //----
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 2, 7, 13, NULL), ==,
                         1))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==,
                         JCNTRL_CELL_HEX_NEIGHBOR_(W, E, S, B)))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 10.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 2.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 15.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 42.5))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 16.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 14.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 16.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.5, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.5, 16.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.5, 14.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.5, 16.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 15.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 15.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 14.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.0, 16.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 2.25, 14.75, 44.0), ==, 1))
      ret = 1;

    //----
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 6, 13, NULL), ==,
                         1))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==,
                         JCNTRL_CELL_HEX_NEIGHBOR_(W, S, N, B)))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 10.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 3.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 13.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 42.5))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 12.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 12.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 12.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 12.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 12.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 12.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 13.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 13.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 13.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 13.0, 45.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 12.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 12.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.25, 13.75, 44.0), ==, 1))
      ret = 1;

    //----
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 7, 12, NULL), ==,
                         1))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==,
                         JCNTRL_CELL_HEX_NEIGHBOR_(W, S, B, T)))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 8.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 3.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 15.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 38.0))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 36.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 38.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 38.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 38.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 38.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.25, 14.75, 39.0), ==, 1))
      ret = 1;

    //----
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 7, 13,
                                                (int[]){0, 5, 0, 11, 0, 25}),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(h.neighbors, ==,
                         JCNTRL_CELL_HEX_NEIGHBOR_(W, E, S, N, B, T)))
      ret = 1;

    if (!test_compare_dd(jcntrl_cell_hex_volume(&h), ==, 10.0))
      ret = 1;

    jcntrl_cell_hex_center(&h, center);
    if (!test_compare_dd(center[0], ==, 3.5))
      ret = 1;
    if (!test_compare_dd(center[1], ==, 15.0))
      ret = 1;
    if (!test_compare_dd(center[2], ==, 42.5))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 0.0, 0.0, 0.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 14.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.5, 16.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 40.0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 40.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 15.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 15.0, 45.0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 14.0, 42.5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 14.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.0, 16.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 4.0, 16.0, 42.5), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_contain(&h, 3.25, 14.75, 44.0), ==, 1))
      ret = 1;

    //---
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, -1, 5, 11, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 4, 11, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 5, 10, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 4, 7, 13, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 8, 13, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 7, 14, NULL), ==,
                         0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 3, 7, 14,
                                                (int[]){0, 5, 0, 11, 0, 99}),
                         ==, 0))
      ret = 1;
  } while (0);

  grid_data_feeder_clean(&feeder);
  return ret;
}
