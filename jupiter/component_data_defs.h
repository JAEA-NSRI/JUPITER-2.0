#ifndef JUPITER_COMPONENT_DATA_DEFS_H
#define JUPITER_COMPONENT_DATA_DEFS_H

#include "csv.h"
#include "geometry/bitarray.h"
#include "geometry/list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Phase names
 */
enum component_phase_name
{
  COMPONENT_PHASE_SOLID = 0,
  COMPONENT_PHASE_LIQUID,
  COMPONENT_PHASE_GAS,

  COMPONENT_PHASE_MAX,
};
typedef enum component_phase_name component_phase_name;

/**
 * @brief Bitwise flags for component_phase_name
 */
struct component_phases
{
  geom_bitarray_n(phases, COMPONENT_PHASE_MAX);
};
typedef struct component_phases component_phases;

/**
 * @brief Component-based variable array nales
 */
enum component_variable_name
{
  COMPONENT_VARIABLE_PHASE_COMP = 0,    /*!< phase_value.comps */
  COMPONENT_VARIABLE_SOLID_VOF,         /*!< variable.fs */
  COMPONENT_VARIABLE_LIQUID_VOF,        /*!< variable.fl */
  COMPONENT_VARIABLE_DELTA_MELT,        /*!< variable.df */
  COMPONENT_VARIABLE_DELTA_SOLID,       /*!< variable.dfs */
  COMPONENT_VARIABLE_VOLUME_FRACTION,   /*!< variable.Vf */
  COMPONENT_VARIABLE_MOLAR_FRACTION,    /*!< variable.Y */
  COMPONENT_VARIABLE_MASS_SOURCE_GAS,   /*!< variable.mass_source_g */
  COMPONENT_VARIABLE_LPT_MASS_FRACTION, /*!< LPTX_VI_MASS_FRACTION */

  COMPONENT_VARIABLE_MAX,
};
typedef enum component_variable_name component_variable_name;

/**
 * @brief Bitwise flags for component_variables
 */
struct component_variables
{
  geom_bitarray_n(variables, COMPONENT_VARIABLE_MAX);
};
typedef struct component_variables component_variables;

enum component_data_jupiter_id
{
  JUPITER_ID_INVALID = -9999, ///< Invalid JUPITER ID. You can assume no
                              ///< component_data exists with this value.
};

/**
 * @brief Component metadata
 *
 * @p comp_index represents array index for `variable.fs`, `variable.fl`,
 * `variable.Vf` and `variable.Y`. Since the array length of these variable may
 * vary, you should use component_data_index() (data of cdomain and flags is
 * required) to get properly (e.g. variable.Y for gas-only components exists,
 * but variable.fs does not).
 *
 * @p phases can only be one of SOLID+LIQUID, LIQUID (for solid_form is NONE) or
 * GAS. (s.a. component_phases_is_valid(). This function just checks validity on
 * JUPITER's structural way. So, it also returns true for SOLID only component,
 * which is safely acceptable)
 */
struct component_data
{
  struct geom_list list;    ///< Linked list
  int comp_index;           ///< Array index for fs, fl, df, dfs, Vf and Y
  int phase_comps_index;    ///< Array index for phase_value.comps
  int mass_source_g_index;  ///< Array index for variable.mass_source_g
  int lpt_mass_fraction_index; ///< Array index for LPTX_VI_MASS_FRACTIONS
  int jupiter_id;          ///< JUPITER ID number (specified by input)
  int generated;           ///< Whether internally generated entry
  component_phases phases; ///< Existing phases
  char *fname;             ///< Property input file name (if applicable)
  csv_data *csv;           ///< Property input data (if applicable)
};
typedef struct component_data component_data;
#define component_data_entry(ptr) \
  geom_list_entry(ptr, struct component_data, list)

#ifdef __cplusplus
}
#endif

#endif
