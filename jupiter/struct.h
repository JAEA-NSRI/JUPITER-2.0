/*=============================================
   Structures                  2011 / 07 / 15 |
            Kenta Sugihara & Susumu Yamashita |
---------------------------------------------*/
#ifndef STRUCT_H

#define STRUCT_H

#ifdef JUPITER_MPI
#include "mpi.h"
#endif

#ifdef JUPITER_DOUBLE
  typedef double type;
  #ifdef JUPITER_MPI
    #define MPI_TYPE MPI_DOUBLE
  #endif
#else
  typedef float type;
  #ifdef JUPITER_MPI
    #define MPI_TYPE MPI_FLOAT
  #endif
#endif

#include "random/defs.h"

/* YSE: Use CSV input definitions */
#include "csv.h"
#include "geometry/defs.h"

/* YSE: Using double linked-list in geometry library */
#include "geometry/list.h"

/* YSE: Definitions from field controlling/statistic calculation library */
#include "control/defs.h"
#include "control/executive_data.h"
#include "control/write_fv_csv.h"

/* YSE: Temperature dependent property */
#include "tempdep_properties.h"

/* YSE: Diffusivity calculation data */
#include "dccalc_defs.h"

#include "init_component.h"
#include "controllable_type.h"
#include "trip_control.h"
#include "heat_source.h"

#ifdef LPT
#include "lpt/LPTdefs.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#endif

#include "component_info_defs.h"
#include "component_data_defs.h"
#include "oxidation_defs.h"

#ifdef __cplusplus
  extern "C" {
#endif

/* Defined in grid_data_feeder_data.h */
struct jupiter_grid_data_feeder;
typedef struct jupiter_grid_data_feeder jupiter_grid_data_feeder;

#ifdef GPU
  // GPU
  typedef struct
  {
    int numGPUs, numGPUs_per_node;
    int gdx, gdy, gdz, bdx, bdy, bdz;
    dim3 grid, block;
} gpu_param;
#endif

// MPI
typedef struct
{
  int  npe_glob, npe,  npe_x,  npe_y,  npe_z, npe_xy, npe_a; // number of processor elements < 2016 Added by KKE (npe_glob, npe_a)
  int  rank_glob, rank, rank_x, rank_y, rank_z,// rank < 2016 Added by KKE (rank_glob)
       rank_yz,rank_xz,rank_xy;
  int  nrk[6];// neighbor rank[0:z-, 1:z+, 2:y-, 3:y+, 4:x-, 5:x+]
#ifdef JUPITER_MPI
  MPI_Comm CommSx, CommSy, CommSz, CommLx, CommLy, CommLz;
  MPI_Comm CommJUPITER; // < 2016 Added by KKE
#endif
  /* YSE: Add flags whether radiation processes are running */
  int  radiation_running;

  /* GPUs */
  int myGPU;  /*!< The GPU device number to be used */
  int totGPU; /*!< Number of GPUs on this node (got from system) */

  jcntrl_mpi_controller *control_controller;
} mpi_param;

//===========================================
// physical & computational parameters
//-------------------------------------------
#define WELD (119)
#define CUT  (110)
#define ON   (1)
#define OFF  (0)

#define WALL (3)
#define SLIP (5)
#define OUT  (7)
#define INLET (9) /* YSE: added */

#define INSULATION (11)
#define ISOTHERMAL (13)
#define DIFFUSION  (17)
#define PERIODIC_RADIATION (19) // < 2016 Added by KKE

enum interface_capturing_scheme
{
  PLIC,
  THINC,
  THINC_WLIC,
  THINC_AWLIC
};

/* YSE: added MPI connection boundary */
/**
 * @brief MPI-connection local boundary
 *
 * This symbol is used for per-cell boundary data as MPI-connection
 * boundary.
 *
 * @note For outer-boundary cells with this value considered invalid.
 *
 * @note The all boundary data where `mpi->nrk` on its direction is
 *       not `-1` are ignored even if the per-cell boundary data is
 *       not set to the BOUNDARY_MPI condition. (You cannot set
 *       internal boundary)
 */
#define BOUNDARY_MPI (99)

/**
 * @brief Inlet direction
 */
enum surface_inlet_dir
{
  SURFACE_INLET_DIR_INVALID,
  SURFACE_INLET_DIR_NORMAL, /*!< Normal to boundary (surface) */
};
typedef enum surface_inlet_dir surface_inlet_dir;

/* YSE: Add boundary data */
struct inlet_component_data;

/**
 * @brief pressure condition on OUT boundary
 */
enum out_p_cond {
  OUT_P_COND_INVALID = -1, /*!< Invalid value */
  OUT_P_COND_CONST = 0,    /*!< Constant pressure */
  OUT_P_COND_NEUMANN,      /*!< Neumann condition */
};
typedef enum out_p_cond out_p_cond;

/**
 * @brief boundary meta data
 */
struct boundary_meta_data {
  int id;   /*!< ID number of boundary data */
  int used; /*!< Marking for iterating over defined boundary data */
};

/**
 * @brief fluid-specific boundary data
 *
 * @note struct boundary_init_data is used for initialization.
 *       struct fluid_boundary_data is used for calculation.
 */
struct fluid_boundary_data {
  struct geom_list list;
  struct boundary_meta_data meta; /*!< Common metadata */
  int cond;                       /*!< Condition (INLET/OUT/WALL/SLIP/...) */
  trip_control control;           /*!< Control method for (INLET/OUT) */
  controllable_type inlet_vel_u;  /*!< Inlet U (X-axis) velocity for INLET */
  controllable_type inlet_vel_v;  /*!< Inlet V (Y-axis) velocity for INLET */
  controllable_type inlet_vel_w;  /*!< Inlet W (Z-axis) velocity for INLET */
  enum out_p_cond out_p_cond;     /*!< Pressure condition for OUT */
  controllable_type const_p;      /*!< Constant pressure for OUT */
  struct inlet_component_data *comps; /*!< Component data */
};
typedef struct fluid_boundary_data fluid_boundary_data;
#define fluid_boundary_entry(ptr) \
  geom_list_entry(ptr, struct fluid_boundary_data, list)

/**
 * @brief thermal-specific boundary data
 */
struct thermal_boundary_data {
  struct geom_list list;
  struct boundary_meta_data meta; /*!< Common metadata */
  int cond;                       /*!< Condition (ISOTHERMAL/INSULATION/...) */
  trip_control control;           /*!< Control method for (ISOTHERMAL) */
  controllable_type temperature;  /*!< Boundary temperature for ISOTHERMAL */
  double diffusion_limit;         /*!< Limit temperature for DIFFUSION */
};
typedef struct thermal_boundary_data thermal_boundary_data;
#define thermal_boundary_entry(ptr) \
  geom_list_entry(ptr, struct thermal_boundary_data, list)

struct surface_boundary_data {
  struct geom_list list;
  struct boundary_meta_data meta; /*!< Common metadata */
  const char *geom_name; /*!< Bound Geom_name if given */
  int cond; /*!< Condition (currently only supports INLET) */
  surface_inlet_dir inlet_dir; /*!< Inlet direction */
  trip_control control; /*!< Control method for INLET */
  controllable_type normal_inlet_vel; /*!< Inlet velocity for NORMAL INLET */
  struct inlet_component_data *comps; /*!< Component data array */
};
typedef struct surface_boundary_data surface_boundary_data;
#define surface_boundary_entry(ptr) \
  geom_list_entry(ptr, struct surface_boundary_data, list)

enum binary_output_mode {
  BINARY_OUTPUT_INVALID,
  BINARY_OUTPUT_BYPROCESS,    ///< Each process read/write to independent files.
  BINARY_OUTPUT_UNIFY_GATHER, ///< Unify each processes' data with MPI_Gather
  BINARY_OUTPUT_UNIFY_MPI,    ///< Unify each processes' data with MPI-I/O.
};
typedef enum binary_output_mode binary_output_mode;

struct data_spec {
  int outf;             /*!< Output flags */
  const char *filename_template; /*!< Template to use */
  const char *name;     /*!< Component name */
};

/**
 * @brief Porous/IBM Flags
 */
enum solid_form
{
  SOLID_FORM_INVALID = -1, ///< Invalid
  SOLID_FORM_UNUSED = 0,  ///< Undefined form (no solid)
  SOLID_FORM_IBM = 1,     ///< IBM form
  SOLID_FORM_POROUS = 2,  ///< Porous body
};

/**
 * @brief Request origin of updating level set
 */
enum update_level_set_reason
{
  UPDATE_LEVEL_SET_BY_NONE, /*!< No reason (this does not mean not to update) */
  UPDATE_LEVEL_SET_BY_INPUT, /*!< Forcibly updated by input flags */
  UPDATE_LEVEL_SET_BY_RESTART,
  /*!< Level sets were updated in previous calculation */
  UPDATE_LEVEL_SET_BY_PHASE_CHANGE, /*!< Required by phase change */
  UPDATE_LEVEL_SET_BY_INITIAL_VOF, /*!< Required by initial VOF */
  UPDATE_LEVEL_SET_BY_CONTROLLED_VOF, /*!< Required by controlled VOF */
  UPDATE_LEVEL_SET_BY_LIQUID_INLET, /*!< Inlet condition with liquids */
};
typedef enum update_level_set_reason update_level_set_reason;

/**
 * @brief Flags which relates to updating level set functions
 *
 * @p reason stores only the last reason. The reason will not be
 * inherited after restart.
 */
struct update_level_set_flags
{
  int do_update;    /*!< Update level set while time-step integration */
  int force_update; /*!< Input option that allows to set to update lset */
  int describe_reason; /*!< Whether describe reason when marked as update */
  update_level_set_reason reason; /*!< Reason of update */
};
typedef struct update_level_set_flags update_level_set_flags;

enum write_field_variables_format
{
  WRITE_FIELD_VARIABLES_FORMAT_INVALID,
  WRITE_FIELD_VARIABLES_FORMAT_CSV,
};

struct write_field_variables
{
  struct geom_list list;
  jcntrl_write_fv_csv *writer;
};
typedef struct write_field_variables write_field_variables;
#define write_field_variables_entry(ptr) \
  geom_list_entry(ptr, struct write_field_variables, list)

typedef struct
{
  int  slv,       // solver[WELD, CUT]
       restart,   // Index number of the restart file[0,1,2,...]
       post_s,    // Start index number of the post processing
       post_e,    // End index number of the post processing
       print,     // printf[ON, OFF]
       debug,     // debug mode[ON, OFF]
       geom_in;   // input geometry [ON, OFF]
  //--- MPI
  int  pex, pey, pez, pea; // < 2016 Added by KKE (pea)
  int  numGPU;             /*!< Number of GPUs per node (as input value) */
  // Physical model
  int  fluid_dynamics,        // Solve NS eq.[ON, OFF]
       porosity,              // (cut only)porosity[ON, OFF]
       assist_gas,            // assist gas[ON, OFF]
       heat_eq,               // Energy equation[ON, OFF]
       phase_change,          // phase change[ON, OFF]
       eutectic,              // eutectic reaction [ON, OFF]
       melting,               // melting[ON, OFF]
       solidification,        // solidification[ON, OFF]
       vaporization,          // vaporization[ON, OFF]
       condensation,          // condensation[ON, OFF]
       surface_tension,       // surface_tension[ON, OFF]
       oxidation,             // oxidation [ON, OFF]
       radiation,             // radiation [ON, OFF] < 2016 Added by KKE
       solute_diff,            // solute diffusion [ON, OFF] , 2017/9/20
       temperature_dependency, // temperature dependency of physical properties[ON, OFF] <= Fe only!
       laser,                 // Laser irradiation [ON, OFF]
       wettability;         // Contact angle (Sussman)
  //--- Numerical model
  int  WENO,        // weno method[ON, OFF(= 3rd-up-FD)]
       heat_tvd3,   // 3rd-order TVD Runge-Kutta in heat conduction eq.[ON, OFF]
       visc_tvd3,   // 3rd-order TVD Runge-Kutta in advection eq.[ON, OFF]
       interface_capturing_scheme, // PLIC, AWLIC, THINC, WLIC
       PHASEFIELD,  // PhaseField method[ON(PhaseField), OFF(Specified interface capturing scheme)]
       IBM,         // immersed boundary[ON, OFF]
       outflow,     // outflow boundary[ON, OFF]
       mesh_ueq,    // unequally-spaced mesh[ON, OFF]
       vof_adv_fls, // type of vof advection
       porous,      // porous model
       two_energy,  // two energy model[ON, OFF(= one energy model)]
       multi_layer_no_coalescence,     // Both VOF and LS are defined on multi-layer. [ON, OFF]
       multi_layer_less_coalescence,   // Only LS is defined on multi-layer.　[ON, OFF]
       multi_layer, // Not specified by user. ON if multi_layer_no_coalescence=ON or multi_layer_less_coalescence=ON.　[ON, OFF]
       film_drainage;     //  film drainage model　[ON, OFF]
       //--- Boundary condition
  int  bc_xm, // x- [WALL(3), SLIP(7) or OUT(7)]
       bc_xp, // x+ [WALL(3), SLIP(7) or OUT(7)]
       bc_ym, // y- [WALL(3), SLIP(7) or OUT(7)]
       bc_yp, // y+ [WALL(3), SLIP(7) or OUT(7)]
       bc_zm, // z- [WALL(3), SLIP(7) or OUT(7)]
       bc_zp; // z+ [WALL(3), SLIP(7) or OUT(7)]
  int  bct_xm, // x- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
       bct_xp, // x+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
       bct_ym, // y- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
       bct_yp, // y+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
       bct_zm, // z- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
       bct_zp; // z+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  int  bcrad_xm, // x- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
       bcrad_xp, // x+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
       bcrad_ym, // y- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
       bcrad_yp, // y+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
       bcrad_zm, // z- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
       bcrad_zp; // z+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE

  int validate_VOF_init; // validate VOF value after initialization
  int validate_VOF_print_max; // Maximum number of errors to be printed

  //--- Output data
  int  binary,
       gnuplot;
  // radiation exit < 2016 Added by KKE
  int irad_exit;

  /* Oxidation flags */
  int ox_kp_model;   /*!< Oxidation reaction rate model */

  /* H2 Absorption flags */
  int h2_absorp_eval;  /*!< Enable calculation of post-evaluation model */
  int h2_absorp_eval_p_change; /*!< Whether include P change */

  /* LPT flags */
  int lpt_calc; ///< LPT calculation [ON, OFF]
  int lpt_wbcal; ///< LPT wb calc [ON, OFF]
  int lpt_ipttim; /*!< LPT time scheme */
  int lpt_heat;   /*!< LPT heat exchange */
  int lpt_brownian; /*!< LPT Brownian force */
  int lpt_thermophoretic; /*!< LPT thermophoretic force */
  int lpt_use_constant_Cc; /*!< LPT Use constant Cunningham correction */

  FILE *fp; // file pointer
  // ...
  /* YSE: Add file pointers to log input process */
#ifdef JUPITER_MPI
  MPI_File list_fp_mpi;  /*!< MPI capable file pointer */
  MPI_Comm list_fp_comm; /*!< Communicator in list_fp_mpi (for MPI-2 compat.) */
#endif
  FILE *list_fp;      /*!< C standard file pointer */
  char *list_fp_name; /*!< File name for list_fp or list_fp_mpi */
  int list_fp_open;   /*!< Whether list_fp is opened (i.e., not stdout) */

  /* YSE: Add output flags */
  int use_double_binary;
  binary_output_mode output_mode;
  binary_output_mode restart_output_mode;
  binary_output_mode restart_input_mode; /*!< If comp_data is not available */

  /* Misc program control */
  update_level_set_flags update_level_set_ls;  /*!< ls update flag */
  update_level_set_flags update_level_set_lls; /*!< lls update flag */
  update_level_set_flags update_level_set_ll;  /*!< ll update flag */

  init_component rebuild_components; /*!< Components that JUPITER re-processes
                                        geometry input on restart */

  int has_non_uniform_grid; /*!< non_uniform_grid_[xyz] is used */

  /**
   * @brief Flags and structure specification declaration srtucture
   *        for output data
   */
  struct data_output_spec {
    /* Directories */
    char *readdir;  ///< Directory to read from
    char *writedir; ///< Directory to write to

    /* Filename templetes */
    /**
     * @brief Filename templates
     */
    struct filename_template_data {
      char *time;       ///< Filename template for chronological data
      char *comp_based; ///< Filename template for component based data (such as fl, fs, ...)
      char *others; ///< Filename template for other data files.
    } filename_template; ///< Filename template to be used

    /* Output flags */
    int mat; ///< Matetial (overall flags; used as default value on processing input)
    struct data_spec time;   ///< Time data
    struct data_spec fs;     ///< Soild VOF
    struct data_spec fl;     ///< Liquid VOF
    struct data_spec fs_ibm; ///< Solid VOF (IBM solids only)
    struct data_spec u;      ///< Staggered U velocity
    struct data_spec v;      ///< Staggered V velocity
    struct data_spec w;      ///< Staggered W velocity
    struct data_spec uvw;    ///< Cell-center UVW vector
    struct data_spec p;      ///< Pressure
    struct data_spec t;      ///< Temperature
    struct data_spec lls;    ///< Level set between Gas and Solid+Liquid
    struct data_spec ls;     ///< Level set between Solid and Gas+Liquid
    struct data_spec ll;     ///< Level set between Liquid and Gas+Solid

    /* Multi_layer */
    struct data_spec fl_layer;    ///< layer-devided fl_0
    struct data_spec fls_layer;   ///< layer-devided fl_0 + fs_sum
    struct data_spec ll_layer;    ///< Level-set generated by fl_layer
    struct data_spec lls_layer;   ///< Level-set generated by fls_layer
    struct data_spec curv_layer;   ///< curvature
    struct data_spec label_layer; ///< label for each bubble

    /* liquid film flag for film_drainage model */
    struct data_spec liquid_film; ///< label for each bubble

    struct data_spec ls_ibm; ///< Level set between IBM solids and any others
    struct data_spec ox_dt;  ///< Oxidation thickness (cumlative)
    struct data_spec ox_dt_local; ///< Oxidation thickness (current cell)
    struct data_spec ox_flag; ///< Oxidation status flags
    struct data_spec ox_lset; ///< Oxidation target level set function
    struct data_spec ox_vof; ///< Oxidation target VOF function
    struct data_spec ox_q;   ///< Heat generated by Oxidation
    struct data_spec ox_h2;  ///< Amount of H2 to be generated [kg/m3]
    struct data_spec ox_f_h2o; ///< Region of enough H2O is present
    struct data_spec ox_lset_h2o; ///< Level set function of enough H2O
    struct data_spec ox_lset_h2o_s; ///< Save value of ox_lset_h2o
    struct data_spec ox_kp;  ///< Oxidation reaction rate (material data)
    struct data_spec ox_dens;///< Density used for Oxidation (material data)
    struct data_spec ox_recess_rate; ///< Recession rate [m/s2] (material data)
    struct data_spec h2_absorp_eval; ///< H2 averaged absorption rate
    struct data_spec h2_absorp_Ks;   ///< Ks values in H2 absorption
    struct data_spec h2_absorp_P;    ///< averaged pressure in H2 absorption
    struct data_spec h2_absorp_T;    ///< averaged temperature in H2 absorption
    struct data_spec h2_absorp_Zr; ///< Whether any surface cells are pure Zr
    struct data_spec df;             ///< VOF delta for melting
    struct data_spec dfs;    ///< VOF delta for solidification
    struct data_spec entha;  ///< Enthalpy for melting/solidification
    struct data_spec mushy;  ///< Mushy zone in melting/solidification
    struct data_spec lpt_ctrl; ///< LPT control data
    struct data_spec lpt_ptid; ///< LPT particle ID (LPTX only)
    struct data_spec lpt_oid; ///< LPT origin ID (LPTX only)
    struct data_spec lpt_xpt; ///< LPT particle X position
    struct data_spec lpt_ypt; ///< LPT particle Y position
    struct data_spec lpt_zpt; ///< LPT particle Z position
    struct data_spec lpt_pospt; ///< LPT particle position vector
    struct data_spec lpt_uxpt; ///< LPT particle X velocity
    struct data_spec lpt_uypt; ///< LPT particle Y velocity
    struct data_spec lpt_uzpt; ///< LPT particle Z velocity
    struct data_spec lpt_upt; ///< LPT particle velocity vector
    struct data_spec lpt_uf; ///< LPT fluid velocity vector around particle
    struct data_spec lpt_muf;
    ///< LPT fluid viscosity around particle (LPTX only; LPT does not support)
    struct data_spec lpt_densf;
    ///< LPT fluid density around particle (LPTX only; LPT does not support)
    struct data_spec lpt_cpf;
    ///< LPT fluid specific heat around particle (LPTX only)
    struct data_spec lpt_thcf;
    ///< LPT fluid thermal conductivity around partcle (LPTX only)
    struct data_spec lpt_tempf;
    ///< LPT fluid temperature around particle (LPTX only)
    struct data_spec lpt_gradTf;
    ///< LPT fluid temperature gradient around particle (LPTX only)
    struct data_spec lpt_pathf;
    ///< LPT mean free path around particle (LPTX only)
    struct data_spec lpt_mwf;
    ///< LPT fluid mean molecular weight around particle (LPTX only)
    struct data_spec lpt_timpt; ///< LPT particle activation time
    struct data_spec lpt_fuxpt; ///< LPT particle X velocity on time integration scheme
    struct data_spec lpt_fuypt; ///< LPT particle Y velocity on time integration scheme
    struct data_spec lpt_fuzpt; ///< LPT particle Z velocity on time integration scheme
    struct data_spec lpt_fupt; ///< LPT particle velocity vector on time integration scheme
    struct data_spec lpt_dTdt; ///< LPT temperature diff on time integration scheme (LPTX only)
    struct data_spec lpt_fduxt; ///< LPT particle X partial velocity
    struct data_spec lpt_fduyt; ///< LPT particle Y partial velocity
    struct data_spec lpt_fduzt; ///< LPT particle Z partial velocity
    struct data_spec lpt_fdt; ///< LPT particle partial velocity vector
    struct data_spec lpt_diapt; ///< LPT particle diameter
    struct data_spec lpt_denspt; ///< LPT density
    struct data_spec lpt_cppt; ///< LPT specific heat (LPTX only)
    struct data_spec lpt_thcpt; ///< LPT thermal conductivity (LPTX only)
    struct data_spec lpt_temppt; ///< LPT temperature (LPTX only)
    struct data_spec lpt_htrpt; ///< LPT heat transfer rate (LPTX only)
    struct data_spec lpt_tothtpt; ///< LPT total heat transfer (LPTX only)
    struct data_spec lpt_inipospt; ///< LPT initial position (LPTX only)
    struct data_spec lpt_iniupt; ///< LPT initial velocity (LPTX only)
    struct data_spec lpt_initemppt; ///< LPT initial temperature (LPTX only)
    struct data_spec lpt_exit; ///< LPT exited flag (LPTX only)
    struct data_spec lpt_flags; ///< LPT (bytewise) flags (LPTX only)
    struct data_spec lpt_parceln; ///< LPT particles in parcel (LPTX only)
    struct data_spec lpt_fbpt;    ///< LPT Brownian force (LPTX only)
    struct data_spec lpt_fTpt;    ///< LPT Thermophoretic force (LPTX only)
    struct data_spec lpt_seed;    ///< LPT random seed (LPTX only)
    struct data_spec lpt_Y;       ///< LPT mass fraction (LPTX only)
    struct data_spec lpt_ewall;   ///< LPT coefficient of restitution on wall
    struct data_spec rad;    ///< Radiation heat source
    struct data_spec q;      ///< Total (except for rad) input heat source
    struct data_spec qgeom;  ///< Fixed-position/time-independent heat source
    struct data_spec qpt;    ///< Heat transfer from particles
    struct data_spec Y;      ///< Solute mole fraction
    struct data_spec Yt;     ///< Sum of Y.
    struct data_spec Vf;     ///< Solute volume occupation fraction
    struct data_spec flux;   ///< Solute flux
    struct data_spec dens;   ///< Cell-normalized density
    struct data_spec denss;  ///< Cell-normalized solid density
    struct data_spec densf;  ///< Cell-normalized fluid density (dens_f)
    struct data_spec thc;    ///< Cell-normalized thermal conductivity
    struct data_spec thcs;   ///< Cell-normalized solid thermal conductivity
    struct data_spec thcf;   ///< Cell-normalized fluid thermal conductivity
    struct data_spec specht; ///< Cell-normalized specific heat
    struct data_spec spechts; ///< Cell-normalized solid specific heat
    struct data_spec spechtf; ///< Cell-normalized fluid specific heat (c_f)
    struct data_spec mu;     ///< Cell-normalized viscosity
    struct data_spec t_liq;  ///< Cell-normalized liquidus temperature
    struct data_spec t_soli; ///< Cell-normalized solidus temperature
    struct data_spec diff_g; ///< Cell-normalized gas diffusivity
    struct data_spec latent; ///< Cell-normalized latent heat
    struct data_spec emi;    ///< Cell-normalized emissivity
    struct data_spec bnd;    ///< Boundary data
    struct data_spec eps;    ///< prosity
    struct data_spec perm;    ///< permeability
    struct data_spec epss;    ///< Effective prosity
    struct data_spec tf;    ///< Temperature for fluid phase
    struct data_spec ts;    ///< Temperature for solid phase
    struct data_spec sgm;   ///< Equivalent property for rho*c
    struct data_spec uplsflg; ///< Level set update flags
    struct data_spec bnd_norm_u;
    ///< Surface normal vector on Staggared U velocity
    struct data_spec bnd_norm_v;
    ///< Surface normal vector on Staggared V velocity
    struct data_spec bnd_norm_w;
    ///< Surface normal vector on Staggared W velocity
    struct data_spec div_u; ///< Diverence of velocity (source term of poisson)
    struct data_spec mass_source_g; ///< Mass source for gasous phases
    struct data_spec comp_data;     ///< Component metadata
  } restart_data, ///< Specification for restart data I/O
    output_data;  ///< Specification for output data I/O

} flags;

typedef struct
{
  type fs_xs, fs_xe, // initial VOF(solid) area
       fs_ys, fs_ye,
       fs_zs, fs_ze;
  type fl_xs, fl_xe, // initial VOF(liquid) area
       fl_ys, fl_ye,
       fl_zs, fl_ze;
  type px,py; // initial position of piercing hole
  type pr;    // radius of piercing hole
} initial_vof_profile;

/* YSE: Liquidus/Solidus Temperature data (defined in tmcalc.h) */
struct tm_table_param;
struct tm_func2_param;

/**
 * @brief Struct that express the input data of solute_diffusivity as is.
 *
 * The member @p valid_id_combination determines whether pointer or value
 * of union is used.
 */
struct dc_calc_param_input {
  struct geom_list list;
  struct dc_calc_input_idinfo {
    struct component_info_data id; /*!< Component info */
    csv_row    *row;    /*!< CSV row pointer where read from */
    csv_column *column; /*!< CSV column pointer where read from */
  } diffusing, /*!< Diffusing Material info */
    base;      /*!< Base Material info */

  int missing;                /*!< Whether unspecified on input */
  struct dc_calc_param  data; /*!< Parameter */
};
#define dc_calc_param_input_entry(ptr) \
  geom_list_entry(ptr, struct dc_calc_param_input, list)

struct particle_set_input {
  struct geom_list list;
#ifdef LPT
  struct LPT_pset set;
#endif
#ifdef LPTX
  LPTX_particle_init_set *set;
#endif
};
#define particle_set_input_entry(ptr) \
  geom_list_entry(ptr, struct particle_set_input, list)

/**
 * @brief Block size should be used for solver
 */
struct solver_block_size
{
  int nxblock; /*!< X-axis block size for block-wise Jacobian precondition */
  int nyblock; /*!< Y-axis block size for block-wise Jacobian precondition */
  int nzblock; /*!< Z-axis block size for block-wise Jacobian precondition */
};

enum non_uniform_grid_function
{
  NON_UNIFORM_GRID_FUNC_INVALID = -1,    ///< (invalid)
  NON_UNIFORM_GRID_FUNC_CONST = 0,       ///< Constant delta
  NON_UNIFORM_GRID_FUNC_CONST_RATIO_INC, ///< Increasing constant ratio delta
  NON_UNIFORM_GRID_FUNC_CONST_RATIO_DEC, ///< Decreasing constant ratio delta
  NON_UNIFORM_GRID_FUNC_SINE,            ///< Half sine curve delta
  NON_UNIFORM_GRID_FUNC_QSINE_B,         ///< First quater sine curve delta
  NON_UNIFORM_GRID_FUNC_QSINE_E,         ///< Second quater sine curve delta
};
typedef enum non_uniform_grid_function non_uniform_grid_function;

/*
 * The definition of variable_delta_funcs is hidden (private).
 * It's in vardelta.c.
 */
struct variable_delta_funcs;

/**
 * @brief Variable delta input definition
 */
struct non_uniform_grid_input_data
{
  struct geom_list list;                    /*!< List of regions */
  non_uniform_grid_function func;             /*!< Function enum value */
  type high;        /*!< Upper bound of region */
  int ndiv;         /*!< Number of division for a region */
  const struct non_uniform_grid_funcs *funcs; /*!< Delegation set */
  void *func_param;                         /*!< function specific parameters */
};
#define non_uniform_grid_input_entry(ptr) \
  geom_list_entry(ptr, struct non_uniform_grid_input_data, list)

typedef struct
{
  //--- stencl
  int   stm, // - direction
        stp; // + direction
  int   NumberOfComponent; ///< Number of total components
  int   NBaseComponent;    ///< Number of Solid-Liquid phase components
  int   NGasComponent;     ///< Number of Gas only components
  int   NIComponent;       ///< Number of components in input
  int   NIBaseComponent;   ///< Number of Solid-Liquid components in input
  int   NIGasComponent;    ///< Number of Gas only components in input
  int   NumberOfLayer;            ///< Number of layers for multi_layer model
  //--- spatial domain values
  int   nx, ny, nz, n, nxy,// num comp cell (local)
        mx, my, mz, m, mxy,// <= including boundary(ghost) cell
        step;
  int   nlat, nlon; // < 2016 Added by KKE
  type  dt, dti, dt_rad; // < 2016 Added by KKE (dt_rad)
  type  grav_x, Lx, dx, dxi; /* dx stores min(dxv), dxi = 1/dx */
  type  grav_y, Ly, dy, dyi;
  type  grav_z, Lz, dz, dzi;
  type  *x,  *y,  *z;  /*!< alias for xv, yv and zv */
  type  *xv, *yv, *zv; /*!< 'cell vertex' (for staggered grid) coordinates */
  type  *xc, *yc, *zc; /*!< 'cell center' (for regular grid) coordinates */
  /*
   * Coordinates:
   *   x-staggared mesh (e.g. u) (yz-cell face center): (xv, yc, zc)
   *   y-staggared mesh (e.g. v) (xz-cell face center): (xc, yv, zc)
   *   z-staggared mesh (e.g. w) (xy-cell face center): (xc, yc, zv)
   *   scalar mesh               (cell center)        : (xc, yc, zc)
   *                             (cell vertex)        : (xv, yv, zv)
   */
  type  *dxv, *dyv, *dzv;     ///< Delta of xv, yv or zv
  type  *dxc, *dyc, *dzc;     ///< Delta of xc, yc or zc
  type  *dxcp, *dycp, *dzcp;  ///< Delta from xc to cell face in positive dir
  type  *dxcn, *dycn, *dzcn;  ///< Delta from xc to cell face in negative dir
  type  *dxvp, *dyvp, *dzvp;  ///< Delta from cell face to xc in positive dir
  type  *dxvn, *dyvn, *dzvn;  ///< Delta from cell face to xc in negative dir
  /*
   * Mathemetical relations (numerically these may not be equal):
   *  - dxcn[i] + dxcp[i] == dxv[i] (dxcn[i] == dxcp[i] == dxv[i] / 2.0)
   *  - dxvp[i] + dxvn[i + 1] == dxc[i]
   */
  struct non_uniform_grid_input_data non_uniform_grid_x_head;
  struct non_uniform_grid_input_data non_uniform_grid_y_head;
  struct non_uniform_grid_input_data non_uniform_grid_z_head;

  //--- Oxidation
  struct ox_component_info ox_zry;  ///< Oxidation info for Zircaloy
  struct ox_component_info ox_zro2; ///< Oxidation info for Zirconium Dioxide
  struct ox_component_info ox_h2;   ///< Oxidation info for Hydrogen
  struct ox_component_info ox_h2o;  ///< Oxidation info for Water
  tempdep_property ox_kp; ///< Oxidation reaction rate for OX_KPMODEL_TEMPDEP.
  type  ox_q_fac;  ///< Factor to Oxidation Heat (0 to disable heating)
  type  ox_diff_h2_sol; ///< Diffusion coefficient of generating H2 in solid.
  type  ox_diff_h2_srf; ///< Diffusion coefficient of generating H2 on surface.
  type  ox_recess_init; ///< Initial assumed thickness for recession
                        ///< (0 or negative to disable recession)
  type  ox_recess_min;  ///< Minimum recession thickness
  tempdep_property ox_recess_rate; ///< Oxidation recession rate
  type  ox_h2o_threshold; ///< Threshold value for H2O fraction that evaluates
                          ///< that enough H2O present for the cell.
  type  ox_h2o_lset_min_to_recess; ///< Minimum required increase in
                                   ///< the level set function of enough
                                   ///< H2O to change status to recessing.
  size_t ox_nox_calc; ///< Number of oxidation calculaton performed (cell x step)
  size_t ox_nre_calc; ///< Number of recession calculaton performed (cell x step)
  //--- H2 Absorption
  type  h2_absorp_base_p; ///< Reference pressure for H2 absorption evaluation
  //--- Surface Tension
  type  width;// surface width

  //--- Phase Field parameters
  type c_delta,  //PF param.[1/4] width (2.0 ~ 3.0)
       lambda,   //PF param.[2/4] Interface smoothness (0.01 ~ 0.10)
       sigma_pf,  //PF param.[3/4] Surface tension (depends on physical prop.)
       mobility; //PF param.[4/4] Intensity of phase field (anti)diffusion (depends on cases. 1.0 ~ 3.0)
  
  //--- Contact angle
  type contact_angle;
  int CA_iteration;

  //--- Film drainage
  type film_cell;
  type rapture_thickness;
  type height_threshold;

  //--- Porous model
  type d_p; // Particle size [m]
  type h_f; // Heat transfer coefficient [W/m^2/K]
  type a_sf; // Specific surface [1/m]
  type U_ref, L_ref; // Representative velocity and length
  type Forch_coef;

  type  coef_lsts; // Coefficient of time step of level-set iteration
  int ls_iteration;   // Number of level-set iteration

  //--- Substeps
  int   nsub_step_t;// number of substeps for heat conduction
  int   nsub_step_mu;// number of substeps for heat conduction
  //--- Global values (for MPI)
  int   gnx,gny,gnz,gn,// num comp cell (global)
        gmx,gmy,gmz,gm;
  type  gLx,gLy,gLz,
        *gx,*gy,*gz;
  //--- time values
  int   icnt;
  type  time, tend,
        cfl_num, diff_num;
  type  restart_dump_time;
  //--- for output
  int   iout, view, outs, viewflg; /* YSE: Added viewflg */
  type  tout, dtout;
  type  vel_max, t_max, tf_max, ts_max;
  initial_vof_profile *vof;
  //--- boundary condition
  controllable_type  tw_xm,// x- [Wall temperature (west)]
                     tw_xp,// x+ [Wall temperature (east)]
                     tw_ym,// y- [Wall temperature (south)]
                     tw_yp,// y+ [Wall temperature (north)]
                     tw_zm,// z- [Wall temperature (bottom)]
                     tw_zp;// z+ [Wall temperature (top)]
  type y_ent;
  type mass_before, mass_after;
  type flow_rate;
  type gas_jet_flow_rate;
  type pconst;
  int icnt_end; // < 2016 Modified by KKE

  // 輻射反復計算用変数 (2016 Added by KKE)
  int   picard_max, picard_out, newton_max;
  type  E_cell_err_max, tmp_cell_err_max, dtmp_cell_err_max;

  /* phase change (vap/cond) data */
  struct component_info_data vap_cond_liquid_id;
  /*!< Liquid phase material to vapor/cond */

  /* LPT parameter data */
  char *lpt_outname; /*!< LPT data output name (for LPT module's feature) */
  type lpt_outinterval; /*!< LPT output interval */

  type lpt_wallref_xm; /*!< LPT coefficient of restitution at X- wall */
  type lpt_wallref_xp; /*!< LPT coefficient of restitution at X+ wall */
  type lpt_wallref_ym; /*!< LPT coefficient of restitution at Y- wall */
  type lpt_wallref_yp; /*!< LPT coefficient of restitution at Y+ wall */
  type lpt_wallref_zm; /*!< LPT coefficient of restitution at Z- wall */
  type lpt_wallref_zp; /*!< LPT coefficient of restitution at Z+ wall */
  type lpt_cc; /*!< LPT Cunningham correction (brownian force only) */
  type lpt_cc_A1; /*!< LPT Cunningham correction coeff A1 */
  type lpt_cc_A2; /*!< LPT Cunningham correction coeff A2 */
  type lpt_cc_A3; /*!< LPT Cunningham correction coeff A3 */
  type lpt_tp_Cs; /*!< LPT Thermophoretic force const Cs */
  type lpt_tp_Cm; /*!< LPT Thermophoretic force const Cm */
  type lpt_tp_Ct; /*!< LPT Thermophoretic force const Ct */

  struct particle_set_input lpt_particle_set_head; /*!< Head of particle set */
  struct component_info lpt_mass_fractions; /*!< Mass fractions for LPT */

  //--- misc
  controllable_type reference_pressure; /*!< reference pressure [Pa] */

  //--- mass source
  struct component_info mass_source_g_comps;
  /*!< component ids in variables.mass_source_g */

  /* YSE: Heat source info data */
  heat_source_param heat_sources_head;

  /* YSE: Boundary data head */
  fluid_boundary_data   fluid_boundary_head;
  thermal_boundary_data thermal_boundary_head;
  surface_boundary_data surface_boundary_head;

  /* YSE: boundary domain data */
  int nbx;   /*!< Boundary domain size for X-axis */
  int nby;   /*!< Boundary domain size for Y-axis */
  int nbz;   /*!< Boundary domain size for Z-axis */
  int stmb;  /*!< Near-to-0 (-) side stencil size for boundary data */
  int stpb;  /*!< Far-to-0 (+) side stencil size for boundary data */

  /* YSE: Inlet velocity data from param.txt */
  /**
   * @brief Inlet velocity data for from param.txt input file
   */
  struct vin_data {
    controllable_type u; /*!< U inlet velocity */
    controllable_type v; /*!< V inlet velocity */
    controllable_type w; /*!< W inlet velocity */
    struct inlet_component_data *comps; /*!< Inlet component data */
  } vin_xm, /*!< X- Inlet velocity data (valid only if bc_xm == INLET) */
    vin_xp, /*!< X+ Inlet velocity data (valid only if bc_xp == INLET) */
    vin_ym, /*!< Y- Inlet velocity data (valid only if bc_ym == INLET) */
    vin_yp, /*!< Y+ Inlet velocity data (valid only if bc_yp == INLET) */
    vin_zm, /*!< Z- Inlet velocity data (valid only if bc_zm == INLET) */
    vin_zp; /*!< Z+ Inlet velocity data (valid only if bc_zp == INLET) */

  /**
   * @brief Pressure condition data from param.txt input file
   */
  struct pout_data {
    out_p_cond cond; /*!< Condition */
    controllable_type const_p; /*!< Constant pressure */
  } p_xm, /*!< X- pressure condition data (valid only if bc_xm == OUT) */
    p_xp, /*!< X+ pressure condition data (valid only if bc_xp == OUT) */
    p_ym, /*!< Y- pressure condition data (valid only if bc_ym == OUT) */
    p_yp, /*!< Y+ pressure condition data (valid only if bc_yp == OUT) */
    p_zm, /*!< Z- pressure condition data (valid only if bc_zm == OUT) */
    p_zp; /*!< Z+ pressure condition data (valid only if bc_zp == OUT) */

  /* YSE: control executive data for time ($time) and dt ($delta-t) */
  jcntrl_executive fv_time;
  jcntrl_executive fv_delta_t;

  write_field_variables write_field_variables_head;

  /* Solver parameters */
  struct solver_block_size solver_u_block_size; /*!< for U (X-axis staggared) */
  struct solver_block_size solver_v_block_size; /*!< for V (Y-axis staggared) */
  struct solver_block_size solver_w_block_size; /*!< for W (Z-axis staggared) */
  struct solver_block_size solver_p_block_size; /*!< for P and other scalars (cell center) */

  /*Fukuda enthaply monitor*/
  type init_total_enthalpy; 
} domain;

/*
 * Yamashita: parameters for laser melting simulation
 * 2020 4/7
*/
typedef struct
{
  // laser power & density
  type  pw0,  // power[W]
        qm,   // energy density[W m-2]
        r0,   // radius[m]
        d0,   // diameter (2*r0) [m]
        alpha,// absorption coefficient
        R,
        lambda; // wave length[m]
  // nozzle parameter
  type  lsr_x, lsr_y, lsr_z,// origin of laser position [m]
        nzl_x, nzl_y, nzl_z,// origin of nozzle position [m]
        nzl_r,       // nozzle radius
        work_dist;   // working distance [m]
  type  sweep_vel,// sweep velocity [m min-1]
        swp_vel,  // sweep velocity [m s-1]
        flow_vol, // flow volume [l/min]
        gas_tmp,  // assist gas temperature[K]
        gas_vel;  // assist gas velocity [m s-1]
  type  q_sum;
} laser;

/*
 * YSE: To improve manageability, use an array of structure which
 * corresponds to each components, instead of a collection of array of
 * primitive type (i.e., `type`).
 *
 * So `phv->tm_soli[i]` will be `phv->comp[i].tm_soli`.
 */
/**
 * @brief phase value data for each component
 *
 * Access with `phv->comp[i].tm_soli` etc.
 */
struct phase_value_component
{
  type  tm_soli, ///< solidus temperature for A [K]
        tm_liq, ///< liquidus temperature for A [K]
        tb; ///< boiling temperature[K]
  type  lh; ///< latent heat (solid-liquid)
  type  lhv; ///< latent heat (liquid-gas)
  type  molar_mass; ///< Molar density [g mol-1], added by Chai
  enum solid_form sform;  ///< Form of solid
  type  poros;
  type  permea;

  tempdep_property
        beta, ///< coefficient of volumetric expansion
        radf, ///< radiation factor
  //    radf_l, // radiation factor
  //    radf_s, // radiation factor
        sigma;///< coefficient of surface tension
  tempdep_property rho_s; ///< Solid density [kg m-3]
  tempdep_property rho_l; ///< Liquid density [kg m-3]
  tempdep_property rho_g; ///< Gas density [kg m-3]
  tempdep_property emi_s; ///< emissivity (for radiation) < 2016 Added by KKE
  tempdep_property emi_l; ///< emissivity (for radiation) < 2016 Added by KKE
  tempdep_property emi_g; ///< emissivity (for radiation) < 2016 Added by KKE
  tempdep_property mu_s;  ///< Solid viscosity [Pa s]
  tempdep_property mu_l;  ///< Liquid viscosity [Pa s]
  tempdep_property mu_g;  ///< Gas viscosity [Pa s]
  tempdep_property thc_s; ///< Solid thermal conductivity [W m-1 K-1]
  tempdep_property thc_l; ///< Liquid thermal conductivity [W m-1 K-1]
  tempdep_property thc_g; ///< Gas thermal conductivity [W m-1 K-1]
  tempdep_property specht_s; ///< Solid specific heat capacity at constant pressure [J kg-1 K-1]
  tempdep_property specht_l; ///< Liquid specific heat capacity at constant pressure [J kg-1 K-1]
  tempdep_property specht_g; ///< Gas specific heat capacity at constant pressure [J kg-1 K-1]

  tempdep_property saturated_pressure;
  ///< Saturated pressure for wall or aerosol condensation [Pa]
  tempdep_property condensation_E;
  ///< Internal energy that will be removed from the domain upon wall
  ///  or aerosol condensation and chemisorption. [J kg-1]

};
typedef struct phase_value_component phase_value_component;

typedef struct
{
  phase_value_component *comps; ///< Array of each component data.

  type  tr;// room    temperature[K]
  tempdep_property rho_g, mu_g, thc_g, specht_g;//,radf_g;
  type  molar_mass_g; ///< Molar mass
  type  sol_tmp, liq_tmp, gas_tmp; // initial temperature[K]
  type  rad_W;// total radiation heat [W]

  //type  nu_s_A,  nu_l_A, nu_g;  // kinetic viscosity [Pa s m3 kg-1]

  /* YSE: Add use of table */
  struct tm_table_param *liq_tables; ///< Tables of Liquidus Temeprature by Y
  struct tm_table_param *sol_tables; ///< Tables of Solidus Tempertaure by Y
  struct tm_func2_param *liq_funcs;  ///< Functions of Liquidus Temperture by Y
  struct tm_func2_param *sol_funcs;  ///< Functions of Solidus Temperture by Y

  /* YSE: Diffusion coeff definitions */
  struct dc_calc_param **diff_params; ///< Functions of Diffusion coeff by Y
  struct dc_calc_param_input diff_input_head; ///< Parsed input data of diffusion coeff by Y.

  struct dc_calc_param **diff_g_params; ///< Functions of diffusion coeffs by Yg
  struct dc_calc_param_input diff_g_input_head; ///< Parsed input data of diffusion coeffs by Yg.
} phase_value;

enum diffusion_coeff_commutativity
{
  DIFF_S_COMMUTATITVE = 0, ///< Commutativity of phv->diff_params
  DIFF_G_COMMUTATITVE = 1, ///< Commutativity of phv->diff_g_params
};

typedef struct
{
  type  total, // total time
        total_job, // total time per 1 job
        dtctl, // dt_control()
        vof,   // vof_advection()
        multi_layer, // multi_layer()
        rk3,   // tvd_runge_kutta3()
        heat,  // heat_conduction()
        radiation, // radiation() < 2016 Added by KKE
        phase, // phase_change()
        eutectic, // eutectic()
        div,   // divergence_free()
        oxide, // oxidation()
        matel, // materials()
        hsource, // heat_source()
        data,  // output_data()
        lpt,   ///< LPT
        solute; // solute transport 2017/09/22
  type  job_start; // YSE: Add JOB start time.
  type  control_update; // update_control_values()
} timer_main;

typedef struct
{
  int     argc;
  char  **argv;
  flags       *flg; // computational flags
  domain      *cdo; // computational domain and time
  phase_value *phv; // phase values
#ifdef GPU
  gpu_param   *gpu; // gpu parameter
#endif
  mpi_param   *mpi; // mpi parameter
  timer_main  *time;// timer
  laser       *lsr; // laser paramter

  /* YSE: file names of inputs */
  char *flags_file; // file name of flags
  char *param_file; // file name of parameters
  char *plist_file; // file name of property list
  char *geom_file;  // file name of geometry list
  char *control_file; // file name of control list

  /* YSE: Parsed CSV data for each input files */
  csv_data *flags_data;    // parsed data of flags
  csv_data *param_data;    // parsed data of parameters
  csv_data *plist_data;    // parsed data of plist
  csv_data *geom_data;     // parsed data of geom
  csv_data *control_data;  // parsed data of control

  component_data comps_data_head; ///< Component metadata

  /* YSE: List of geometry data */
  struct geom_data *geom_sets;     // geometry set data
  struct geom_data *control_sets;  // geometry set data for controling domain

  /* YSE: Save error status to postpone the termination of program */
  int status;               // Error status

  /* Grid Feeder (provider of `:all` grid, NULL if anyone refers it) */
  jupiter_grid_data_feeder *grid_feeder;

  /* YSE: Control and Statistic calculation executive manager */
  jcntrl_executive_manager *controls;
  controllable_type control_head; ///< This is head of all controllable variable
} parameter;



//===========================================
//  Computational arrays (pointers)
//-------------------------------------------
typedef struct
{
  type *dens,   // density[kg m-3]
       *mu,     // viscosity[Pa s]
       *nu,     // kinetic viscosity[Pa s m3/kg]
       *specht, // specific heat capacity at constant pressure[J kg-1 K-1]
       *thc,    // thermal conductivity[W m-1 K-1]
       *thcs,    // thermal conductivity for two energy model
       *thcf,    // thermal conductivity for two energy model
       *denss,  // density for two energy model
       *st,     // surface tension [N.m-1]
       *q,      // heat source[W m-3]
       *Dcg,    // YSE: diffusion coefficients for gaseous phase
       *latent; // added 2017/11/16
  type *t_soli, *t_liq, // added 2017/11/16
       *rad,    // radiation heat[W m-2] < 2016 Added by KKE
       *emi,    // Emissivity (for radiation) < 2016 Added by KKE
       *Vp;     //Partial volume of species, added by Chai
//       *rad_f;    // radiation heat[W m-2]
  type *div_b, // (before) source term of poisson eq.
       *div_a; // (before) source term of poisson eq.
  type *div_u; // divergence of u (information only, not required for calc)

  // Oxidation specific material (information only, not required for calc)
  type *ox_kp; /// Reaction rate [kg2/m4.s]
  type *ox_dens; /// Density used for Oxidation [kg/m3]
  type *ox_recess_k; /// Recession rate [m/s2]

  type *dens_f, *c_f; // for Porus medium model
  /*
   * Solute transport specific material (also information only, not
   * required for calc)
   *
   * Binary diffusivities and Effective diffusivities are dependent
   * only on the surface Y and T (temperature), and surface variables
   * are just the averge of two cell values which is next to. So for
   * writing conception, the all direction of a cell (T, B, N, S, E, W)
   * are available, but just stores like a staggared mesh.
   */
  type *D_bin_x; /// Binary diffusivity on X axis direction
  type *D_bin_y; /// Binary diffusivity on Y axis direction
  type *D_bin_z; /// Binary diffusivity on Z axis direction
  type *D_mul_x; /// Effective diffusivity on X axis direction
  type *D_mul_y; /// Effective diffusivity on Y axis direction
  type *D_mul_z; /// Effective diffusivity on Z axis direction
} material;

typedef struct
{
  type *u, // velocity(x)[m s-1]
       *v, // velocity(y)[m s-1]
       *w, // velocity(z)[m s-1]
       *t, // temperature[K]
       *p; // pressure[Pa]
  type *fl, // VOF(liquid)
       *fs, // VOF(solid)
       *Y,  // Molar fraction: added by Chai
       *Yt, // Molar fraction: added by Chai
       *Vf, // volume fraction for solute_diffusion calculation: added by Chai 2019/9/2
       *Sy,
       *fg,  // VOF (gas)! 2017/09/23
       *fl_sum,
       *fs_sum,
       *fls,// VOF(liquid+solid)
       *ll, // Level Set(liquid)
       *ls, // Level Set(solid)
       *lls;// Level Set(liquid+solid)
  type *fs_ibm; // VOF(IBM solid only)
  type *ls_ibm; // Level Set(IBM solid only)
  type *qgeom; // YSE: Fixed geometric heat source [W m-3] (warning: qgeom will be NULL if no Geom_fixed_heat_source is defined)
  type *df_oxd;
  type *df, *dfs; // Susumu, 2020/07/17
  // work array for time integration
  type *nvlx, *nvly, *nvlz, *curv,
       *nvsx, *nvsy, *nvsz;
  type *nvibmx, *nvibmy, *nvibmz;
  type *g1u, *g2u,
       *g1v, *g2v,
       *g1t, *g2t;
  type *vfx,
       *vfy,
       *vfz; // < 2017/09/22
  type *vfxg,
       *vfyg,
       *vfzg; // < 2017/09/23
  type *Y0,*Y0_G; // 2017/09/23
  type *flg_obst_A, *flg_obst_B;
  type *vel_c;
  type *work, *work_multi;
  type mass0,mass,vol0,vol;
  /* YSE: Replace oxidation variables */
  // oxidation
  type *ox_dt; ///< Oxidation thickness (cumlative) [m]
  type *ox_dt_local; ///< Oxidation thickness (current cell) [m]
  int  *ox_flag; ///< Oxidation status flags
  type *ox_lset; ///< Oxidation target level set function [m]
  type *ox_vof; ///< Oxidation target VOF function [-]
  type *ox_q;   ///< Heat source by oxidation [W m-3]
  type *ox_h2;  ///< Amount of H2 to be generated [kg m-3]
  type *ox_f_h2o; ///< 0 if H2O is enough, 1 if not [-]
  type *ox_lset_h2o; ///< Level set function of the region that enough
                     ///< H2O is present. Note the that the region
                     ///< that H2O is present is negative. [m]
  type *ox_lset_h2o_s; ///< Saved value of ox_lset_h2o. The value only exists
                       ///< where the oxidation has started, otherwise 0. [m]
  // H2 Absorption
  type *h2_absorp_eval; ///< Post predicted H2 averaged absorption rate (H/Zr)
  type *h2_absorp_Ks;   ///< Ks values [Pa^(-1/2)]
  type *h2_absorp_P;    ///< averaged pressure for H2 absorption
  type *h2_absorp_T;    ///< averaged temperature for H2 absorption
  type *h2_absorp_Zr;   ///< stores 1 if any surface cells are remained
                        ///< unoxidized

  /*added by Chai for new phase change module*/
  type *t_pre, *entha;
  int *mushy;
  /*added by Susumu for Porous medium */
  type *eps;  // Porosity
  type *perm; // Permeability
  type *sgm;  // Equivalent property for rho*c
  type *epss; // Effective porosity
  type *tf;   // Temperature of fluid phase (Only used for the two energy model)
  type *ts;   // Temperature of solid phase (Only used for the two energy model)

  /* Local copy of LPT variables for initialization etc. */
  type *lpt_pewall;

#ifdef LPTX
  LPTX_param *lpt_param; ///< LPTX parameter.
#endif
  type *qpt; ///< Heat transfer from particles

  /* Mass source */
  type *mass_source_g; /*!< Total mass source in gases [kg] */

  /* YSE: Added boundary array */
  struct boundary_array {
    struct fluid_boundary_data   **fl;
    struct thermal_boundary_data **th;
  } bnd_W, bnd_E, bnd_S, bnd_N, bnd_B, bnd_T;

  struct surface_boundary_data **surface_bnd;
  type *bnd_norm_u; /*!< surface normal for X staggared */
  type *bnd_norm_v; /*!< surface normal for Y staggared */
  type *bnd_norm_w; /*!< surface normal for Z staggared */

  type Dg_max;//added by Chai

  /*Fukuda: enthaply monitor*/
  type *enthalpy;
  type *init_enthalpy;
  type *enthalpy_time_derivative; 

  /*Fukuda multi_layer*/
  int *bubble_cnt;
  type *fl_layer;
  type *fls_layer;
  type *ll_layer;
  type *lls_layer;
  type *nvlx_layer;
  type *nvly_layer;
  type *nvlz_layer;
  type *curv_layer;
  int *label_layer;
  int *is_orifice_layer;
  int is_orifice_layer_initialized;

  /*Fukuda film_drainage*/
  type *liquid_film; // if the cell is not film liquid_film = 0.0. if the cell is in film, it returns the thickness (m)
  
} variable;

#define   MAX2(x, y) ((x) > (y) ? (x) : (y))
#define   MIN2(x, y) ((x) < (y) ? (x) : (y))
#define   MAX3(x, y, z) ( (MAX2(x, y)) > (z) ? (MAX2(x, y)) : (z) )
#define   MIN3(x, y, z) ( (MIN2(x, y)) < (z) ? (MIN2(x, y)) : (z) )

/* YSE: New geometry interface data */
/**
 * @brief Boundary direction ids.
 */
enum boundary_direction {
  BOUNDARY_DIR_NONE   = 0x000, /*!< None */
  BOUNDARY_DIR_TOP    = 0x001, /*!< Top (z+) */
  BOUNDARY_DIR_BOTTOM = 0x002, /*!< Bottom (z-) */
  BOUNDARY_DIR_NORTH  = 0x004, /*!< North (y+) */
  BOUNDARY_DIR_SOUTH  = 0x008, /*!< South (y-) */
  BOUNDARY_DIR_EAST   = 0x010, /*!< East (x+) */
  BOUNDARY_DIR_WEST   = 0x020, /*!< West (x-) */
  BOUNDARY_DIR_ALL    = 0x100, /*!< All */
  BOUNDARY_DIR_X      = 0x200, /*!< X (East and West) */
  BOUNDARY_DIR_Y      = 0x400, /*!< Y (North and South) */
  BOUNDARY_DIR_Z      = 0x800, /*!< Z (Top and Bottom) */
};
typedef enum boundary_direction boundary_direction;

/**
 * @brief VOF specific initialization data.
 */
struct init_vof_data
{
  struct component_info_data comp; /*!< Material ID number */
  geom_vof_phase phase;            /*!< Phase to set */
};

enum init_lpt_pewall_component {
  INIT_LPT_PEWALL_INVALID,
  INIT_LPT_PEWALL_NORMAL,  /*!< Normal */
  INIT_LPT_PEWALL_TANGENT, /*!< Tangent (not supported) */
};

/**
 * @brief boundary LPT wall resistitution coeff data.
 */
struct init_lpt_pewall_data {
  enum init_lpt_pewall_component component;   /*!< Direction component */
  int is_boundary;        /*!< Whether this is for external boundary or not */
  boundary_direction dir; /*!< direction of external boundary */
};

/**
 * @brief BOUNDARY specific initialization data.
 */
struct boundary_init_data {
  boundary_direction dir; /*!< direction of boundary */
  double threshold;   /*!< VOF threshold value */
  fluid_boundary_data lhead; /*!< Local fluid boundary head */
  fluid_boundary_data *data; /*!< boundary data */
};

/**
 * @brief TBOUNDARY specific initialization data.
 */
struct tboundary_init_data {
  boundary_direction dir; /*!< direction of boundary */
  double threshold;     /*!< VOF threshold value */
  thermal_boundary_data lhead; /*!< Local thermal boundary head */
  thermal_boundary_data *data; /*!< boundary data */
};

/**
 * @biref SURFACE_BOUNDARY specific initialization data
 */
struct surface_boundary_init_data {
  double threshold; /*!< VOF threshold value */
  surface_boundary_data lhead; /*!< Local surface boundary head */
  surface_boundary_data *data; /*!< boundary data */
};

/**
 * @brief Extra data for geom_data
 */
struct jupiter_geom_ext_data {
  int print_matrix;   /*!< Whether transformation matrix will printed */
};
typedef struct jupiter_geom_ext_data jupiter_geom_ext_data;

/**
 * @brief Extra data for geom_data_element
 */
struct jupiter_geom_ext_eldata {
  char *dump_file;
  /*!< File name to dump geometry data */
  binary_output_mode dump_united;
  /*!< Whether generate single file or unified for dump */
};
typedef struct jupiter_geom_ext_eldata jupiter_geom_ext_eldata;

/**
 * @brief Extra data for geom_file_data
 */
struct jupiter_geom_ext_file_data {
  binary_output_mode read_mode;
};
typedef struct jupiter_geom_ext_file_data jupiter_geom_ext_file_data;

/**
 * @brief Extra data for geom_shape_element
 */
struct jupiter_geom_ext_shp_eldata {
  const char *file;  /*!< Geometry input file name */
  csv_row *row;  /*!< Row of the CSV where the shape defined */
  geom_error stk_error; /*!< Stack error for occuring */
  controllable_geometry_entry control_entry_head;
  /*!< Head entry of control data */
};
typedef struct jupiter_geom_ext_shp_eldata jupiter_geom_ext_shp_eldata;

/**
 * @brief Extra data for geom_surface_shape_element
 */
struct jupiter_geom_ext_sshp_eldata {
  const char *file;  /*!< Geometry input file name */
  csv_row *row;  /*!< Row of the CSV where the surface shape defined */
  geom_error stk_error; /*!< Stack error for occurring */
  controllable_geometry_entry control_entry_head;
  /*!< Head entry of control data */
};
typedef struct jupiter_geom_ext_sshp_eldata jupiter_geom_ext_sshp_eldata;

/**
 * @brief Extra data for geom_init_element
 */
struct jupiter_geom_ext_init_eldata {
  controllable_geometry_entry control_entry_head;
  /*!< Head entry of control data */
};
typedef struct jupiter_geom_ext_init_eldata jupiter_geom_ext_init_eldata;

enum jupiter_geom_enum_vartype {
  JUPITER_VARTYPE_OUTPUT_MODE = GEOM_VARTYPE_ENUMTYPE_MIN + 1,
  JUPITER_VARTYPE_BOUNDARY,
  JUPITER_VARTYPE_TBOUNDARY,
  JUPITER_VARTYPE_INLET_DIR,
  JUPITER_VARTYPE_LAST,
};
static char jupiter_vartype_last_must_be_less_than_max[GEOM_VARTYPE_ENUMTYPE_MAX - JUPITER_VARTYPE_LAST + 1];

/**
 * @brief JUPITER-specific arguments for initializations.
 */
struct jupiter_init_func_args {
  ptrdiff_t cell; /*!< cell index */
};

enum control_nametype
{
  CONTROL_NAMETYPE_INVALID,          /*!< Unknown name type */
  CONTROL_NAMETYPE_GRID,             /*!< Name specifies a grid */
  CONTROL_NAMETYPE_MASK,             /*!< Name specifies a mask */
  CONTROL_NAMETYPE_GEOMETRY,         /*!< Name specifies a geometry */
  CONTROL_NAMETYPE_VARIABLE_NAME,    /*!< Name specifies a variable in grid */
  CONTROL_NAMETYPE_FIELD_VARIABLE,   /*!< Name specifies a field variable */
  CONTROL_NAMETYPE_LOGICAL_VARIABLE, /*!< Name specifies a logical variable */
};
typedef enum control_nametype control_nametype;

#define CONTROL_KEYCHAR_GRID ":"    /*!< Prefix char for grid */
#define CONTROL_KEYCHAR_MASK "@"    /*!< Prefix char for masks */
#define CONTROL_KEYCHAR_GEOM "#"    /*!< Prefix char for geometry */
#define CONTROL_KEYCHAR_VARNAME "*" /*!< Prefix char for variable names */
#define CONTROL_KEYCHAR_FVAR "$"    /*!< Prefix char for field variables */
#define CONTROL_KEYCHAR_LVAR "?"    /*!< Prefix char for logical vaiables */

#define CONTROL_KEYNAME_MAKE(x,name) (CONTROL_KEYCHAR_##x name)
#define CONTROL_KEYNAME_GRID(name) CONTROL_KEYNAME_MAKE(GRID, name)
#define CONTROL_KEYNAME_MASK(name) CONTROL_KEYNAME_MAKE(MASK, name)
#define CONTROL_KEYNAME_VARNAME(name) CONTROL_KEYNAME_MAKE(VARNAME, name)
#define CONTROL_KEYNAME_FVAR(name) CONTROL_KEYNAME_MAKE(FVAR, name)
#define CONTROL_KEYNAME_LVAR(name) CONTROL_KEYNAME_MAKE(LVAR, name)

/*
 * YSE: Adding Doxygen header at bottom, because its own heading is
 *      present at the head.
 */
/**
 * @file struct.h
 * @brief JUPITER data structure definitions.
 */

#ifdef __cplusplus
}
#endif

#endif
