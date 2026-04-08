#ifndef JUPITER_OXIDATION_DEFS_H
#define JUPITER_OXIDATION_DEFS_H

#include "component_info_frac_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Oxidation component information
 */
struct ox_component_info
{
  struct component_info_frac comps;
};

//----

/**
 * @brief Oxidation status flags
 */
enum ox_state_flag
{
  OX_STATE_UNCHANGED = -1,    ///< Unchanged (only allowed for the return
                              ///< value from a function)
  OX_STATE_GAS = 0,           ///< Gas region (used in Hydrogen generation)
  OX_STATE_OUT_OF_BOUNDS = 1, ///< Outside of oxidation bounds
  OX_STATE_IN_BOUNDS = 2,     ///< Inside of oxidation bounds
  OX_STATE_STARTED = 3,       ///< Oxidation started
  OX_STATE_FINISHED = 4,      ///< Oxidation finished
  OX_STATE_RECESSING = 5,     ///< Oxidation recessing
};

enum ox_reaction_rate_model
{
  OX_RRMODEL_INVALID = -1, ///< Invalid model
  OX_RRMODEL_TEMPDEP = 0,  ///< Use tempdep_calc()

  OX_RRMODEL_URBANIC_HEIDRICK,  ///< Urbanic-Heidrick
  OX_RRMODEL_BAKER_JUST,        ///< Baker-Just
  OX_RRMODEL_CATHCART_PAWEL,    ///< Cathcart-Pawel
  OX_RRMODEL_LEISTIKOW_SCHANZ,  ///< Leistikow-Schanz
  OX_RRMODEL_PRATER_COURTRIGHT, ///< Prater-Cortright
};

#ifdef __cplusplus
}
#endif

#endif
