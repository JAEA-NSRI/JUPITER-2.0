/**
 * @file lptx/param.h
 */

#ifndef JUPITER_LPTX_PARAM_H
#define JUPITER_LPTX_PARAM_H

#include "defs.h"
#include "vector.h"

#include <jupiter/random/random.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

JUPITER_LPTX_DECL_START

/**
 * The default value of coefficient A1 of Cunningham correction formula:
 *
 * \[
 *  C_c = 1 + \frac{2\lambda}{d_p}
 *   \left(A_1 + A_2 \exp\left(-A_3 \frac{d_p}{2 \lambda}\right)\right)
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_cunningham_correction_A1(void);

/**
 * The default value of coefficient A2 of Cunningham correction formula
 *
 * \[
 *  C_c = 1 + \frac{2\lambda}{d_p}
 *   \left(A_1 + A_2 \exp\left(-A_3 \frac{d_p}{2 \lambda}\right)\right)
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_cunningham_correction_A2(void);

/**
 * The default value of coefficient A3 of Cunningham correction formula
 *
 * \[
 *  C_c = 1 + \frac{2\lambda}{d_p}
 *   \left(A_1 + A_2 \exp\left(-A_3 \frac{d_p}{2 \lambda}\right)\right)
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_cunningham_correction_A3(void);

/**
 * The default value of coefficient Cs of thermophoretic force formula
 *
 * \[
 *  D_{T,p} = \frac{6\pi d_p \mu^2 C_s \left(K + C_t Kn\right)}{
 *                  \rho\left(1+3 C_m Kn\right)\left(1+2K+2C_t Kn\right)}
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_thermophoretic_constant_Cs(void);

/**
 * The default value of coefficient Cm of thermophoretic force formula
 *
 * \[
 *  D_{T,p} = \frac{6\pi d_p \mu^2 C_s \left(K + C_t Kn\right)}{
 *                  \rho\left(1+3 C_m Kn\right)\left(1+2K+2C_t Kn\right)}
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_thermophoretic_constant_Cm(void);

/**
 * The default value of coefficient Ct of thermophoretic force formula
 *
 * \[
 *  D_{T,p} = \frac{6\pi d_p \mu^2 C_s \left(K + C_t Kn\right)}{
 *                  \rho\left(1+3 C_m Kn\right)\left(1+2K+2C_t Kn\right)}
 * \]
 */
JUPITER_LPTX_DECL
LPTX_type LPTX_thermophoretic_constant_Ct(void);

/**
 * @brief Allocate new LPT parameter set
 * @retval NULL allocation failed
 */
JUPITER_LPTX_DECL
LPTX_param *LPTX_param_new(void);

JUPITER_LPTX_DECL
void LPTX_param_delete(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_delete_all_init_sets(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_delete_all_particles(LPTX_param *param);

/**
 * @brief Join particle sets into single particle set
 * @return Joined particle set
 * @retval NULL Allocation failed
 *
 * Joins particle sets into single particle set. Returning particle set will not
 * be a member of @p param and particle sets in @p param is not modified. If you
 * want to replace, you may want to inplace version.
 *
 * If no particle sets exist in @p param, new particle set with 0 particles will
 * be returned.
 *
 * If only one particle set exist in @p param, this function returns it instead
 * of allocating new one. You can check this by `LPTX_particle_set_param() ==
 * param`.
 *
 * This function just concatenate particle sets. This function will never strip
 * particles, and resulting particle set still includes unused particle entry.
 */
JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_param_join_particle_sets(LPTX_param *param);

/**
 * @brief Join particle sets into single particle set inplace
 * @return Joined particle set (invalid particle set if 0 particles in it)
 * @retval NULL Allocation failed
 *
 * Joins particle set into single particle set. If successfully joined, the
 * particle set in @p param will be replaced by new one.
 *
 * If no particle set exists in @p param, this function returns a pointer to
 * fake particle set that evaluates to LPTX_particle_set_number_of_particle() to
 * be 0. Passing returned pointer to another functions causes unexpected
 * behavior.
 *
 * If only one particle set exists in @p param, this function does nothing and
 * returns it.
 *
 * This function just concatenate particle sets. This function will never strip
 * particles, and resulting particle set still includes unused particle entry.
 *
 */
JUPITER_LPTX_DECL
const LPTX_particle_set *
LPTX_param_join_particle_sets_inplace(LPTX_param *param);

/**
 * @brief Sets number of vectors when allocating new particle set
 * @param number_of_vectors Number of vectors per particle
 * @param numbers_of_data Array sizes per particle for each vector
 * @retval LPTX_true Successfully set
 * @retval LPTX_false Allocation failed
 *
 * When NULL has been passed for @p nubmers_of_data, this function
 * fills vector sizes with 0.
 *
 * @warning Changing number of vectors or array sizes for each vector
 * does not affects already allocated particle sets.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_set_particle_vectors(LPTX_param *param,
                                          LPTX_idtype number_of_vectors,
                                          const LPTX_idtype *numbers_of_data);

/**
 * @brief Get number of vectors for allocating new particle set
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_number_of_particle_vectors(LPTX_param *param);

/**
 * @brief Get size of each vectors for allocating new particle set
 */
JUPITER_LPTX_DECL
const LPTX_idtype *
LPTX_param_get_number_of_particle_vector_sizes(LPTX_param *param);

/**
 * @brief Get size of specified vector for allocating new particle set
 */
JUPITER_LPTX_DECL
LPTX_idtype
LPTX_param_get_number_of_particle_vector_size(LPTX_param *param,
                                              LPTX_idtype vector_index);

/**
 * @brief Set size of specified vector when allocating new particle set
 */
JUPITER_LPTX_DECL
void LPTX_param_set_number_of_particle_vector_size(LPTX_param *param,
                                                   LPTX_idtype vector_index,
                                                   LPTX_idtype vector_size);

/**
 * @brief Create new particle set using param's vector configuration
 * @param param LPTX parameter data to refer
 * @param number_of_particles Number of particles to allocate
 * @return Allocated particle set
 */
JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_param_new_particle_set(LPTX_param *param,
                                               LPTX_idtype number_of_particles);

/**
 * @brief Get number of particle vectors for each particle sets.
 *
 * @note If the number of particle vectors differs for each sets, this function
 *       returns their maximum.
 *
 * @note This function returns 0 if no particle set allocated in the @p param.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_allocated_particle_vectors(LPTX_param *param);

/**
 * @brief Get the sizes of each particle vectors for each particle sets.
 *
 * @note If the size of particle vectors differs for each sets at corresponding
 *       indices, this function returns their maximum.
 *
 * @p out requires array of size of LPTX_param_get_allocated_particle_vectors().
 * This computes size for first @p nout vectors.
 */
JUPITER_LPTX_DECL
void LPTX_param_get_allocated_particle_vector_sizes(LPTX_idtype *out,
                                                    LPTX_idtype nout,
                                                    LPTX_param *param);
/**
 * @brief Get the size of the vector at specified index for each particle sets
 *
 * @note If the size of particle vectors differs for each sets at given
 *       index, this function returns their maximum.
 */
JUPITER_LPTX_DECL
LPTX_idtype
LPTX_param_get_allocated_particle_vector_size(LPTX_param *param,
                                              LPTX_idtype vector_index);

/**
 * @brief Get number of particle vectors for each particle sets.
 *
 * @note If the number of particle vectors differs for each sets, this function
 *       returns their maximum.
 *
 * @note This function returns 0 if no particle set counted.
 *
 * @note This function returns -1 if communication error occured.
 *
 * Same to LPTX_param_get_allocated_particle_vectors(), except to
 * collect for the return value the communicator set in @p param.
 * Thus, this function is MPI collective.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_global_particle_vectors(LPTX_param *param, int *r);

/**
 * @brief Get the sizes of each particle vectors for each particle sets.
 * @return MPI error if any, MPI_SUCCESS on no error. 0 if MPI is not enabled
 *
 * @note If the size of particle vectors differs for each sets at corresponding
 *       indices, this function returns their maximum.
 *
 * @p out should be array of size of LPTX_param_get_global_particle_vectors().
 * This computes size for first @p nout vectors. @p nout must be equal on each
 * ranks.
 *
 * Same to LPTX_param_get_allocated_particle_vector_sizes(), except to
 * collect for the return value the communicator set in @p param.
 * Thus, this function is MPI collective.
 */
JUPITER_LPTX_DECL
int LPTX_param_get_global_particle_vector_sizes(LPTX_idtype *out, int nout,
                                                LPTX_param *param);
/**
 * @brief Get the size of the vector at specified index for each particle sets.
 *
 * @note If the size of particle vectors differs for each sets at given
 *       index, this function returns their maximum.
 *
 * @note This function returns -1 if communication error occured.
 *
 * Same to LPTX_param_get_allocated_particle_vector_sizes(), except to
 * collect for the return value the communicator set in @p param.
 * Thus, this function is MPI collective.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_global_particle_vector_size(LPTX_param *param,
                                                       LPTX_idtype vector_index,
                                                       int *r);

JUPITER_LPTX_DECL
void LPTX_param_set_seed(LPTX_param *param, jupiter_random_seed seed);

JUPITER_LPTX_DECL
void LPTX_param_set_random_seed(LPTX_param *param);

JUPITER_LPTX_DECL
jupiter_random_seed LPTX_param_get_seed(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_time_scheme(LPTX_param *param, LPTX_time_scheme scheme);

JUPITER_LPTX_DECL
LPTX_time_scheme LPTX_param_time_scheme(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_heat_scheme(LPTX_param *param, LPTX_heat_scheme scheme);

JUPITER_LPTX_DECL
LPTX_heat_scheme LPTX_param_heat_scheme(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_gravity(LPTX_param *param, LPTX_vector gravity);

JUPITER_LPTX_DECL
LPTX_vector LPTX_param_gravity(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_brownian_force(LPTX_param *param, LPTX_bool f);

JUPITER_LPTX_DECL
LPTX_bool LPTX_param_brownian_force(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_thermophoretic_force(LPTX_param *param, LPTX_bool f);

JUPITER_LPTX_DECL
LPTX_bool LPTX_param_thermophoretic_force(LPTX_param *param);

/**
 * @brief Returns true if any external force (excluding gravity) models
 * are enabled.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_external_force(LPTX_param *param);

/**
 * @brief Returns true if heat exchange is enabled
 *
 * This function is semantically equivalent to `LPTX_param_heat_scheme(param) !=
 * LPTX_HEAT_OFF`, but will return false for invalid enum values.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_heat_exchange(LPTX_param *param);

/**
 * @brief Returns true if heat exchange is enabled with defining heat transfer
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_heat_exchange_by_htr(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_use_constant_cunningham_correction(LPTX_param *param,
                                                       LPTX_bool f);

JUPITER_LPTX_DECL
LPTX_bool LPTX_param_use_constant_cunningham_correction(LPTX_param *param);

/**
 * Sets constant Stokes-Cunningham correction.
 *
 * @note The drag force model is **not** Stokes-Cunningham law in LPTX.
 */
JUPITER_LPTX_DECL
void LPTX_param_set_cunningham_correction(LPTX_param *param, LPTX_type cc);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_cunningham_correction(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_cunningham_correction_A1(LPTX_param *param,
                                             LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_cunningham_correction_A1(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_cunningham_correction_A2(LPTX_param *param,
                                             LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_cunningham_correction_A2(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_cunningham_correction_A3(LPTX_param *param,
                                             LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_cunningham_correction_A3(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_thermophoretic_force_constant_Cs(LPTX_param *param,
                                                     LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_thermophoretic_force_constant_Cs(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_thermophoretic_force_constant_Cm(LPTX_param *param,
                                                     LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_thermophoretic_force_constant_Cm(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_thermophoretic_force_constant_Ct(LPTX_param *param,
                                                     LPTX_type value);

JUPITER_LPTX_DECL
LPTX_type LPTX_param_thermophoretic_force_constant_Ct(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_number_of_substep(LPTX_param *param,
                                      LPTX_idtype number_of_substep);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_number_of_substep(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_set_max_number_of_substep(LPTX_param *param,
                                          LPTX_idtype max_number_of_substep);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_max_number_of_substep(LPTX_param *param);

/**
 * Returns true if calculation condition requires fluid temperature.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_fluid_temperature(LPTX_param *param);

/**
 * Returns true if calculation condition requires fluid temperature gradient.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_fluid_temperature_grad(LPTX_param *param);

/**
 * Returns true if calculation condition requires fluid specific heat
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_fluid_specific_heat(LPTX_param *param);

/**
 * Returns true if calculation condition requires fluid thermal conductivity
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_fluid_thermal_conductivity(LPTX_param *param);

/**
 * Returns true if calculation condition requires mean free path for fluid
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_mean_free_path(LPTX_param *param);

/**
 * Returns true if calculation condition requires fluid molecular weight
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_fluid_molecular_weight(LPTX_param *param);

/**
 * Returns true if calculation condition requires (initial) particle temperature
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_temperature(LPTX_param *param);

/**
 * Returns true if calculation condition requires particle specific heat
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_specific_heat(LPTX_param *param);

/**
 * Returns true if calculation condition requires particle thermal conductivity
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_param_want_thermal_conductivity(LPTX_param *param);

JUPITER_LPTX_DECL
const LPTX_particle_stat *LPTX_param_get_cumulative_stat(LPTX_param *param);

JUPITER_LPTX_DECL
const LPTX_particle_stat *LPTX_param_get_last_step_stat(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_reset_cumulative_stat(LPTX_param *param);

JUPITER_LPTX_DECL
void LPTX_param_reset_last_stat(LPTX_param *param);

/**
 * Increment tracked count by given value.
 */
JUPITER_LPTX_DECL
void LPTX_param_count_tracked(LPTX_param *param, LPTX_idtype icount);

JUPITER_LPTX_DECL
void LPTX_param_count_exited(LPTX_param *param, LPTX_idtype icount);

JUPITER_LPTX_DECL
void LPTX_param_count_sent(LPTX_param *param, LPTX_idtype icount);

JUPITER_LPTX_DECL
void LPTX_param_count_recved(LPTX_param *param, LPTX_idtype icount);

/**
 * Fill allocated entry for statistics
 *
 * Cumulative statistics entry assigns maximum of each call of this function
 *
 * This function shall not be called within OpenMP parallel region, but it is
 * safe while using atomic write.
 */
JUPITER_LPTX_DECL
void LPTX_param_count_allocated(LPTX_param *param);

#ifdef JUPITER_LPTX_MPI
JUPITER_LPTX_DECL
MPI_Comm LPTX_param_mpi_comm(LPTX_param *param);

/**
 * Communicator will be copied. See MPI_Comm_dup() for returning value.
 */
JUPITER_LPTX_DECL
int LPTX_param_set_mpi_comm(LPTX_param *param, MPI_Comm comm);

JUPITER_LPTX_DECL
int LPTX_param_mpi_errhandler(LPTX_param *param, MPI_Errhandler *handler);

/**
 * LPTX allows any kind of MPI error handler. But some functions may return MPI
 * error value even if MPI_ERRORS_ARE_FATAL is used for the handler.
 */
JUPITER_LPTX_DECL
int LPTX_param_set_mpi_errhandler(LPTX_param *param, MPI_Errhandler handler);
#endif

JUPITER_LPTX_DECL
LPTX_particle_init_set *LPTX_param_get_init_set(LPTX_param *param, int n);

JUPITER_LPTX_DECL
LPTX_particle_init_set *LPTX_param_get_first_init_set(LPTX_param *param);

JUPITER_LPTX_DECL
LPTX_particle_init_set *LPTX_param_get_last_init_set(LPTX_param *param);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_total_particles(LPTX_param *param);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_param_get_total_particles_in_init_sets(LPTX_param *param);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_param_get_particle_set(LPTX_param *param, int n);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_param_get_first_particle_set(LPTX_param *param);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_param_get_last_particle_set(LPTX_param *param);

#define LPTX__foreach_impl(p, param, firstf, nextf) \
  for (p = firstf(param); p; p = nextf(p))

#define LPTX__foreach_safe_impl(p, n, param, firstf, nextf) \
  for (p = firstf(param), n = p ? nextf(p) : NULL; p;       \
       p = n, n ? nextf(n) : NULL)

#define LPTX_foreach_particle_sets(setp, param)                      \
  LPTX__foreach_impl(setp, param, LPTX_param_get_first_particle_set, \
                     LPTX_particle_set_next)

#define LPTX_foreach_particle_sets_safe(setp, setn, param)   \
  LPTX__foreach_safe_impl(setp, setn, param,                 \
                          LPTX_param_get_first_particle_set, \
                          LPTX_particle_set_next)

#define LPTX_foreach_init_sets(setp, param)                      \
  LPTX__foreach_impl(setp, param, LPTX_param_get_first_init_set, \
                     LPTX_particle_init_set_next)

#define LPTX_foreach_init_sets_safe(setp, setn, param)                      \
  LPTX__foreach_safe_impl(setp, setn, param, LPTX_param_get_first_init_set, \
                          LPTX_particle_init_set_next)

#define LPTX_reverse_foreach_particle_sets(setp, param)             \
  LPTX__foreach_impl(setp, param, LPTX_param_get_last_particle_set, \
                     LPTX_particle_set_prev)

#define LPTX_reverse_foreach_particle_sets_safe(setp, setn, param)             \
  LPTX__foreach_safe_impl(setp, setn, param, LPTX_param_get_last_particle_set, \
                          LPTX_particle_set_prev)

#define LPTX_reverse_foreach_init_sets(setp, param)             \
  LPTX__foreach_impl(setp, param, LPTX_param_get_last_init_set, \
                     LPTX_particle_init_set_prev)

#define LPTX_reverse_foreach_init_sets_safe(setp, setn, param)             \
  LPTX__foreach_safe_impl(setp, setn, param, LPTX_param_get_last_init_set, \
                          LPTX_particle_init_set_prev)

/**
 * @brief Invoke given function for each particle sets in @p param
 *
 * The loop will be broken if given function returns LPTX_true.
 *
 * Macro version may be more flexible.
 *
 * This function is thread safe to call in OpenMP parallel region, but has no
 * synchronization between threads.
 */
JUPITER_LPTX_DECL
void LPTX_param_foreach_particle_sets(LPTX_param *param,
                                      LPTX_cb_foreach_particle_sets *func,
                                      void *arg);

/**
 * @brief Invoke given function for each particle sets in @p param
 *
 * The loop will be broken if given function returns LPTX_true.
 *
 * Macro version may be more flexible.
 *
 * This function is thread safe to call in OpenMP parallel region, but has no
 * synchronization between threads.
 */
JUPITER_LPTX_DECL
void LPTX_param_foreach_particle_sets_safe(LPTX_param *param,
                                           LPTX_cb_foreach_particle_sets *func,
                                           void *arg);

JUPITER_LPTX_DECL
void LPTX_param_foreach_particle_set_range(LPTX_param *param, LPTX_idtype start,
                                           LPTX_idtype last,
                                           LPTX_bool pts_range,
                                           LPTX_cb_foreach_particle_range *func,
                                           void *arg);

JUPITER_LPTX_DECL
void LPTX_param_foreach_particle_set_range_safe(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last, LPTX_bool pts_range,
  LPTX_cb_foreach_particle_range *func, void *arg);

/**
 * @brief Invoke given function for each particles in @p param
 * @param parallel Whether apply `omp for` (not `omp parallel for`) for
 *                 particles
 *
 * The loop will be aborted if given function returns LPTX_true. However,
 * the loop will be aborted only by the particle set boundary if parallelized.
 *
 * @note This function also calls @p func for unused particle entries.
 * @note @p parallel has no effect if OpenMP is not enabled
 *
 * The 'safe' version of this function does not exist since you are not allowed
 * to add or delete particles (besides changing the unused flag).
 *
 * This function is not thread safe **in global** if @p parallel is LPTX_true.
 * You can call this function from single thread team at once (for same @p
 * param).
 *
 * @p func will not be simultanously executed from different particle set (if @p
 * parallel is LPTX_true)
 */
JUPITER_LPTX_DECL
void LPTX_param_foreach_particles(LPTX_param *param, LPTX_bool parallel,
                                  LPTX_cb_foreach_particles *func, void *arg);

/**
 * @brief Invoke given function for given range of particles in @p param
 * @param parallel Whether apply `omp for` (not `omp parallel for`)
 * @param start Start index (inclusive)
 * @param last Last index (exclusive)
 * @param countif Function for counting particle.
 * @param countarg User data for @p countif
 * @param func Function to call
 * @param funcarg User data for @p func
 * @return the number of particles processed
 *
 * The loop will be aborted if @p func returns LPTX_true. However,
 * the loop will be aborted only by the particle set boundary if parallelized.
 *
 * This function is not thread safe **in global** if @p parallel is LPTX_true.
 * You can call this function from single thread team at once (for same @p
 * param and same range).
 *
 * @p func will not be simultanously executed from different particle set (if @p
 * parallel is LPTX_true)
 */
JUPITER_LPTX_DECL LPTX_idtype LPTX_param_foreach_particle_range(
  LPTX_param *param, LPTX_bool parallel, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particles *countif, void *countarg,
  LPTX_cb_foreach_particles *func, void *funcarg);

/**
 * @brief Invoke given function for each init sets in @p param
 *
 * The loop will be broken if given function returns LPTX_true.
 *
 * Macro version may be more flexible.
 *
 * This function is thread safe to call in OpenMP parallel region, but has no
 * synchronization between threads.
 */
JUPITER_LPTX_DECL
void LPTX_param_foreach_init_sets(LPTX_param *param,
                                  LPTX_cb_foreach_init_sets *func, void *arg);

/**
 * @brief Invoke given function for each init sets in @p param
 *
 * The loop will be broken if given function returns LPTX_true.
 *
 * Macro version may be more flexible.
 *
 * This function is thread safe to call in OpenMP parallel region, but has no
 * synchronization between threads.
 */
JUPITER_LPTX_DECL
void LPTX_param_foreach_init_sets_safe(LPTX_param *param,
                                       LPTX_cb_foreach_init_sets *func,
                                       void *arg);

/**
 * @brief Redistribute particles by user-defined function
 * @param param Particle set(s) to distribute
 * @param mpirank_func Callback function to select the destination rank.
 * @param arg Additinal free argument to be passed on @p mpirank_func.
 * @return Last MPI communication error, if applicable.
 * @retval 0 MPI is not enabled.
 * @retval MPI_SUCCESS No error
 * @retval MPI_ERR_COMM Communicator is not set
 *
 * @note The value of MPI_SUCCESS is 0. See the MPI standard.
 *
 * This function does nothing is MPI is not enabled for LPTX.
 */
JUPITER_LPTX_DECL
int LPTX_param_redistribute_particles(LPTX_param *param,
                                      LPTX_cb_mpirank *mpirank_func, void *arg);

/**
 * @brief Redistribute particles by rectangular domain
 * @param param Particle set
 * @param lb Coordinate of the 'lower left' corner of the domain in this rank
 * @param ub Coordinate of the 'upper right' corner of the domain in this rank
 * @param flg Flags for detect inside
 * @return Last MPI communication error, if applicable.
 * @retval 0 MPI is not enabled.
 * @retval MPI_SUCCESS No error
 * @retval MPI_ERR_COMM Communicator is not set
 *
 * @note The behavior is undefined if the domain overlaps among MPI ranks.
 * @note The value of MPI_SUCCESS is 0. See the MPI standard.
 *
 * This function does nothing is MPI is not enabled for LPTX.
 */
JUPITER_LPTX_DECL
int LPTX_param_redistribute_particles_rect_v(LPTX_param *param, LPTX_vector lb,
                                             LPTX_vector ub,
                                             LPTX_vector_rect_flags flg);

/**
 * @brief Gather particles
 * @param param Particle set
 * @param root Destination rank
 * @return Last MPI communication error, if applicable.
 * @retval 0 MPI is not enabled.
 * @retval MPI_SUCCESS No error
 * @retval MPI_ERR_COMM Communicator is not set
 *
 * This function does nothing is MPI is not enabled for LPTX.
 */
JUPITER_LPTX_DECL
int LPTX_param_gather_particles(LPTX_param *param, int root);

JUPITER_LPTX_DECL_END

#endif
