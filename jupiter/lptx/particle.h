/**
 * @file lptx/particle.h
 */
#ifndef JUPITER_LPTX_PARTICLE_H
#define JUPITER_LPTX_PARTICLE_H

#include "defs.h"
#include "jupiter/geometry/bitarray.h"
#include "jupiter/random/random.h"
#include "vector.h"

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

JUPITER_LPTX_DECL_START

/**
 * @brief Create new particle set
 * @param number_of_particles Number of particles for new set
 * @param number_of_vectors Number of vectors per particle
 * @param numbers_of_data Array sizes per particle for each vector
 * @return Allocated particle set
 *
 * The @p number_of_vectors should be greater than or equal to LPTX_NUM_VECTORS,
 * but even if it's not, the calculation treats the required vectors' size as
 * zero and will not emit errors. Extra vectors beyond LPTX_NUM_VECTORS can be
 * used for user-defined parameters.
 */
JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_new(LPTX_idtype number_of_particles,
                                         LPTX_idtype number_of_vectors,
                                         const LPTX_idtype *numbers_of_data);

/**
 * @brief Create new particle set with same vector sizes to base
 * @param number_particles Number of particles for new set
 * @param base Base set to use vector sizes from
 * @return Allocated particle set
 */
JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_replicate(LPTX_idtype number_of_particles,
                                               const LPTX_particle_set *base);

/**
 * @brief Generate particle set from init set.
 * @param param Parameter to obtain init set chain
 * @param old Old set of data to overwrite
 */
JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_from_init_set(LPTX_param *param,
                                                   LPTX_particle_set *old);

JUPITER_LPTX_DECL
void LPTX_particle_set_delete(LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_param *LPTX_particle_set_param(LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_next(LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_prev(LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_insert_next(LPTX_particle_set *prev,
                                                 LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_insert_prev(LPTX_particle_set *next,
                                                 LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_append(LPTX_param *param,
                                            LPTX_particle_set *set);

JUPITER_LPTX_DECL
LPTX_particle_set *LPTX_particle_set_prepend(LPTX_param *param,
                                             LPTX_particle_set *set);

/**
 * @brief Returns number of particles for storage.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_number_of_particles(const LPTX_particle_set *set);

/**
 * @brief Returns number of vectors per particle
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_number_of_vectors(const LPTX_particle_set *set);

/**
 * @brief Returns total number of vector elements per perticle
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_number_of_data(const LPTX_particle_set *set);

/**
 * @brief Returns array of each vector sizes per particle
 */
JUPITER_LPTX_DECL
const LPTX_idtype *LPTX_particle_set_vector_sizes(const LPTX_particle_set *set);

/**
 * @brief Sort particles by user-defined order
 *
 * @note Memory location of each particle will not be modified.
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_sort_particles(LPTX_particle_set *set,
                                      LPTX_cb_particle_compn *comp, void *arg);

/**
 * @brief Sort particles by user-defined order
 *
 * Same as LPTX_particle_set_sort_particles(). Just force to use insertion sort
 * regardless of internal condition. This function is public for testing
 * purpose.
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_isort_particles(LPTX_particle_set *set,
                                       LPTX_cb_particle_compn *comp, void *arg);

/**
 * @brief Sort particles by user-defined order
 *
 * Same as LPTX_particle_set_sort_particles(). Just force to use quick sort
 * regardless of internal condition. This function is public for testing
 * purpose.
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_qsort_particles(LPTX_particle_set *set,
                                       LPTX_cb_particle_compn *comp, void *arg);

/**
 * @brief Binary search particles by user-defined criteria
 * @return Index of a found particle
 *
 * @note Returning index is the index of 'sorted' array.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_binary_search(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg);

/**
 * @brief Binary search for lower bound of particles by user-defined criteria
 * @return First index that @p comp returns 0 or positive
 *
 * Particles must be sorted in order of @p comp require.
 *
 * If @p comp returns -1 (or negative) for all particles, returns the number of
 * particles in @p set.
 *
 * @note Returning index is the index of 'sorted' array.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_binary_lbound(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg);

/**
 * @brief Binary search for upper bound of particles by user-defined criteria
 * @return First index that @p comp returns 1 (or positive)
 *
 * Particles must be sorted in order of @p comp require.
 *
 * If @p comp returns 0 or negative for all particles, returns the number of
 * particles in @p set.
 *
 * @note Returning index is the index of 'sorted' array.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_binary_ubound(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg);

/**
 * @brief Binary partition particles by user-defined criteria
 * @param first Sets first index that @p comp returns 0 or positive
 * @param last Sets first index that @p comp returns 1 (or positive)
 *
 * Particles must be sorted in order of @p comp require.
 *
 * @p first or @p last must not be NULL, while this function exists for
 * optimization purpose. Currently, this is textually same as calling lbound and
 * ubound separetely, but compiler optimization may fuse them. Also, algorithm
 * might be improved for getting both later.
 *
 * If @p comp returns -1 (or negative) for all items, @p first and @p last will
 * be set to number of particles in @p set.
 *
 * If @p comp does not return 1 (or positive) for any items, @p last will
 * be set to number of partilces in @p set.
 *
 * If @p comp return 1 (or positive) for all items, @p first and @p last will
 * be set to 0.
 *
 * @note Returning index is the index of 'sorted' array.
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_binary_partition(LPTX_particle_set *set,
                                        LPTX_idtype *first, LPTX_idtype *last,
                                        LPTX_cb_particle_comp1 *comp,
                                        void *arg);

/**
 * @brief Get array index of particle from 'sorted' array.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_array_index_from_sorted(LPTX_particle_set *set,
                                                      LPTX_idtype sorted_index);

/**
 * @brief Get index of sorted array from particle index.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_sorted_index_from_array(LPTX_particle_set *set,
                                                      LPTX_idtype array_index);

/**
 * @brief Sort particles by its ID
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_sort_particles_id(LPTX_particle_set *set);

/**
 * @brief Sort particles by active or exited
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_sort_particles_active(LPTX_particle_set *set);

/**
 * @brief Returns number of used (allocated) particles.
 *
 * @warning This function assumes particles have been sorted
 *          by LPTX_particle_set_sort_particles().
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_used_particles_sorted(LPTX_particle_set *set);

/**
 * @brief Returns index of specific ID.
 * @retval -1 Not found.
 *
 * @note This function excludes free particle.
 *
 * @warning This function assumes particle has been sorted
 *          by LPTX_particle_set_sort_particles().
 *
 * Do not confuse with LPTX_particle_set_set_fluid_index_*() functions.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_find_index(LPTX_particle_set *set,
                                         LPTX_idtype id);

/**
 * @brief Check whether sets @p a and @p b are mergeable.
 *
 * Checks number of vectors and vector sizes are equal.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_set_are_mergeable(LPTX_particle_set *a,
                                          LPTX_particle_set *b);

JUPITER_LPTX_DECL
void LPTX_assert_particle_set_are_mergeable_impl(LPTX_particle_set *a,
                                                 LPTX_particle_set *b,
                                                 const char *aname,
                                                 const char *bname,
                                                 const char *file, long line);

/**
 * @brief Assert sets @p a and @p b are mergeable.
 */
#define LPTX_assert_particle_set_are_mergeable(a, b) \
  LPTX_assert_particle_set_are_mergeable_impl(a, b, #a, #b, __FILE__, __LINE__)

/**
 * @brief Merges particles from (one or) two sets.
 * @rerval LPTX_true Successfully processed
 * @retval LPTX_false Memory allocation failed (including number of
 *         particles overflows)
 *
 * To simplify, memory is allocated for the size of the sum of the number of
 * used particles in @p a and @p b, and ID collisions are not taken into
 * account. If no particle exists in both @p a and @p b, @p outp will be set
 * to NULL.
 *
 * The merge will be performed by ID. The @p func is used to select or merge
 * properties of particles when their IDs collide. Technically, it is allowed to
 * remove particle for @p outp (by setting used flag to LPTX_false) in @p func,
 * but this function does not expect such behavior.
 *
 * @p a and @p b will be sorted by its ID, resulting the particle order in @p
 * outp is sorted by ID.
 *
 * In this version @p outp will be allocated new particle set.
 *
 * This function is not OpenMP-parallelized (yet).
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_set_merge(LPTX_particle_set **outp,
                                  LPTX_particle_set *a, LPTX_particle_set *b,
                                  LPTX_cb_particle_set_merge *func, void *arg);

/**
 * @brief Get particle data at specified index
 * @param set Particle set
 * @param index Index to get
 * @param vectors Pointer to pointer of LPTX_particle_vector to store the array
 *        of vectors [out]
 * @param number_of_vectors Pointer to LPTX_idtype to store the number of
 *        vectors [out]
 * @return Pointer to base particle data
 *
 * To obtain vector data, do following:
 *
 * ```c
 * const LPTX_particle *particle;
 * const LPTX_particle_vector *const vectors;
 * LPTX_idtype number_fo_vectors;
 *
 * particle = PLTX_particle_set_get_particle_at(set, index, &vectors,
 *                                              &number_of_vectors);
 * value = LPTX_paritcle_vector_getv(vectors[0], ...);
 * ```
 *
 * @p vector and @p number_of_vectors are allowed to be NULL. Then, this
 * function will not return vectors.
 */
JUPITER_LPTX_DECL
const LPTX_particle *
LPTX_particle_set_get_particle_at(LPTX_particle_set *set, LPTX_idtype index,
                                  const LPTX_particle_vector **const vectors,
                                  LPTX_idtype *number_of_vectors);

/**
 * @brief Set particle data at specified index
 * @param set Particle set
 * @param index Index to set
 * @param p Base particle data
 * @param number_of_vectors Number of vectors in @p vectors
 * @param vector_vars List of variables of vectors to set
 * @param vectors List of vectors to set
 */
JUPITER_LPTX_DECL
void LPTX_particle_set_set_particle_at(LPTX_particle_set *set,
                                       LPTX_idtype index,
                                       const LPTX_particle *p,
                                       LPTX_idtype number_of_vectors,
                                       const LPTX_particle_vectors *vector_vars,
                                       const LPTX_particle_vector *vectors);

/**
 * @brief Filter out particles from set
 * @param outp Collection of filtered out particles [out]
 * @param stripped Collection of stripped set of paritcles [out]
 * @param set Input particle set
 * @param cond Filter condition (return LPTX_true to remove)
 * @param arg Arguemnt for @p cond
 * @retval LPTX_true Successfully filtered
 * @retval LPTX_false Memory allocation failed
 *
 * @p outp is NULL, just discards (set used flag to false) matching particles
 * from @p sets, and not copy to new *outp set.
 *
 * @p stripped is not NULL, allocates new stripped sets without filtered out
 * particles. Note that input @p sets will not be deleted (i.e., passing `&sets`
 * will cause memory leak). Also in this case, @p set is not modified.
 *
 * This function require O(N) extra storage space, returns LPTX_false when
 * allocation failed (i.e., this function still may return LPTX_false even if
 * both @p outp and @p filteredp are NULL).
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_set_filter(LPTX_particle_set **outp,
                                   LPTX_particle_set **stripped,
                                   LPTX_particle_set *set,
                                   LPTX_cb_particle_if *cond, void *arg);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_get_particle_id_at(LPTX_particle_set *set,
                                                 LPTX_idtype index);

JUPITER_LPTX_DECL
void LPTX_particle_set_set_particle_id_at(LPTX_particle_set *set,
                                          LPTX_idtype index, LPTX_idtype value);

/* Scalar value get/set template */
/**
 * undef LPTX_DECL_SETGET_DEL to use LPTX_DECL_SET_FN, LPTX_DECL_GET_FN macros
 * for definition
 */
#define LPTX_DECL_SETGET_DECL JUPITER_LPTX_DECL

#define LPTX__DECL_EXPND_AV(...) __VA_ARGS__

#define LPTX__DECL_EXPND_AC(_3, _2, _1, i, ...) LPTX__DECL_EXPND_AE##i
#define LPTX__DECL_EXPND_AN(...) \
  LPTX__DECL_EXPND_AC(__VA_ARGS__, 3, 2, 1, LPTX_empty)

#define LPTX__DECL_EXPND_AE2(N, type, val) LPTX_DECL_EXPND_A_##N(type, val)
#define LPTX__DECL_EXPND_AE3(_, type, N, val) LPTX_DECL_EXPND_A_##N(type, val)
#define LPTX__DECL_EXPND_AEE(N, I, ...) I(N, __VA_ARGS__)
#define LPTX__DECL_EXPND_AE(N, I, ...) LPTX__DECL_EXPND_AEE(N, I, __VA_ARGS__)

#define LPTX__DECL_EXPND_A(N, t) \
  LPTX__DECL_EXPND_AE(N, LPTX__DECL_EXPND_AN t, LPTX__DECL_EXPND_AV t)

#define LPTX__DECL_EXPND_I0(N, ...)

#define LPTX__DECL_EXPND_I1(N, t1, ...) , LPTX__DECL_EXPND_A(N, t1)

#define LPTX__DECL_EXPND_I2(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I1(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I3(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I2(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I4(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I3(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I5(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I4(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I6(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I5(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I7(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I6(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I8(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I7(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I9(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I8(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I10(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I9(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I11(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I10(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I12(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I11(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I13(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I12(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I14(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I13(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I15(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I14(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I16(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I15(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I17(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I16(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I18(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I17(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I19(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I18(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_I20(N, t1, ...) \
  LPTX__DECL_EXPND_I1(N, t1, ) LPTX__DECL_EXPND_I19(N, __VA_ARGS__)

#define LPTX__DECL_EXPND_C(_20, _19, _18, _17, _16, _15, _14, _13, _12, _11, \
                           _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0, i,   \
                           ...)                                              \
  i

#define LPTX__DECL_EXPND_N(...)                                               \
  LPTX__DECL_EXPND_C(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
                     9, 8, 7, 6, 5, 4, 3, 2, 1, 0, LPTX_empty)

#define LPTX__DECL_EXPND_E(N, I, ...) LPTX__DECL_EXPND_I##I(N, __VA_ARGS__)

#define LPTX__DECL_EXPND(N, I, ...) LPTX__DECL_EXPND_E(N, I, __VA_ARGS__)

#define LPTX_DECL_EXPND(N, ...) \
  LPTX__DECL_EXPND(N, LPTX__DECL_EXPND_N(__VA_ARGS__), __VA_ARGS__)

#define LPTX_DECL_EXPND_A_SET(type, var) const type *var
#define LPTX_DECL_EXPND_A_GET(type, var) type *var
#define LPTX_DECL_EXPND_A_SCL(type, var) type var

#define LPTX_DECL_SETGET_FN__SNGL(name, dir, ...) \
  name(LPTX_particle_set *set __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__PACK(name, dir, ...) \
  name(LPTX_param *param __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__SIMPLE(f, name, dir, ...)               \
  LPTX_idtype f(name, dir, LPTX_comma LPTX_idtype start __VA_ARGS__, \
                LPTX_idtype size)

#define LPTX_DECL_SETGET_FN_SNGL(name, dir, ...) \
  LPTX_DECL_SETGET_FN__SIMPLE(LPTX_DECL_SETGET_FN__SNGL, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PACK(name, dir, ...) \
  LPTX_DECL_SETGET_FN__SIMPLE(LPTX_DECL_SETGET_FN__PACK, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__RECT(f, name, dir, ...) \
  void f(name, dir, LPTX_comma const LPTX_rectilinear_grid *grid __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SRECT(name, dir, ...) \
  LPTX_DECL_SETGET_FN__RECT(LPTX_DECL_SETGET_FN__SNGL, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PRECT(name, dir, ...) \
  LPTX_DECL_SETGET_FN__RECT(LPTX_DECL_SETGET_FN__PACK, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__RECT(f, name, dir, ...) \
  void f(name, dir, LPTX_comma const LPTX_rectilinear_grid *grid __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SRECT(name, dir, ...) \
  LPTX_DECL_SETGET_FN__RECT(LPTX_DECL_SETGET_FN__SNGL, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PRECT(name, dir, ...) \
  LPTX_DECL_SETGET_FN__RECT(LPTX_DECL_SETGET_FN__PACK, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__RECTX(f, name, dir, coords_type, ...)           \
  void f(name, dir, __VA_ARGS__, const coords_type *coords_i,                \
         const coords_type *coords_j, const coords_type *coords_k,           \
         LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx,     \
         LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, \
         LPTX_idtype stmz)

#define LPTX_DECL_SETGET_FN_SRECTT(name, dir, ...)                            \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__SNGL, name, dir, LPTX_type, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PRECTT(name, dir, ...)                            \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__PACK, name, dir, LPTX_type, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SRECTF(name, dir, ...)                        \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__SNGL, name, dir, float, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PRECTF(name, dir, ...)                        \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__PACK, name, dir, float, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SRECTD(name, dir, ...)                         \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__SNGL, name, dir, double, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PRECTD(name, dir, ...)                         \
  LPTX_DECL_SETGET_FN__RECTX(LPTX_DECL_SETGET_FN__PACK, name, dir, double, \
                             __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__FLCB(f, cbtype, name, dir, ...) \
  void f(name, dir, LPTX_comma cbtype *func, void *arg __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFLCV(name, dir, ...)      \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__SNGL, \
                            LPTX_cb_fluid_variable_v, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFLCV(name, dir, ...)      \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__PACK, \
                            LPTX_cb_fluid_variable_v, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFLCF(name, dir, ...)      \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__SNGL, \
                            LPTX_cb_fluid_variable_f, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFLCF(name, dir, ...)      \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__PACK, \
                            LPTX_cb_fluid_variable_f, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFLCI(name, dir, ...)                           \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__SNGL, LPTX_cb_fluid_index, \
                            name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFLCI(name, dir, ...)                           \
  LPTX_DECL_SETGET_FN__FLCB(LPTX_DECL_SETGET_FN__PACK, LPTX_cb_fluid_index, \
                            name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN__FLCBD(f, cbtype, name, dir, ...)            \
  void f(name, dir, LPTX_comma LPTX_cb_particle_if *cond, void *condarg, \
         cbtype *func, void *funcarg __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFCDV(name, dir, ...)       \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__SNGL, \
                             LPTX_cb_fluid_variable_v, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFCDV(name, dir, ...)       \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__PACK, \
                             LPTX_cb_fluid_variable_v, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFCDF(name, dir, ...)       \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__SNGL, \
                             LPTX_cb_fluid_variable_f, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFCDF(name, dir, ...)       \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__PACK, \
                             LPTX_cb_fluid_variable_f, name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_SFCDI(name, dir, ...)                            \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__SNGL, LPTX_cb_fluid_index, \
                             name, dir, __VA_ARGS__)

#define LPTX_DECL_SETGET_FN_PFCDI(name, dir, ...)                            \
  LPTX_DECL_SETGET_FN__FLCBD(LPTX_DECL_SETGET_FN__PACK, LPTX_cb_fluid_index, \
                             name, dir, __VA_ARGS__)

#define LPTX__DECL_FN(name, dir, type, ...) \
  LPTX_DECL_SETGET_DECL                     \
  LPTX_DECL_SETGET_FN_##type(name, dir, LPTX_DECL_EXPND(dir, __VA_ARGS__))

/**
 * @def LPTX_DECL_SET_FN(name, type, args...)
 * @brief Declare particle parameter setter function
 * @param name Function name to declare (no prefix or no postfix added)
 * @param type Function prototype template name. See below
 * @param args... Parameters and type
 *
 * @p args is parenthesized pair of parameter of type, for example:
 *
 * * `(LPTX_idtype, x)`: Expands to `const LPTX_idtype *x`
 * * `(LPTX_idtype, GET, x)`: Expands to `LPTX_idtype *x`
 * * `(LPTX_idtype, SCL, x)`: Expands to `LPTX_idtype x`
 *
 * @p type is function prototype template name:
 *
 * - `SNGL`: Setter for single particle set, from simple array(s)
 *   `LPTX_idtype name(LPTX_particle_set *set, LPTX_idtype start, [args...,]
 *   LPTX_idtype size)`
 *
 * - `PACK`: Setter for flattened particle sets, from simple array(s)
 *   `LPTX_idtype name(LPTX_param *param, LPTX_idtype start, [args...,]
 *   LPTX_idtype size)`
 *
 * - `SRECT`: Setter for single particle set, from fluid field with
 *   rectilinear grid
 *   `void name(LPTX_particle_set *set, const LPTX_rectilinear_grid *grid,
 *   [args...])`
 *
 * - `PRECT`: Setter for flattened particle sets, from fluid field with
 *   rectilinear grid
 *   `void name(LPTX_param *param, const LPTX_rectilinear_grid *grid,
 *   [args...])`
 *
 * - `SRECT[TFD]`: Setter for single particle set, from fluid field with
 *   raw rectilinear grid parameters ([TFD] suffix for LPTX_type, float, double
 *   coordinates parameters respectively)
 *   `void name(LPTX_particle_set *set, [args...], const [TFD] *coords_i,
 *   const [TFD] *coords_j, const [TFD] *coords_k, LPTX_idtype nx, LPTX_idtype
 *   ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype mz, LPTX_idtype stmx,
 *   LPTX_idtype stmy, LPTX_idtype stmz)`
 *
 * - `PRECT[TFD]`: Setter for flattened particle sets, from fluid field with
 *   raw rectilinear grid parameters ([TFD] suffix for LPTX_type, float, double
 *   coordinates parameters respectively)
 *   `void name(LPTX_param *param, [args...], const [TFD] *coords_i,
 *   const [TFD] *coords_j, const [TFD]  *coords_k, LPTX_idtype nx, LPTX_idtype
 *   ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype mz, LPTX_idtype stmx,
 *   LPTX_idtype stmy, LPTX_idtype stmz)`
 *
 * - `SFLCV`: Setter for single particle set, using custom vector function
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_variable_v *func,
 *   void *arg, [args...])`
 *
 * - `PFLCV`: Setter for flattened particle sets, using custom vector function
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_variable_v *func,
 *   void *arg, [args...])`
 *
 * - `SFLCF`: Setter for single particle set, using custom scalar function
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_variable_f *func,
 *   void *arg, [args...])`
 *
 * - `PFLCF`: Setter for flattened particle sets, using custom scalar function
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_variable_f *func,
 *   void *arg, [args...])`
 *
 * - `SFLCI`: Setter for single particle set, using custom index setter
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_index *func,
 *   void *arg, [args...])`
 *
 * - `PFLCI`: Setter for flattened particle sets, using custom index setter
 *   `void name(LPTX_particle_set *set, LPTX_cb_fluid_index *func,
 *   void *arg, [args...])`
 *
 * - `SFCDV`: Setter for single particle set, using custom vector function,
 *   with custom set conditon
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_variable_v *func, void *funcarg, [args...])`
 *
 * - `PFCDV`: Setter for flattened particle sets, using custom vector function
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_variable_v *func, void *funcarg, [args...])`
 *
 * - `SFCDF`: Setter for single particle set, using custom scalar function
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_variable_f *func, void *funcarg, [args...])`
 *
 * - `PFCDF`: Setter for flattened particle sets, using custom scalar function
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_variable_f *func, void *funcarg, [args...])`
 *
 * - `SFCDI`: Setter for single particle set, using custom index setter
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_index *func, void *funcarg, [args...])`
 *
 * - `PFCDI`: Setter for flattened particle sets, using custom index setter
 *   `void name(LPTX_particle_set *set, LPTX_cb_particle_if *cond,
 *   void *condarg, LPTX_cb_fluid_index *func, void *funcarg, [args...])`
 *
 * `SRECT[TFD]` and `PRECT[TFD]` are deprecated.
 */
#define LPTX_DECL_SET_FN(name, ...) \
  LPTX__DECL_FN(name, SET, __VA_ARGS__, LPTX_empty)

/**
 * @def LPTX_DECL_GET_FN(name, type, args...)
 * @brief Declare particle parameter setter function
 * @param name Function name to declare (no prefix or no postfix added)
 * @param type Function prototype template name. See below
 * @param args... Parameters and type
 *
 * @p args is parenthesized pair of parameter of type, for example:
 *
 * * `(LPTX_idtype, x)`: Expands to `LPTX_idtype *x`
 * * `(LPTX_idtype, SET, x)`: Expands to `const LPTX_idtype *x`
 * * `(LPTX_idtype, SCL, x)`: Expands to `LPTX_idtype x`
 *
 * @p type is function prototype template name:
 *
 * - `SNGL`: Getter for single particle set, to simple array(s)
 *   `LPTX_idtype name(LPTX_particle_set *set, LPTX_idtype start, [args...,]
 *   LPTX_idtype size)`
 *
 * - `PACK`: Getter for flattened particle sets, to simple array(s)
 *   `LPTX_idtype name(LPTX_param *param, LPTX_idtype start, [args...,]
 *   LPTX_idtype size)`
 *
 * @note All types in LPTX_DECL_SET_FN() can also be used (i.e., declarable),
 * but they are algorithmically not implementable.
 */
#define LPTX_DECL_GET_FN(name, ...) \
  LPTX__DECL_FN(name, GET, __VA_ARGS__, LPTX_empty)

/* set/get particle id */
LPTX_DECL_SET_FN(LPTX_particle_set_set_particle_id, SNGL, (LPTX_idtype, ids));
LPTX_DECL_GET_FN(LPTX_particle_set_get_particle_id, SNGL, (LPTX_idtype, ids));
LPTX_DECL_SET_FN(LPTX_set_particle_particle_id, PACK, (LPTX_idtype, ids));
LPTX_DECL_GET_FN(LPTX_get_particle_particle_id, PACK, (LPTX_idtype, ids));

/* set/get origin id */
LPTX_DECL_SET_FN(LPTX_particle_set_set_origin_id, SNGL, (LPTX_idtype, ids));
LPTX_DECL_GET_FN(LPTX_particle_set_get_origin_id, SNGL, (LPTX_idtype, ids));
LPTX_DECL_SET_FN(LPTX_set_particle_origin_id, PACK, (LPTX_idtype, ids));
LPTX_DECL_GET_FN(LPTX_get_particle_origin_id, PACK, (LPTX_idtype, ids));

/* set/get fluid velocity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_aos, PACK,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_t_aos, PACK,
                 (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_f_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_aos, PACK,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_d_aos, PACK,
                 (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_struct, SRECT,
                 (LPTX_rectilinear_vector, velocity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_struct, PRECT,
                 (LPTX_rectilinear_vector, velocity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_struct, SRECTT, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_struct, PRECTT, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_struct, SRECTF, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_struct, PRECTF, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_struct, SRECTD,
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_struct, PRECTD, //
                 (double, x), (double, y), (double, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_cb, SFLCV);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_cb, PFLCV);

/* set/get fluid density */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density, SNGL,
                 (LPTX_type, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density, SNGL,
                 (LPTX_type, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density, PACK, (LPTX_type, density));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density, PACK, (LPTX_type, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_f, SNGL, (float, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density_f, SNGL, (float, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_f, PACK, (float, density));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density_f, PACK, (float, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_d, SNGL,
                 (double, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density_d, SNGL,
                 (double, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_d, PACK, (double, density));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density_d, PACK, (double, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_struct, SRECTT,
                 (LPTX_type, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_struct, PRECTT,
                 (LPTX_type, density));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_f_struct, SRECTF,
                 (float, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_f_struct, PRECTF,
                 (float, density));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_d_struct, SRECTD,
                 (double, density));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_d_struct, PRECTD,
                 (double, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_cb, PFLCF);

/* set/get fluid viscosity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity, SNGL,
                 (LPTX_type, viscosity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity, SNGL,
                 (LPTX_type, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity, PACK,
                 (LPTX_type, viscosity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity, PACK,
                 (LPTX_type, viscosity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_f, SNGL,
                 (float, viscosity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity_f, SNGL,
                 (float, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_f, PACK, (float, viscosity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity_f, PACK, (float, viscosity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_d, SNGL,
                 (double, viscosity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity_d, SNGL,
                 (double, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_d, PACK,
                 (double, viscosity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity_d, PACK,
                 (double, viscosity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, viscosity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_struct, SRECTT,
                 (LPTX_type, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_struct, PRECTT,
                 (LPTX_type, viscosity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_f_struct, SRECTF,
                 (float, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_f_struct, PRECTF,
                 (float, viscosity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_d_struct, SRECTD,
                 (double, viscosity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_d_struct, PRECTD,
                 (double, viscosity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_cb, PFLCF);

/* set/get fluid specific heat */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat, SNGL,
                 (LPTX_type, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat, SNGL,
                 (LPTX_type, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat, PACK,
                 (LPTX_type, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat, PACK,
                 (LPTX_type, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_f, SNGL,
                 (float, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat_f, SNGL,
                 (float, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_f, PACK,
                 (float, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat_f, PACK,
                 (float, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_d, SNGL,
                 (double, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat_d, SNGL,
                 (double, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_d, PACK,
                 (double, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat_d, PACK,
                 (double, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_struct, SRECTT,
                 (LPTX_type, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_struct, PRECTT,
                 (LPTX_type, specific_heat));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_f_struct, SRECTF,
                 (float, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_f_struct, PRECTF,
                 (float, specific_heat));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_d_struct, SRECTD,
                 (double, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_d_struct, PRECTD,
                 (double, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_cb, PFLCF);

/* set/get fluid thermal conductivity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_struct_s,
                 SRECT, (LPTX_rectilinear_scalar, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_struct,
                 SRECTT, (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_struct, PRECTT,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_f_struct,
                 SRECTF, (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_f_struct, PRECTF,
                 (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_d_struct,
                 SRECTD, (double, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_d_struct, PRECTD,
                 (double, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_cb, PFLCF);

/* set/get fluid tempretaure */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature, PACK,
                 (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature, PACK,
                 (LPTX_type, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_f, PACK,
                 (float, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_f, PACK,
                 (float, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_d, PACK,
                 (double, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_d, PACK,
                 (double, temperature));

/**
 * Thermal conductivity arguemnt is reserved for future use. Current
 * implementation does not use it.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, temperature),
                 (LPTX_rectilinear_scalar, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, temperature),
                 (LPTX_rectilinear_scalar, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_struct, SRECTT,
                 (LPTX_type, temperature), (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_struct, PRECTT,
                 (LPTX_type, temperature), (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_f_struct, SRECTF,
                 (float, temperature), (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_f_struct, PRECTF,
                 (float, temperature), (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_d_struct, SRECTD,
                 (double, temperature), (double, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_d_struct, PRECTD,
                 (double, temperature), (double, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_cb, PFLCF);

/* set/get fluid temperature gradient */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad, PACK,
                 (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad, PACK,
                 (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_aos, PACK,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_t_aos, PACK,
                 (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_aos, PACK,
                 (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_f_aos, PACK,
                 (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_aos, PACK,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_d_aos, PACK,
                 (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

/**
 * @note The temperature gradient will be interpolated from temperature gradient
 * set in the fluid field. LPT module will not compute them from temperature.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_struct, SRECT,
                 (LPTX_rectilinear_vector, temperature_grad));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_struct, PRECT,
                 (LPTX_rectilinear_vector, temperature_grad));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_struct, SRECTT,
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_struct, PRECTT,
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_struct, SRECTF,
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_struct, PRECTF,
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_struct, SRECTD,
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_struct, PRECTD,
                 (double, x), (double, y), (double, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_cb, SFLCV);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_cb, PFLCV);

/* set/get mean free path (of fluid) */
LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path, SNGL,
                 (LPTX_type, lambda));
LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path, SNGL,
                 (LPTX_type, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path, PACK, (LPTX_type, lambda));
LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path, PACK, (LPTX_type, lambda));

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_f, SNGL, (float, lambda));
LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path_f, SNGL, (float, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_f, PACK, (float, lambda));
LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path_f, PACK, (float, lambda));

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_d, SNGL,
                 (double, lambda));
LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path_d, SNGL,
                 (double, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_d, PACK, (double, lambda));
LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path_d, PACK, (double, lambda));

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, lambda));

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_struct, SRECTT,
                 (LPTX_type, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_struct, PRECTT,
                 (LPTX_type, lambda));
LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_f_struct, SRECTF,
                 (float, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_f_struct, PRECTF,
                 (float, lambda));
LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_d_struct, SRECTD,
                 (double, lambda));
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_d_struct, PRECTD,
                 (double, lambda));

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_cb, PFLCF);

/* set/get fluid molecular weight */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight, SNGL,
                 (LPTX_type, molecular_weight));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight, SNGL,
                 (LPTX_type, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight, PACK,
                 (LPTX_type, molecular_weight));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight, PACK,
                 (LPTX_type, molecular_weight));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_f, SNGL,
                 (float, molecular_weight));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight_f, SNGL,
                 (float, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_f, PACK,
                 (float, molecular_weight));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight_f, PACK,
                 (float, molecular_weight));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_d, SNGL,
                 (double, molecular_weight));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight_d, SNGL,
                 (double, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_d, PACK,
                 (double, molecular_weight));
LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight_d, PACK,
                 (double, molecular_weight));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, molecular_weight));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_struct, SRECTT,
                 (LPTX_type, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_struct, PRECTT,
                 (LPTX_type, molecular_weight));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_f_struct, SRECTF,
                 (float, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_f_struct, PRECTF,
                 (float, molecular_weight));
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_d_struct, SRECTD,
                 (double, molecular_weight));
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_d_struct, PRECTD,
                 (double, molecular_weight));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_cb, SFLCF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_cb, PFLCF);

/* set/get particle position */
LPTX_DECL_SET_FN(LPTX_particle_set_set_position, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_position, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_position, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_position_t_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_position_t_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_f_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_f_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_position_f_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_position_f_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_d_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_d_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_position_d_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_position_d_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle velocity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_t_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_t_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_f_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_f_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_f_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_f_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_d_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_d_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_d_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_d_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle current time */
LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time, SNGL, (LPTX_type, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time, SNGL, (LPTX_type, time));
LPTX_DECL_SET_FN(LPTX_set_particle_current_time, PACK, (LPTX_type, time));
LPTX_DECL_GET_FN(LPTX_get_particle_current_time, PACK, (LPTX_type, time));

LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time_f, SNGL, (float, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time_f, SNGL, (float, time));
LPTX_DECL_SET_FN(LPTX_set_particle_current_time_f, PACK, (float, time));
LPTX_DECL_GET_FN(LPTX_get_particle_current_time_f, PACK, (float, time));

LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time_d, SNGL, (double, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time_d, SNGL, (double, time));
LPTX_DECL_SET_FN(LPTX_set_particle_current_time_d, PACK, (double, time));
LPTX_DECL_GET_FN(LPTX_get_particle_current_time_d, PACK, (double, time));

/* set/get particle start time */
LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time, SNGL, (LPTX_type, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time, SNGL, (LPTX_type, time));
LPTX_DECL_SET_FN(LPTX_set_particle_start_time, PACK, (LPTX_type, time));
LPTX_DECL_GET_FN(LPTX_get_particle_start_time, PACK, (LPTX_type, time));

LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time_f, SNGL, (float, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time_f, SNGL, (float, time));
LPTX_DECL_SET_FN(LPTX_set_particle_start_time_f, PACK, (float, time));
LPTX_DECL_GET_FN(LPTX_get_particle_start_time_f, PACK, (float, time));

LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time_d, SNGL, (double, time));
LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time_d, SNGL, (double, time));
LPTX_DECL_SET_FN(LPTX_set_particle_start_time_d, PACK, (double, time));
LPTX_DECL_GET_FN(LPTX_get_particle_start_time_d, PACK, (double, time));

/* set/get particle density */
LPTX_DECL_SET_FN(LPTX_particle_set_set_density, SNGL, (LPTX_type, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_density, SNGL, (LPTX_type, density));
LPTX_DECL_SET_FN(LPTX_set_particle_density, PACK, (LPTX_type, density));
LPTX_DECL_GET_FN(LPTX_get_particle_density, PACK, (LPTX_type, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_density_f, SNGL, (float, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_density_f, SNGL, (float, density));
LPTX_DECL_SET_FN(LPTX_set_particle_density_f, PACK, (float, density));
LPTX_DECL_GET_FN(LPTX_get_particle_density_f, PACK, (float, density));

LPTX_DECL_SET_FN(LPTX_particle_set_set_density_d, SNGL, (double, density));
LPTX_DECL_GET_FN(LPTX_particle_set_get_density_d, SNGL, (double, density));
LPTX_DECL_SET_FN(LPTX_set_particle_density_d, PACK, (double, density));
LPTX_DECL_GET_FN(LPTX_get_particle_density_d, PACK, (double, density));

/* set/get particle speicfic heat */
LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat, SNGL,
                 (LPTX_type, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat, SNGL,
                 (LPTX_type, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat, PACK,
                 (LPTX_type, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat, PACK,
                 (LPTX_type, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat_f, SNGL,
                 (float, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat_f, SNGL,
                 (float, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat_f, PACK,
                 (float, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat_f, PACK,
                 (float, specific_heat));

LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat_d, SNGL,
                 (double, specific_heat));
LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat_d, SNGL,
                 (double, specific_heat));
LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat_d, PACK,
                 (double, specific_heat));
LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat_d, PACK,
                 (double, specific_heat));

/* set/get particle thermal conductivity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity));

LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity));
LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity));
LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity));

/* set/get heat transfer rate */
LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate, SNGL,
                 (LPTX_type, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate, SNGL,
                 (LPTX_type, heat_transfer_rate));
LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate, PACK,
                 (LPTX_type, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate, PACK,
                 (LPTX_type, heat_transfer_rate));

LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate_f, SNGL,
                 (float, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate_f, SNGL,
                 (float, heat_transfer_rate));
LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate_f, PACK,
                 (float, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate_f, PACK,
                 (float, heat_transfer_rate));

LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate_d, SNGL,
                 (double, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate_d, SNGL,
                 (double, heat_transfer_rate));
LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate_d, PACK,
                 (double, heat_transfer_rate));
LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate_d, PACK,
                 (double, heat_transfer_rate));

/* set/get total heat transfer */
LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer, SNGL,
                 (LPTX_type, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer, SNGL,
                 (LPTX_type, total_heat_transfer));
LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer, PACK,
                 (LPTX_type, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer, PACK,
                 (LPTX_type, total_heat_transfer));

LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer_f, SNGL,
                 (float, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer_f, SNGL,
                 (float, total_heat_transfer));
LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer_f, PACK,
                 (float, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer_f, PACK,
                 (float, total_heat_transfer));

LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer_d, SNGL,
                 (double, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer_d, SNGL,
                 (double, total_heat_transfer));
LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer_d, PACK,
                 (double, total_heat_transfer));
LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer_d, PACK,
                 (double, total_heat_transfer));

/* set/get particle diameter */
LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter, SNGL, (LPTX_type, diameter));
LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter, SNGL, (LPTX_type, diameter));
LPTX_DECL_SET_FN(LPTX_set_particle_diameter, PACK, (LPTX_type, diameter));
LPTX_DECL_GET_FN(LPTX_get_particle_diameter, PACK, (LPTX_type, diameter));

LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter_f, SNGL, (float, diameter));
LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter_f, SNGL, (float, diameter));
LPTX_DECL_SET_FN(LPTX_set_particle_diameter_f, PACK, (float, diameter));
LPTX_DECL_GET_FN(LPTX_get_particle_diameter_f, PACK, (float, diameter));

LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter_d, SNGL, (double, diameter));
LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter_d, SNGL, (double, diameter));
LPTX_DECL_SET_FN(LPTX_set_particle_diameter_d, PACK, (double, diameter));
LPTX_DECL_GET_FN(LPTX_get_particle_diameter_d, PACK, (double, diameter));

/* set/get particle fupt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fupt, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fupt, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fupt, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fupt, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fuptt_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fuptt_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptf_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptf_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fuptf_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fuptf_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptd_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptd_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fuptd_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fuptd_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fuptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fuptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fupt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fupt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fuptd_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fuptd_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle fdut */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fdut, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdut, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fdut, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fdut, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fdutt_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fdutt_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutf_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutf_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fdutf_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fdutf_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutd_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutd_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fdutd_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fdutd_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fdutt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fdutt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fdut_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fdut_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fdutd_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fdutd_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle fxpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fxpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fxpt, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fxpt, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fxptt_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fxptt_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptf_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptf_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fxptf_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fxptf_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptd_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptd_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fxptd_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fxptd_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fxptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fxptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fxpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fxpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fxptd_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fxptd_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle dTdt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdt, SNGL, (LPTX_type, dTdt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdt, SNGL, (LPTX_type, dTdt));
LPTX_DECL_SET_FN(LPTX_set_particle_dTdt, PACK, (LPTX_type, dTdt));
LPTX_DECL_GET_FN(LPTX_get_particle_dTdt, PACK, (LPTX_type, dTdt));

LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdtf, SNGL, (float, dTdt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdtf, SNGL, (float, dTdt));
LPTX_DECL_SET_FN(LPTX_set_particle_dTdtf, PACK, (float, dTdt));
LPTX_DECL_GET_FN(LPTX_get_particle_dTdtf, PACK, (float, dTdt));

LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdtd, SNGL, (double, dTdt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdtd, SNGL, (double, dTdt));
LPTX_DECL_SET_FN(LPTX_set_particle_dTdtd, PACK, (double, dTdt));
LPTX_DECL_GET_FN(LPTX_get_particle_dTdtd, PACK, (double, dTdt));

/* set/get particle fbpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fbpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fbpt, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fbpt, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fbptt_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fbptt_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptf_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptf_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fbptf_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fbptf_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptd_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptd_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fbptd_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fbptd_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fbptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fbptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fbpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fbpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fbptd_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fbptd_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle fTpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fTpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTpt, SNGL, (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_fTpt, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_fTpt, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptt_aos, SNGL, (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fTptt_aos, PACK, (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fTptt_aos, PACK, (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptf_aos, SNGL, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptf_aos, SNGL, (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fTptf_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fTptf_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptd_aos, SNGL, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptd_aos, SNGL, (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_fTptd_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_fTptd_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fTptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fTptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptf_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fTpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fTpt_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptd_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_fTptd_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_fTptd_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get particle dlpin (distance to interface; unused in LPTX) */
LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpin, SNGL, (LPTX_type, dlpin));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpin, SNGL, (LPTX_type, dlpin));
LPTX_DECL_SET_FN(LPTX_set_particle_dlpin, PACK, (LPTX_type, dlpin));
LPTX_DECL_GET_FN(LPTX_get_particle_dlpin, PACK, (LPTX_type, dlpin));

LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpinf, SNGL, (float, dlpin));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpinf, SNGL, (float, dlpin));
LPTX_DECL_SET_FN(LPTX_set_particle_dlpinf, PACK, (float, dlpin));
LPTX_DECL_GET_FN(LPTX_get_particle_dlpinf, PACK, (float, dlpin));

LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpind, SNGL, (double, dlpin));
LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpind, SNGL, (double, dlpin));
LPTX_DECL_SET_FN(LPTX_set_particle_dlpind, PACK, (double, dlpin));
LPTX_DECL_GET_FN(LPTX_get_particle_dlpind, PACK, (double, dlpin));

/* set/get particle icfpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_icfpt, SNGL, (LPTX_idtype, icfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_icfpt, SNGL, (LPTX_idtype, icfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_icfpt, PACK, (LPTX_idtype, icfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_icfpt, PACK, (LPTX_idtype, icfpt));

LPTX_DECL_SET_FN(LPTX_particle_set_set_icfpti, SNGL, (int, icfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_icfpti, SNGL, (int, icfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_icfpti, PACK, (int, icfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_icfpti, PACK, (int, icfpt));

/* set/get particle jcfpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_jcfpt, SNGL, (LPTX_idtype, jcfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_jcfpt, SNGL, (LPTX_idtype, jcfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_jcfpt, PACK, (LPTX_idtype, jcfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_jcfpt, PACK, (LPTX_idtype, jcfpt));

LPTX_DECL_SET_FN(LPTX_particle_set_set_jcfpti, SNGL, (int, jcfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_jcfpti, SNGL, (int, jcfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_jcfpti, PACK, (int, jcfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_jcfpti, PACK, (int, jcfpt));

/* set/get particle kcfpt */
LPTX_DECL_SET_FN(LPTX_particle_set_set_kcfpt, SNGL, (LPTX_idtype, kcfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_kcfpt, SNGL, (LPTX_idtype, kcfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_kcfpt, PACK, (LPTX_idtype, kcfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_kcfpt, PACK, (LPTX_idtype, kcfpt));

LPTX_DECL_SET_FN(LPTX_particle_set_set_kcfpti, SNGL, (int, kcfpt));
LPTX_DECL_GET_FN(LPTX_particle_set_get_kcfpti, SNGL, (int, kcfpt));
LPTX_DECL_SET_FN(LPTX_set_particle_kcfpti, PACK, (int, kcfpt));
LPTX_DECL_GET_FN(LPTX_get_particle_kcfpti, PACK, (int, kcfpt));

/* set/get parceln (number of particles in parcel) */
LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln, SNGL, (LPTX_type, parceln));
LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln, SNGL, (LPTX_type, parceln));
LPTX_DECL_SET_FN(LPTX_set_particle_parceln, PACK, (LPTX_type, parceln));
LPTX_DECL_GET_FN(LPTX_get_particle_parceln, PACK, (LPTX_type, parceln));

LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln_f, SNGL, (float, parceln));
LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln_f, SNGL, (float, parceln));
LPTX_DECL_SET_FN(LPTX_set_particle_parceln_f, PACK, (float, parceln));
LPTX_DECL_GET_FN(LPTX_get_particle_parceln_f, PACK, (float, parceln));

LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln_d, SNGL, (double, parceln));
LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln_d, SNGL, (double, parceln));
LPTX_DECL_SET_FN(LPTX_set_particle_parceln_d, PACK, (double, parceln));
LPTX_DECL_GET_FN(LPTX_get_particle_parceln_d, PACK, (double, parceln));

/* set/get particle temperature */
LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_temperature, PACK, (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_temperature, PACK, (LPTX_type, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_temperature_f, PACK, (float, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_temperature_f, PACK, (float, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_temperature_d, PACK, (double, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_temperature_d, PACK, (double, temperature));

/* set/get initial position */
LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_t_aos, PACK,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_t_aos, PACK,
                 (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_f_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_f_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_d_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_d_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get initial velocity */
LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity, SNGL,
                 (LPTX_vector, vector));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity, PACK, (LPTX_vector, vector));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity, PACK, (LPTX_vector, vector));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_t_aos, PACK,
                 (LPTX_type, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_t_aos, PACK,
                 (LPTX_type, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_f_aos, SNGL,
                 (float, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_f_aos, PACK, (float, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_f_aos, PACK, (float, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_d_aos, SNGL,
                 (double, aosvec));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_d_aos, PACK, (double, aosvec));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_d_aos, PACK, (double, aosvec));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));
LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z));

/* set/get initial temperature */
LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature, SNGL,
                 (LPTX_type, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature, PACK,
                 (LPTX_type, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature, PACK,
                 (LPTX_type, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature_f, SNGL,
                 (float, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature_f, PACK,
                 (float, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature_f, PACK,
                 (float, temperature));

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature_d, SNGL,
                 (double, temperature));
LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature_d, PACK,
                 (double, temperature));
LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature_d, PACK,
                 (double, temperature));

/* set/get particle-bound random seed */
LPTX_DECL_SET_FN(LPTX_particle_set_set_ptseed, SNGL,
                 (jupiter_random_seed, seed));
LPTX_DECL_GET_FN(LPTX_particle_set_get_ptseed, SNGL,
                 (jupiter_random_seed, seed));
LPTX_DECL_SET_FN(LPTX_set_particle_ptseed, PACK, (jupiter_random_seed, seed));
LPTX_DECL_GET_FN(LPTX_get_particle_ptseed, PACK, (jupiter_random_seed, seed));

/*
 * uint64_t is assumed. See struct_defs.h for more detail.
 *
 * Consumes JUPITER_RANDOM_SEED_SIZE elements of uint64_t per one particle.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_ptseed_i, SNGL, (uint64_t, seed));
LPTX_DECL_GET_FN(LPTX_particle_set_get_ptseed_i, SNGL, (uint64_t, seed));
LPTX_DECL_SET_FN(LPTX_set_particle_ptseed_i, PACK, (uint64_t, seed));
LPTX_DECL_GET_FN(LPTX_get_particle_ptseed_i, PACK, (uint64_t, seed));

/* set/get particle flags */
LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall, SNGL,
                 (LPTX_particle_flags, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall, SNGL,
                 (LPTX_particle_flags, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flagsall, PACK,
                 (LPTX_particle_flags, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flagsall, PACK,
                 (LPTX_particle_flags, flags));

/*
 * Consumes LPTX_particle_flags_N elements (i.e., as-is stored in memory) of
 * geom_bitarray_element_type per particle.
 *
 * @note The geom_bitarray_element_type is unsuitable for interchange through a
 * file (e.g., because of endianness) since the size of
 * geom_bitarray_element_type is often larger than 1 byte.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall_g, SNGL,
                 (geom_bitarray_element_type, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall_g, SNGL,
                 (geom_bitarray_element_type, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flagsall_g, PACK,
                 (geom_bitarray_element_type, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flagsall_g, PACK,
                 (geom_bitarray_element_type, flags));

/*
 * Consumes LPTX_PTFLAG_MAX elements (i.e., element per flag) of char type per
 * particle.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall_c, SNGL, (char, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall_c, SNGL, (char, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flagsall_c, PACK, (char, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flagsall_c, PACK, (char, flags));

/*
 * Extract or Set given bit
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_b, SNGL,
                 (LPTX_particle_flag, SCL, bit), (LPTX_bool, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_b, SNGL,
                 (LPTX_particle_flag, SCL, bit), (LPTX_bool, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flag_b, PACK, (LPTX_particle_flag, SCL, bit),
                 (LPTX_bool, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flag_b, PACK, (LPTX_particle_flag, SCL, bit),
                 (LPTX_bool, flags));

LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_c, SNGL,
                 (LPTX_particle_flag, SCL, bit), (char, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_c, SNGL,
                 (LPTX_particle_flag, SCL, bit), (char, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flag_c, PACK, (LPTX_particle_flag, SCL, bit),
                 (char, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flag_c, PACK, (LPTX_particle_flag, SCL, bit),
                 (char, flags));

LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_i, SNGL,
                 (LPTX_particle_flag, SCL, bit), (int, flags));
LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_i, SNGL,
                 (LPTX_particle_flag, SCL, bit), (int, flags));
LPTX_DECL_SET_FN(LPTX_set_particle_flag_i, PACK, (LPTX_particle_flag, SCL, bit),
                 (int, flags));
LPTX_DECL_GET_FN(LPTX_get_particle_flag_i, PACK, (LPTX_particle_flag, SCL, bit),
                 (int, flags));

/* set/get particle allocation flag */
LPTX_DECL_SET_FN(LPTX_particle_set_set_used, SNGL, (LPTX_bool, is_used));
LPTX_DECL_GET_FN(LPTX_particle_set_get_used, SNGL, (LPTX_bool, is_used));
LPTX_DECL_SET_FN(LPTX_set_particle_used, PACK, (LPTX_bool, is_used));
LPTX_DECL_GET_FN(LPTX_get_particle_used, PACK, (LPTX_bool, is_used));

LPTX_DECL_SET_FN(LPTX_particle_set_set_used_i, SNGL, (int, is_used));
LPTX_DECL_GET_FN(LPTX_particle_set_get_used_i, SNGL, (int, is_used));
LPTX_DECL_SET_FN(LPTX_set_particle_used_i, PACK, (int, is_used));
LPTX_DECL_GET_FN(LPTX_get_particle_used_i, PACK, (int, is_used));

/* set/get particle exit flag */
LPTX_DECL_SET_FN(LPTX_particle_set_set_exited, SNGL, (LPTX_bool, is_exited));
LPTX_DECL_GET_FN(LPTX_particle_set_get_exited, SNGL, (LPTX_bool, is_exited));
LPTX_DECL_SET_FN(LPTX_set_particle_exited, PACK, (LPTX_bool, is_exited));
LPTX_DECL_GET_FN(LPTX_get_particle_exited, PACK, (LPTX_bool, is_exited));

LPTX_DECL_SET_FN(LPTX_particle_set_set_exited_i, SNGL, (int, is_exited));
LPTX_DECL_GET_FN(LPTX_particle_set_get_exited_i, SNGL, (int, is_exited));
LPTX_DECL_SET_FN(LPTX_set_particle_exited_i, PACK, (int, is_exited));
LPTX_DECL_GET_FN(LPTX_get_particle_exited_i, PACK, (int, is_exited));

/* set/get particle vector element (all components) */

/**
 * Consumes @p number_of_components elements per particle. While the @p size is
 * the size of @p array, @p size / @p number_of_components particles starting
 * from @p start will be assigned or retrieved.
 *
 * If @p number_of_components is greater than the @p set has, 0 will be assigned
 * for the @p array on get, and they will be ignored on set.
 *
 * If @p number_of_components is less than the @p set has, first @p
 * number_of_components is set or get, and further components will be left
 * unchanged (on set).
 *
 * If the @p set does not have a vector at the @p vector_index, this function
 * treats it like a zero-length vector.
 *
 * To avoid the second condition, use
 * LPTX_param_get_global_particle_vector_size() (global maximum),
 * LPTX_param_get_allocated_particle_vector_size() (process-local maximum) or
 * LPTX_particle_set_vector_sizes()[vector_index] (set-local) to choose the best
 * @p number_of_components.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_t_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_t_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_t_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_t_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components));

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_d_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_d_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_d_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_d_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components));

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_f_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_f_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_f_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_f_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components));

/**
 * Array of pointer to array of each component data. The size of component
 * pointer array should be least @p number_of_components sized. Thie size of
 * each data arrays should be least @p size sized.
 *
 * If @p number_of_components is greater than the @p set has, 0 will be assigned
 * for the @p array on get, and they will be ignored on set.
 *
 * If @p number_of_components is less than the @p set has, first @p
 * number_of_components is set or get, and further componets will be left
 * unchanged (on set).
 *
 * If the @p set does not have a vector at the @p vector_index, this function
 * treats it like a zero-length vector.
 *
 * To avoid the second condition, use
 * LPTX_param_get_global_particle_vector_size() (global maximum),
 * LPTX_param_get_allocated_particle_vector_size() (process-local maximum) or
 * LPTX_particle_set_vector_sizes()[vector_index] (set-local) to choose the best
 * @p number_of_components.
 *
 * While it's hard to cast around `LPTX_type *const *` or `const LPTX_type
 * *const *` types, `LPTX_type **` and `const LPTX_type **` instead is used.
 * However, the function in/out scheme means that.
 */
LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_t_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_t_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_t_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_t_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components));

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_d_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_d_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_d_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_d_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components));

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_f_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_f_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_SET_FN(LPTX_set_particle_vector_f_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components));
LPTX_DECL_GET_FN(LPTX_get_particle_vector_f_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components));

/* set fluid index from fluid grid */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index, SRECTT);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index, PRECTT);

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_f, SRECTF);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_f, PRECTF);

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_d, SRECTD);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_d, PRECTD);

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_s, SRECT);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_s, PRECT);

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_c, SFLCI);
LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_c, PFLCI);

/**
 * @brief Compute MPI rank for
 *
 * The rank locating algorithm needs to be implemented in @p func.
 *
 * @sa LPTcst in LPT module
 *
 * Since this function is just a framework, so it's always available.
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_calc_mpirank_c(LPTX_particle_set *set,
                                             LPTX_idtype start,
                                             int size_of_rank, int *rank,
                                             LPTX_cb_mpirank *func, void *arg);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_set_calc_mpirank_rect_v(
  LPTX_particle_set *set, LPTX_idtype start, int size_of_rank, int *rank,
  int nproc, const LPTX_vector *lb, const LPTX_vector *ut, const int *flgs);

JUPITER_LPTX_DECL_END

#endif
