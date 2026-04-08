#ifndef JUPITER_TEST_CONTROL_TEST_GRID_DATA_FEEDER_H
#define JUPITER_TEST_CONTROL_TEST_GRID_DATA_FEEDER_H

#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/executive_data.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/control/struct_data.h"

#include <string.h>

JUPITER_CONTROL_DECL_BEGIN

/**
 * @warning grid_data_feeder has no responsible for deallocating
 * cell_arrays, since it's public.
 */
struct grid_data_feeder
{
  jcntrl_executive executive;
  int number_of_pieces;            // Number of pieces
  int piece_index[3];              // Piece index
  jcntrl_extent whole_extent;      // Extent of whole data
  jcntrl_extent local_data_extent; // Extent of data array
  jcntrl_data_array *mask_array;   // Mask array
  jcntrl_struct_grid struct_grid;  // Struct grid data set
  jcntrl_size_type ncell_arrays;   // Size of cell arrays
  jcntrl_data_array **cell_arrays; // List of cell arrays
};
#define grid_data_feeder__ancestor jcntrl_executive
#define grid_data_feeder__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(grid_data_feeder);

JCNTRL_SHARED_METADATA_INIT_DECL(grid_data_feeder);

int grid_data_feeder_init(struct grid_data_feeder *p);

/**
 * This function also cleans up arrays in @p p->cell_arrays (because
 * grid_data_feeder_set() generates arrays so that a user won't take ownership
 * of them).
 *
 * To keep them after cleaning, take extra ownership of arrays assigned in cell
 * arrays.
 */
void grid_data_feeder_clean(struct grid_data_feeder *p);

static inline void grid_data_feeder_set_whole_extent(struct grid_data_feeder *p,
                                                     int extent[6])
{
  p->whole_extent = jcntrl_extent_c(extent);
}

static inline void grid_data_feeder_set_local_extent(struct grid_data_feeder *p,
                                                     int extent[6])
{
  p->local_data_extent = jcntrl_extent_c(extent);
}

struct grid_data_feeder_set_ary_info
{
  jcntrl_size_type n;
  const jcntrl_shared_object_data *p;
  void *ary;
  const char *name;
  jcntrl_size_type namelen;
};

int grid_data_feeder_set_grid(
  struct grid_data_feeder *p, jcntrl_extent data_extent,
  struct grid_data_feeder_set_ary_info *xary,
  struct grid_data_feeder_set_ary_info *yary,
  struct grid_data_feeder_set_ary_info *zary,
  struct grid_data_feeder_set_ary_info *mask, //
  jcntrl_size_type ncell_arrays, jcntrl_data_array **cell_arrays,
  struct grid_data_feeder_set_ary_info *cell_arrays_info);

#define grid_data_feeder_ary_unwrap(...) __VA_ARGS__
#define grid_data_feeder_ary_wrap(rawtype, data) \
  ((rawtype[])grid_data_feeder_ary_unwrap data)

static inline int grid_data_strlen(const char *nm)
{
  if (nm)
    return strlen(nm);
  return 0;
}

#define grid_data_feeder_ary_base_i(arytype, rawtype, data, nm)              \
  ((struct grid_data_feeder_set_ary_info){                                   \
    .n = sizeof(grid_data_feeder_ary_wrap(rawtype, data)) / sizeof(rawtype), \
    .p = JCNTRL_METADATA_INIT(arytype)(),                                    \
    .ary = grid_data_feeder_ary_wrap(rawtype, data),                         \
    .name = nm,                                                              \
    .namelen = grid_data_strlen(nm),                                         \
  })

#define grid_data_feeder_ary_base_j(key, n, data, name)                      \
  grid_data_feeder_ary_base_j##key(n, data, name, (name ? strlen(name) : 0), \
                                   &(struct grid_data_feeder_set_ary_info){0})

static inline struct grid_data_feeder_set_ary_info *
grid_data_feeder_ary_base_jc(jcntrl_size_type n, char *d, const char *name,
                             jcntrl_size_type namelen,
                             struct grid_data_feeder_set_ary_info *p)
{
  p->n = n;
  p->p = jcntrl_char_array_metadata_init();
  p->ary = d;
  p->name = name;
  p->namelen = namelen;
  return p;
}

static inline struct grid_data_feeder_set_ary_info *
grid_data_feeder_ary_base_jb(jcntrl_size_type n, char *d, const char *name,
                             jcntrl_size_type namelen,
                             struct grid_data_feeder_set_ary_info *p)
{
  p->n = n;
  p->p = jcntrl_bool_array_metadata_init();
  p->ary = d;
  p->name = name;
  p->namelen = namelen;
  return p;
}

static inline struct grid_data_feeder_set_ary_info *
grid_data_feeder_ary_base_ji(jcntrl_size_type n, int *d, const char *name,
                             jcntrl_size_type namelen,
                             struct grid_data_feeder_set_ary_info *p)
{
  p->n = n;
  p->p = jcntrl_int_array_metadata_init();
  p->ary = d;
  p->name = name;
  p->namelen = namelen;
  return p;
}

static inline struct grid_data_feeder_set_ary_info *
grid_data_feeder_ary_base_jd(jcntrl_size_type n, double *d, const char *name,
                             jcntrl_size_type namelen,
                             struct grid_data_feeder_set_ary_info *p)
{
  p->n = n;
  p->p = jcntrl_double_array_metadata_init();
  p->ary = d;
  p->name = name;
  p->namelen = namelen;
  return p;
}

static inline struct grid_data_feeder_set_ary_info *
grid_data_feeder_ary_base_js(jcntrl_size_type n, jcntrl_size_type *d,
                             const char *name, jcntrl_size_type namelen,
                             struct grid_data_feeder_set_ary_info *p)
{
  p->n = n;
  p->p = jcntrl_size_array_metadata_init();
  p->ary = d;
  p->name = name;
  p->namelen = namelen;
  return p;
}

#define grid_data_feeder_ary_base_i1(arytype, rawtype, data) \
  grid_data_feeder_ary_base_i(arytype, rawtype, data, NULL)

#define grid_data_feeder_ary_base_i2(arytype, rawtype, data, name) \
  grid_data_feeder_ary_base_i(arytype, rawtype, data, name)

#define grid_data_feeder_ary_base_p(_1, _2, n, ...) n

#define grid_data_feeder_ary_base_n(...) \
  grid_data_feeder_ary_base_p(__VA_ARGS__, 2, 1, 0)

#define grid_data_feeder_ary_base_x(f, n, ...) f##n(__VA_ARGS__)

#define grid_data_feeder_ary_base_e(f, n, ...) \
  grid_data_feeder_ary_base_x(f, n, __VA_ARGS__)

#define grid_data_feeder_ary_base(arytype, rawtype, ...)                \
  grid_data_feeder_ary_base_e(grid_data_feeder_ary_base_i,              \
                              grid_data_feeder_ary_base_n(__VA_ARGS__), \
                              arytype, rawtype, __VA_ARGS__)

#define grid_data_feeder_ary_c(...) \
  grid_data_feeder_ary_base(jcntrl_char_array, char, __VA_ARGS__)

#define grid_data_feeder_ary_b(...) \
  grid_data_feeder_ary_base(jcntrl_bool_array, char, __VA_ARGS__)

#define grid_data_feeder_ary_i(...) \
  grid_data_feeder_ary_base(jcntrl_int_array, int, __VA_ARGS__)

#define grid_data_feeder_ary_d(...) \
  grid_data_feeder_ary_base(jcntrl_double_array, double, __VA_ARGS__)

#define grid_data_feeder_ary_s(...) \
  grid_data_feeder_ary_base(jcntrl_size_array, jcntrl_size_type, __VA_ARGS__)

#define grid_data_feeder_ary_dummy
#define grid_data_feeder_ary_NOMASK NULL
#define grid_data_feeder_ary_0 NULL

#define grid_data_feeder_aryp_NOMASK NULL
#define grid_data_feeder_aryp_0 NULL
#define grid_data_feeder_aryp_c &grid_data_feeder_ary_c
#define grid_data_feeder_aryp_b &grid_data_feeder_ary_b
#define grid_data_feeder_aryp_i &grid_data_feeder_ary_i
#define grid_data_feeder_aryp_d &grid_data_feeder_ary_d
#define grid_data_feeder_aryp_s &grid_data_feeder_ary_s

#define grid_data_feeder_ary_base_j1(key, n, data) \
  grid_data_feeder_ary_base_j(key, n, data, NULL)

#define grid_data_feeder_ary_base_j2(key, n, data, name) \
  grid_data_feeder_ary_base_j(key, n, data, name)

#define grid_data_feeder_ary_basep(key, n, ...)                              \
  grid_data_feeder_ary_base_e(grid_data_feeder_ary_base_j,                   \
                              grid_data_feeder_ary_base_n(__VA_ARGS__), key, \
                              n, __VA_ARGS__)

#define grid_data_feeder_aryp_cp(n, ...) \
  grid_data_feeder_ary_basep(c, n, __VA_ARGS__)

#define grid_data_feeder_aryp_bp(n, ...) \
  grid_data_feeder_ary_basep(b, n, __VA_ARGS__)

#define grid_data_feeder_aryp_ip(n, ...) \
  grid_data_feeder_ary_basep(i, n, __VA_ARGS__)

#define grid_data_feeder_aryp_dp(n, ...) \
  grid_data_feeder_ary_basep(d, n, __VA_ARGS__)

#define grid_data_feeder_aryp_sp(n, ...) \
  grid_data_feeder_ary_basep(s, n, __VA_ARGS__)

#define grid_data_feeder_ary_cp *grid_data_feeder_aryp_cp
#define grid_data_feeder_ary_bp *grid_data_feeder_aryp_bp
#define grid_data_feeder_ary_ip *grid_data_feeder_aryp_ip
#define grid_data_feeder_ary_dp *grid_data_feeder_aryp_dp
#define grid_data_feeder_ary_sp *grid_data_feeder_aryp_sp

#define grid_data_feeder_arysym(x) grid_data_feeder_ary_##x
#define grid_data_feeder_arysymp(x) grid_data_feeder_aryp_##x

#define grid_data_cary_j1(x, ...) grid_data_feeder_arysym(x)

#define grid_data_cary_j2(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_feeder_arysym(y)

#define grid_data_cary_j3(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j2(y, __VA_ARGS__)

#define grid_data_cary_j4(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j3(y, __VA_ARGS__)

#define grid_data_cary_j5(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j4(y, __VA_ARGS__)

#define grid_data_cary_j6(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j5(y, __VA_ARGS__)

#define grid_data_cary_j7(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j6(y, __VA_ARGS__)

#define grid_data_cary_j8(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j7(y, __VA_ARGS__)

#define grid_data_cary_j9(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j8(y, __VA_ARGS__)

#define grid_data_cary_j10(x, y, ...) \
  grid_data_feeder_arysym(x), grid_data_cary_j9(y, __VA_ARGS__)

#define grid_data_cary_k0(n, ...) NULL
#define grid_data_cary_k1(n, x, ...) grid_data_feeder_arysymp(x)
#define grid_data_cary_k2(n, x, ...) grid_data_feeder_arysymp(x)

#define grid_data_cary_c0(n) NULL
#define grid_data_cary_c1(n) NULL
#define grid_data_cary_c2(n) ((jcntrl_data_array *[n]){NULL})

#define grid_data_cary_i0(n, ...) NULL
#define grid_data_cary_i1(n, ...) NULL
#define grid_data_cary_i2(n, x, ...) \
  ((struct grid_data_feeder_set_ary_info[n]){grid_data_cary_j##n(__VA_ARGS__)})

#define grid_data_feeder_cary_m(n, t, ...)                        \
  grid_data_cary_k##t(n, __VA_ARGS__), n, grid_data_cary_c##t(n), \
    grid_data_cary_i##t(n, __VA_ARGS__)

#define grid_data_feeder_cary_p(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, n, \
                                ...)                                           \
  n

#define grid_data_feeder_cary_n(...) \
  grid_data_feeder_cary_p(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0)

#define grid_data_feeder_cary_f(...) \
  grid_data_feeder_cary_p(__VA_ARGS__, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0)

#define grid_data_feeder_cary_e(f, n, t, ...) f(n, t, __VA_ARGS__)
#define grid_data_feeder_cary_x(n, t, ...) \
  grid_data_feeder_cary_e(grid_data_feeder_cary_m, n, t, __VA_ARGS__)

#define grid_data_feeder_cary_y(...)                            \
  grid_data_feeder_cary_x(grid_data_feeder_cary_n(__VA_ARGS__), \
                          grid_data_feeder_cary_f(__VA_ARGS__), __VA_ARGS__)

#define grid_data_feeder_set_v(p, data_extent, xary, yary, zary, ...)       \
  grid_data_feeder_set_grid(p, data_extent, grid_data_feeder_arysymp(xary), \
                            grid_data_feeder_arysymp(yary),                 \
                            grid_data_feeder_arysymp(zary),                 \
                            grid_data_feeder_cary_y(__VA_ARGS__))

/**
 * Uasges:
 *
 * A. Only for Grid data
 * ```
 * grid_data_feeder_set(&feeder, extent, d(({ x coordinate data })),
 *                                       d(({ y coordinate data })),
 *                                       d(({ z coordinate data })));
 * ```
 *
 * B. Grid data with mask
 * ```
 * grid_data_feeder_set(&feeder, extent, d(({ x coordinate data })),
 *                                       d(({ y coordinate data })),
 *                                       d(({ z coordinate data })),
 *                                       b(({ mask data }));
 * ```
 *
 * C. Grid data with cell array data (Use NOMASK or 0 instead of NULL, because
 *    it will be expanded before token concatenation)
 * ```
 * grid_data_feeder_set(&feeder, extent,
 *                      d(({ x coordinate data })), d(({ y coordinate data })),
 *                      d(({ z coordinate data })), NOMASK,
 *                      d(({ cell array 1 data }), name1),
 *                      d(({ cell array 2 data }), name2),
 *                      ...
 *                      d(({ cell array 9 data }), name9));
 * ```
 *
 * D. Grid data with mask and cell array data
 * ```
 * grid_data_feeder_set(&feeder, extent,
 *                      d(({ x coordinate data })), d(({ y coordinate data })),
 *                      d(({ z coordinate data })), b(({ mask data })),
 *                      d(({ cell array 1 data }), name1),
 *                      d(({ cell array 2 data }), name2),
 *                      ...
 *                      d(({ cell array 9 data }), name9));
 * ```
 *
 * Each array types are following convension:
 *
 *  - `d(({...}) [, name])` -> jcntrl_double_array
 *  - `b(({...}) [, name])` -> jcntrl_bool_array
 *  - `i(({...}) [, name])` -> jcntrl_int_array
 *  - `c(({...}) [, name])` -> jcntrl_char_array
 *  - `s(({...}) [, name])` -> jcntrl_size_array
 *
 *  - `dp(n, ptr [, name])` -> jcntrl_double_array
 *  - `bp(n, ptr [, name])` -> jcntrl_bool_array
 *  - `ip(n, ptr [, name])` -> jcntrl_int_array
 *  - `cp(n, ptr [, name])` -> jcntrl_char_array
 *  - `sp(n, ptr [, name])` -> jcntrl_size_array
 *
 * @note The parentheses which wraps array is required even if the content does
 *       not contain comma.
 */
#define grid_data_feeder_set(p, data_extent, xary, yary, ...)     \
  grid_data_feeder_set_v(p, data_extent, xary, yary, __VA_ARGS__, \
                         grid_data_feeder_ary_dummy)

JUPITER_CONTROL_DECL_END

#endif
