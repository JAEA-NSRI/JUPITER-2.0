#ifndef JUPITER_TMCALC_DEFS_H
#define JUPITER_TMCALC_DEFS_H

#include "component_info_defs.h"
#include "table/table.h"

#ifdef __cplusplus
extern "C" {
#endif

enum tm_func2_model
{
  TM_FUNC_INVALID = -1,
  TM_FUNC_LIQUIDUS_FE_ZR = 0,  /*!< Fe-Zr linear approximated function: tm_calc_Fe_Zr_liq */
  TM_FUNC_LIQUIDUS_FE_B,       /*!< Fe-B linear approximated function: tm_calc_Fe_B_liq */
};
typedef enum tm_func2_model tm_func2_model;

/**
 * @brief Liquidus or Solidus Table organizing data
 *
 * @p yid.d can be NULL. Then, table is assumed to be binary.
 */
struct tm_table_param
{
  table_data *table;              /*!< table data */
  char *table_file;               /*!< file name of table data */
  struct component_info_data xid; /*!< Material ID for X */
  struct component_info_data yid; /*!< Material ID for Y */
  struct component_info_data rid; /*!< Material ID for Ramainder */
  struct tm_table_param *next;
};

/**
 * @brief Liquidus or Solidus function parameters
 *
 * @p yid.d can be NULL. Then, complement is treated as sum of others
 */
struct tm_func2_param
{
  tm_func2_model model;           /*!< function to be used */
  struct component_info_data xid; /*!< Material ID as input */
  struct component_info_data yid; /*!< Material ID for complement */
  struct tm_func2_param *next;
};

#ifdef __cplusplus
}
#endif

#endif
