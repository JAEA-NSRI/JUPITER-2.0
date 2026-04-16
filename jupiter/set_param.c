#include "component_data.h"
#include "component_data_defs.h"
#include "component_info.h"
#include "control/defs.h"
#include "control/error.h"
#include "control/mpi_controller.h"
#include "control/shared_object.h"
#include "control/shared_object_priv.h"
#include "controllable_type.h"
#include "geometry/bitarray.h"
#include "geometry/defs.h"
#include "geometry/list.h"
#include "geometry_source.h"
#include "grid_data_feeder.h"
#include "heat_source.h"
#include "if_binary.h"
#include "non_uniform_grid.h"
#include "os/asprintf.h"
#include "os/os.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* YSE: Using strlen, errno */
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#ifdef GPU
#include <cuda.h>
#include <cutil.h>
#endif

#include "struct.h"
#include "func.h"

/* YSE: Using CSV utilities */
#include "csv.h"
#include "csvutil.h"

/* YSE: Using geomtry library */
#include "geometry/data.h"

/* YSE: Using control library */
#include "control/manager.h"
#include "control/executive.h"

/* YSE: New option parser */
#include "optparse.h"

/* YSE: Table Free */
#include "table/table.h"

/* YSE: tm_table_[param|func2] */
#include "tmcalc.h"

/* YSE: dc_calc_param */
#include "dccalc.h"

/* YSE: for deallocation of boundary data */
#include "boundary_util.h"

/* YSE: Temperature dependent property data */
#include "tempdep_properties.h"

/* YSE: Free oxidation data */
#include "oxidation.h"

/* YSE: Control utility functions */
#include "field_control.h"
#include "fv_time.h"

#ifdef LPTX
#include "lptx/init_set.h"
#endif

enum solid_form *create_solid_form_array(phase_value_component *comps,
                                         int nbcompo)
{
  CSVASSERT(comps);
  CSVASSERT(nbcompo > 0);

  enum solid_form *sform =
    (enum solid_form *)calloc(nbcompo, sizeof(enum solid_form));
  if (!sform)
    return sform;

  for (int i = 0; i < nbcompo; ++i) {
    sform[i] = comps->sform;
  }
  return sform;
}

/* YSE: Common CSV reading and error processing function */
static csv_error open_read_csv(const char *fname, csv_data **csv_out)
{
  FILE *fp;
  int is_opened;
  csv_error r;
  long el;
  long ec;

  if (!fname) return 0;

  is_opened = 0;

  if (strcmp(fname, "-") == 0) {
    fp = stdin;
  } else {
    fp = fopen(fname, "rb");
    if (!fp) {
      csvperror(fname, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);
      return 1;
    }
    is_opened = 1;
  }

  errno = 0;
  r = readCSV(fp, csv_out, &el, &ec);

  if (is_opened) {
    fclose(fp);
  }

  if (r != CSV_ERR_SUCC) {
    csvperror(fname, el, ec, CSV_EL_ERROR, NULL, r, errno, 0, NULL);
  }

  return r;
}

/* YSE: Add function to read list of physical property files (`plist.txt`)
 *
 * Note: There was only one parameter* argument present in
 *       original version, we explicitly show what data is used or set
 *       via arguments.
 */
/**
 * @brief Read property data files
 * @param ncompo Number of components
 * @param plist_file Filename of where @p prop_files are read
 * @param comp_data_head Component data head
 * @return 0 if no fatal errors
 *
 * The input list must have size of least (ncompo + 1).
 * First element of list is for gas, second is for id 0, and so on.
 *
 * The result list has size of (ncompo + 2).
 * First element of list is for gas, second is for id 0, and so on.
 * Last element is always NULL.
 *
 * This function allocates space for prop_data and Read CSV data.
 *
 * For CSV error such as column not found etc., this function returns
 * 0 and set status to ON. If return value is not 0, a fatal error
 * occured.
 */
static int
read_property_files(const char *plist_file, component_data *comp_data_head,
                    int *status)
{
  struct geom_list *lp, *lh;
  int stat;

  stat = OFF;
  lh = &comp_data_head->list;
  geom_list_foreach(lp, lh) {
    component_data *cdata;
    FILE *fp;
    long l, c;
    csv_error cerr;

    cdata = component_data_entry(lp);

    if (cdata->generated)
      continue;

    if (!cdata->fname) {
      /* print only if user specified a file for list */
      if (plist_file) {
        csvperrorf(plist_file, 0, 0, CSV_EL_ERROR, NULL,
                   "No file specified for material id %d", cdata->jupiter_id);
      }
      stat = ON;
      continue;
    }

    errno = 0;
    fp = fopen(cdata->fname, "r");
    if (!fp) {
      csvperror(cdata->fname, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_FOPEN, errno, 0,
                NULL);
      stat = ON;
      continue;
    }

    errno = 0;
    cerr = readCSV(fp, &cdata->csv, &l, &c);
    if (cerr != CSV_ERR_SUCC) {
      csvperror(cdata->fname, l, c, CSV_EL_ERROR, NULL, cerr, errno, 0, NULL);
      stat = ON;
    }
    fclose(fp);
  }

  if (status) {
    if (stat != OFF) *status = ON;
  }

  return 0;
}

/**
 * @brief Set property list parameters
 * @param plist_data CSV data to read from
 * @param plist_file corresponding filename to plist_data
 * @param comp_data_head List of available components
 * @param status Pointer to store result.
 * @return 0 if successfully processed, otherwise failed.
 *
 * component_data will be allocated with generated == 0.
 *
 * For CSV error such as column not found etc., this function returns
 * 0 and set status to ON. If return value is not 0, a fatal error
 * occured.
 */
static int set_property_list_parameters(csv_data *plist_data,
                                        const char *plist_file,
                                        component_data *comp_data_head,
                                        int *status)
{
  csv_row *row;
  csv_column *col;
  int stat;
  SET_P_INIT(plist_data, plist_file, &row, &col);

  CSVASSERT(comp_data_head);

  stat = OFF;

  row = NULL;
  if (plist_data) {
    row = findCSVRow(plist_data, "Material_file", strlen("Material_file"));
  }
  for (; row; row = findCSVRowNext(row)) {
    component_data *p;
    int id;
    char *prop_file;
    col = getColumnOfCSV(row, 0);

    SET_P_NEXT_REQUIRED(&id, int, -1, &stat);
    p = component_data_find_by_jupiter_id(comp_data_head, id);
    if (!p || p->generated) {
      SET_P_PERROR(WARN, "This material id is not used");
      p = NULL;
    }

    SET_P_NEXT_REQUIRED(&prop_file, charp, NULL, &stat);
    if (prop_file) {
      if (p && p->fname) {
        SET_P_PERROR(ERROR,
                     "A file is already assigned for same id %d, '%s'",
                     p->jupiter_id, p->fname);
        stat = ON;
        free(prop_file);
        prop_file = NULL;
      }
    } else {
      SET_P_PERROR(ERROR, "No files specified (or memory allocation failed)");
      stat = ON;
    }
    if (p && prop_file)
      p->fname = prop_file;
  }

  if (stat != OFF) {
    if (status)
      *status = stat;
  }

  return 0;
}

/*
 * YSE: Adjust parameters to get CSV data from parent function.
 *
 * Note: Same reason to set_property_list_parameters,
 *       we wrote down the arguments explicitly used in the function.
 */
/**
 * @brief Allocate and set flag data.
 * @param flags_fname Filename of flag data
 * @param flags_csv CSV data of flags.
 * @param param_fname Filename of parameter data.
 * @param param_csv CSV data of parameter.
 * @param exec_argument CSV parsed data of command line arguments.
 * @param stat Set to ON if any errors occured.
 * @return Allocated pointer to flag data, NULL if allocation failed.
 */
flags *init_flag(const char *flags_fname, csv_data *flags_csv,
                 const char *param_fname, csv_data *param_csv,
                 jupiter_options *exec_argument, int *stat)
{
  flags *flg;
  /* YSE: Use calloc to clear allocated region and return if not allocated */
  flg = (flags *)calloc( sizeof(flags), 1 );
  if (!flg) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }

  /*
   * YSE: Initialize MPI-related variable (actual value of
   *      MPI_FILE_NULL is implementation dependent).
   */
#ifdef JUPITER_MPI
  flg->list_fp_mpi = MPI_FILE_NULL;
  flg->list_fp_comm = MPI_COMM_NULL;
#endif

  /* YSE: Adjust arguments to set_flags */
  set_flags(flg, flags_fname, flags_csv, param_fname, param_csv,
            exec_argument, stat);
  return flg;
}

timer_main *init_timer_main(int argc, char *argv[])
{
  timer_main *time;
  time = (timer_main *) malloc( sizeof(timer_main) );
  /* YSE: Return NULL if cannot be allocated. */
  if (!time) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }

  time->total = 0.0;
  time->dtctl = 0.0;
  time->vof   = 0.0;
  time->rk3   = 0.0;
  time->rk3_level_set = 0.0;
  time->rk3_advection = 0.0;
  time->rk3_viscosity = 0.0;
  time->rk3_surface_tension = 0.0;
  time->rk3_forchheimer = 0.0;
  time->rk3_overhead = 0.0;
  time->heat  = 0.0;
  time->radiation  = 0.0; // << 2016 Added by KKE
  time->phase = 0.0;
  time->eutectic = 0.0;
  time->div   = 0.0;
  time->matel = 0.0;
  time->solute = 0.0;
  time->hsource = 0.0;
  time->data  = 0.0;
  return time;
}

#ifdef GPU
// using GPU library
gpu_param  *init_gpu(int argc, char *argv[], int gpu_id, flags *flg)
//==================================================
{
  gpu_param *gpu;
  gpu = (gpu_param *) malloc( sizeof(gpu_param) );
  CUT_DEVICE_INIT( argc, argv );
  cudaGetDeviceCount(&gpu->numGPUs);
  cudaSetDevice(gpu_id);
  if (flg->print == ON) printf("Initailize GPU ...\n");
  return gpu;
}
#endif

static void calc_local_range(int rank, int n, int stm, int stp, //
                             int *is, int *ie)
{
  *is = rank * n;
  *ie = *is + n + stp + 1;
  *is -= stm;
}

static type calc_local_min_dv(int n, int stm, type *dv)
{
  type min = dv[stm];
  for (int j = 1; j < n; ++j)
    if (min > dv[j + stm])
      min = dv[j + stm];

  return min;
}

static type calc_global_min_dv(int n, int stm, type *v)
{
  type min;
  type *dv = (type *)calloc(n, sizeof(type));
  if (!dv)
    return HUGE_VAL;

  non_uniform_grid_set_derived_vars(n, v + stm, NULL, dv, NULL, NULL, NULL,
                                    NULL, NULL);
  min = calc_local_min_dv(n, 0, dv);
  free(dv);
  return min;
}

int init_mesh(domain *cdo, mpi_param *mpi, flags *flg)
{
  int  j;
  /* YSE: Assert the pointers must be present */
  type *x, *y, *z, *gx, *gy, *gz;

  CSVASSERT(cdo);
  CSVASSERT(mpi);
  CSVASSERT(flg);

  x  = cdo->x,  y  = cdo->y,  z  = cdo->z;
  gx = cdo->gx, gy = cdo->gy, gz = cdo->gz;

  CSVASSERT(x);
  CSVASSERT(y);
  CSVASSERT(z);
  CSVASSERT(gx);
  CSVASSERT(gy);
  CSVASSERT(gz);
  CSVASSERT(x == cdo->xv);
  CSVASSERT(y == cdo->yv);
  CSVASSERT(z == cdo->zv);

  if (flg->has_non_uniform_grid == ON) {
    int gis, gie, gjs, gje, gks, gke;
    int lis, lie, ljs, lje, lks, lke;
    int circ = 0;

    calc_local_range(0, cdo->gnx, cdo->stm, cdo->stp, &gis, &gie);
    calc_local_range(0, cdo->gny, cdo->stm, cdo->stp, &gjs, &gje);
    calc_local_range(0, cdo->gnz, cdo->stm, cdo->stp, &gks, &gke);
    calc_local_range(mpi->rank_x, cdo->nx, cdo->stm, cdo->stp, &lis, &lie);
    calc_local_range(mpi->rank_y, cdo->ny, cdo->stm, cdo->stp, &ljs, &lje);
    calc_local_range(mpi->rank_z, cdo->nz, cdo->stm, cdo->stp, &lks, &lke);

    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_x_head, gis, gie, circ, gx);
    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_y_head, gjs, gje, circ, gy);
    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_z_head, gks, gke, circ, gz);
    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_x_head, lis, lie, circ, x);
    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_y_head, ljs, lje, circ, y);
    non_uniform_grid_build_mesh(&cdo->non_uniform_grid_z_head, lks, lke, circ, z);

  } else {
    type dx, dy, dz;
    dx = cdo->Lx / cdo->nx;
    dy = cdo->Ly / cdo->ny;
    dz = cdo->Lz / cdo->nz;

    // coordinates(global)
    for(j=0; j<cdo->gmx+1; j++) gx[j] = (j - cdo->stm)*dx;
    for(j=0; j<cdo->gmy+1; j++) gy[j] = (j - cdo->stm)*dy;
    for(j=0; j<cdo->gmz+1; j++) gz[j] = (j - cdo->stm)*dz;

    // coordinates(local)
    for(j=0; j<cdo->mx+1; j++) x[j] = gx[mpi->rank_x * cdo->nx + j];
    for(j=0; j<cdo->my+1; j++) y[j] = gy[mpi->rank_y * cdo->ny + j];
    for(j=0; j<cdo->mz+1; j++) z[j] = gz[mpi->rank_z * cdo->nz + j];
  }

  non_uniform_grid_set_derived_vars(cdo->mx, cdo->xv, cdo->xc, cdo->dxv,
                                    cdo->dxc, cdo->dxcp, cdo->dxcn, cdo->dxvp,
                                    cdo->dxvn);
  non_uniform_grid_set_derived_vars(cdo->my, cdo->yv, cdo->yc, cdo->dyv,
                                    cdo->dyc, cdo->dycp, cdo->dycn, cdo->dyvp,
                                    cdo->dyvn);
  non_uniform_grid_set_derived_vars(cdo->mz, cdo->zv, cdo->zc, cdo->dzv,
                                    cdo->dzc, cdo->dzcp, cdo->dzcn, cdo->dzvp,
                                    cdo->dzvn);

  if (flg->has_non_uniform_grid == ON) {
    int inited = 0;
    type mind[3];

#ifdef JUPITER_MPI
    MPI_Initialized(&inited);
#endif
    if (inited) {
      mind[0] = calc_local_min_dv(cdo->nx, cdo->stm, cdo->dxv);
      mind[1] = calc_local_min_dv(cdo->ny, cdo->stm, cdo->dyv);
      mind[2] = calc_local_min_dv(cdo->nz, cdo->stm, cdo->dzv);

#ifdef JUPITER_MPI
      MPI_Allreduce(MPI_IN_PLACE, mind, 3, MPI_TYPE, MPI_MIN, mpi->CommJUPITER);
#endif
    } else {
      mind[0] = calc_global_min_dv(cdo->gnx, cdo->stm, cdo->gx);
      mind[1] = calc_global_min_dv(cdo->gny, cdo->stm, cdo->gy);
      mind[2] = calc_global_min_dv(cdo->gnz, cdo->stm, cdo->gz);
    }

    cdo->dx = mind[0];
    cdo->dy = mind[1];
    cdo->dz = mind[2];
  } else {
    cdo->dx = cdo->Lx / cdo->nx;
    cdo->dy = cdo->Ly / cdo->ny;
    cdo->dz = cdo->Lz / cdo->nz;
  }
  cdo->dxi = 1.0 / cdo->dx;
  cdo->dyi = 1.0 / cdo->dy;
  cdo->dzi = 1.0 / cdo->dz;
  cdo->width = 3.0 * cdo->dx; // surface width (for surface tension)

  return 0;
}

static void domain_vin_data_init(struct vin_data *p)
{
  p->comps = NULL;
  controllable_type_init(&p->u);
  controllable_type_init(&p->v);
  controllable_type_init(&p->w);
}

static void domain_pout_data_init(struct pout_data *p)
{
  controllable_type_init(&p->const_p);
}

/*
 * YSE: Adjust parameters to get CSV data from parent function.
 *
 * Note: Same reason to set_property_list_parameters,
 *       we wrote down the arguments explicitly used in the function.
 */
/**
 * @brief Allocate and set domain data.
 * @param flag flag data
 * @param mpi MPI parallelization info structure
 * @param param_fname Filename of parameter data
 * @param param_csv CSV data of parameter
 * @param controls Control data sets
 * @param control_head Head item of controllable_type.
 * @param stat Set to ON if any errors occured.
 * @return Allocated pointer to domain data, NULL if allocation failed.
 */
domain  *init_cdomain(flags *flg, mpi_param *mpi,
                      const char *param_fname, csv_data *param_csv,
                      jcntrl_executive_manager *controls,
                      controllable_type *control_head,
                      component_data *comp_data_head, int *stat)
//==================================================
{
  domain  *cdo;

  /* YSE: Use calloc to allocate memory, and return NULL if not allocated. */
  cdo = (domain *) calloc( sizeof(domain), 1 );
  if (!cdo) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }

  cdo->vof = (initial_vof_profile *) calloc( sizeof(initial_vof_profile), 1 );
  if (!cdo->vof) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    free(cdo);
    return NULL;
  }

  /* YSE: Init boundary chain head */
  fluid_boundary_data_init(&cdo->fluid_boundary_head);
  thermal_boundary_data_init(&cdo->thermal_boundary_head);
  surface_boundary_data_init(&cdo->surface_boundary_head);
  component_info_init(&cdo->lpt_mass_fractions);
  component_info_init(&cdo->mass_source_g_comps);
  ox_component_info_init(&cdo->ox_zry);
  ox_component_info_init(&cdo->ox_zro2);
  ox_component_info_init(&cdo->ox_h2);
  ox_component_info_init(&cdo->ox_h2o);
  write_field_variables_init(&cdo->write_field_variables_head);
  domain_vin_data_init(&cdo->vin_xm);
  domain_vin_data_init(&cdo->vin_xp);
  domain_vin_data_init(&cdo->vin_ym);
  domain_vin_data_init(&cdo->vin_yp);
  domain_vin_data_init(&cdo->vin_zm);
  domain_vin_data_init(&cdo->vin_zp);
  domain_pout_data_init(&cdo->p_xm);
  domain_pout_data_init(&cdo->p_xp);
  domain_pout_data_init(&cdo->p_ym);
  domain_pout_data_init(&cdo->p_yp);
  domain_pout_data_init(&cdo->p_zm);
  domain_pout_data_init(&cdo->p_zp);

  if (!jupiter_fv_time_init(cdo)) {
    free(cdo->vof);
    free(cdo);
    return NULL;
  }
  if (!jupiter_fv_delta_t_init(cdo)) {
    jupiter_fv_time_clean(cdo);
    free(cdo->vof);
    free(cdo);
    return NULL;
  }

  if (controls) {
    if (!jcntrl_executive_manager_add(controls, &cdo->fv_time)) {
      if (stat)
        *stat = ON;
    }
    if (!jcntrl_executive_manager_add(controls, &cdo->fv_delta_t)) {
      if (stat)
        *stat = ON;
    }
  }

  // set domain values
  /* YSE: Adjust argument */
  set_cdomain(flg, cdo, mpi, param_fname, param_csv, controls, control_head,
              comp_data_head, stat);
  cdo->icnt = 0;
  // allocate array of coordinates
  if (cdo->mx > 0 && cdo->my > 0 && cdo->mz > 0) {
    cdo->x  = (type *) malloc( sizeof(type)*(cdo->mx +1) );
    cdo->y  = (type *) malloc( sizeof(type)*(cdo->my +1) );
    cdo->z  = (type *) malloc( sizeof(type)*(cdo->mz +1) );
    cdo->gx = (type *) malloc( sizeof(type)*(cdo->gmx+1) );
    cdo->gy = (type *) malloc( sizeof(type)*(cdo->gmy+1) );
    cdo->gz = (type *) malloc( sizeof(type)*(cdo->gmz+1) );
    cdo->xv = cdo->x;
    cdo->yv = cdo->y;
    cdo->zv = cdo->z;
    cdo->xc = (type *)malloc(sizeof(type) * (cdo->mx));
    cdo->yc = (type *)malloc(sizeof(type) * (cdo->my));
    cdo->zc = (type *)malloc(sizeof(type) * (cdo->mz));
    cdo->dxv = (type *)malloc(sizeof(type) * (cdo->mx));
    cdo->dyv = (type *)malloc(sizeof(type) * (cdo->my));
    cdo->dzv = (type *)malloc(sizeof(type) * (cdo->mz));
    cdo->dxc = (type *)malloc(sizeof(type) * (cdo->mx - 1));
    cdo->dyc = (type *)malloc(sizeof(type) * (cdo->my - 1));
    cdo->dzc = (type *)malloc(sizeof(type) * (cdo->mz - 1));
    cdo->dxcp = (type *)malloc(sizeof(type) * (cdo->mx));
    cdo->dycp = (type *)malloc(sizeof(type) * (cdo->my));
    cdo->dzcp = (type *)malloc(sizeof(type) * (cdo->mz));
    cdo->dxcn = (type *)malloc(sizeof(type) * (cdo->mx));
    cdo->dycn = (type *)malloc(sizeof(type) * (cdo->my));
    cdo->dzcn = (type *)malloc(sizeof(type) * (cdo->mz));
    cdo->dxvp = (type *)malloc(sizeof(type) * (cdo->mx + 1));
    cdo->dyvp = (type *)malloc(sizeof(type) * (cdo->my + 1));
    cdo->dzvp = (type *)malloc(sizeof(type) * (cdo->mz + 1));
    cdo->dxvn = (type *)malloc(sizeof(type) * (cdo->mx + 1));
    cdo->dyvn = (type *)malloc(sizeof(type) * (cdo->my + 1));
    cdo->dzvn = (type *)malloc(sizeof(type) * (cdo->mz + 1));
    // set coordinate on cell-node
    init_mesh(cdo, mpi, flg);
  } else {
    if (stat) {
      if (*stat != ON)
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Tried allocate mesh array with size 0");
      *stat = ON;
    }
  }

  if (flg->print == ON) printf("Set computational domain...\n");
  return  cdo;
}

/* YSE: Add argument to pass CSV data */
/**
 * @brief Initialize phase data.
 * @param flg Flag data
 * @param cdo Computational domain data
 * @param plist_data CSV data of property list
 * @param plist_file file name correspond to plist_data
 * @param comp_data_head component data list
 * @param stat Set to ON if errors occured
 * @return Initialized data. If failed, returns NULL.
 *
 * See documentation of set_phase function for argument details.
 */
phase_value *init_phase(flags *flg, domain *cdo,
                        csv_data *plist_data, const char *plist_file,
                        csv_data *param_data, const char *param_file,
                        component_data *comp_data_head, int *stat)
//==================================================
{
  /* YSE: Add declaration of counter, separate declaration and set value */
  phase_value *phv;

  /* YSE: Assert parameters */
  CSVASSERT(flg);
  CSVASSERT(cdo);

  /* YSE: Allocate phase value and return if could not. */
  phv = malloc_phase_value(cdo, flg);
  //phv = (phase_value *) malloc( sizeof(phase_value) );
  if (!phv) return NULL;

  //--- set phisical values for three phase
  /* YSE: Adjust argument */
  set_phase(phv, flg, cdo, plist_file, plist_data, param_file, param_data,
            comp_data_head, stat);
  if (flg->print == ON) printf("Set phisical values for three phase...\n");
  return phv;
}

// Add laser parameter set function
laser *laser_param(const char *fname, csv_data *csv, int *stat){
  laser *lsr;
  lsr = (laser*)calloc(sizeof(laser),1);
  if (!lsr) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }
  set_laser(lsr, fname, csv, stat);
  return lsr;
}

#ifdef GPU
void set_GridBlock(gpu_param *gpu, domain *cdo)
//==================================================
{
  int  BDx = 32,
       BDy =  8;
  // Default setting of Grid & Block
  gpu->bdx = MIN2(BDx, cdo->nx);
  gpu->bdy = MIN2(BDy, cdo->ny);
  gpu->gdx = (cdo->nx - 1)/gpu->bdx + 1;
  gpu->gdy = (cdo->ny - 1)/gpu->bdy + 1;
  // Grid
  gpu->grid.x = gpu->gdx;
  gpu->grid.y = gpu->gdy;
  gpu->grid.z = 1;
  // Block
  gpu->block.x = gpu->bdx;
  gpu->block.y = 1;
  gpu->block.z = 1;

  printf("Set GPU Grids & Bloks ...\n");
}
#endif

/* YSE: add set_parameters_options */
parameter  *set_parameters(int *argc, char **argv[], enum set_parameters_options sopts, int parallel)
//========== called from main.c ====================
{
  parameter *prm;
  /* YSE: Options structure */
  int r;
  jupiter_options opts;

  /* YSE: Test paramaters */
  CSVASSERT(argc);
  CSVASSERT(argv);

  /* YSE: Use calloc for allocating memory to clear the content */
  prm = (parameter *) calloc( sizeof(parameter), 1 );
  /* YSE: Return if not allocated */
  if (!prm) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }

  /* YSE: Initialize controls list head */
  jcntrl_error_callback_set(field_control_errorhandler, NULL);
  controllable_type_init(&prm->control_head);
  component_data_init(&prm->comps_data_head);
  jupiter_geometry_source_install();
  prm->grid_feeder = jupiter_grid_data_feeder_new();

  /* YSE: Initialize status */
  prm->status = OFF;

  /* YSE: Reset errno (MPI_Init may set errno) */
  errno = 0;

  /* YSE: Parse options */
  jupiter_options_init(&opts, "0", jupiter_print_levels_non_debug());
  r = jupiter_optparse(&opts, argc, argv);
  if (r != CSV_ERR_SUCC) {
    prm->status = ON;
  }

  /* YSE: Read CSV data for each argument */
  if (sopts & SET_PARAMETERS_READ_PARAM) {
    if (opts.param_file) {
      prm->param_file = jupiter_strdup(opts.param_file);
    } else {
      csvperrorf((*argv)[0], 0, 0, CSV_EL_ERROR, NULL,
                 "No parameter input file given (-input)");
      prm->status = ON;
    }

    r = open_read_csv(prm->param_file, &prm->param_data);
    if (r != CSV_ERR_SUCC) prm->status = ON;
  } else {
    prm->param_file = NULL;
    prm->param_data = NULL;
  }

  if (sopts & SET_PARAMETERS_READ_FLAGS) {
    if (opts.flags_file) {
      prm->flags_file = jupiter_strdup(opts.flags_file);
    } else {
      csvperrorf((*argv)[0], 0, 0, CSV_EL_ERROR, NULL,
                 "No flags input file given (-flags)");
      prm->status = ON;
    }

    r = open_read_csv(prm->flags_file, &prm->flags_data);
    if (r != CSV_ERR_SUCC) prm->status = ON;
  } else {
    prm->flags_file = NULL;
    prm->flags_data = NULL;
  }

  if (sopts & SET_PARAMETERS_READ_PLIST) {
    if (opts.plist_file) {
      prm->plist_file = jupiter_strdup(opts.plist_file);
    } else {
      csvperrorf((*argv)[0], 0, 0, CSV_EL_ERROR, NULL,
                 "No property list input file given (-plist)");
      prm->status = ON;
    }

    r = open_read_csv(prm->plist_file, &prm->plist_data);
    if (r != CSV_ERR_SUCC) prm->status = ON;
  } else {
    prm->plist_file = NULL;
    prm->plist_data = NULL;
  }

  if (sopts & SET_PARAMETERS_READ_GEOMETRY) {
    if (opts.geom_file) {
      prm->geom_file = jupiter_strdup(opts.geom_file);
    }

    r = open_read_csv(prm->geom_file, &prm->geom_data);
    if (r != CSV_ERR_SUCC) prm->status = ON;
  } else {
    prm->geom_file = NULL;
    prm->geom_data = NULL;
  }

  if (sopts & SET_PARAMETERS_READ_CONTROL) {
    if (opts.control_file) {
      prm->control_file = jupiter_strdup(opts.control_file);
    }

    r = open_read_csv(prm->control_file, &prm->control_data);
    if (r != CSV_ERR_SUCC) prm->status = ON;
  } else {
    prm->control_file = NULL;
    prm->control_data = NULL;
  }

  prm->argc = *argc;
  prm->argv = *argv;

  if (prm->status == ON) return prm;
  if (sopts == SET_PARAMETERS_NONE) return prm;

  /* YSE: Initialize control executive manager */
  prm->controls = jcntrl_executive_manager_new();
  if (!prm->controls) {
    free_parameter(prm);
    return NULL;
  }

  if (prm->grid_feeder) {
    jcntrl_executive *exe;

    jupiter_grid_data_feeder_set_prm(prm->grid_feeder, prm);

    exe = jupiter_grid_data_feeder_executive(prm->grid_feeder);
    if (!jcntrl_executive_manager_add(prm->controls, exe)) {
      free_parameter(prm);
      return NULL;
    }

    /* Keep ownership for prm */
    jcntrl_shared_object_take_ownership(jcntrl_executive_object(exe));
  }

  if (sopts & SET_PARAMETERS_CONTROL) {
    read_control_names(prm->controls, prm->control_data, prm->control_file,
                       &prm->status);
    /* NOTE: Assumes `geom_in, ON` */
    if ((sopts & SET_PARAMETERS_GEOMETRY) && prm->geom_data) {
      int geom_num;
      geom_num = get_geom_num(prm->geom_data, prm->geom_file, NULL);
      if (geom_num > 0) {
        read_geom_names(prm->geom_data, prm->geom_file, prm->controls, geom_num,
                        &prm->status);
      }
    }
    if ((sopts & SET_PARAMETERS_CONTROL_GEOMETRY) && prm->control_data) {
      int geom_num;
      geom_num = get_geom_num(prm->control_data, prm->control_file, NULL);
      if (geom_num > 0) {
        read_geom_names(prm->control_data, prm->control_file, prm->controls,
                        geom_num, &prm->status);
      }
    }
  }

  if (sopts & SET_PARAMETERS_DOMAIN) {
    /* YSE: Pass CSV data for each functions */
    prm->flg = init_flag(prm->flags_file, prm->flags_data,
                         prm->param_file, prm->param_data,
                         &opts, &prm->status);
    if (!prm->flg) {
      free_parameter(prm);
      return NULL;
    }

    prm->time= init_timer_main(*argc, *argv);
    /* YSE: Add conditional for not allocated */
    if (!prm->time) {
      free_parameter(prm);
      return NULL;
    }
  } else {
    prm->flg = NULL;
  }

  if (sopts & SET_PARAMETERS_MPI) {
    int pex;
    int pey;
    int pez;
    int pea;
    int ngpu;

    if (parallel) {
      pex = prm->flg->pex;
      pey = prm->flg->pey;
      pez = prm->flg->pez;
      pea = prm->flg->pea;
      ngpu = prm->flg->numGPU;
    } else {
      pex = 1;
      pey = 1;
      pez = 1;
      pea = 0;
      ngpu = 0;
    }
    prm->mpi = mpi_init_param(*argc, *argv, pex, pey, pez, pea, ngpu,
                              &prm->status);
    /* YSE: Add conditional for not allocated */
    if (!prm->mpi) {
      free_parameter(prm);
      return NULL;
    }
  } else {
    prm->mpi = NULL;
  }

  if ((sopts & SET_PARAMETERS_LISTFP) && prm->flg && prm->mpi) {
    /* YSE: Add call to set (Open) input list result file */
    set_input_list_fp(prm->flg, prm->mpi, prm->flags_data, prm->flags_file,
                      &prm->status);
  }

#ifdef GPU
  if (sopts & SET_PARAMETERS_GPU) {
    prm->gpu = init_gpu(argc, argv, 0, prm->flg);
    /* YSE: Add conditional for not allocated */
    if (!prm->gpu) {
      free_parameter(prm);
      return NULL;
    }
  } else {
    prm->gpu = NULL;
  }
#endif

  if (sopts & SET_PARAMETERS_DOMAIN) {
    /* YSE: Pass CSV data for each functions */
    prm->cdo = init_cdomain(prm->flg, prm->mpi,
                            prm->param_file, prm->param_data,
                            prm->controls, &prm->control_head,
                            &prm->comps_data_head, &prm->status);
    /* Set laser parameter */
    prm->lsr = laser_param(prm->param_file, prm->param_data, &prm->status);
    /* YSE: Add conditional for not allocated */
    if (!prm->cdo || !prm->lsr) {
      free_parameter(prm);
      return NULL;
    }
  } else {
    prm->cdo = NULL;
  }

  if (sopts & SET_PARAMETERS_CONTROL) {
    CSVASSERT(prm->cdo);

    if (prm->control_data) {
      set_controls(prm->control_data, prm->control_file, prm->controls,
                   &prm->status, prm->flg, prm->cdo, prm->mpi);
    }
  }

  if (prm->controls) {
    controllable_type_update_all(prm->controls, &prm->control_head);
  }

  /* Read proprety list and read property CSV files */
  if (sopts & SET_PARAMETERS_READ_PROPS) {
    CSVASSERT(prm->cdo);
    r = set_property_list_parameters(prm->plist_data, prm->plist_file,
                                     &prm->comps_data_head, &prm->status);
    if (r != 0) {
      free_parameter(prm);
      return NULL;
    }

    /* Separated reading CSV files from set_property_list_parameters() */
    r = read_property_files(prm->plist_file, &prm->comps_data_head,
                            &prm->status);
    if (r != 0) {
      free_parameter(prm);
      return NULL;
    }
  }

  /* YSE: Pass CSV data for each functions */
  if (sopts & SET_PARAMETERS_PROPERTIES) {
    prm->phv = init_phase(prm->flg, prm->cdo,
                          prm->plist_data, prm->plist_file,
                          prm->param_data, prm->param_file,
                          &prm->comps_data_head, &prm->status);
    /* YSE: Add conditional for not allocated */
    if (!prm->phv) {
      free_parameter(prm);
      return NULL;
    }
  } else {
    prm->phv = NULL;
  }

  if (sopts & SET_PARAMETERS_GEOMETRY) {
    CSVASSERT(prm->flg);
    CSVASSERT(prm->cdo);

    prm->geom_sets = NULL;

    if (prm->flg->geom_in == ON) {
      int geom_num;
      csv_column *found_col;

      if (!prm->geom_file) {
        csvperrorf((*argv)[0], 0, 0, CSV_EL_ERROR, NULL,
                   "No geometry input file given (-geom)");
      }

      geom_num = get_geom_num(prm->geom_data, prm->geom_file, &found_col);
      if (geom_num == 0) {
        long l = -1;
        long c = -1;
        const char *val = NULL;

        if (found_col) {
          l = getCSVTextLineOrigin(found_col);
          c = getCSVTextColumnOrigin(found_col);
          val = getCSVValue(found_col);
        }
        csvperrorf(prm->geom_file, l, c, CSV_EL_WARN, val,
                   "Are you going to calculate without any initialization?");
      }
      if (geom_num >= 0) {
        prm->geom_sets =
          set_geom(prm->geom_data, prm->geom_file, &prm->comps_data_head,
                   prm->cdo->gnx, prm->cdo->gny, prm->cdo->gnz, geom_num,
                   &prm->status, prm->controls, &prm->control_head);

        if (prm->geom_sets) {
          jupiter_init_func_binary_set_cdo_to_all_data(prm->geom_sets,
                                                       prm->cdo);
          add_geom_sources(prm->controls, prm->cdo, prm->geom_sets);
        }
      }
    }
  } else {
    prm->geom_sets = NULL;
  }

  if (sopts & SET_PARAMETERS_CONTROL_GEOMETRY) {
    int geom_num;

    CSVASSERT(prm->flg);
    CSVASSERT(prm->cdo);

    if (!prm->control_file) {
      csvperrorf((*argv)[0], 0, 0, CSV_EL_INFO, NULL,
                 "No control input file given (-control)");
    }

    prm->control_sets = NULL;

    geom_num = get_geom_num(prm->control_data, prm->control_file, NULL);
    if (geom_num >= 0) {
      prm->control_sets =
        set_geom(prm->control_data, prm->control_file, &prm->comps_data_head,
                 prm->cdo->gnx, prm->cdo->gny, prm->cdo->gnz, geom_num,
                 &prm->status, prm->controls, &prm->control_head);

      if (prm->control_sets) {
        jupiter_init_func_binary_set_cdo_to_all_data(prm->control_sets,
                                                     prm->cdo);
        add_geom_sources(prm->controls, prm->cdo, prm->control_sets);
      }
    }
  } else {
    prm->control_sets = NULL;
  }

#ifdef GPU
  if (prm->gpu && prm->cdo) {
    set_GridBlock(prm->gpu, prm->cdo);// <= GPU
  }
#endif

  /*
   * Set indices for mass_source_g
   */
  if (prm->cdo) {
    int nc;
    nc = component_info_ncompo(&prm->cdo->mass_source_g_comps);
    for (int i = 0; i < nc; ++i) {
      component_data *d;
      d = component_info_getc(&prm->cdo->mass_source_g_comps, i);
      if (d)
        d->mass_source_g_index = i;
    }
  }

  /*
   * Add extra components to comps_data_head before here
   */
  if (prm->cdo)
    component_data_update_index(&prm->comps_data_head, prm->cdo, &prm->status);

  if (prm->cdo && prm->phv && prm->flg && prm->flg->solute_diff == ON) {
    ptrdiff_t np;
    set_solute_diffc_build_params(&prm->phv->diff_input_head,
                                  prm->cdo->NBaseComponent,
                                  &prm->phv->diff_params, &np, &prm->status);
    set_solute_diffc_build_params_g(&prm->phv->diff_g_input_head,
                                    prm->cdo->NBaseComponent,
                                    prm->cdo->NGasComponent,
                                    &prm->phv->diff_g_params, &np,
                                    &prm->status);
  }

  if(prm->mpi && prm->flg && prm->flg->radiation== ON){
    mpi_bcast_radiation_configuration(prm, prm->flg->fp);
  }

  if (prm->controls) {
    controllable_type_update_all(prm->controls, &prm->control_head);
  }

  if (sopts == SET_PARAMETERS_ALL) {
    print_param(prm);
  }

  return prm;
}

/* YSE: print_param is moved to print_param.c */

static type print_timer_line(FILE *fp, const char *name, type value, type total)
{
  type percent = 0.0;

  if (total > 0.0) {
    percent = 100.0*value/total;
  }
  fprintf(fp, "%-20s = %9.3e [sec/step],  %2.2f [percent]\n", name, value, percent);
  return percent;
}

void print_timer(parameter *prm)
{
  timer_main *t = prm->time;
  FILE *fp = prm->flg->fp;
  flags *flg = prm->flg;
  type percent_sum = 0.0;
  t->total = t->dtctl + t->vof + t->multi_layer + t->rk3 + t->heat + t->eutectic + t->solute +
             t->radiation + t->phase + t->div + t->oxide + t->matel +
             t->hsource + t->lpt + t->control_update +
             t->data; // << 2016 Added by KKE

  /* YSE: Total job must be measured in directly. */
  t->total_job  = cpu_time() - t->job_start;
#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &t->total_job, 1,
                MPI_TYPE, MPI_MAX, prm->mpi->CommJUPITER);
#endif
  if(prm->mpi->rank == 0 && prm->cdo->viewflg == 1) {
    percent_sum += print_timer_line(fp, "dt_control", t->dtctl, t->total);
    percent_sum += print_timer_line(fp, "vof_advection", t->vof, t->total);
    if (flg->multi_layer == ON || t->multi_layer > 0.0)
      percent_sum += print_timer_line(fp, "multi_layer", t->multi_layer, t->total);
    percent_sum += print_timer_line(fp, "level_set", t->rk3_level_set, t->total);
    percent_sum += print_timer_line(fp, "advection", t->rk3_advection, t->total);
    if (flg->visc_tvd3 == ON || t->rk3_viscosity > 0.0)
      percent_sum += print_timer_line(fp, "viscosity", t->rk3_viscosity, t->total);
    percent_sum += print_timer_line(fp, "surf_tension", t->rk3_surface_tension, t->total);
    if (flg->porous == ON || t->rk3_forchheimer > 0.0)
      percent_sum += print_timer_line(fp, "Forchheimer", t->rk3_forchheimer, t->total);
    percent_sum += print_timer_line(fp, "rk3_overhead", t->rk3_overhead, t->total);
    if (flg->heat_eq == ON || t->heat > 0.0)
      percent_sum += print_timer_line(fp, "heat_conduction", t->heat, t->total);
    if (flg->radiation == ON || t->radiation > 0.0)
      percent_sum += print_timer_line(fp, "radiation", t->radiation, t->total);
    if (flg->phase_change == ON || t->phase > 0.0)
      percent_sum += print_timer_line(fp, "phase_change", t->phase, t->total);
    if (t->eutectic > 0.0)
      percent_sum += print_timer_line(fp, "eutectic", t->eutectic, t->total);
    percent_sum += print_timer_line(fp, "divergence_free", t->div, t->total);
    if (t->solute > 0.0)
      percent_sum += print_timer_line(fp, "solute diff", t->solute, t->total);
    if (t->oxide > 0.0)
      percent_sum += print_timer_line(fp, "oxidation", t->oxide, t->total);
    percent_sum += print_timer_line(fp, "materials", t->matel, t->total);
    if (t->hsource > 0.0)
      percent_sum += print_timer_line(fp, "hsource", t->hsource, t->total);
#ifdef HAVE_LPT
    if (t->lpt > 0.0)
      percent_sum += print_timer_line(fp, "particle track", t->lpt, t->total);
#endif
    percent_sum += print_timer_line(fp, "update fv/stats", t->control_update, t->total);
    percent_sum += print_timer_line(fp, "output_data", t->data, t->total);
    fprintf(fp, "percent_sum       = %9.3e [percent]\n", percent_sum);
    fprintf(fp, "----------- total = %9.3e [sec/step]\n", t->total);
    fprintf(fp, "    < job total > = %9.3e [sec/step]\n", t->total_job);
    fprintf(fp, "\n");
  }
}

static void deallocate_fname_templates(const struct filename_template_data *p)
{
  free(p->time);
  free(p->comp_based);
  free(p->others);
}

static void deallocate_output_spec(const struct data_output_spec *p)
{
  free(p->readdir);
  free(p->writedir);
  deallocate_fname_templates(&p->filename_template);
}

/* YSE: Add deallocator of parameter and its subsidiaries */
void free_flags(flags *flg)
{
  if (flg) {
#ifdef JUPITER_MPI
    if (flg->list_fp_mpi != MPI_FILE_NULL)
      MPI_File_close(&flg->list_fp_mpi);
    if (flg->list_fp_comm != MPI_COMM_NULL)
      MPI_Comm_free(&flg->list_fp_comm);
#endif
    if (flg->list_fp) {
      if (flg->list_fp != stdout && flg->list_fp != stderr) {
        fclose(flg->list_fp);
      }
    }
    free(flg->list_fp_name);
    deallocate_output_spec(&flg->restart_data);
    deallocate_output_spec(&flg->output_data);
  }
  free(flg);
}

static void free_domain_vin_data(struct vin_data *d)
{
  if (!d)
    return;

  if (d->comps)
    inlet_component_data_delete(d->comps);

  controllable_type_remove_from_list(&d->u);
  controllable_type_remove_from_list(&d->v);
  controllable_type_remove_from_list(&d->w);
}

static void free_domain_pout_data(struct pout_data *d)
{
  if (!d)
    return;

  controllable_type_remove_from_list(&d->const_p);
}

static void particle_set_input_delete_all(struct particle_set_input *head)
{
  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe(lp, ln, lh) {
    struct particle_set_input *p;
    p = particle_set_input_entry(lp);
#ifdef LPTX
    if (p->set)
      LPTX_particle_init_set_delete(p->set);
#endif
    free(p);
  }
}

void free_domain(domain *cdo)
{
  if (cdo) {
    CSVASSERT(cdo->xv == cdo->x);
    CSVASSERT(cdo->yv == cdo->y);
    CSVASSERT(cdo->zv == cdo->z);

    free(cdo->vof);
    free(cdo->x);
    free(cdo->y);
    free(cdo->z);
    free(cdo->gx);
    free(cdo->gy);
    free(cdo->gz);
    free(cdo->xc);
    free(cdo->yc);
    free(cdo->zc);
    free(cdo->dxv);
    free(cdo->dyv);
    free(cdo->dzv);
    free(cdo->dxc);
    free(cdo->dyc);
    free(cdo->dzc);
    free(cdo->dxcp);
    free(cdo->dycp);
    free(cdo->dzcp);
    free(cdo->dxcn);
    free(cdo->dycn);
    free(cdo->dzcn);
    free(cdo->dxvp);
    free(cdo->dyvp);
    free(cdo->dzvp);
    free(cdo->dxvn);
    free(cdo->dyvn);
    free(cdo->dzvn);
    free_domain_vin_data(&cdo->vin_xm);
    free_domain_vin_data(&cdo->vin_xp);
    free_domain_vin_data(&cdo->vin_ym);
    free_domain_vin_data(&cdo->vin_yp);
    free_domain_vin_data(&cdo->vin_zm);
    free_domain_vin_data(&cdo->vin_zp);
    free_domain_pout_data(&cdo->p_xm);
    free_domain_pout_data(&cdo->p_xp);
    free_domain_pout_data(&cdo->p_ym);
    free_domain_pout_data(&cdo->p_yp);
    free_domain_pout_data(&cdo->p_zm);
    free_domain_pout_data(&cdo->p_zp);
    heat_source_param_delete_all(&cdo->heat_sources_head);
    fluid_boundary_data_delete_all(&cdo->fluid_boundary_head);
    thermal_boundary_data_delete_all(&cdo->thermal_boundary_head);
    surface_boundary_data_delete_all(&cdo->surface_boundary_head);
    ox_component_info_clear(&cdo->ox_zry);
    ox_component_info_clear(&cdo->ox_zro2);
    ox_component_info_clear(&cdo->ox_h2);
    ox_component_info_clear(&cdo->ox_h2o);
    free(cdo->lpt_outname);
    particle_set_input_delete_all(&cdo->lpt_particle_set_head);
    jcntrl_executive_delete(&cdo->fv_time);
    jcntrl_executive_delete(&cdo->fv_delta_t);
    write_field_variables_delete_all(&cdo->write_field_variables_head);
    component_info_clear(&cdo->lpt_mass_fractions);
    component_info_clear(&cdo->mass_source_g_comps);
    non_uniform_grid_input_data_delete_all(&cdo->non_uniform_grid_x_head);
    non_uniform_grid_input_data_delete_all(&cdo->non_uniform_grid_y_head);
    non_uniform_grid_input_data_delete_all(&cdo->non_uniform_grid_z_head);
  }
  free(cdo);
}

static void free_laser(laser *lsr)
{
  free(lsr);
}

static void free_tm_table_chain(struct tm_table_param *p)
{
  struct tm_table_param *n;
  if (!p) return;

  n = p->next;
  for (; p; p = n, n = (n ? n->next : NULL)) {
    table_free(p->table);
    free(p->table_file);
    free(p);
  }
}

static void free_tm_func2_chain(struct tm_func2_param *p)
{
  struct tm_func2_param *n;
  if (!p) return;

  n = p->next;
  for (; p; p = n, n = (n ? n->next : NULL)) {
    free(p);
  }
}

static void free_dc_param(struct dc_calc_param *p, size_t psz)
{
  ptrdiff_t i;

  if (!p) return;

  for (i = 0; i < psz; ++i) {
    dc_calc_param_clean(&p[i]);
  }
}

static void free_dc_param_input(struct dc_calc_param_input *ip)
{
  struct geom_list *lp, *lh, *ln;
  struct dc_calc_param_input *p;

  lh = &ip->list;
  geom_list_foreach_safe(lp, ln, lh) {
    p = dc_calc_param_input_entry(lp);
    dc_calc_param_clean(&p->data);
    free(p);
  }
}


static void clean_tempdep(phase_value_component *comp)
{
  tempdep_property_clean(&comp->beta);
  tempdep_property_clean(&comp->sigma);
  tempdep_property_clean(&comp->radf);
  tempdep_property_clean(&comp->rho_s);
  tempdep_property_clean(&comp->rho_l);
  tempdep_property_clean(&comp->emi_s);
  tempdep_property_clean(&comp->emi_l);
  tempdep_property_clean(&comp->emi_g);
  tempdep_property_clean(&comp->mu_s);
  tempdep_property_clean(&comp->mu_l);
  tempdep_property_clean(&comp->thc_s);
  tempdep_property_clean(&comp->thc_l);
  tempdep_property_clean(&comp->specht_s);
  tempdep_property_clean(&comp->specht_l);
}

static void free_phase(phase_value *phv, int ncompo, int nbcompo, int ngcompo)
{
  if (phv) {
    int i;
    if (phv->comps) {
      for (i = 0; i < ncompo; ++i) {
        clean_tempdep(&phv->comps[i]);
      }
      free(phv->comps);
    }
    tempdep_property_clean(&phv->rho_g);
    tempdep_property_clean(&phv->specht_g);
    tempdep_property_clean(&phv->mu_g);
    tempdep_property_clean(&phv->thc_g);
    free_tm_table_chain(phv->liq_tables);
    free_tm_table_chain(phv->sol_tables);
    free_tm_func2_chain(phv->liq_funcs);
    free_tm_func2_chain(phv->sol_funcs);
    free(phv->diff_params);
    free(phv->diff_g_params);
    free_dc_param_input(&phv->diff_input_head);
    free_dc_param_input(&phv->diff_g_input_head);
  }
  free(phv);
}

static void free_mpi(mpi_param *mpi)
{
  if (mpi) {
#ifdef JUPITER_MPI
    if (mpi->CommJUPITER != MPI_COMM_WORLD) {
      MPI_Comm_free(&mpi->CommJUPITER);
    }
    if (mpi->CommLx != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommLx);
    }
    if (mpi->CommLy != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommLy);
    }
    if (mpi->CommLz != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommLz);
    }
    if (mpi->CommSx != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommSx);
    }
    if (mpi->CommSy != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommSy);
    }
    if (mpi->CommSz != MPI_COMM_NULL) {
      MPI_Comm_free(&mpi->CommSz);
    }
#endif
    if (mpi->control_controller)
      jcntrl_mpi_controller_delete(mpi->control_controller);
  }
  free(mpi);
}

#ifdef GPU
static void free_gpu(gpu_param *gpu)
{
  free(gpu);
}
#endif

static void free_timer(timer_main *timer)
{
  free(timer);
}

void free_parameter(parameter *prm)
{
  int ncompo = 0;
  int nbcompo = 0;
  int ngcompo = 0;
  int i;

  if (prm) {
    if (prm->cdo) {
      ncompo = prm->cdo->NIComponent;
      nbcompo = prm->cdo->NIBaseComponent;
      ngcompo = prm->cdo->NIGasComponent;
    }
    if (prm->grid_feeder)
      jupiter_grid_data_feeder_delete(prm->grid_feeder);
    if (prm->controls) {
      jcntrl_executive_manager_delete(prm->controls);
    }

    free_flags(prm->flg);
    free_domain(prm->cdo);
    free_laser(prm->lsr);
    free_phase(prm->phv, ncompo, nbcompo, ngcompo + 1);
#ifdef GPU
    free_gpu(prm->gpu);
#endif
    free_mpi(prm->mpi);
    free_timer(prm->time);
    geom_data_delete(prm->geom_sets);
    geom_data_delete(prm->control_sets);

    freeCSV(prm->flags_data);
    freeCSV(prm->param_data);
    freeCSV(prm->plist_data);
    freeCSV(prm->geom_data);
    freeCSV(prm->control_data);
    free(prm->geom_file);
    free(prm->plist_file);
    free(prm->param_file);
    free(prm->flags_file);
    free(prm->control_file);

    component_data_delete_all(&prm->comps_data_head);
  }
  free(prm);
}
