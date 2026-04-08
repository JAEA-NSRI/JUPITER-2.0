#ifndef JUPITER_DCCALC_DEFS_H
#define JUPITER_DCCALC_DEFS_H

#ifndef DCCALC_STANDALONE
#include "tempdep_properties.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum dc_func2_model
{
#ifndef DCCALC_STANDALONE
  DC_FUNCS_TEMPDEP_PROPERTY = -1, /*!< Use tempdep_property */
#endif
  DC_FUNCS_INVALID = 0,     /*!< Invalid function */
  DC_FUNCS_SUS304_B4C,      /*!< SUS304 in B4C */
  DC_FUNCS_B4C_SUS304,      /*!< B4C in SUS304 */
  DC_FUNCS_SUS304_ZIRCALOY, /*!< SUS304 in Zircaloy */
  DC_FUNCS_ZIRCALOY_SUS304, /*!< Zircaloy in SUS304 */
  DC_FUNCS_B4C_ZIRCALOY,    /*!< B4C in Zircaloy */
  DC_FUNCS_ZIRCALOY_B4C,    /*!< Zircaloy in B4C */
  DC_FUNCS_UO2_ZIRCALOY,    /*!< UO2 in Zircaloy */
  DC_FUNCS_ZIRCALOY_UO2,    /*!< Zircaloy in UO2 */
};
typedef enum dc_func2_model dc_func2_model;

/**
 * @brief parameter structure for compute solute diffusivity
 */
struct dc_calc_param
{
  dc_func2_model model; /*!< model function */
#ifndef DCCALC_STANDALONE
  tempdep_property prop; /*!< tempdep property */
#endif
};

#ifdef __cplusplus
}
#endif

#endif
