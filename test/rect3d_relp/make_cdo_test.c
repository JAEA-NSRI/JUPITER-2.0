
#include <jupiter/func.h>
#include <jupiter/struct.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#include <math.h>

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

int make_cdo_test(void)
{
  int r;
  domain cdo;
  mpi_param mpi;

  set_self_comm_mpi(&mpi);
  memset(&cdo, 0, sizeof(domain));

  r = 0;
  if (!make_cdo_v(&cdo, 4, 5, 6, 1, 2, 0.0, 0.1, 0.3, 0.6, 0.9, 1.0, 5.0, 6.0,
                  7.0, 7.5, 9.0, 0., 10., 40., 50., 55., 60., 70.))
    r = 1;

  test_fail_if(!test_compare_ii(cdo.mx, ==, 4));
  test_fail_if(!test_compare_ii(cdo.my, ==, 5));
  test_fail_if(!test_compare_ii(cdo.mz, ==, 6));
  test_fail_if(!test_compare_ii(cdo.stm, ==, 1));
  test_fail_if(!test_compare_ii(cdo.stp, ==, 2));
  test_fail_if(!test_compare_ii(cdo.nx, ==, 1));
  test_fail_if(!test_compare_ii(cdo.ny, ==, 2));
  test_fail_if(!test_compare_ii(cdo.nz, ==, 3));
  test_fail_if(!test_compare_ii(cdo.n, ==, 6));
  test_fail_if(!test_compare_ii(cdo.m, ==, 120));
  test_fail_if(!test_compare_ii(cdo.nxy, ==, 2));
  test_fail_if(!test_compare_ii(cdo.mxy, ==, 20));

  test_fail_if(!test_compare_dd(cdo.xv[0], ==, 0.0));
  test_fail_if(!test_compare_dd(cdo.xv[1], ==, 0.1));
  test_fail_if(!test_compare_dd(cdo.xv[2], ==, 0.3));
  test_fail_if(!test_compare_dd(cdo.xv[3], ==, 0.6));
  test_fail_if(!test_compare_dd(cdo.xv[4], ==, 0.9));
  test_fail_if(!test_compare_eps(cdo.Lx, 0.2, 1e-6));

  test_fail_if(!test_compare_eps(cdo.xc[0], 0.05, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[1], 0.20, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[2], 0.45, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[3], 0.75, 1e-6));

  test_fail_if(!test_compare_eps(cdo.dxv[0], 0.1, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxv[1], 0.2, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxv[2], 0.3, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxv[3], 0.3, 1e-6));

  test_fail_if(!test_compare_eps(cdo.dxc[0], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxc[1], 0.25, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxc[2], 0.30, 1e-6));

  test_fail_if(!test_compare_eps(cdo.dxcn[0], 0.05, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcn[1], 0.10, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcn[2], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcn[3], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcp[0], 0.05, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcp[1], 0.10, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcp[2], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxcp[3], 0.15, 1e-6));

  test_fail_if(!test_compare_dd(cdo.dxvn[0], <=, -HUGE_VAL));
  test_fail_if(!test_compare_eps(cdo.dxvn[1], 0.05, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvn[2], 0.10, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvn[3], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvn[4], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvp[0], 0.05, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvp[1], 0.10, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvp[2], 0.15, 1e-6));
  test_fail_if(!test_compare_eps(cdo.dxvp[3], 0.15, 1e-6));
  test_fail_if(!test_compare_dd(cdo.dxvp[4], >=, HUGE_VAL));

  test_fail_if(!test_compare_dd(cdo.yv[0], ==, 1.0));
  test_fail_if(!test_compare_dd(cdo.yv[1], ==, 5.0));
  test_fail_if(!test_compare_dd(cdo.yv[2], ==, 6.0));
  test_fail_if(!test_compare_dd(cdo.yv[3], ==, 7.0));
  test_fail_if(!test_compare_dd(cdo.yv[4], ==, 7.5));
  test_fail_if(!test_compare_dd(cdo.yv[5], ==, 9.0));
  test_fail_if(!test_compare_eps(cdo.Ly, 2.0, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.yc[0], 3.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.yc[1], 5.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.yc[2], 6.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.yc[3], 7.25, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.yc[4], 8.25, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dyv[0], 4.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyv[1], 1.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyv[2], 1.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyv[3], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyv[4], 1.5, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dyc[0], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyc[1], 1.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyc[2], 0.75, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyc[3], 1.0, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dycn[0], 2.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycn[1], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycn[2], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycn[3], 0.25, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycn[4], 0.75, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycp[0], 2.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycp[1], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycp[2], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycp[3], 0.25, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dycp[4], 0.75, 1.0e-6));

  test_fail_if(!test_compare_dd(cdo.dyvn[0], <=, -HUGE_VAL));
  test_fail_if(!test_compare_eps(cdo.dyvn[1], 2.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvn[2], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvn[3], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvn[4], 0.25, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvn[5], 0.75, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvp[0], 2.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvp[1], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvp[2], 0.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvp[3], 0.25, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dyvp[4], 0.75, 1.0e-6));
  test_fail_if(!test_compare_dd(cdo.dyvp[5], >=, HUGE_VAL));

  test_fail_if(!test_compare_dd(cdo.zv[0], ==, 0.0));
  test_fail_if(!test_compare_dd(cdo.zv[1], ==, 10.0));
  test_fail_if(!test_compare_dd(cdo.zv[2], ==, 40.0));
  test_fail_if(!test_compare_dd(cdo.zv[3], ==, 50.0));
  test_fail_if(!test_compare_dd(cdo.zv[4], ==, 55.0));
  test_fail_if(!test_compare_dd(cdo.zv[5], ==, 60.0));
  test_fail_if(!test_compare_dd(cdo.zv[6], ==, 70.0));
  test_fail_if(!test_compare_eps(cdo.Lz, 45.0, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.zc[0], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zc[1], 25.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zc[2], 45.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zc[3], 52.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zc[4], 57.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zc[5], 65.0, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dzv[0], 10.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzv[1], 30.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzv[2], 10.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzv[3], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzv[4], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzv[5], 10.0, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dzc[0], 20.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzc[1], 20.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzc[2], 7.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzc[3], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzc[4], 7.5, 1.0e-6));

  test_fail_if(!test_compare_eps(cdo.dzcn[0], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcn[1], 15.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcn[2], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcn[3], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcn[4], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcn[5], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[0], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[1], 15.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[2], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[3], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[4], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzcp[5], 5.0, 1.0e-6));
  test_fail_if(!test_compare_dd(cdo.dzvn[0], <=, -HUGE_VAL));
  test_fail_if(!test_compare_eps(cdo.dzvn[1], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvn[2], 15.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvn[3], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvn[4], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvn[5], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvn[6], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[0], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[1], 15.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[2], 5.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[3], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[4], 2.5, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.dzvp[5], 5.0, 1.0e-6));
  test_fail_if(!test_compare_dd(cdo.dzvp[6], >=, HUGE_VAL));

  memset(&cdo, 0, sizeof(domain));

  if (!make_cdo_c(&mpi, &cdo, 6, 5, 4, 1, 1, 0.5, 1.5, 2.0))
    r = 1;

  test_fail_if(!test_compare_ii(cdo.mx, ==, 6));
  test_fail_if(!test_compare_ii(cdo.my, ==, 5));
  test_fail_if(!test_compare_ii(cdo.mz, ==, 4));
  test_fail_if(!test_compare_ii(cdo.stm, ==, 1));
  test_fail_if(!test_compare_ii(cdo.stp, ==, 1));
  test_fail_if(!test_compare_ii(cdo.nx, ==, 4));
  test_fail_if(!test_compare_ii(cdo.ny, ==, 3));
  test_fail_if(!test_compare_ii(cdo.nz, ==, 2));
  test_fail_if(!test_compare_ii(cdo.n, ==, 24));
  test_fail_if(!test_compare_ii(cdo.m, ==, 120));
  test_fail_if(!test_compare_ii(cdo.nxy, ==, 12));
  test_fail_if(!test_compare_ii(cdo.mxy, ==, 30));

  test_fail_if(!test_compare_dd(cdo.gLx, ==, 0.5));
  test_fail_if(!test_compare_dd(cdo.gLy, ==, 1.5));
  test_fail_if(!test_compare_dd(cdo.gLz, ==, 2.0));

  test_fail_if(!test_compare_eps(cdo.xv[0], -0.125, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[1], 0.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[2], 0.125, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[3], 0.25, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[4], 0.375, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[5], 0.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xv[6], 0.625, 1e-6));

  test_fail_if(!test_compare_eps(cdo.xc[0], -0.0625, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[1], 0.0625, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[2], 0.1875, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[3], 0.3125, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[4], 0.4375, 1e-6));
  test_fail_if(!test_compare_eps(cdo.xc[5], 0.5625, 1e-6));

  test_fail_if(!test_compare_eps(cdo.yv[0], -0.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yv[1], 0.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yv[2], 0.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yv[3], 1.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yv[4], 1.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yv[5], 2.0, 1e-6));

  test_fail_if(!test_compare_eps(cdo.yc[0], -0.25, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yc[1], 0.25, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yc[2], 0.75, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yc[3], 1.25, 1e-6));
  test_fail_if(!test_compare_eps(cdo.yc[4], 1.75, 1e-6));

  test_fail_if(!test_compare_eps(cdo.zv[0], -1.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zv[1], 0.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zv[2], 1.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zv[3], 2.0, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zv[4], 3.0, 1e-6));

  test_fail_if(!test_compare_eps(cdo.zc[0], -0.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zc[1], 0.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zc[2], 1.5, 1e-6));
  test_fail_if(!test_compare_eps(cdo.zc[3], 2.5, 1e-6));

  return r;
}
