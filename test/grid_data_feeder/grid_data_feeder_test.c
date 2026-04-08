
#include "jupiter/control/cell_data.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/grid_data.h"
#include "jupiter/control/information.h"
#include "jupiter/control/output.h"
#include "jupiter/func.h"
#include "jupiter/grid_data_feeder.h"
#include "jupiter/struct.h"
#include "test-util.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

static int grid_data_feeder_check_ary(jcntrl_cell_data *cdata, const char *name,
                                      domain *cdo, void *ptr, const char *file,
                                      long line, const char *ptrdesc)
{
  int r;
  jcntrl_data_array *ary;

  fprintf(stderr, "..... name = \"%s\"\n", name);
  if (!ptr) {
    return test_compare_x_pp(jcntrl_cell_data_get_array_by_name(cdata, name,
                                                                strlen(name)),
                             ==, NULL, ptrdesc, file, line);
  }

  r = 1;
  if (!test_compare_l_pp((ary =
                            jcntrl_cell_data_get_array_by_name(cdata, name,
                                                               strlen(name))),
                         !=, NULL, file, line))
    return 0;

  if (!test_compare_l_ii(jcntrl_data_array_get_ntuple(ary), ==, cdo->m, file,
                         line))
    r = 0;

  if (!test_compare_x_pp(jcntrl_data_array_get(ary), ==, ptr, ptrdesc, file,
                         line))
    r = 0;

  return r;
}

static int grid_data_feeder_check_aryv(jcntrl_cell_data *cdata, domain *cdo,
                                       void *ptr, const char *file, long line,
                                       const char *ptrdesc, const char *namefmt,
                                       va_list ap)
{
  char *buf;
  int r;
  r = test_compare_vasprintf(&buf, namefmt, ap);
  if (!test_compare_ii(r, >=, 0))
    return 0;

  r = grid_data_feeder_check_ary(cdata, buf, cdo, ptr, file, line, ptrdesc);
  free(buf);
  return r;
}

static int grid_data_feeder_check_aryf(jcntrl_cell_data *cdata, domain *cdo,
                                       void *ptr, const char *file, long line,
                                       const char *ptrdesc, const char *namefmt,
                                       ...)
{
  int r;
  va_list ap;
  va_start(ap, namefmt);
  r = grid_data_feeder_check_aryv(cdata, cdo, ptr, file, line, ptrdesc, namefmt,
                                  ap);
  va_end(ap);
  return r;
}

#define array_check_desc(ptr, ptrstr)                                        \
  (ptr ? "jcntrl_data_array_get(ary) == " ptrstr                             \
       : "jcntrl_cell_data_get_array_by_name(cdata, name, strlen(name)) == " \
         "NULL")

#define grid_data_feeder_check_ary(cdata, name, cdo, ptr)               \
  grid_data_feeder_check_ary(cdata, name, cdo, ptr, __FILE__, __LINE__, \
                             array_check_desc(ptr, #ptr))

#define grid_data_feeder_check_aryf(cdata, cdo, ptr, ...)          \
  grid_data_feeder_check_aryf(cdata, cdo, ptr, __FILE__, __LINE__, \
                              array_check_desc(ptr, #ptr), __VA_ARGS__)

static int grid_data_feeder_test_main(int argc, char **argv);

int main(int argc, char **argv)
{
  int r;
#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif

  r = grid_data_feeder_test_main(argc, argv);

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif
  return r;
}

int grid_data_feeder_test_main(int argc, char **argv)
{
  int r = 0;
  variable *val;
  material *mtl;
  parameter *prm;

  prm = NULL;
  val = NULL;
  mtl = NULL;

  do {
    prm = set_parameters(&argc, &argv, SET_PARAMETERS_ALL, ON);
    if (!test_compare_pp(prm, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(prm->cdo, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(prm->flg, !=, NULL)) {
      r = 1;
      break;
    }

    val = malloc_variable(prm->cdo, prm->flg);
    if (!test_compare_pp(val, !=, NULL)) {
      r = 1;
      break;
    }

    mtl = malloc_material(prm->cdo, prm->flg);
    if (!test_compare_pp(mtl, !=, NULL)) {
      r = 1;
      break;
    }

    fflush(stderr);
    fflush(stdout);
  } while (0);

  do {
    const int *extent;
    jcntrl_output *outp;
    jcntrl_information *info;
    jcntrl_executive *exe;
    jcntrl_shared_object *obj;
    jcntrl_grid_data *grid;
    jcntrl_data_array *ary;
    jcntrl_cell_data *cdata;

    if (r)
      break;

    fprintf(stderr, "\n\n===== Start grid_data_feeder test\n");
    if (!test_compare_pp(prm->grid_feeder, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(jupiter_grid_data_feeder_prm(prm->grid_feeder), ==,
                         prm))
      r = 1;

    jupiter_grid_data_feeder_set_val(prm->grid_feeder, val);
    jupiter_grid_data_feeder_set_mtl(prm->grid_feeder, mtl);

    if (!test_compare_pp((exe = jupiter_grid_data_feeder_executive(
                            prm->grid_feeder)),
                         !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      r = 1;

    if (!test_compare_pp((outp = jcntrl_executive_output_port(exe, 0)), !=,
                         NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(outp)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp((grid = jcntrl_grid_data_downcast(obj)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp((ary = jcntrl_grid_data_x_coords(grid)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_data_array_get(ary), ==, (void *)prm->cdo->x))
      r = 1;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(ary), ==,
                         prm->cdo->mx + 1))
      r = 1;

    if (!test_compare_pp((ary = jcntrl_grid_data_y_coords(grid)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_data_array_get(ary), ==, (void *)prm->cdo->y))
      r = 1;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(ary), ==,
                         prm->cdo->my + 1))
      r = 1;

    if (!test_compare_pp((ary = jcntrl_grid_data_z_coords(grid)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_data_array_get(ary), ==, (void *)prm->cdo->z))
      r = 1;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(ary), ==,
                         prm->cdo->mz + 1))
      r = 1;

    extent = NULL;
    if (!test_compare_pp((extent = jcntrl_grid_data_extent(grid)), !=, NULL))
      r = 1;

    if (extent) {
      if (!test_compare_ii(extent[0], ==,
                           prm->mpi->rank_x * prm->cdo->nx - prm->cdo->stm))
        r = 1;

      if (!test_compare_ii(extent[2], ==,
                           prm->mpi->rank_y * prm->cdo->ny - prm->cdo->stm))
        r = 1;

      if (!test_compare_ii(extent[4], ==,
                           prm->mpi->rank_z * prm->cdo->nz - prm->cdo->stm))
        r = 1;

      if (!test_compare_ii(extent[1], ==, extent[0] + prm->cdo->mx))
        r = 1;

      if (!test_compare_ii(extent[3], ==, extent[2] + prm->cdo->my))
        r = 1;

      if (!test_compare_ii(extent[5], ==, extent[4] + prm->cdo->mz))
        r = 1;
    }

    extent = NULL;
    if (!test_compare_pp((extent = jcntrl_information_get_extent(
                            info, JCNTRL_INFO_PIECE_EXTENT)),
                         !=, NULL))
      r = 1;

    if (extent) {
      if (!test_compare_ii(extent[0], ==, prm->mpi->rank_x * prm->cdo->nx))
        r = 1;

      if (!test_compare_ii(extent[2], ==, prm->mpi->rank_y * prm->cdo->ny))
        r = 1;

      if (!test_compare_ii(extent[4], ==, prm->mpi->rank_z * prm->cdo->nz))
        r = 1;

      if (!test_compare_ii(extent[1], ==, extent[0] + prm->cdo->nx))
        r = 1;

      if (!test_compare_ii(extent[3], ==, extent[2] + prm->cdo->ny))
        r = 1;

      if (!test_compare_ii(extent[5], ==, extent[4] + prm->cdo->nz))
        r = 1;
    }

    extent = NULL;
    if (!test_compare_pp((extent = jcntrl_information_get_extent(
                            info, JCNTRL_INFO_WHOLE_EXTENT)),
                         !=, NULL))
      r = 1;

    if (extent) {
      if (!test_compare_ii(extent[0], ==, 0))
        r = 1;

      if (!test_compare_ii(extent[2], ==, 0))
        r = 1;

      if (!test_compare_ii(extent[4], ==, 0))
        r = 1;

      if (!test_compare_ii(extent[1], ==, extent[0] + prm->cdo->gnx))
        r = 1;

      if (!test_compare_ii(extent[3], ==, extent[2] + prm->cdo->gny))
        r = 1;

      if (!test_compare_ii(extent[5], ==, extent[4] + prm->cdo->gnz))
        r = 1;
    }

    cdata = NULL;
    if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(grid)), !=, NULL))
      r = 1;

    if (cdata) {
      if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->t, "%stemperature",
                                       CONTROL_KEYCHAR_VARNAME))
        r = 1;
      if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->p, "%spressure",
                                       CONTROL_KEYCHAR_VARNAME))
        r = 1;

      if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->u, "%svelocity-x",
                                       CONTROL_KEYCHAR_VARNAME))
        r = 1;
      if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->v, "%svelocity-y",
                                       CONTROL_KEYCHAR_VARNAME))
        r = 1;
      if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->w, "%svelocity-z",
                                       CONTROL_KEYCHAR_VARNAME))
        r = 1;

      if (prm->flg->solute_diff == ON) {
        if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->fs,
                                         "%ssolid-vof-%d",
                                         CONTROL_KEYCHAR_VARNAME, 0))
          r = 1;

        for (int i = 1; i < prm->cdo->NBaseComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo, NULL,
                                           "%ssolid-vof-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }

        if (!grid_data_feeder_check_aryf(cdata, prm->cdo, val->fl,
                                         "%sliquid-vof-%d",
                                         CONTROL_KEYCHAR_VARNAME, 0))
          r = 1;

        for (int i = 1; i < prm->cdo->NBaseComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo, NULL,
                                           "%sliquid-vof-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }

        for (int i = 0; i < prm->cdo->NumberOfComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo,
                                           val->Y + i * prm->cdo->m,
                                           "%ssolute-Y-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }
      } else {
        for (int i = 0; i < prm->cdo->NBaseComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo,
                                           val->fs + i * prm->cdo->m,
                                           "%ssolid-vof-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }

        for (int i = 0; i < prm->cdo->NBaseComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo,
                                           val->fl + i * prm->cdo->m,
                                           "%sliquid-vof-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }

        for (int i = 0; i < prm->cdo->NumberOfComponent; ++i) {
          if (!grid_data_feeder_check_aryf(cdata, prm->cdo, NULL,
                                           "%ssolute-Y-%d",
                                           CONTROL_KEYCHAR_VARNAME, i))
            r = 1;
        }
      }
    }

  } while (0);

  if (prm)
    free_parameter(prm);
  if (val)
    free_variable(val);
  if (mtl)
    free_material(mtl);

  return r;
}
