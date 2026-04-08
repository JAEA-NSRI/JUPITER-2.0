#ifndef CSVUTIL_EXTRA_H_
#define CSVUTIL_EXTRA_H_

#include "common.h"
#include "component_data_defs.h"
#include "struct.h"
#include "geometry/defs.h"
#include "tmcalc.h"
#include "dccalc.h"
#include "tempdep_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

enum jupiter_geom_key
{
  JUPITER_GEOM_KEY_INVALID,     /*!< None of followings */
  JUPITER_GEOM_KEY_FILE,        /*!< Geom_file */
  JUPITER_GEOM_KEY_NAME,        /*!< Geom_name */
  JUPITER_GEOM_KEY_SIZE,        /*!< Geom_size */
  JUPITER_GEOM_KEY_REPEAT,      /*!< Geom_repeat */
  JUPITER_GEOM_KEY_OFFSET,      /*!< Geom_offset */
  JUPITER_GEOM_KEY_ORIGIN,      /*!< Geom_origin */
  JUPITER_GEOM_KEY_BOUNDARY,    /*!< Geom_boundary */
  JUPITER_GEOM_KEY_TBOUNDARY,   /*!< Geom_tboundary */
  JUPITER_GEOM_KEY_SURFACE_BOUNDARY, /*!< Geom_surface_boundary */
  JUPITER_GEOM_KEY_SHAPE,       /*!< Geom_shape */
  JUPITER_GEOM_KEY_SURFACE_SHAPE, /*!< Geom_surface_shape */
  JUPITER_GEOM_KEY_NUM_SUBCELL, /*!< Geom_num_subcell */
  JUPITER_GEOM_KEY_VOF,         /*!< Geom_vof */
  JUPITER_GEOM_KEY_MATERIAL,    /*!< Geom_material (Historically used) */
  JUPITER_GEOM_KEY_VELOCITY_U,  /*!< Geom_velocity_u */
  JUPITER_GEOM_KEY_VELOCITY_V,  /*!< Geom_velocity_v */
  JUPITER_GEOM_KEY_VELOCITY_W,  /*!< Geom_velocity_w */
  JUPITER_GEOM_KEY_VELOCITY,    /*!< Geom_velocity (Historically used) */
  JUPITER_GEOM_KEY_TEMPERATURE, /*!< Geom_temperature */
  JUPITER_GEOM_KEY_PRESSURE,    /*!< Geom_pressure */
  JUPITER_GEOM_KEY_FIXED_HSOURCE, /*!< Geom_fixed_heat_source */
  JUPITER_GEOM_KEY_LPT_WALLREF_BN, /*!< Geom_LPT_boundary_wall_reflection */
  JUPITER_GEOM_KEY_LPT_WALLREF_IN, /*!< Geom_LPT_wall_reflection */
  JUPITER_GEOM_KEY_DUMP,        /*!< Geom_dump */
};
typedef enum jupiter_geom_key jupiter_geom_key;

JUPITER_DECL
int str_to_bool(const char *p);
JUPITER_DECL
int str_to_interface_capturing_scheme(const char *p);
JUPITER_DECL
int str_to_boundary(const char *p);
JUPITER_DECL
int str_to_tboundary(const char *p);
JUPITER_DECL
out_p_cond str_to_out_p_cond(const char *p);
JUPITER_DECL
geom_vof_phase str_to_vof_phase(const char *p);
JUPITER_DECL
geom_data_operator str_to_geom_data_op(const char *p);
JUPITER_DECL
geom_shape_operator str_to_geom_shape_op(const char *p);
JUPITER_DECL
geom_shape str_to_geom_shape(const char *p);
JUPITER_DECL
geom_surface_shape str_to_geom_surface_shape(const char *p);
JUPITER_DECL
geom_init_func str_to_init_func(const char *p);
JUPITER_DECL
trip_control str_to_trip_control(const char *p);
JUPITER_DECL
boundary_direction str_to_boundary_dir(const char *p);
JUPITER_DECL
surface_inlet_dir str_to_inlet_dir(const char *p);
JUPITER_DECL
tm_func2_model str_to_tm_func2_model(const char *p);
JUPITER_DECL
dc_func2_model str_to_dc_func2_model(const char *p);
JUPITER_DECL
enum solid_form str_to_solid_form(const char *p);
JUPITER_DECL
binary_output_mode str_to_binary_output_mode(const char *p);
JUPITER_DECL
tempdep_property_type str_to_tempdep_property_type(const char *p);
JUPITER_DECL
enum ox_reaction_rate_model str_to_ox_kp_model(const char *p);
JUPITER_DECL
component_phase_name str_to_component_phase(const char *p);
JUPITER_DECL
int str_to_LPTts(const char *p);
JUPITER_DECL
int str_to_LPTht(const char *p);
JUPITER_DECL
int str_to_postp(const char *p);
JUPITER_DECL
int str_to_maskp(const char *p);
JUPITER_DECL
int str_to_fieldp(const char *p);
JUPITER_DECL
int str_to_jcntrl_lop(const char *p);
JUPITER_DECL
int str_to_jcntrl_compp(const char *p);
JUPITER_DECL
int str_to_fv_tabbnd(const char *p);
JUPITER_DECL
int str_to_write_fv_csv_mode(const char *p);
JUPITER_DECL
int str_to_write_fv_form(const char *p);
JUPITER_DECL
non_uniform_grid_function str_to_non_uniform_grid_func(const char *p);

JUPITER_DECL
int LPTts_invalid(void);
JUPITER_DECL
int LPTht_invalid(void);

/**
 * @brief Get Geometries' keyname from text.
 * @param text text to determine
 * @return Geometries' keyname, JUPITER_GEOM_KEY_INVALID if not valid
 */
JUPITER_DECL
jupiter_geom_key set_geom_get_keyname(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* CSVUTIL_EXTRA_H_ */
