/*=============================================
   prototype declaration       2011 / 07 / 15 |
             Kenta Sugihara & Susumu Yamashita|
---------------------------------------------*/
#ifndef FUNC_H
#define FUNC_H

/* YSE: Add to use the type defined in csv.h */
#include "common.h"
#include "struct.h"
#include "csv.h"
#include "optparse.h"
#include "geometry/svector_defs.h"
#include "geometry_source.h"
#include "lpt.h"
#include "control/defs.h"
#include "init_component.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//====================
// param.c
//--------------------
/* YSE: Add CSV file/and data parameters */
JUPITER_DECL
void set_flags(flags *flg,
               const char *flags_fname, csv_data *flags_csv,
               const char *param_fname, csv_data *param_csv,
               jupiter_options *exec_argument, int *stat);

/* YSE: Add file process in flags data which requires mpi_param */
JUPITER_DECL
void set_input_list_fp(flags *flg, mpi_param *mpi, csv_data *flags_data,
                       const char *flags_fname, int *stat);

/* YSE: Add plist file */
JUPITER_DECL
void set_phase(phase_value *phv, flags *flg, domain *cdo,
               const char *plist_file, csv_data *plist_data,
               const char *param_file, csv_data *param_data,
               component_data *comp_data_head, int *stat);

// cutting
JUPITER_DECL
void set_cdomain(flags *flg, domain *cdo, mpi_param *mpi, const char *fname,
                 csv_data *csv, jcntrl_executive_manager *manager,
                 controllable_type *control_head,
                 component_data *comp_data_head, int *stat);

// Add laser function
JUPITER_DECL
void set_laser(laser *lsr, const char *fname, csv_data *csv, int *stat);

/**
 * Build dc_calc_param array for solutes
 */
JUPITER_DECL
void set_solute_diffc_build_params(struct dc_calc_param_input *iparam_head,
                                   int NBCompo, struct dc_calc_param ***params,
                                   ptrdiff_t *nparam, int *stat);

/**
 * Build dc_calc_param array for gases
 */
JUPITER_DECL
void set_solute_diffc_build_params_g(struct dc_calc_param_input *iparam_head,
                                     int NBCompo, int NGCompo,
                                     struct dc_calc_param ***params,
                                     ptrdiff_t *nparam, int *stat);

/**
 * @brief Whether use equations for uniform grid.
 * @param flg Flag
 * @retval 0 Use the equation for non-uniform grid
 * @retval 1 Use the equation for uniform grid
 *
 * This function always returns 0 if the CMake variable
 * `JUPITER_USE_UNIFORM_GRID_EQUATION` is set to `OFF` (or
 * `JUPITER_USE_UNIFORM_GRID_EQ` is not defined when compiliing).
 *
 * @note The flag is based on whether the `variable_delta_[xyz]` is used. The
 *       `variable_delta_[xyz]` with single `CONST` function generates a
 *       uniform grid, but this function will return false (0).
 */
static inline int use_uniform_grid_eq(flags *flg)
{
#ifdef JUPITER_USE_UNIFORM_GRID_EQ
  return flg->has_non_uniform_grid != ON;
#else
  return 0;
#endif
}

//====================
// set_geom.c
//--------------------
/**
 * @brief Get NumberOfGeom value
 * @param geom_csv CSV data to read from
 * @param geom_file correspoding file name
 * @param ret Sets pointer to the column if non-null pointer given.
 * @return Number of geometries to read, negative value if error
 */
JUPITER_DECL
int get_geom_num(csv_data *geom_csv, const char *geom_file, csv_column **ret);

/**
 * @brief Parse CSV data of Geometry data and convert to struture data.
 * @param geom_data CSV data of geometry
 * @param geom_file File name corresponds to geom_data.
 * @param comp_data_head List of available comopnents
 * @param gnx Number of total cells over the world, X-direction
 * @param gny Number of total cells over the world, Y-direction
 * @param gnz Number of total cells over the world, Z-direction
 * @param geom_num Number of geometry entries to read (negative for nolimit)
 * @param status If not NULL, set it to ON if some errors occured
 * @param manager Controlling executive manager
 * @param control_head Controlling data type head
 * @return Constructed geometry structure. NULL for allocation failure,
 *         NULL CSV is given, or geom_num is 0.
 *
 * @p sform can be omitted. If not given, it'll skip the solid model
 * check (whether a solid model is enabled or not) on `Geom_vof` entry.
 */
JUPITER_DECL
geom_data *set_geom(csv_data *geom_csv, const char *geom_file,
                    component_data *comp_data_head, int gnx, int gny, int gnz,
                    int geom_num, int *status,
                    jcntrl_executive_manager *manager,
                    controllable_type *control_head);

JUPITER_DECL
void read_geom_names(csv_data *geom_csv, const char *geom_file,
                     jcntrl_executive_manager *manager, int geom_num,
                     int *status);

/**
 * @brief Add jupiter_geometry_sources for each geometry that has name
 * @param manager executive manager to add
 * @param cdo JUPITER domain data to bind
 * @param data geometry data to get from
 * @retval 0 success
 * @retval non-0 Failed
 */
JUPITER_DECL
int add_geom_sources(jcntrl_executive_manager *manager, domain *cdo,
                     geom_data *data);

/**
 * @brief Add jupiter_geometry_sources for an geometry element
 * @param manager executive manager to add
 * @param element geometry element to bind
 * @param cdo JUPITER domain data to bind
 * @param source returns the generated source
 * @retval 0 success (including element has no name)
 * @retval non-0 failed
 *
 * If @p element has a name, generates the jupiter_geometry_source and
 * add to the @p manager and returns it through @p source.
 *
 * If @p element has no name, @p source will be set to NULL and
 * returns success.
 *
 * If the name which is set to @p element is already bound, reset with
 * given element and domain.
 */
JUPITER_DECL
int add_geom_source_for_element(jcntrl_executive_manager *manager,
                                geom_data_element *element, domain *cdo,
                                jupiter_geometry_source **source);

//====================
// set_param.c
//--------------------
/* YSE: Add customization of input processing for external utility */
/**
 * @brief customization of input process
 *
 * Use bitwise-or (ex. `SET_PARAMETERS_READ_PARAM |
 * SET_PARAMETERS_FLAGS | SET_PARAMETERS_DOMAIN`) to combine.
 *
 * - `SET_PARAMETERS_DOMAIN` requires `SET_PARAMATERS_READ_PARAM` and
 *   `SET_PARAMETERS_READ_FLAGS` are set.
 * - `SET_PARAMETERS_READ_PROPS` requires `SET_PARAMETERS_READ_PLIST` and
 *   `SET_PARAMETERS_DOMAIN` are set.
 * - `SET_PARAMETERS_PROPERTIES` requires `SET_PARAMETERS_READ_PROPS`
 *   and `SET_PARAMETRES_DOMAIN` are set.
 * - `SET_PARAMETERS_GEOMETRY` requires `SET_PARAMETERS_READ_GEOMETRY`
 *   and `SET_PARAMETERS_DOMAIN` are set.
 * - `SET_PARAMETERS_MPI` requires `SET_PARAMETERS_DOMAIN` is set.
 * - `SET_PARAMETERS_MPI_NOT_PARALLEL` requires `SET_PARAMETERS_DOMAIN`
 *   is set, and `SET_PARAMETERS_MPI` must not be set.
 * - `SET_PARAMETERS_GPU` requires `SET_PARAMETERS_DOMAIN` is set.
 * - `SET_PARAMETERS_LISTFP` requires `SET_PARAMETERS_DOMAIN` and
 *   `SET_PARAMETERS_MPI` is set.
 */
enum set_parameters_options
{
  SET_PARAMETERS_NONE = 0x0,        /*!< Do nothing, just parse command lines */
  SET_PARAMETERS_READ_PARAM = 0x01, /*!< Read parameter file */
  SET_PARAMETERS_READ_FLAGS = 0x02, /*!< Read flags file */
  SET_PARAMETERS_READ_PLIST = 0x04, /*!< Read property list file */
  SET_PARAMETERS_READ_GEOMETRY = 0x08, /*!< Read geometry file */
  SET_PARAMETERS_READ_PROPS = 0x10,    /*!< Read each property files */
  SET_PARAMETERS_READ_CONTROL = 0x20,  /*!< Read control file */
  SET_PARAMETERS_DOMAIN = 0x0100,      /*!< Initialize domain and flags */
  SET_PARAMETERS_PROPERTIES = 0x0200,  /*!< Initialize properties */
  SET_PARAMETERS_GEOMETRY = 0x0400,    /*!< Initialize geometry data */
  SET_PARAMETERS_MPI = 0x0800,
  /*!< Initialize MPI (This option is always available even if MPI is
       not enabled. It's just ignored if not) */
  SET_PARAMETERS_GPU = 0x2000,
  /*!< Initialize GPU (This option is always available even if GPU is
       not enabled. It's just ignored if not) */
  SET_PARAMETERS_LISTFP = 0x1000, /*!< Initialize flags' fp (open the file) */
  SET_PARAMETERS_CONTROL_GEOMETRY = 0x4000,
  /*!< Initialize geometry part of control data */
  SET_PARAMETERS_CONTROL = 0x8000,
  /*!< Initialize field and logical variables, masks, and stats definitions */

  SET_PARAMETERS_ALL = 0xfffffff, /*!< Initialize all parameters */
};

/**
 * @brief Collect sform data from comps array.
 * @param comps component array to get from.
 * @param nbcompo Number of base components.
 * @return allocated array, NULL if allocation failed
 *
 * This method does not handle errors.
 *
 * Make sure to deallocate returned array with free().
 */
JUPITER_DECL
enum solid_form *create_solid_form_array(phase_value_component *comps,
                                         int nbcompo);

/**
 * @brief Parse command line arguments and set parameters.
 * @param argc Number of arguments (in/out)
 * @param argv Array of arguments (in/out)
 * @param set_opts Options for partial initialization
 * @param parallel Whether running in MPI parallel or not
 * @return Initialized parameter structure, NULL if allocation failed.
 *
 * @p argc and @p argv will be modified.
 *
 * Reading status (wrong parameter, invalid configuration, etc.) will
 * be stored in `status` member.
 *
 * @p parallel is not true (0), initialize MPI parameters without
 * parallelization, and sets fake parameters for it. This generates
 * error if MPI processes are running more than 1. @p parallel does
 * not take effect if JUPITER is built without MPI or @p set_opts does
 * not have `SET_PARAMETERS_MPI`.
 */
JUPITER_DECL
parameter  *set_parameters(int *argc, char **argv[], enum set_parameters_options set_opts, int parallel);
JUPITER_DECL
void print_timer(parameter *prm);
JUPITER_DECL
int init_mesh(domain *cdo, mpi_param *mpi, flags *flg);

JUPITER_DECL
flags *init_flag(const char *flags_fname, csv_data *flags_csv,
                 const char *param_fname, csv_data *param_csv,
                 jupiter_options *exec_argument, int *stat);

JUPITER_DECL
domain *init_cdomain(flags *flg, mpi_param *mpi, const char *param_fname,
                     csv_data *param_csv, jcntrl_executive_manager *controls,
                     controllable_type *control_head,
                     component_data *comp_data_head, int *stat);

/* YSE: Add deallocator of parameter (incomplete) */
/**
 * @brief Free whole content of parameter
 * @param prm Pointer to parameter to deallocate
 *
 * This implies free_domain and free_flags.
 */
JUPITER_DECL
void free_parameter(parameter *prm);

/**
 * @brief Free domain data
 */
JUPITER_DECL
void free_domain(domain *cdo);

/**
 * @brief Free flag data
 */
JUPITER_DECL
void free_flags(flags *flg);

/* YSE: Add to debug the content of arrays */
//====================
// print_param.c
//--------------------
/**
 * @brief Visualize value of val into text (or terminal) in form of ASCII art.
 * @param prm Parameter data
 * @param val Array of data
 * @param mx X-size of val (without stencil)
 * @param my Y-size of val (without stencil)
 * @param mz Z-size of val (without stencil)
 * @param stm Stencil size of Minus direction
 * @param stp Stencil size of Plus direction
 * @param xpos Array of X-position in Structured grid
 * @param ypos Array of Y-position in Structured grid
 * @param zpos Array of Z-position in Structured grid
 * @param min  Minimum value of range
 * @param max  Maximum value of range
 * @param title Title of data
 * @param axes Print order
 *
 * xpos, ypos, zpos must be array of mx + stm + stp + 1, my + stm + stp + 1,
 * mz + stm + stp + 1 to include stencil region. These parameters can be
 * NULL, the axis value will not be printed in this case.
 *
 * axes must be one of 'xyz', 'xzy', 'yxz', 'yzx', 'zxy' or 'zyx'.
 * The first char is horizontal, second char is vertical axis, and last
 * char is slice direction.
 */
JUPITER_DECL
void dumb_visualizer(parameter *prm,
                     type *val, int mx, int my, int mz,
                     int stm, int stp,
                     type *xpos, type *ypos, type *zpos,
                     type min, type max, const char *title,
                     const char *axes);

JUPITER_DECL
void dumb_visualize_boundary(parameter *prm, domain *cdo,
                             struct boundary_array *val, int nx, int ny,
                             const char *dirx, const char *diry,
                             const char *title);

JUPITER_DECL
void report_surface_boundary_area(flags *flg, mpi_param *mpi,
                                  struct surface_boundary_data *head,
                                  struct surface_boundary_data **array,
                                  type *bnd_array_u, type *bnd_array_v,
                                  type *bnd_array_w,            //
                                  type *x, type *y, type *z,    //
                                  int mx, int my, int mz,       //
                                  int stmx, int stmy, int stmz, //
                                  int stpx, int stpy, int stpz);

/* YSE: Moved from set_param.c */
/**
 * @brief Print whole parameters of prm into file or terminal
 * @param prm parameter data
 */
JUPITER_DECL
void print_param(parameter *prm);
/* YSE: end */

//====================
// alloc_variable.c
//--------------------
JUPITER_DECL
phase_value *malloc_phase_value(domain *cdo, flags *flg); // added flg, 2017/9/20
JUPITER_DECL
variable  *malloc_variable(domain *cdo, flags *flg);
JUPITER_DECL
material  *malloc_material(domain *cdo, flags *flg); // added flg, 2017/9/20
JUPITER_DECL
void free_variable(variable *v);
JUPITER_DECL
void free_material(material *m); // YSE: added

//====================
// initial.c
//--------------------
JUPITER_DECL
void  init_variables(variable *val, material *mtl, parameter *prm);
//int init_VOF(type *fl_A, type *fl_B, type *fs_A, type *fs_B, type *flg_obst_A, type *flg_obst_B, parameter *prm);
JUPITER_DECL
int init_VOF(type *fl, type *fs, type *flg_obst_A, type *flg_obst_B, parameter *prm);
JUPITER_DECL
int init_Y(variable *val, parameter *prm); //  2017/09/22
JUPITER_DECL
int init_vel(type *u, type *v, type *w, parameter *prm);
//int init_temp(type *t, type *fl, type *fs, int *flg_obst, parameter *prm);
JUPITER_DECL
int init_temp(variable *val, parameter *prm);
JUPITER_DECL
int set_fuel_rod(int flag, type value, type R, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst,  parameter *prm);
JUPITER_DECL
int set_fuel_rod_orifice(int flag, type R_in, type R_out, type value, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst,  parameter *prm);
JUPITER_DECL
int set_fuel_channel(int flag, type lx, type ly, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst,  parameter *prm);
JUPITER_DECL
void init_Vf(variable *val, material *mtl, parameter *prm);//added by Chai
JUPITER_DECL
void init_partial_volume (variable *val, material *mtl, parameter *prm);//added by Chai
JUPITER_DECL
void set_diff_func(variable *val, material *mtl, parameter *prm);//added by Chai

/* YSE: Make public */
JUPITER_DECL
int init_boundary(mpi_param *mpi, domain *cdo, flags *flg, variable *val);

/**
 * @brief Initialize LPT module
 * @param restart 0 if the job is initial calculation, otherwise restarted job
 * @param flg Flags data to share
 * @param cdo Domain information to share
 * @param mpi MPI information to share
 * @param val LPT variable data to share
 * @param stat Sets to 1 when error occurs, if given
 *
 * This function is available even if LPT is not compiled in, but
 * in this case, this function does nothing.
 *
 * This function is MPI collective.
 *
 * This function contains OpenMP parallelization.
 */
JUPITER_DECL
void init_lpt(int restart, flags *flg, domain *cdo, mpi_param *mpi,
              variable *val, int *stat);

/**
 * @brief Send constant continium field variables to LPT module
 * @param cdo Domain data
 * @param val Variable data to send
 * @param stat Sets to 1 when error occurs, if given
 *
 * Currently, this function sends only lpt_pewall.
 *
 * This function can be MPI collective. But @p stat is not shared
 * among each ranks. You may share the result of @p stat after the
 * call.
 *
 * This function contains OpenMP parallelization.
 *
 * This function is available even if LPT is not compiled in, but
 * in this case, this function does nothing.
 */
JUPITER_DECL
void lpt_send_constant_field_vars(domain *cdo, variable *val, int *stat);

/**
 * @brief Tells LPT module to call JUPITER's error message function
 *
 * Setting error function is csvperrorf().
 *
 * This function is available even if LPT is not compiled in, but
 * in this case, this function does nothing.
 */
JUPITER_DECL
void lpt_set_jupiter_error_function(void);

/**
 * @brief Post-initialization and/or restart read check
 */
JUPITER_DECL
void post_initial_check(parameter *prm, variable *val, material *mtl);

//====================
// multi_layer.c
//--------------------
JUPITER_DECL
type multi_layer(variable *val, parameter *prm, int flag);
JUPITER_DECL
void define_fl_layer_from_fl(variable *val, parameter *prm);
JUPITER_DECL
void layer_sum_up(variable *val, parameter *prm);
JUPITER_DECL
void search_liquid_film(variable *val, parameter *prm);

//====================
// thinc.c
//--------------------
JUPITER_DECL
type clip(type f);
JUPITER_DECL
type advection_vof(type *f, type *u, type *v, type *w, type *ls, type *fs, type *fs_ibm, int icompo, int ilayer,variable *val, parameter *prm); //2017/09/22
JUPITER_DECL
type vof_advection(variable *val, parameter *prm);
JUPITER_DECL
type calc_surface_boundary_vel(type u, struct surface_boundary_data *sb, type bnd_norm);

//====================
// plic.c
//--------------------
JUPITER_DECL
int plic(type *f, type *flx, type *fly, type *flz, type *nvx, type *nvy, type *nvz, type *alpha, type *u, type *v, type *w, type *lss, type *fs, type *fs_ibm, int icompo, int ilayer, type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, struct surface_boundary_data **surf_bnd_array, parameter *prm, variable *val, int direction_flag);

//====================
// phasefield.c  [Sugihara] 2021/05/19
//--------------------
JUPITER_DECL
type advection_phase_field(type *f, type *u, type *v, type *w, type *ll, type *fs, type *fs_ibm, variable *val, parameter *prm); //[Sugihara] 2021/05/19

//====================
// level_set.c
//--------------------
JUPITER_DECL
type Level_Set(int init_flg, int itr_max, type *ls, type *fs, parameter *prm);
JUPITER_DECL
type Level_Set_contact(int init_flg, int itr_max, type *ls, type *nvlx, type *nvly, type *nvlz, type *nvsx, type *nvsy, type *nvsz, type *fs, parameter *prm);
JUPITER_DECL
void level_set_all(variable *val, parameter *prm);


//====================
// materials.c
//--------------------
JUPITER_DECL
type materials(material *mtl, variable *val, parameter *prm);

//====================
// heat_source.c
//--------------------
JUPITER_DECL
type heat_source(variable *val, material *mtl, parameter *prm);

//====================
// eutectic.c
//--------------------
JUPITER_DECL
type eutectic(variable *val, material *mtl, parameter *prm);

//====================
// dt_control.c
//--------------------
JUPITER_DECL
type dt_control(variable *val, material *mtl, parameter *prm);

//====================
// boundary.c
//--------------------
//type bcu(type *u, type *v, type *w, variable *val, parameter *prm);
JUPITER_DECL
type bcu(type *u, type *v, type *w, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type bcu_correct(type *u, type *v, type *w, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type bcf(type *f, parameter *prm);
JUPITER_DECL
type bcp(type *f, variable *val, parameter *prm);
JUPITER_DECL
type bcs(type *f, type *v, variable *val, parameter *prm, material *mtl);
JUPITER_DECL
type bcf_VOF(int flag, type *f, variable *val, parameter *prm);
JUPITER_DECL
type bcf_VOF_layer(int flag, type *f, variable *val, parameter *prm);
JUPITER_DECL
type bct(type *f, variable *val, parameter *prm);
JUPITER_DECL
type bcn(type *nwx, type *nwy, type *nwz, parameter *prm);
JUPITER_DECL
type bcc(type *f, parameter *prm);

//====================
// radiation.c
//--------------------
JUPITER_DECL
type radiation(variable *val, material *mtl, parameter *prm);

//====================
// oxidation.c
//--------------------
JUPITER_DECL
type oxidation(variable *val, material *mtl, parameter *prm);

//====================
// communication.c
//--------------------
/**
 * @brief Set MPI parameters for specific rank (even not mine)
 * @param mpi Pointer to parameter to set
 * @param npex Number of ranks in X axis
 * @param npey Number of ranks in Y axis
 * @param npez Number of ranks in Z axis
 * @param npea Number of ranks in angle axis of radiation
 * @param numGPU unused (Please specify 0)
 * @param rank The rank number to set for.
 * @param status Sets ON when errors occured, if non-NULL given.
 *
 * If MPI is enabled and initialized (MPI_Initialized returns true),
 * `mpi->npe_glob`, `mpi->rank_glob` sets to number of processes and rank of
 * `MPI_COMM_WORLD`. Otherwise, `mpi->npe_glob` and `mpi->rank_glob` will be set
 * to 1 and 0 respectively.
 *
 * MPI_Comm parameters (e.g. CommJUPITER) will remain unchanged. This function
 * never sets or modifies them.
 */
JUPITER_DECL
void set_mpi_for_rank(mpi_param *mpi, int npex, int npey, int npez, int npea,
                      int numGPU, int rank, int *status);
JUPITER_DECL
mpi_param *mpi_init_param(int argc, char *argv[], int npex, int npey, int npez,
                          int npea, int numGPU, int *status);
JUPITER_DECL
void mpi_bcast_radiation_configuration(parameter *prm, FILE *fp);

#ifdef JUPITER_MPI
JUPITER_DECL
int calc_ptr(int *ptr, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void unpack_mpi_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void unpack_mpi_x_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void unpack_mpi_y_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void unpack_mpi_z_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void pack_mpi_x_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void pack_mpi_y_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void pack_mpi_z_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi);
JUPITER_DECL
void mpi_isend_irecv_x_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl, MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi);
JUPITER_DECL
void mpi_isend_irecv_y_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl, MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi);
JUPITER_DECL
void mpi_isend_irecv_z_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl, MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi);
#endif

JUPITER_DECL
void communication(type *f, parameter *prm);
JUPITER_DECL
void communication_VOF(type *f, parameter *prm);
JUPITER_DECL
void communication_rad(variable *val, material *mtl, parameter *prm, int iflg);

/* YSE: Add function to collect an condition is true on all or any ranks */
JUPITER_DECL
int mpi_for_all_rank(mpi_param *mpi, int cond);
JUPITER_DECL
int mpi_for_any_rank(mpi_param *mpi, int cond);

/*
 * ex:
 *   /// If any rank's prm->status is equal to ON... (not restricted to mine)
 *   if (for_any_rank(mpi, prm->status == ON)) {
 */
#ifdef JUPITER_MPI
#define for_all_rank(mpi, cond) mpi_for_all_rank((mpi), (cond))
#define for_any_rank(mpi, cond) mpi_for_any_rank((mpi), (cond))
#else
#define for_all_rank(mpi, cond) (cond)
#define for_any_rank(mpi, cond) (cond)
#endif

//====================
// io.c
//--------------------
JUPITER_DECL
int out_max_temp(type *t, domain *cdo);
JUPITER_DECL
int vtk_out(int icnt, variable *val, material *mtl, domain *cdo);
JUPITER_DECL
int vtk_out_with_bnd(int icnt, variable *val, material *mtl, domain *cdo);
JUPITER_DECL
int binary_out(int iout, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
int binary_out_single(int iout, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
int binary_out_job(variable *val, material *mtl, parameter *prm);
JUPITER_DECL
int restart(int iout, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
int geometry_in(int icnt, variable *val, parameter *prm, init_component tgt);
JUPITER_DECL
int post(int post_s, int post_e, variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type output_data(int flag, variable *val, material *mtl, parameter *prm);

/* YSE: expose more basic functions */
/**
 * @brief Input raw binary data file (generic type)
 * @param mpi MPI connection info
 * @param val Array which read into
 * @param stmx X- stencil size of val
 * @param stmy Y- stencil size of val
 * @param stmz Z- stencil size of val
 * @param stpx X+ stencil size of val
 * @param stpy Y+ stencil size of val
 * @param stpz Z+ stencil size of val
 * @param mx X size of val (including stencil)
 * @param my Y size of val (including stencil)
 * @param mz Z size of val (including stencil)
 * @param unit_size Number of data per element (ex. 3 for vector)
 * @param path Path name to read from.
 * @param output_mode Read mode
 * @param use_float Whether data file is single precision or not.
 * @return 0 if success, otherwise failed.
 *
 * This function is very basic function. The @p path name will not be
 * checked whether it suits for @p output_mode, and no translation
 * will be performed.
 */
JUPITER_DECL
int input_binary_generic(mpi_param *mpi, void *ret,
                         int stmx, int stmy, int stmz,
                         int stpx, int stpy, int stpz,
                         int mx, int my, int mz, int unit_size,
                         const char *path, binary_output_mode output_mode);

/**
 * @brief Input raw binary data file (floating point `type` (and/or float))
 * @param mpi MPI connection info
 * @param val Array which read into
 * @param fval Array to store unconverted float data (if need)
 * @param stmx X- stencil size of val
 * @param stmy Y- stencil size of val
 * @param stmz Z- stencil size of val
 * @param stpx X+ stencil size of val
 * @param stpy Y+ stencil size of val
 * @param stpz Z+ stencil size of val
 * @param mx X size of val (including stencil)
 * @param my Y size of val (including stencil)
 * @param mz Z size of val (including stencil)
 * @param unit_size Number of data per element (ex. 3 for vector)
 * @param path Path name to read from.
 * @param output_mode Read mode
 * @param use_float Whether data file is single precision or not.
 * @return 0 if success, otherwise failed.
 *
 * This function is very basic function. The @p path name will not be
 * checked whether it suits for @p output_mode, and no translation
 * will be performed.
 */
JUPITER_DECL
int input_binary(mpi_param *mpi, type *val, float *fval,
                 int stmx, int stmy, int stmz,
                 int stpx, int stpy, int stpz,
                 int mx, int my, int mz, int unit_size,
                 const char *path, binary_output_mode output_mode,
                 int use_float);

JUPITER_DECL
int make_time_file_name(char **output, const char *directory,
                        const struct data_spec *spec, int iout);

/**
 * @brief Generate file name
 * @param output Pointer to be output
 * @param directory directory of data files
 * @param file_name_template Filename template pattern.
 * @param component_name name of component (u, t, ...)
 * @param component_id component id if applicable, give -1 if not
 * @param iout restart data index number, give -1 for job restart
 * @param rank Rank number to be used for.
 * @param output_mode Binary file output mode to be used for
 * @return 0 if success, others for failed
 *
 * @note Pointer of output will be allocated. You must `free` it
 *       after use.
 * @note When failed, content of `output` is undefined.
 *
 * \p output_mode will be ignored if MPI is not defined at compilation.
 *
 * \p file_name_template can contain following format directions.
 *
 * * `%c` will be replaced by \p component_name.
 * * `%i` will be replaced by \p component_id.
 * * `%n` will be replaced by \p iout.
 * * `%r` will be replaced by \p rank.
 * * `%%` will be replaced by literal `%`.
 *
 * `%i`, `%n` and `%r` can contain flag, length and precision
 * specifications like `%d` format in C's printf() does. For more details,
 * see format_integers(), using `format_keys` with `rin [s]c`.
 */
JUPITER_DECL
int make_file_name(char **output,
                   const char *directory, const char *file_name_template,
                   const char *component_name,
                   int component_id, int iout, int rank,
                   binary_output_mode output_mode);

/**
 * @brief Input AOS multicomponent floating-point binary file.
 * @param prm JUPITER parameter data.
 * @param directory The directory name read from
 * @param spec Data output specification
 * @param component_id Component ID for the file
 * @param val The array read into.
 * @param iout Number time-index, or -1 for reading job-restart
 * @param unified Unified reading or by-process reading.
 * @param use_float 1 for read float data in double version JUPITER.
 * @param unit_size Number of elements
 * @param nb Number of boundary cells to be included
 * @retval 0 success
 * @retval 1 file error (Not found, permission denied, ...)
 * @retval (others) read error and other fatal error
 */
JUPITER_DECL
int input_binary_nb(parameter *prm, const char *directory,
                    const struct data_spec *spec, int component_id, type *val,
                    int iout, binary_output_mode unified, int use_float,
                    int unit_size, int nb);

/**
 * @brief Input scalar binary file.
 * @param prm JUPITER parameter data.
 * @param directory The directory name read from
 * @param spec Data output specification
 * @param component_id Component ID for the file
 * @param val The array read into.
 * @param iout Number time-index, or -1 for reading job-restart
 * @param unified Unified reading or by-process reading.
 * @param use_float 1 for read float data in double version JUPITER.
 * @param nb Number of boundary cells to be included
 * @retval 0 success
 * @retval 1 file error (Not found, permission denied, ...)
 * @retval (others) read error and other fatal error
 */
JUPITER_DECL
int input_binary_scalar_nb(parameter *prm, const char *directory,
                           const struct data_spec *spec, int component_id,
                           type *val, int iout, binary_output_mode unified,
                           int use_float, int nb);

/**
 * @brief Input AOS vector binary file.
 * @param prm JUPITER parameter data.
 * @param directory The directory name read from
 * @param spec Data output specification
 * @param component_id Component ID for the file
 * @param val The array read into.
 * @param iout Number time-index, or -1 for reading job-restart
 * @param unified Unified reading or by-process reading.
 * @param use_float 1 for read float data in double version JUPITER.
 * @retval 0 success
 * @retval 1 file error (Not found, permission denied, ...)
 * @retval (others) read error and other fatal error
 *
 * This function is equivalent to input_binary_scalar_nb with nb == 0.
 */
JUPITER_DECL
int input_binary_aos_vector(parameter *prm, const char *directory,
                            const struct data_spec *spec, int component_id,
                            type *val, int iout, binary_output_mode unified,
                            int use_float);

/**
 * @brief Input scalar binary file.
 * @param prm JUPITER parameter data.
 * @param directory The directory name read from
 * @param spec Data output specification
 * @param component_id Component ID for the file
 * @param val The array read into.
 * @param iout Number time-index, or -1 for reading job-restart
 * @param unified Unified reading or by-process reading.
 * @param use_float 1 for read float data in double version JUPITER.
 * @retval 0 success
 * @retval 1 file error (Not found, permission denied, ...)
 * @retval (others) read error and other fatal error
 *
 * This function is equivalent to input_binary_scalar_nb with nb == 0.
 */
JUPITER_DECL
int input_binary_scalar(parameter *prm, const char *directory,
                        const struct data_spec *spec, int component_id,
                        type *val, int iout, binary_output_mode unified,
                        int use_float);

/**
 * @brief Input LPT control data
 * @param prm JUPITER parameter data.
 * @param directory The directory name read from
 * @param spec Data output specification
 * @param component_id Componend ID for the file
 * @param data Data to be written (allocated)
 * @param iout Number time-index, or -1 for reading job-restart
 * @param unified Unified reading or by-process reading.
 * @retval 0 success
 * @retval 1 file error (Not found, permission denied, ...)
 * @retval (others) read error and other fatal error
 *
 * @p unifed and @p component_id only affects to the file name read
 * from. This function always the file like by-process reading.
 */
JUPITER_DECL
int input_binary_lpt_ctrl(parameter *prm, const char *directory,
                          const struct data_spec *spec, int component_id,
                          jupiter_lpt_ctrl_data **data, int iout,
                          binary_output_mode unified);

/**
 * @brief Read Time domain control file.
 * @param mpi MPI communication info for distribute read data
 * @param cdo Domain data to set for
 * @param path File name to read from
 * @retval 0 Success
 * @retval non-0 Failed or Parse error
 *
 * @p path in valid only for the process which MPI rank is 0. Other
 * processes can pass NULL for @p path argument.
 *
 * Contrast to output_time_file(), this function is MPI-collective:
 * All processes must call this function.
 */
JUPITER_DECL
int input_time_file(mpi_param *mpi, domain *cdo, const char *path);

/**
 * @brief Read level-set update flag data.
 * @param mpi MPI communication info for distribute read data
 * @param flg Flag data to set for
 * @param path File name to read from
 * @retval 0 Success
 * @retval non-0 Failed or Parse error
 *
 * @p path in valid only for the process which MPI rank is 0. Other
 * processes can pass NULL for @p path argument.
 *
 * Contrast to output_uplsflg_file(), this function is MPI-collective:
 * All processes must call this function.
 */
JUPITER_DECL
int input_uplsflg_file(mpi_param *mpi, flags *flg, const char *path);

/**
 * @brief Calculate dimesion data from stencil and size
 * @param mpi MPI parallelization info
 * @param stmx Stencil size of X- (aka. West boundary).
 * @param stmy Stencil size of Y- (aka. South boundary).
 * @param stmz Stencil size of Z- (aka. Bottom boundary).
 * @param stpx Stencil size of X+ (aka. East boundary).
 * @param stpy Stencil size of Y+ (aka. North boundary).
 * @param stpz Stencil size of Z+ (aka. Top boundary).
 * @param mx X size including Stencil
 * @param my Y size including Stencil
 * @param mz Z size including Stencil
 * @param unit_size Number of element per cell (1 for scalar, 3 for vector)
 * @param local_dim (out) Local array dimension size (i.e. (unit_size, mx, my, mz))
 * @param local_subs (out) Local array subsize
 * @param local_start (out) Local array start point (i.e. (0, stmx, stmy, stmz))
 * @param global_dim (out) Global array dimension size (sum of local_subs)
 * @param global_subs (out) Global array subsize (for this rank)
 * @param global_start (out) Global array start point (for this rank)
 * @param global_sizes (out) Global array sizes (for each rank)
 * @param global_starts (out) Global array start points (for each rank)
 * @param nranks (out) Size of global_sizes and global_starts.
 * @return 0 if success, otherwise failed.
 *
 * If you don't need Global information, give NULL for global_dim.
 *
 * If you don't need size or start information of all ranks, give NULL
 * for global_sizes.
 *
 * This function allocates required memory for global_sizes and global_starts,
 * and set it to global_sizes.
 *
 * Use free(global_sizes) to deallocate allocated memory. This also
 * deallocates pointer of global_starts.
 *
 * @note This function is MPI-Collective call to get global information.
 * @note This function won't fail if global information is not required.
 * @note Result order of local_dim etc. is (cell-vector, X, Y, Z),
 *       for using with MPI_ORDER_FORTRAN. (aka. column-major)
 *
 * @note unit_size must be equal on all process ranks.
 */
JUPITER_DECL
int calculate_dim(mpi_param *mpi,
                  int stmx, int stmy, int stmz,
                  int stpx, int stpy, int stpz,
                  int mx, int my, int mz, int unit_size,
                  int local_dim[4], int local_subs[4], int local_start[4],
                  int global_dim[4], int global_subs[4], int global_start[4],
                  int **global_sizes, int **global_starts, int *nranks);

/**
 * @brief Initialize mpi_param with MPI_COMM_SELF for self-only-communication
 * @param mpi Container to initialize
 *
 * All "number of PEs" to 1, all "rank index" to 0, and MPI communicator to
 * set `MPI_COMM_SELF`.
 *
 * This function does not allocate any resources.
 */
JUPITER_DECL
void set_self_comm_mpi(mpi_param *mpi);

/**
 * @brief Output raw binary data file (generic type)
 * @param mpi MPI connection info
 * @param data Array which write
 * @param stmx X- stencil size of val
 * @param stmy Y- stencil size of val
 * @param stmz Z- stencil size of val
 * @param stpx X+ stencil size of val
 * @param stpy Y+ stencil size of val
 * @param stpz Z+ stencil size of val
 * @param mx X size of val (including stencil)
 * @param my Y size of val (including stencil)
 * @param mz Z size of val (including stencil)
 * @param unit_size Number of bytes per element
 * @param path Path name to write to.
 * @param output_mode Write mode
 * @return 0 if success, otherwise failed.
 *
 * This function is very basic function. The @p path name will not be
 * checked whether it suits for @p output_mode (for example, it points
 * a same file in the shared-filesystem for MPI-IO mode), and no
 * translation will be performed.
 */
JUPITER_DECL
int output_binary_generic(mpi_param *mpi, void *data,
                          int stmx, int stmy, int stmz,
                          int stpx, int stpy, int stpz,
                          int mx, int my, int mz, int unit_size,
                          const char *path, binary_output_mode output_mode);

/**
 * @brief Output raw binary data file (floating point `type`)
 * @param mpi MPI connection info
 * @param val Array which write
 * @param stmx X- stencil size of val
 * @param stmy Y- stencil size of val
 * @param stmz Z- stencil size of val
 * @param stpx X+ stencil size of val
 * @param stpy Y+ stencil size of val
 * @param stpz Z+ stencil size of val
 * @param mx X size of val (including stencil)
 * @param my Y size of val (including stencil)
 * @param mz Z size of val (including stencil)
 * @param unit_size Number of data per element (ex. 3 for vector)
 * @param path Path name to read from.
 * @param output_mode Write mode
 * @param use_float Whether data file is single precision or not.
 * @return 0 if success, otherwise failed.
 *
 *
 * This function is very basic function. The @p path name will not be
 * checked whether it suits for @p output_mode (for example, it points
 * a same file in the shared-filesystem for MPI-IO mode), and no
 * translation will be performed.
 */
JUPITER_DECL
int output_binary(mpi_param *mpi, type *val,
                  int stmx, int stmy, int stmz,
                  int stpx, int stpy, int stpz,
                  int mx, int my, int mz, int unit_size,
                  const char *path, binary_output_mode output_mode,
                  int use_float);

/**
 * @brief Read unified and partially defined binary data file
 *
 * @param mpi  MPI Parameter
 * @param val  Location to store the result.
 * @param stmx X- stencil size of val
 * @param stmy Y- stencil size of val
 * @param stmz Z- stencil size of val
 * @param stpx X+ stencil size of val
 * @param stpy Y+ stencil size of val
 * @param stpz Z+ stencil size of val
 * @param mx   X size (including stencil) of val
 * @param my   Y size (including stencil) of val
 * @param mz   Z size (including stencil) of val
 * @param unit_size Number of elements for each cell
 * @param path   Path to data file
 * @param use_float non-0 for data file is stored in float, otherwise JUPITER's configuration used
 * @param origin Global origin of data file
 * @param size   Size of the data file
 * @param repeat Number of repeat count
 * @param offset Offset for repeating
 *
 * @return 0 if success, otherwise failed.
 *
 * \p mpi will be used for obtaining global location of the \p val.
 *
 * This function is collective, but this function does not use MPI on
 * IO, because it cannot be performed in collective.
 */
JUPITER_DECL
int input_geometry_binary_unified(mpi_param *mpi, type *val,
                                  int stmx, int stmy, int stmz,
                                  int stpx, int stpy, int stpz,
                                  int mx, int my, int mz, int unit_size,
                                  const char *path, int use_float,
                                  geom_svec3 origin, geom_svec3 size,
                                  geom_svec3 repeat, geom_svec3 offset);

/**
 * @brief Perform general geometry operation
 * @param val Variable structure to modify data
 * @param prm Parameter data to obtain MPI and domain information
 * @param tgt Components to apply (Use bitwise-OR)
 * @param modified Modified components (out, bitwise-OR used)
 * @param data Geometry data set to apply
 * @retval 0  success
 * @retval 1- failed
 */
JUPITER_DECL
int geometry_in_with(variable *val, parameter *prm, init_component tgt,
                     init_component *modified, geom_data *data);

//====================
// tvd_runge_kutta.c
//--------------------
JUPITER_DECL
type tvd_runge_kutta_3(variable *val, material *mtl, parameter *prm);

//====================
// advection.c
//--------------------
/**
 * @brief Solve advection equation for a variable
 * @param flag staggared flag
 * @param ft Result variable
 * @param f Variable to solve
 * @param u X-axis velocity
 * @param v Y-axis velocity
 * @param w Z-axis velocity
 * @param eps porosity
 * @param fs_ibm solid vof in IBM
 * @param prm JUPITER parameters
 * @return 0.0.
 *
 * * @p fs_ibm should be solid VOF of IBM model.
 * * @p eps can be NULL if porous model is not enabled.
 * * @p flag value must be one of following:
 *
 *   | @p flag value | Meaning              |
 *   |---------------|----------------------|
 *   | 1             | Cell center variable |
 *   | 2             | X cell face variable |
 *   | 3             | Y cell face variable |
 *   | 4             | Z cell face variable |
 *
 * * Allocates extra work memory in the first call when it was required.
 */
JUPITER_DECL
type advection_eq(int flag, type *ft, type *f, type *u, type *v, type *w,
                  type *eps, type *fs_ibm,
                  struct surface_boundary_data **surf_bnd_array,
                  parameter *prm);

//====================
// viscosity.c
//--------------------
JUPITER_DECL
type visc_eq(int tvdflag, type *ut, type *vt, type *wt, type *u, type *v, type *w, type *dens, type *mu, variable *val, parameter *prm);
JUPITER_DECL
type heat_conduction(type *t, material *mtl, variable *val, parameter *prm);
JUPITER_DECL
type heat_conduction_tvd3(type *t, material *mtl, variable *val, parameter *prm);
JUPITER_DECL
type heat_conduction_imp(type *t, material *mtl, variable *val, parameter *prm);
JUPITER_DECL
type film_thickness_on_cell_face(type *film, int j, int offset);
JUPITER_DECL
type viscosity_augmentation(type *film, int j1, int j2, int offset, type rapture_thickness);


//====================
// divergence_free.c
//--------------------
JUPITER_DECL
type divergence_free(variable *val, material *mtl, parameter *prm);

//====================
// phase_change.c
//--------------------
JUPITER_DECL
type phase_change(variable *val, material *mtl, parameter *prm); //added by Chai
JUPITER_DECL
int t_update(type *t, type delt_t, type t_liq, type t_sol, type entha, type specht, type latent);//added by Chai
JUPITER_DECL
void cp_update(domain *cdo, flags *flg, phase_value *phv, type *fl, type *fs, int *mushy, type *specht, type *latent, type *t_liq, type *t_soli, type *temp);
JUPITER_DECL
int fsfl_update(type *fl, type *fs, type delt_t, type latent, type specht);//added by Chai

/**
 * @brief Expand packed mushy array to cell-by-cell array
 * @param imushy the size of input_address and input_comp
 * @param input_address address array of mushy zone
 * @param input_comp component id array of mushy zone
 * @param output output array
 * @param m size of output array
 * @return 0 for success, otherwise failed
 *
 * The content of `output` would be -1 for non mushy zone, 0 or
 * positive numbers for its component IDs. This spec comes from
 * avoiding overflow on component IDs, and it's not good for boolean
 * operation.
 *
 * If `input_comp` is `NULL`, 0 is used for mushy zone.
 */
JUPITER_DECL
int expand_mushy_array(int imushy, int *input_address, int *input_comp,
                       int *output, int m);

/**
 * @brief Compress cell-by-cell mushy array to packed array
 * @param input input array
 * @param m size of input array
 * @param output_address address output array
 * @param output_comp component ID output array
 * @param imushy sets the number of elements
 * @return 0 for success, otherwise failed
 *
 * If `output_comp` is `NULL`, the component values in `input` will be
 * ignored.
 *
 * @note This method does not take care of calculation domain, and
 *       whole array of @p input will be processed. Cells with value 0
 *       will be added for the mushy zone even if outside of the
 *       calculation domain. Please make sure to fill with -1 for the
 *       outside of the calculation domain before calling this
 *       function.
 */
JUPITER_DECL
int compress_mushy_array(int *input, int m,
                         int *output_address, int *output_comp, int *imushy);


//====================
// surface.c
//--------------------
JUPITER_DECL
void normal_vector(type *nvx, type *nvy, type *nvz, type *curv, type *ls, domain *cdo);
JUPITER_DECL
void normal_vector_cell(type *nvx, type *nvy, type *nvz, type *curv, type *f, domain *cdo);
//int surface_tension_eq(type *ut, type *vt, type *wt, type *st, type *dens, type *nvx, type *nvy, type *nvz,
 //                       type *curv, type *ll, type *ls, parameter *prm);

JUPITER_DECL
int surface_tension_eq(type *tt, type *ut, type *vt, type *wt, type *st, type *dens, 
                        variable *val, parameter *prm);
JUPITER_DECL
int Marangoni(type width, type *tt, type *fl, type *fs, type *ll, type *lls,
              type *nvx, type *nvy, type *nyz, type *s_xx, type  *s_yy, type *s_zz, parameter *prm);
//====================
// forch.c
//--------------------
JUPITER_DECL
int Forchheimer(type mdt, type *ut, type *vt, type *wt, variable *val, material *mtl, parameter *prm);

//====================
// IBM.c
//--------------------
JUPITER_DECL
void IBM_vector(type *lss, type *fs, type *u, type *v, type *w, type *nvsx,
                type *nvsy, type *nvsz,
                struct surface_boundary_data **surf_bnd_array,
                type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, domain *cdo);
JUPITER_DECL
type IBM_solid_ratio(type fw, type fe);

/**
 * @brief divergence mass source by IBM
 * @param u Staggared x velocity
 * @param v Staggared y velocity
 * @param w Staggared z velocity
 * @param f IBM VOF array
 * @param j Array index to compute
 * @param cdo Computational domain information
 * @return mass source value by IBM method (qs)
 */
JUPITER_DECL
type IBM_divergence_vof_qs(type *u, type *v, type *w, type *f,
                           struct surface_boundary_data **surf_bnd_array,
                           type *bnd_norm_u, type *bnd_norm_v,
                           type *bnd_norm_w,
                           int j, domain *cdo);

JUPITER_DECL
int IBM_flux_corr_vof_direction_split(type *flm, type *flp, int j, type *f, domain *cdo, int direction_flag);
JUPITER_DECL
int IBM_flux_corr_vof(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int j, type *f, domain *cdo);
JUPITER_DECL
int outflow_flux_direction_split(type *flm, type *flp, int j, int n, mpi_param *mpi,int direction_flag);
JUPITER_DECL
int outflow_flux(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int jx, int jy, int jz, int nx, int ny, int nz, mpi_param *mpi);
JUPITER_DECL
int outflow_flux2(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int jx, int jy, int jz, int nx, int ny, int nz, mpi_param *mpi, flags *flg);

//====================
// solute_transport.c < 2017/09/22
//--------------------
JUPITER_DECL
type solute_transport(variable *val, material *mtl, parameter *prm);

//====================
// lpt.c
//--------------------
/**
 * @brief Compute Lagrangean Particle Tracking
 * @param val Variable data of continuous domain
 * @param mtl Material data of continuous domain
 * @param prm Parameter data
 * @return Elapsed time [s]
 *
 * If any error occurs, `prm->status` will be set to `ON`.
 */
JUPITER_DECL
type calc_lpt(variable *val, material *mtl, parameter *prm);

/* Other functions are declared in lpt.h */

//====================
// gnuplot.c
//--------------------
JUPITER_DECL
int gnuplot_plot1d_TC1_temp(parameter *prm);
JUPITER_DECL
int gnuplot_plot1d_TC2_temp(parameter *prm);
JUPITER_DECL
int gnuplot_plot1d_TC3_temp(parameter *prm);
JUPITER_DECL
int gnuplot_plot1d_kerf_temp(parameter *prm);
JUPITER_DECL
int gnuplot_plot1d_kerf_depth(parameter *prm);
JUPITER_DECL
int gnuplot_plot1d_heat_input(parameter *prm);

//====================
// analysis.c
//--------------------
JUPITER_DECL
type analysis_point_data(variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type analysis_line_data(variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type analysis_surface_data(variable *val, material *mtl, parameter *prm);
JUPITER_DECL
type analysis_mass_flowrate(variable *val, material *mtl, parameter *prm);

//====================
// zero clear
//--------------------
JUPITER_DECL
void zero_clear(type *, int);
JUPITER_DECL
void zero_clear_int(int *, int);

/* YSE: Add declation of ccse_poisson */
#ifdef CCSE
//====================
// ccse_poisson.c
//--------------------
/**
 * @brief Solve matrix with predefined data definition
 * @param flag Flag for solving variable
 * @param f   variable data for solving (in/out)
 * @param div rhs data
 * @param dens density data (effective for solving pressure or velocity)
 * @param mtl material data
 * @param val other variable data
 * @param prm JUPITER parameter data
 * @return Number of iterations computed.
 *
 * The meaing of @p flag is following:
 *
 * ------------------------------------
 *  value     variable for solving
 * -------- ---------------------------
 *      0     X velocity U
 *      1     Y velocity V
 *      2     Z velocity W
 *      3     Pressure (Poisson eq) P
 *      4     Temperature
 * ------------------------------------
 */
JUPITER_DECL
int ccse_poisson(int flag, type *f, type *div, type *dens, material *mtl, variable *val, parameter *prm);

/**
 * @brief Function prototype for make_array_matrix
 * @param topo_flag Value location flag.
 * @param A lhs matrix data (out)
 * @param b rhs vector data (in/out)
 * @param stmx X- stencil size
 * @param stmy Y- stencil size
 * @param stmz Z- stencil size
 * @param stpx X+ stencil size
 * @param stpy Y+ stencil size
 * @param stpz Z+ stencil size
 * @param mx X array size
 * @param my Y array size
 * @param mz Z array size
 * @param prm JUPITER parameter data given for ccse_poisson_f().
 * @param arg Extra argument given for ccse_poisson_f().
 * @return (unused)
 */
typedef int make_matrix_array_func(int topo_flag, type *A, type *b,
                                   int stmx, int stmy, int stmz,
                                   int stpx, int stpy, int stpz,
                                   int mx, int my, int mz,
                                   parameter *prm, void *arg);

/**
 * @brief Solve matrix for general use.
 * @param topo_flag Value location flag.
 * @param name Name of variable
 * @param f   variable data for solving (in/out)
 * @param div rhs vector data (in)
 * @param prm JUPITER parameter data
 * @param itermax Maximum number of iterations (typically 30000)
 * @param rtolmax Maximum allowable relative tolerance (typically 1e-6 to 1e-8)
 * @param abstolmax Maximum allowable absolute torelance (typically 1e-50)
 * @param make_matrix_arr_f Function to make lhs matrix
 * @param arg Extra argument for @p make_matrix_arr_f.
 * @return Number of iterations computed.
 *
 * The meaing of @p topo_flag is following:
 *
 * ------------------------------
 *  value      location of value
 * ---------- -------------------
 * 0 or other  Cell-center
 *      1      X-face
 *      2      Y-face
 *      3      Z-face
 * ------------------------------
 */
JUPITER_DECL
int ccse_poisson_f(int topo_flag, const char *name, type *f, type *div,
                   parameter *prm, int itrmax, type rtolmax, type abstolmax,
                   make_matrix_array_func *make_matrix_arr_f, void *arg);

#endif

//====================
// field_control.c
//--------------------
JUPITER_DECL
type update_control_values(variable *val, material *mtl, parameter *prm);

//====================
// init_component.c
//--------------------
/**
 * @brief Sets components should be initialized on geometry_in().
 * @param dest init_component to set to.
 * @param prm JUPITER parameter set
 *
 * The disabled bits in @p dest will be left disabled.
 * Some enabled bits in @p dest may be disabled in certain condition.
 */
JUPITER_DECL
void init_component_for_initial(init_component *dest, parameter *prm);

/**
 * @brief Sets components should be processed on update_control_values().
 * @param dest init_component to set to.
 * @param prm JUPITER parameter set
 *
 * The disabled bits in @p dest will be left disabled.
 * Some enabled bits in @p dest may be disabled in certain condition.
 */
JUPITER_DECL
void init_component_for_update_control(init_component *dest, parameter *prm);

//====================
// validete_VOF_init.c
//--------------------
/**
 * @brief Validate VOF and write errors
 * @param mpi MPI parameters
 * @param cdo Computational domain parameters
 * @param flg Flags
 * @param num_print Number of print output
 * @param Y   Mole fraction (for solute_diff enabled)
 * @param fs  Solid VOF(s)
 * @param fl  Liquid VOF(s)
 * @param status Sets to ON if any check errors found
 * @return 0 if successfully performed, otherwise something fatal errors ocurred
 *         (e.g. memory allocation failure)
 *
 * Expected to be called while initialization, but can be performed in any
 * timing. This function is MPI collective.
 *
 * Even if @p num_print is less than 1, this function performs check. Just not
 * printing the errors.
 */
JUPITER_DECL
int validate_VOF_init(mpi_param *mpi, domain *cdo, flags *flg, int num_print,
                      type *Y, type *fs, type *fl, int *status);

#ifdef __cplusplus
}
#endif

#endif
