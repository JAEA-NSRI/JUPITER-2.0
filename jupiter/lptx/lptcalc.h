#ifndef JUPITER_LPTX_LPTCALC_H
#define JUPITER_LPTX_LPTCALC_H

#include "defs.h"

JUPITER_LPTX_DECL_START

/**
 * @brief Perform Single time integration
 * @param param particle parameters
 * @param integrate_data Integration data
 */
JUPITER_LPTX_DECL
void LPTX_single_time_integrate(
  LPTX_param *param, const LPTX_single_time_integrate_data *integrate_data);

/**
 * @p func should contain (but can be skipped for your choice):
 *
 *  1. Locate fluid cells for each particle
 *  2. Update density, viscosity and velocity of fluid for each particle
 *  3. Call LPTX_single_time_integrate().
 *  4. Redistribute particles (MPI processes)
 *
 * Number of calls of @p func will be following:
 *
 * - Second-order Adams-Bashforth:
 *   * If @p stepno is 1, loops @p number_of_substep + 1 times.
 *   * Otherwise, loops @p number_of_substep times.
 * - Second-order Runge-Kutta:
 *   * Loops 2 * @p number_of_substep times.
 * - Third-order low-storage Runge-Kutta:
 *   * Loops 3 * @p number_of_substep times.
 *
 * You can abort the loop with returning LPTX_false (as failure) from @p func.
 *
 * You should pass values passed to LPTX_single_time_integrate() in @p func
 * as-is, but we allows to modify them ... if you know what you are doing.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_substep_loop_n(LPTX_param *param, LPTX_idtype stepno,
                              LPTX_idtype number_of_substeps,
                              LPTX_type current_time, LPTX_type time_step_width,
                              LPTX_cb_substep_loop *func, void *args);

/**
 * This function is same as calling LPTX_substep_loop_n() with number of substep
 * in @p param.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_substep_loop_f(LPTX_param *param, LPTX_idtype stepno,
                              LPTX_type current_time, LPTX_type time_step_width,
                              LPTX_cb_substep_loop *func, void *args);

/**
 * @brief Update number of required substeps
 * @param param Parameter to update
 * @param time_step_width Time step width to be applied
 * @return r MPI error value
 * @retval 0 MPI is not enabled (always succeeds)
 * @retval MPI_SUCCESS No error
 * @retval MPI_ERR_COMM Communicator is not set.
 *
 * Update number of substep according current fluid and particle properties,
 * e.g., fluid velocity, density and viscosity.
 *
 * This function is MPI-collective. This function assumes that all processes
 * pass same @p time_step_width.
 */
JUPITER_LPTX_DECL
int LPTX_update_number_of_substep(LPTX_param *param, LPTX_type time_step_width);

/**
 * @brief Calculate collision (interface for any models)
 * @param param Parameter to update
 * @param func Collision calculation function
 * @param arg Argument for @p func
 * @return Number of calculated collisions
 * @retval -1 (Internal) memory allocation failed
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_calc_collision(LPTX_param *param, LPTX_cb_collision_func *func,
                                void *arg);

JUPITER_LPTX_DECL_END

#endif
