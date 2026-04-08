#ifndef JUPITER_TEST_RECT3D_RELP_TEST_H
#define JUPITER_TEST_RECT3D_RELP_TEST_H

#include <jupiter/common_util.h>
#include <jupiter/csvutil.h>
#include <jupiter/func.h>
#include <jupiter/non_uniform_grid.h>
#include <jupiter/random/random.h>
#include <jupiter/struct.h>

#include "test-util.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define get_ary_size(mx, my, mz) (((size_t)(mx) + (my) + (mz)) * 8 + 6)

static inline type *allocate_from_ary(type **ary, int n)
{
  type *p = *ary;
  *ary += n;
  return p;
}

static inline int make_cdo(domain *cdo, int mx, int my, int mz, int stm,
                           int stp, size_t szary, type *ary)
{
  CSVASSERT(mx > 0);
  CSVASSERT(my > 0);
  CSVASSERT(mz > 0);
  CSVASSERT(stm >= 0);
  CSVASSERT(stp >= 0);
  CSVASSERT(mx > stm + stp);
  CSVASSERT(my > stm + stp);
  CSVASSERT(mz > stm + stp);
  CSVASSERT(szary >= get_ary_size(mx, my, mz));

  memset(cdo, 0, sizeof(domain));

  cdo->mx = mx;
  cdo->my = my;
  cdo->mz = mz;
  cdo->stm = stm;
  cdo->stp = stp;
  cdo->nx = mx - stm - stp;
  cdo->ny = my - stm - stp;
  cdo->nz = mz - stm - stp;
  cdo->mxy = cdo->mx * cdo->my;
  cdo->nxy = cdo->nx * cdo->ny;
  cdo->m = cdo->mxy * cdo->mz;
  cdo->n = cdo->nxy * cdo->nz;
  cdo->gmx = cdo->mx;
  cdo->gmy = cdo->my;
  cdo->gmz = cdo->mz;
  cdo->gnx = cdo->nx;
  cdo->gny = cdo->ny;
  cdo->gnz = cdo->nz;
  cdo->xv = allocate_from_ary(&ary, cdo->mx + 1);
  cdo->yv = allocate_from_ary(&ary, cdo->my + 1);
  cdo->zv = allocate_from_ary(&ary, cdo->mz + 1);
  cdo->x = cdo->xv;
  cdo->y = cdo->yv;
  cdo->z = cdo->zv;
  cdo->gx = cdo->xv;
  cdo->gy = cdo->yv;
  cdo->gz = cdo->zv;
  cdo->xc = allocate_from_ary(&ary, cdo->mx);
  cdo->yc = allocate_from_ary(&ary, cdo->my);
  cdo->zc = allocate_from_ary(&ary, cdo->mz);
  cdo->dxv = allocate_from_ary(&ary, cdo->mx);
  cdo->dyv = allocate_from_ary(&ary, cdo->my);
  cdo->dzv = allocate_from_ary(&ary, cdo->mz);
  cdo->dxc = allocate_from_ary(&ary, cdo->mx - 1);
  cdo->dyc = allocate_from_ary(&ary, cdo->my - 1);
  cdo->dzc = allocate_from_ary(&ary, cdo->mz - 1);
  cdo->dxcp = allocate_from_ary(&ary, cdo->mx);
  cdo->dycp = allocate_from_ary(&ary, cdo->my);
  cdo->dzcp = allocate_from_ary(&ary, cdo->mz);
  cdo->dxcn = allocate_from_ary(&ary, cdo->mx);
  cdo->dycn = allocate_from_ary(&ary, cdo->my);
  cdo->dzcn = allocate_from_ary(&ary, cdo->mz);
  cdo->dxvp = allocate_from_ary(&ary, cdo->mx + 1);
  cdo->dyvp = allocate_from_ary(&ary, cdo->my + 1);
  cdo->dzvp = allocate_from_ary(&ary, cdo->mz + 1);
  cdo->dxvn = allocate_from_ary(&ary, cdo->mx + 1);
  cdo->dyvn = allocate_from_ary(&ary, cdo->my + 1);
  cdo->dzvn = allocate_from_ary(&ary, cdo->mz + 1);
  cdo->Lx = cdo->xv[cdo->nx + stm] - cdo->xv[stm];
  cdo->Ly = cdo->yv[cdo->ny + stm] - cdo->yv[stm];
  cdo->Lz = cdo->zv[cdo->nz + stm] - cdo->zv[stm];
  cdo->gLx = cdo->Lx;
  cdo->gLy = cdo->Ly;
  cdo->gLz = cdo->Lz;
  cdo->dx = cdo->Lx / cdo->nx;
  cdo->dy = cdo->Ly / cdo->ny;
  cdo->dz = cdo->Lz / cdo->nz;

  non_uniform_grid_set_derived_vars(cdo->mx, cdo->xv, cdo->xc, cdo->dxv,
                                    cdo->dxc, cdo->dxcp, cdo->dxcn, cdo->dxvp,
                                    cdo->dxvn);
  non_uniform_grid_set_derived_vars(cdo->my, cdo->yv, cdo->yc, cdo->dyv,
                                    cdo->dyc, cdo->dycp, cdo->dycn, cdo->dyvp,
                                    cdo->dyvn);
  non_uniform_grid_set_derived_vars(cdo->mz, cdo->zv, cdo->zc, cdo->dzv,
                                    cdo->dzc, cdo->dzcp, cdo->dzcn, cdo->dzvp,
                                    cdo->dzvn);
  return 1;
}

#define make_cdo_chk(mx, my, mz, stm, stp)                             \
  (test_compare_ii(mx, >, 0) && test_compare_ii(my, >, 0) &&           \
   test_compare_ii(mz, >, 0) && test_compare_ii(stm, >=, 0) &&         \
   test_compare_ii(stp, >=, 0) && test_compare_ii(mx, >, stm + stp) && \
   test_compare_ii(my, >, stm + stp) && test_compare_ii(mz, >, stm + stp))

#define make_cdo_v_args_chk(mx, my, mz, ...)          \
  (test_compare_uu(sizeof((type[]){__VA_ARGS__}), ==, \
                   sizeof(type) * (mx + my + mz + 3)))

/* make variable width domain */
#define make_cdo_v(cdo, mx, my, mz, stm, stp, ...)               \
  (make_cdo_chk(mx, my, mz, stm, stp) &&                         \
   make_cdo_v_args_chk(mx, my, mz, __VA_ARGS__) &&               \
   make_cdo(cdo, mx, my, mz, stm, stp, get_ary_size(mx, my, mz), \
            (type[get_ary_size(mx, my, mz)]){__VA_ARGS__}))

/* make constant width domain */
#define make_cdo_c(mpi, cdo, mx, my, mz, stm, stp, lx, ly, lz)          \
  (make_cdo_chk(mx, my, mz, stm, stp) &&                                \
   make_cdo(cdo, mx, my, mz, stm, stp, get_ary_size(mx, my, mz),        \
            (type[get_ary_size(mx, my, mz)]){[mx - stp] = lx,           \
                                             [mx + 1 + my - stp] = ly,  \
                                             [mx + my + 2 + mz - stp] = \
                                               lz}) &&                  \
   !init_mesh(cdo, mpi, &((flags){.has_non_uniform_grid = OFF})))

static inline void rect3d_relp_test_shuffle_axis(
  type *a, int n1, int n2, int n3, jupiter_random_seed *seed,
  ptrdiff_t (*addr)(int i1, int i2, int i3, int n1, int n2, int n3))
{
  if (n1 < 1)
    return;

  for (int i3 = 0; i3 < n3; ++i3) {
    for (int i2 = 0; i2 < n2; ++i2) {
      for (int i1 = 0; i1 < n1; ++i1) {
        int ii;
        ptrdiff_t jf, jt;
        type t;

        ii = jupiter_random_nextn(seed, i1);
        if (ii == i1)
          continue;

        jf = addr(i1, i2, i3, n1, n2, n3);
        jt = addr(ii, i2, i3, n1, n2, n3);
        t = a[jf];
        a[jf] = a[jt];
        a[jt] = t;
      }
    }
  }
}

static inline ptrdiff_t rect3d_relp_test_shuffle_addr_x(int i1, int i2, int i3,
                                                        int n1, int n2, int n3)
{
  return calc_address(i1, i2, i3, n1, n2, n3);
}

static inline ptrdiff_t rect3d_relp_test_shuffle_addr_y(int i1, int i2, int i3,
                                                        int n1, int n2, int n3)
{
  return calc_address(i3, i1, i2, n3, n1, n2);
}

static inline ptrdiff_t rect3d_relp_test_shuffle_addr_z(int i1, int i2, int i3,
                                                        int n1, int n2, int n3)
{
  return calc_address(i2, i3, i1, n2, n3, n1);
}

static inline void rect3d_relp_test_shuffle_X(type *a, int nx, int ny, int nz,
                                              jupiter_random_seed *seed)
{
  rect3d_relp_test_shuffle_axis(a, nx, ny, nz, seed,
                                rect3d_relp_test_shuffle_addr_x);
}

static inline void rect3d_relp_test_shuffle_Y(type *a, int nx, int ny, int nz,
                                              jupiter_random_seed *seed)
{
  rect3d_relp_test_shuffle_axis(a, ny, nz, nx, seed,
                                rect3d_relp_test_shuffle_addr_y);
}

static inline void rect3d_relp_test_shuffle_Z(type *a, int nx, int ny, int nz,
                                              jupiter_random_seed *seed)
{
  rect3d_relp_test_shuffle_axis(a, nz, nx, ny, seed,
                                rect3d_relp_test_shuffle_addr_z);
}

static inline type *rect3d_relp_test_shuffle(type *a, int nx, int ny, int nz,
                                             jupiter_random_seed *seed)
{
  if (!seed) {
    seed = &((jupiter_random_seed){0});
    jupiter_random_seed_fill_random(seed);
  }

  rect3d_relp_test_shuffle_X(a, nx, ny, nz, seed);
  rect3d_relp_test_shuffle_Y(a, nx, ny, nz, seed);
  rect3d_relp_test_shuffle_Z(a, nx, ny, nz, seed);
  return a;
}

static inline type *rect3d_relp_test_init(type *a, size_t n, int exp)
{
  for (size_t i = 0; i < n; ++i) {
#ifdef JUPITER_DOUBLE
    a[i] = ldexp((double)i, exp);
#else
    a[i] = ldexpf((float)i, exp);
#endif
  }
  return a;
}

#define random_type_seq(n, exp) rect3d_relp_test_init((type[n]){0.0}, n, exp)

#define random_type_value(nx, ny, nz, exp, seed)                             \
  rect3d_relp_test_shuffle(random_type_seq((size_t)(nx) * (ny) * (nz), exp), \
                           nx, ny, nz, seed)

/* tests */
int make_cdo_test(void);
int shuffle_test(void);
int rect3d_index_X_test(void);
int rect3d_index_Y_test(void);
int rect3d_index_Z_test(void);
int rect3d_index_C_test(void);
int rect3d_axis_X_test(void);
int rect3d_axis_Y_test(void);
int rect3d_axis_Z_test(void);

int rect3d1_relp_test(void);
int rect3d2_relp_test(void);

int rect3d1_boundary_test(void);

#endif
