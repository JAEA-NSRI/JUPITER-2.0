/**
 * @file tempdep_properties.h
 * @brief Temperature dependent property definitions
 */

#ifndef TEMPDEP_PROPERTIES_H
#define TEMPDEP_PROPERTIES_H

#include "common.h"
#include "table/table.h"
#include "geometry/defs.h"
#include "csv.h"

#ifdef __cplusplus
extern "C" {
#endif

enum tempdep_property_type
{
  TEMPDEP_PROPERTY_INVALID, ///< Invalid Property type
  TEMPDEP_PROPERTY_CONST,   ///< Constant property
  TEMPDEP_PROPERTY_TABLE,   ///< Table-based property
  TEMPDEP_PROPERTY_POLY,    ///< Polynomial property
  TEMPDEP_PROPERTY_POLY_L,  ///< Polynomial (L is from Laurent, allows negative
                            ///< power) property.
  TEMPDEP_PROPERTY_ARRHENIUS, ///< Arrhenius equation
  TEMPDEP_PROPERTY_PIECEWISE, ///< Piecewise function (of any other function)

  /* Oxidation only types */
  TEMPDEP_PROPERTY_OX_URBANIC_HEIDRICK,  ///< Oxidation only: Urbanic-Heidrick
  TEMPDEP_PROPERTY_OX_BAKER_JUST,        ///< Oxidation only: Baker-Just
  TEMPDEP_PROPERTY_OX_CATHCART_PAWEL,    ///< Oxidation only: Cathcart-Pawel
  TEMPDEP_PROPERTY_OX_LEISTIKOW_SCHANZ,  ///< Oxidation only: Leistikow-Schanz
  TEMPDEP_PROPERTY_OX_PRATER_COURTRIGHT, ///< Oxidation only: Prater-Courtright

  TEMPDEP_PROPERTY_OX_RECESSION, ///< Oxidation only: Predefined experimental
                                 ///< recession rate model
};
typedef enum tempdep_property_type tempdep_property_type;

struct tempdep_property;
typedef struct tempdep_property tempdep_property;

/**
 * Type value for `tempdep_property_type` in geom_variant.
 *
 * This is local use. It may collide with the value in different contexts.
 */
#define TEMPDEP_VARTYPE_ENUM_TYPE (GEOM_VARTYPE_ENUMTYPE_MIN + 0)

/**
 * @brief Tempeature domain data
 */
struct tempdep_domain_data
{
  double minT; ///< Minimum applicable temperature
  double maxT; ///< Maximum applicable temperature
};

/**
 * @brief Temperature dependency with a constant value
 */
struct tempdep_const_data
{
  double value; ///< Contant value
};

/**
 * @brief Temperature dependency with a table
 */
struct tempdep_table_data
{
  table_data *data; ///< Table data (Only X-data is used currently)
};

/**
 * @brief Temperature dependency with Polynomial
 *
 * \f[
 *    a_{n} T^n + a_{n-1} T^{n-1} + \cdots + a_1 T + a_0
 * \f]
 */
struct tempdep_poly_data
{
  struct tempdep_domain_data dom; ///< Applicable domain
  int polymax;                    ///< Maximum power of polynomial
  double *vec; ///< Array of coefficients (in order of polymax to 0)
};

/**
 * @brief Temperature dependency with Polynomial (allows negative degrees)
 *
 * \f[
 *    a_{n} T^n + a_{n-1} T^{n-1} + \cdots + a_1 T + a_0 + a_{-1} T^{-1}
 *    + \cdots + a_{m+1} T^{m+1} + a_{m} T^{m}
 * \f]
 *
 * Negative degrees may not be present always (therefore, allows \f$ n
 * > 0 \f$ and \f$ m \ge 0 \f$). The implementation is slightly complex
 * than ::tempdep_poly_data.
 */
struct tempdep_poly_l_data
{
  struct tempdep_domain_data dom; ///< Applicable domain
  int polymax;                    ///< Maximum power of polynomial
  int polymin;                    ///< Minimum power of polynomial
  double *vec; ///< Array of coefficients (in order of polymax to polymin)
};

/**
 * @brief Temperature dependency with the Arrhenius equation(-like)
 *
 * \f[
 *    A \exp\left(\frac{-E}{T}\right)
 * \f]
 *
 * Coefficient \f$ E \f$ can be named as the activation energy divided
 * by universal gas constant or Boltzmann constant. But in this
 * context, this parameter has no explicit meaning, because we just
 * provide an equation which is same form to the Arrhenius equation.
 */
struct tempdep_arrhenius_data
{
  struct tempdep_domain_data dom; ///< Applicable domain
  double coeff_A;                 ///< Coefficient A (pre-exponential factor)
  double coeff_E;                 ///< Coefficient E
};

/**
 * @brief Temperature dependency with Piecewised function (of any type)
 */
struct tempdep_piecewise_data
{
  int npieces;            ///< Number of pieces
  double *domain;         ///< Temperature domain boundaries in ascending order
  tempdep_property *func; ///< Function of each pieces
};

/**
 * @brief Union of all temperature dependency data
 */
union tempdep_property_u
{
  struct tempdep_const_data t_const;
  struct tempdep_table_data t_table;
  struct tempdep_poly_data t_poly;
  struct tempdep_poly_l_data t_poly_l;
  struct tempdep_arrhenius_data t_arrhenius;
  struct tempdep_piecewise_data t_piecewise;
};

/**
 * @brief Temparature dependency data
 *
 * If you want to make some property as temperature dependent, you may
 * modify its type (float or double) into this ::tempdep_property and
 * call tempdep_calc() when reference it.
 */
struct tempdep_property
{
  tempdep_property_type type;
  union tempdep_property_u data;
};

/**
 * @memberof tempdep_property
 * @brief Initialize temperature property data.
 * @param prop Property data to init.
 */
JUPITER_DECL
void tempdep_property_init(tempdep_property *prop);

/**
 * @memberof tempdep_property
 * @brief Set temperature data from CSV data.
 * @param prop Property data to set for.
 * @param type Type (function for property)
 * @param csv CSV data
 * @param fname Filename of CSV data.
 * @param found_row The row contains @p start.
 * @param start Start column position to read in, and sets final position.
 * @return 0 if success, non-0 if failed.
 */
JUPITER_DECL
int tempdep_property_set(tempdep_property *prop, tempdep_property_type type,
                         csv_data *csv, const char *fname, csv_row *found_row,
                         csv_column **start);

/**
 * @memberof tempdep_property
 * @brief Cleanup property data
 * @param prop Property data
 */
JUPITER_DECL
void tempdep_property_clean(tempdep_property *prop);

/**
 * @memberof tempdep_property
 * @brief Create information map of temperature dependent property.
 * @param prop Property data
 * @param err Set error if occured.
 * @return generated map
 *
 * In this (tempdep module) context, unit "I" will be the unit of the
 * property (ex. kg/m3 for density), and unit "L" will be the unit of
 * temperature (ex. K if it's kelvin).
 *
 * Following user-defined enum types may be used:
 * - `TEMPDEP_VARTYPE_ENUM_TYPE` for `enum tempdep_property_type`
 */
JUPITER_DECL
geom_info_map *tempdep_property_create_info_map(tempdep_property *prop,
                                                geom_error *err);

#ifdef __cplusplus
}
#endif

#endif
