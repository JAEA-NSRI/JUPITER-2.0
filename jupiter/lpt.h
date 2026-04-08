/**
 * @file lpt.h
 * @brief LPT header of LPT utility
 */

#ifndef JUPITER_LPT_H
#define JUPITER_LPT_H

#include "common.h"
#ifdef LPTX
#include "lptx/defs.h"
#endif

#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LPTX
enum jupiter_lptx_origin_id
{
  JUPITER_LPTX_ORIGIN_UNDEFINED = LPTX_ORIGIN_UNDEFINED,
  JUPITER_LPTX_ORIGIN_INJECT = 1, ///< Manually injected
  JUPITER_LPTX_ORIGIN_AEROSOL_CONDENSATION = 2, ///< By aerosol condensation
};
#endif

/**
 * @brief The data written
 */
struct jupiter_lpt_ctrl_data
{
  int npt;     ///< Number of particles
  type timprn; ///< Next printing timing [s]
};
typedef struct jupiter_lpt_ctrl_data jupiter_lpt_ctrl_data;

JUPITER_DECL
jupiter_lpt_ctrl_data *new_lpt_ctrl_data(void);

/**
 * @brief Write LPT control data to given file path.
 * @param path filename to write to
 * @param data data to write
 * @retval 0 success
 * @retval 1 pack failure
 * @retval 2 file error
 *
 * Writes LPT control metadata to given path. This function is not
 * MPI-collective and is thread safe. However, the result file may be
 * corrupted if multiple MPI processes or threads write to the same
 * file at once.
 */
JUPITER_DECL
int write_lpt_ctrl_data(const char *path, const jupiter_lpt_ctrl_data *data);

/**
 * @brief Read LPT control data from given file path.
 * @param path filename to read from
 * @param data Pointer to data
 * @retval 0 success
 * @retval 1 pack failure
 * @retval 2 file error
 * @retval 3 memory allocation failure (for @p data)
 *
 * Reads LPT control metadata from specified path. This function is
 * not MPI-collective, and is thread safe.
 *
 * LPT control metadata has version signature. This function will fail
 * if it does not match or does not supported.
 *
 * @p data will be allocated for you. Use delete_lpt_ctrl_data() to
 * deallocate it.
 *
 * If error occured, the content of @p data is undefined.
 */
JUPITER_DECL
int read_lpt_ctrl_data(const char *path, jupiter_lpt_ctrl_data **data);

/**
 * @brief Get LPT control data filled with the content holded by LPT module
 * @return The data got, NULL if allocation failed.
 */
JUPITER_DECL
jupiter_lpt_ctrl_data *get_lpt_ctrl_data(void);

/**
 * @brief Set the content of LPT data to LPT module
 * @param data Data to set
 *
 * This function is *not* thread safe, because corresponding variables
 * of LPT module has global scope.
 *
 * Setting number of particles will be skipped. It can only be set by
 * the number of particle sets (i.e. npset).
 */
JUPITER_DECL
void set_lpt_ctrl_data(jupiter_lpt_ctrl_data *data);

/**
 * @brief Delete LPT control data
 */
JUPITER_DECL
void delete_lpt_ctrl_data(jupiter_lpt_ctrl_data *data);

#ifdef LPTX
/**
 * Set parameters for LPTX_param from JUPITER parameters
 */
JUPITER_DECL
int jLPTX_param_set(LPTX_param *lpt_param, flags *flg, domain *cdo,
                    mpi_param *mpi);

/**
 * Allocate and set flags parameters and return it
 *
 * This function is for testing variable requirements (LPTX_param_want_*()).
 */
JUPITER_DECL
LPTX_param *jLPTX_param_for_flg(flags *flg);
#endif

#ifdef __cplusplus
}
#endif

#endif
