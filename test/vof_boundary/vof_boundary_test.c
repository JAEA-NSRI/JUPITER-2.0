#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jupiter/func.h"
#include "jupiter/boundary_util.h"
#include "jupiter/component_data.h"

typedef struct {
  domain d;
  flags flg;
  mpi_param mpi;
  parameter prm;
  variable val;
  component_data comp[2];
  fluid_boundary_data inlet, wall, out;
} fixture;

static int failures;

static void close_to(type actual, type expected, const char *what)
{
  if (!isfinite(actual) || fabs(actual - expected) > 2.0e-6) {
    fprintf(stderr, "%s: got %.12g, expected %.12g\n", what,
             (double)actual, (double)expected);
    ++failures;
  }
}

static struct boundary_array *face_array(fixture *t, int face)
{
  struct boundary_array *b[6] = {
    &t->val.bnd_B, &t->val.bnd_T, &t->val.bnd_S,
    &t->val.bnd_N, &t->val.bnd_W, &t->val.bnd_E,
  };
  return b[face];
}

static int address(int x, int y, int z)
{
  return x + 15 * y + 225 * z;
}

static void init(fixture *t)
{
  memset(t, 0, sizeof(*t));
  domain *d = &t->d;
  d->nx = d->ny = d->nz = 8;
  d->stm = 3; d->stp = 4;
  d->mx = d->my = d->mz = 15; d->mxy = 225; d->m = 3375;
  d->stmb = d->stpb = 1; d->nbx = d->nby = d->nbz = 10;
  d->NBaseComponent = d->NumberOfComponent = 2;
  d->dx = d->dy = d->dz = 1.; d->dxi = d->dyi = d->dzi = 1.;
  d->dt = .1; d->cfl_num = .2;
  t->prm.cdo = d; t->prm.flg = &t->flg; t->prm.mpi = &t->mpi;
  t->flg.fluid_dynamics = ON;
  t->mpi.npe = 1;
  for (int k = 0; k < 6; ++k) t->mpi.nrk[k] = -1;
  component_data_init(&t->prm.comps_data_head);
  for (int c = 0; c < 2; ++c) {
    component_data_init(&t->comp[c]);
    t->comp[c].comp_index = t->comp[c].jupiter_id = c;
    component_phases_set(&t->comp[c].phases, COMPONENT_PHASE_LIQUID, 1);
    geom_list_insert_prev(&t->prm.comps_data_head.list, &t->comp[c].list);
  }
  t->inlet.cond = INLET; t->wall.cond = WALL; t->out.cond = OUT;
  t->inlet.comps = inlet_component_data_new(2);
  for (int c = 0; c < 2; ++c) {
    struct inlet_component_element *e = inlet_component_data_get(t->inlet.comps, c);
    e->comp.d = &t->comp[c]; e->comp.id = c;
    e->ratio.current_value = c == 0 ? 1. : 3.;
  }
  for (int k = 0; k < 6; ++k) {
    struct boundary_array *b = face_array(t, k);
    b->fl = calloc(100, sizeof(*b->fl));
    for (int j = 0; j < 100; ++j) b->fl[j] = &t->wall;
  }
  t->val.fl = calloc(2 * d->m, sizeof(type));
  t->val.fs = calloc(2 * d->m, sizeof(type));
  t->val.fs_sum = calloc(d->m, sizeof(type));
  t->val.u = calloc(d->m, sizeof(type));
  t->val.v = calloc(d->m, sizeof(type));
  t->val.w = calloc(d->m, sizeof(type));
}

static void destroy(fixture *t)
{
  for (int k = 0; k < 6; ++k) free(face_array(t, k)->fl);
  inlet_component_data_delete(t->inlet.comps);
  free(t->val.fl); free(t->val.fs); free(t->val.fs_sum);
  free(t->val.u); free(t->val.v); free(t->val.w);
}

/* A partial inlet overrides a global wall. The opposite face is OUT. */
static void set_flow(fixture *t, int face)
{
  int axis = 2 - face / 2;
  int a = axis == 0 ? 1 : 0, b = axis == 2 ? 1 : 2;
  type *velocity[3] = {t->val.u, t->val.v, t->val.w};
  for (int ib = 3; ib <= 4; ++ib) {
    for (int ia = 3; ia <= 4; ++ia) {
      int bj = ia + 1 + (ib + 1) * 10;
      face_array(t, face)->fl[bj] = &t->inlet;
      face_array(t, face ^ 1)->fl[bj] = &t->out;
      for (int k = 0; k < 15; ++k) {
        int p[3];
        p[a] = ia + 3; p[b] = ib + 3; p[axis] = k;
        velocity[axis][address(p[0], p[1], p[2])] = face % 2 ? -1. : 1.;
      }
    }
  }
}

static type liquid_volume(fixture *t, int c)
{
  type sum = 0.;
  for (int z = 3; z < 11; ++z)
    for (int y = 3; y < 11; ++y)
      for (int x = 3; x < 11; ++x)
        sum += t->val.fl[c * t->d.m + address(x,y,z)];
  return sum;
}

static void test_ghosts(void)
{
  for (int face = 0; face < 6; ++face) {
    fixture t; init(&t); set_flow(&t, face);
    int axis = 2 - face / 2;
    int a = axis == 0 ? 1 : 0, b = axis == 2 ? 1 : 2;
    for (int j = 0; j < t.d.m; ++j) t.val.fs_sum[j] = .2;
    for (int c = 0; c < 2; ++c) {
      bcf_VOF_component(t.val.fl + c * t.d.m, c, 0, &t.val, &t.prm);
      int layers = face % 2 ? t.d.stp : t.d.stm;
      for (int k = 1; k <= layers; ++k) {
        int p[3]; p[a] = p[b] = 6;
        p[axis] = face % 2 ? 10 + k : 3 - k;
        close_to(t.val.fl[c * t.d.m + address(p[0],p[1],p[2])],
                  c == 0 ? .2 : .6, "mixture/solid inlet ghost");
      }
      close_to(liquid_volume(&t, c), 0., "ghost refresh changed interior");
    }
    /* The aggregate liquid field uses total liquid mixture ratio. */
    t.flg.solute_diff = ON;
    for (int j = 0; j < t.d.m; ++j) t.val.fs[j] = .4;
    bcf_VOF_component(t.val.fl, -2, 0, &t.val, &t.prm);
    int p[3]; p[a] = p[b] = 6; p[axis] = face % 2 ? 11 : 2;
    close_to(t.val.fl[address(p[0],p[1],p[2])], .6, "aggregate liquid inlet ghost");
    destroy(&t);
  }
}

static void test_fluxes(void)
{
  fixture t; init(&t);
  for (int face = 0; face < 6; ++face) {
    int axis = 2 - face / 2;
    int p[3] = {3,3,3}; p[axis] = face % 2 ? 7 : 0;
    int index = 4 + 4 * 10;
    for (int cond = 0; cond < 3; ++cond) {
      fluid_boundary_data *bc = cond == 0 ? &t.inlet : cond == 1 ? &t.out : &t.wall;
      face_array(&t, face)->fl[index] = bc;
      type fm = .2, fp = -.3;
      outflow_flux_direction_split_bc(&fm,&fp,p[0],p[1],p[2],axis,&t.val,&t.prm);
      close_to(face % 2 ? fp : fm, cond == 0 ? (face % 2 ? -.3 : .2) : 0.,
                "physical face inflow limiter");
      fm = -.2; fp = .3;
      outflow_flux_direction_split_bc(&fm,&fp,p[0],p[1],p[2],axis,&t.val,&t.prm);
      close_to(face % 2 ? fp : fm, face % 2 ? .3 : -.2, "outward flux changed");
    }
    /* A wall on the neighboring face cell must not inherit the inlet. */
    face_array(&t, face)->fl[index] = &t.inlet;
    p[axis == 0 ? 1 : 0] = 2;
    type fm = .2, fp = -.3;
    outflow_flux_direction_split_bc(&fm,&fp,p[0],p[1],p[2],axis,&t.val,&t.prm);
    close_to(face % 2 ? fp : fm, 0., "partial inlet leaked into wall");
    t.mpi.nrk[face] = 1;
    fm = .2; fp = -.3;
    outflow_flux_direction_split_bc(&fm,&fp,p[0],p[1],p[2],axis,&t.val,&t.prm);
    close_to(face % 2 ? fp : fm, face % 2 ? -.3 : .2, "MPI face flux changed");
    t.mpi.nrk[face] = -1;
  }
  destroy(&t);
}

static void test_advection(void)
{
  for (int scheme = PLIC; scheme <= THINC_AWLIC; ++scheme) {
    for (int face = 0; face < 6; ++face) {
      for (int order = 0; order < 6; ++order) {
        fixture t; init(&t); set_flow(&t, face);
        t.flg.interface_capturing_scheme = scheme; t.d.icnt = order;
        for (int c = 0; c < 2; ++c) {
          advection_vof(t.val.fl + c * t.d.m, t.val.u, t.val.v, t.val.w,
                         NULL, t.val.fs, t.val.fs_sum, c, 0, &t.val, &t.prm);
          close_to(liquid_volume(&t,c), c == 0 ? .1 : .3,
                    "liquid supplied to empty domain (four inlet cells)");
          close_to(t.d.dt, .1, "PLIC did not restore dt");
        }
        close_to(liquid_volume(&t,0), .1, "other component overwritten");
        destroy(&t);
      }
    }
  }
}

#ifdef JUPITER_MPI
static void test_mpi(void)
{
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (size < 2) return;

  /* Check that an MPI halo is exchanged, not reset to the local INLET. */
  fixture t; init(&t); set_flow(&t, 0);
  int status = 0;
  set_mpi_for_rank(&t.mpi, 1, 1, size, 0, 0, rank, &status);
  t.mpi.CommJUPITER = MPI_COMM_WORLD;
  for (int j = 0; j < t.d.m; ++j) t.val.fl[j] = .1 * (rank + 1);
  bcf_VOF_component(t.val.fl, 0, 0, &t.val, &t.prm);
  if (rank > 0)
    close_to(t.val.fl[address(6,6,2)], .1 * rank, "lower MPI ghost overwritten");
  if (rank + 1 < size)
    close_to(t.val.fl[address(6,6,11)], .1 * (rank + 2), "upper MPI ghost overwritten");
  destroy(&t);

  /* Transport across the rank interface, without external supply. */
  for (int scheme = PLIC; scheme <= THINC_AWLIC; ++scheme) {
    init(&t); set_flow(&t, 0);
    set_mpi_for_rank(&t.mpi, 1, 1, size, 0, 0, rank, &status);
    t.mpi.CommJUPITER = MPI_COMM_WORLD;
    t.flg.interface_capturing_scheme = scheme;
    if (rank == 0) {
      for (int j = 0; j < 100; ++j) t.val.bnd_B.fl[j] = &t.wall;
      for (int y = 6; y <= 7; ++y)
        for (int x = 6; x <= 7; ++x)
          t.val.fl[address(x,y,10)] = 1.;
    }
    advection_vof(t.val.fl, t.val.u, t.val.v, t.val.w, NULL,
                   t.val.fs, t.val.fs_sum, 0, 0, &t.val, &t.prm);
    type local = liquid_volume(&t,0), total;
    MPI_Allreduce(&local, &total, 1, MPI_TYPE, MPI_SUM, MPI_COMM_WORLD);
    close_to(total, 4., "MPI interface lost liquid volume");
    if (rank == 1 && !(local > 0.)) {
      fprintf(stderr, "No liquid crossed the MPI interface (scheme %d)\n", scheme);
      ++failures;
    }
    destroy(&t);
  }
}
#endif

int main(int argc, char **argv)
{
#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif
  test_ghosts();
  test_fluxes();
  test_advection();
#ifdef JUPITER_MPI
  test_mpi();
  MPI_Allreduce(MPI_IN_PLACE, &failures, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Finalize();
#endif
  if (!failures) puts("VOF boundary tests passed: six faces, four schemes, six sweep orders, two components.");
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
