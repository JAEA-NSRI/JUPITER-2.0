
#ifndef JUPITER_CXX_BIND_H
#define JUPITER_CXX_BIND_H

#include <jupiter/optparse.h>
#include <jupiter/struct.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_VTKIOBIND_EXPORT)
#define JUPITER_VTKIOBIND_DECL __declspec(dllexport)
#define JUPITER_VTKIOBIND_DECL_PRIVATE
#elif defined(JUPITER_VTKIOBIND_IMPORT)
#define JUPITER_VTKIOBIND_DECL __declspec(dllimport)
#define JUPITER_VTKIOBIND_DECL_PRIVATE
#else
#define JUPITER_VTKIOBIND_DECL
#define JUPITER_VTKIOBIND_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_VTKIOBIND_EXPORT) || defined(JUPITER_VTKIOBIND_IMPORT)
#define JUPITER_VTKIOBIND_DECL __attribute__((visibility("default")))
#define JUPITER_VTKIOBIND_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_VTKIOBIND_DECL
#define JUPITER_VTKIOBIND_DECL_PRIVATE
#endif
#else
#define JUPITER_VTKIOBIND_DECL
#define JUPITER_VTKIOBIND_DECL_PRIVATE
#endif

enum jupiter_bind_data_type
{
  JUPITER_BIND_TYPE_INT = 1,
  JUPITER_BIND_TYPE_FLOAT = 2,
  JUPITER_BIND_TYPE_DOUBLE = 3,
  JUPITER_BIND_TYPE_CHAR = 4,
};
typedef enum jupiter_bind_data_type jupiter_bind_data_type;

JUPITER_VTKIOBIND_DECL
parameter *set_parameters_file(const char *param_file, const char *flags_file,
                               const char *geom_file, const char *control_file,
                               int *status);

/**
 * Allocate @p len size of @p type.
 */
typedef void *allocate_array_func(size_t len, jupiter_bind_data_type type,
                                  void *arg);

/**
 * @param descriptive_name Descriptive name of the variable
 * @param unit_name Unit name of the variable (e.g., m/s, K, ...)
 * @param raw_name Raw variable name (e.g., fs, fl, t, ...)
 * @param soa_icompo Component index which is stored in SOA basis,
 *        such as fs or fl. If no multi-component variable, it will passes -1.
 * @param val Array data
 * @param ntuple Number of tuples
 * @param unit_size Number of elements per tuple (aka. number of components)
 * @param type Array data type of @p val
 * @param component_names List of component names for each element in tuple, if
 *        available.
 *
 * Unit name is not supported yet (always passes NULL and included in
 * descriptive_name).
 */
typedef int add_attribute_func(const char *descriptive_name,
                               const char *unit_name, const char *raw_name,
                               int soa_icompo, void *val, size_t ntuple,
                               int unit_size, jupiter_bind_data_type type,
                               const char **component_names, void *arg);

typedef int set_time_func(double time, void *arg);

/**
 * Set coordinate of X, Y or Z axis of rectilinear grid
 */
typedef int set_coordinate_func(void *val, int length,
                                jupiter_bind_data_type type, int alloc,
                                void *arg);

/**
 * Set particle positions
 */
typedef int set_lpt_positions_func(void *val, int npt,
                                   jupiter_bind_data_type type, void *arg);

struct convert_fluid_mesh_funcs
{
  allocate_array_func *allocate_array;
  set_coordinate_func *set_x_coordinate;
  set_coordinate_func *set_y_coordinate;
  set_coordinate_func *set_z_coordinate;
  void *arg;
};
typedef struct convert_fluid_mesh_funcs convert_fluid_mesh_funcs;

struct convert_fluid_funcs
{
  allocate_array_func *allocate_array;
  add_attribute_func *add_attribute;
  set_time_func *set_time;
  set_coordinate_func *set_x_coordinate;
  set_coordinate_func *set_y_coordinate;
  set_coordinate_func *set_z_coordinate;
  void *arg;
};
typedef struct convert_fluid_funcs convert_fluid_funcs;

struct convert_particles_funcs
{
  allocate_array_func *allocate_array;
  add_attribute_func *add_attribute;
  set_time_func *set_time;
  set_lpt_positions_func *set_lpt_positions;
  void *arg;
};
typedef struct convert_particles_funcs convert_particles_funcs;

/**
 * Generate mesh based on input files
 */
JUPITER_VTKIOBIND_DECL
int convert_fluid_mesh(parameter *p, const convert_fluid_mesh_funcs *funcs);

/**
 * Convert and load binary data
 */
JUPITER_VTKIOBIND_DECL
int convert_fluid(parameter *p, int iout, const char *input_dir,
                  const convert_fluid_funcs *funcs);

JUPITER_VTKIOBIND_DECL
int convert_particles(parameter *p, int iout, int npt, const char *input_dir,
                      const convert_particles_funcs *funcs);

JUPITER_VTKIOBIND_DECL
void get_global_size(parameter *p, int size[3]);
JUPITER_VTKIOBIND_DECL
void get_local_size(parameter *p, int size[3]);
JUPITER_VTKIOBIND_DECL
void get_stm_size(parameter *p, int size[3]);
JUPITER_VTKIOBIND_DECL
void get_stp_size(parameter *p, int size[3]);
JUPITER_VTKIOBIND_DECL
void get_global_start(parameter *p, int size[3]);
JUPITER_VTKIOBIND_DECL
int get_mpi_rank(parameter *p);
JUPITER_VTKIOBIND_DECL
int get_mpi_nproc(parameter *p);
JUPITER_VTKIOBIND_DECL
char *get_time_file_pattern(parameter *prm, int is_restart);
JUPITER_VTKIOBIND_DECL
int input_time_file_get_index(parameter *prm, const char *base,
                              const char *file, int is_restart, double *time,
                              int *idx);
JUPITER_VTKIOBIND_DECL
const char *get_input_directory(parameter *prm, int is_restart);

JUPITER_VTKIOBIND_DECL
int get_number_of_particles(parameter *prm, int iout, const char *dir);

JUPITER_VTKIOBIND_DECL
jupiter_options *jupiter_optparse_alloc(int *argc, char ***argv);
JUPITER_VTKIOBIND_DECL
const char *jupiter_opt_flags(jupiter_options *p);
JUPITER_VTKIOBIND_DECL
const char *jupiter_opt_param(jupiter_options *p);
JUPITER_VTKIOBIND_DECL
const char *jupiter_opt_geom(jupiter_options *p);
JUPITER_VTKIOBIND_DECL
const char *jupiter_opt_plist(jupiter_options *p);
JUPITER_VTKIOBIND_DECL
const char *jupiter_opt_control(jupiter_options *p);
JUPITER_VTKIOBIND_DECL
void jupiter_opt_delete(jupiter_options *p);

#ifdef __cplusplus
}
#endif

#endif
