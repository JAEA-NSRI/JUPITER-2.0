/**
 * @file lptx/defs.h
 *
 * New LPT module
 *
 * The LPTX's API is not compatible with original LPT.
 */
#ifndef JUPITER_LPTX_DEFS_H
#define JUPITER_LPTX_DEFS_H

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
#define JUPITER_LPTX_DECL_START extern "C" {
#define JUPITER_LPTX_DECL_END }
#else
#define JUPITER_LPTX_DECL_START
#define JUPITER_LPTX_DECL_END
#endif

JUPITER_LPTX_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_LPTX_EXPORT)
#define JUPITER_LPTX_DECL __declspec(dllexport)
#elif defined(JUPITER_LPTX_IMPORT)
#define JUPITER_LPTX_DECL __declspec(dllimport)
#else
#define JUPITER_LPTX_DECL
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_LPTX_EXPORT) || defined(JUPITER_LPTX_IMPORT)
#define JUPITER_LPTX_DECL __attribute__((visibility("default")))
#else
#define JUPITER_LPTX_DECL
#endif
#else
#define JUPITER_LPTX_DECL
#endif

#if defined(JUPITER_LPTX_DOUBLE)
typedef double LPTX_type;
#define LPTX__math(math_func) math_func
#define LPTX__C(value) value
#define LPTX_F(cst) cst
#define LPTX__TYPE_PREFIX DBL_
#define LPTX_MPI_TYPE MPI_DOUBLE
#elif defined(JUPITER_LPTX_SINGLE)
typedef float LPTX_type;
#define LPTX__math(math_func) math_func##f
#define LPTX__C(value) value##f
#define LPTX_F(cst) cst##F
#define LPTX_MPI_TYPE MPI_FLOAT
#define LPTX__TYPE_PREFIX FLT_
#else
#error one of JUPITER_LPTX_DOUBLE or JUPITER_LPTX_SINGLE must be defined
#endif
#ifndef JUPITER_LPTX_MPI
#undef LPTX_MPI_TYPE
#endif

#define LPTX__CONCAT(p, q) p##q
#define LPTX__ECONCAT(p, q) LPTX__CONCAT(p, q)

#define LPTX_math(m) LPTX__math(m)
#define LPTX_C(c) LPTX__C(c)

#ifdef M_PI
#define LPTX_M_PI LPTX_C(M_PI)
#else
#define LPTX_M_PI LPTX_C(3.14159265358979323846)
#endif

#define LPTX_TYPE_RADIX FLT_RADIX
#define LPTX_TYPE_DECIMAL_DIG DECIMAL_DIG
#define LPTX_TYPE_MANT_DIG LPTX__ECONCAT(LPTX__TYPE_PREFIX, MANT_DIG)
#define LPTX_TYPE_DIG LPTX__ECONCAT(LPTX__TYPE_PREFIX, DIG)
#define LPTX_TYPE_MIN_EXP LPTX__ECONCAT(LPTX__TYPE_PREFIX, MIN_EXP)
#define LPTX_TYPE_MAX_EXP LPTX__ECONCAT(LPTX__TYPE_PREFIX, MAX_EXP)
#define LPTX_TYPE_MIN_10_EXP LPTX__ECONCAT(LPTX__TYPE_PREFIX, MIN_10_EXP)
#define LPTX_TYPE_MAX_10_EXP LPTX__ECONCAT(LPTX__TYPE_PREFIX, MAX_10_EXP)
#define LPTX_TYPE_MIN LPTX__ECONCAT(LPTX__TYPE_PREFIX, MIN)
#define LPTX_TYPE_MAX LPTX__ECONCAT(LPTX__TYPE_PREFIX, MAX)
#define LPTX_TYPE_EPSILON LPTX__ECONCAT(LPTX__TYPE_PREFIX, EPSILON)
#define LPTX_TYPE_HUGE_VAL LPTX_F(HUGE_VAL)

#ifdef JUPITER_LPTX_MPI
typedef MPI_Aint LPTX_idtype;
/*
 * MPI_Aint is defined as an integral type which can store any pointer value.
 * So it is possibly larger than intptr_t. In this case, API allows setting
 * values greater than LPTX_IDTYPE_MAX.
 */
#define LPTX_IDTYPE_MIN INTPTR_MIN
#define LPTX_IDTYPE_MAX INTPTR_MAX
#else
typedef ptrdiff_t LPTX_idtype;
#define LPTX_IDTYPE_MIN PTRDIFF_MIN
#define LPTX_IDTYPE_MAX PTRDIFF_MAX
#endif
typedef int LPTX_int;
#define LPTX_INT_MIN INT_MIN
#define LPTX_INT_MAX INT_MAX

typedef unsigned int LPTX_uint;
#define LPTX_UINT_MAX UINT_MAX

enum LPTX_bool
{
  LPTX_false = 0,
  LPTX_true = 1,
};
typedef enum LPTX_bool LPTX_bool;

#ifdef JUPITER_LPTX_MPI
#define LPTX_MPI_TYPE_ID MPI_AINT
#define LPTX_MPI_TYPE_INT MPI_INT
#define LPTX_MPI_TYPE_UINT MPI_UNSIGNED
#endif

enum LPTX_time_scheme
{
  LPTX_TIME_SCHEME_INVALID = -1,
  LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2 = 0, ///< Second-order Adams-Bashforth
  LPTX_TIME_SCHEME_RUNGE_KUTTA_2,         ///< Second-order Runge-Kutta
  LPTX_TIME_SCHEME_RUNGE_KUTTA_3, ///< Third-order low-storage Runge-Kutta
};
typedef enum LPTX_time_scheme LPTX_time_scheme;

enum LPTX_heat_scheme
{
  LPTX_HEAT_INVALID = -1,
  LPTX_HEAT_OFF = 0,       ///< No heat exchange
  LPTX_HEAT_FOLLOW_FLUID,  ///< Just follow fluid temperature
  LPTX_HEAT_RANZ_MARSHALL, ///< Ranz-Marshall model
};
typedef enum LPTX_heat_scheme LPTX_heat_scheme;

/**
 * Particle origin ID
 *
 * Defines the origin of particles. While LPTX particle module does not provide
 * any source for this, values shall be defined by uesr.
 */
enum LPTX_origin_id
{
  LPTX_ORIGIN_UNDEFINED = 0,
  ///< No origin defined (default for fresh new particles)
};

/**
 * Particle state flags
 *
 * @note JUPITER (main code) stores flags to the file in this order. Do not
 * reorder (reindex) each enum values and always add to the end (before
 * LPTX_PTFLAG_MAX).
 */
enum LPTX_particle_flag
{
  LPTX_PT_IS_USED = 0,    ///< Particle storage is used or free
  LPTX_PT_IS_EXITED,      ///< Particle is exited or tracked
  LPTX_PT_IS_COLLIDED,    ///< Particle is collided to another particle
  LPTX_PT_IS_IN_GAS,      ///< Particle is in gaseous region (unused)
  LPTX_PT_IS_IN_LIQUID,   ///< Particle is in liquidus region (unused)
  LPTX_PT_IS_IN_SOLID,    ///< Particle is in solid or wall region (usused)
  LPTX_PT_CAN_COLLIDE,    ///< Paritcle is available for collision
  LPTX_PT_CAN_EVAPORATE,  ///< Particle is available for evaporation
  LPTX_PT_CAN_CONDENSATE, ///< Particle is available for condensation

  LPTX_PTFLAG_MAX,
};
typedef enum LPTX_particle_flag LPTX_particle_flag;

enum LPTX_particle_vectors
{
  LPTX_VI_MASS_FRACTION = 0, ///< Particle mass fractions
  LPTX_NUM_VECTORS,
};
typedef enum LPTX_particle_vectors LPTX_particle_vectors;

#ifdef JUPITER_LPTX_MPI
enum LPTX_MPI_errors
{
  LPTX_MPI_ERR_BASE = MPI_ERR_LASTCODE,
  LPTX_MPI_ERR_NO_MEM,               ///< Allocation failed
  LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH, ///< Vectors in particle sets mismatched
  LPTX_MPI_ERR_REMOTE,               ///< Error detected on remote rank
  LPTX_MPI_ERR_OVERFLOW,             ///< Overflow occured

  LPTX_MPI_ERR_LASTCODE, ///< Last error code
};
typedef enum LPTX_MPI_errors LPTX_MPI_errors;

enum LPTX_MPI_max_constants
{
  LPTX_MPI_MAX_ERROR_STRING_BASE = 256,
  LPTX_MPI_MAX_ERROR_STRING =
    (MPI_MAX_ERROR_STRING < LPTX_MPI_MAX_ERROR_STRING_BASE)
      ? LPTX_MPI_MAX_ERROR_STRING_BASE
      : MPI_MAX_ERROR_STRING, ///< Maximum length from LPTX_MPI_Error_string()
};
#endif

/* time-integration coefficients */
struct LPTX_pcoef;
typedef struct LPTX_pcoef LPTX_pcoef;

struct LPTX_single_time_integrate_data;
typedef struct LPTX_single_time_integrate_data LPTX_single_time_integrate_data;

struct LPTX_particle_vector;
typedef struct LPTX_particle_vector LPTX_particle_vector;

struct LPTX_particle;
typedef struct LPTX_particle LPTX_particle;

struct LPTX_particle_data;
typedef struct LPTX_particle_data LPTX_particle_data;

union LPTX_particle_storage;
typedef union LPTX_particle_storage LPTX_particle_storage;

struct LPTX_particle_set;
typedef struct LPTX_particle_set LPTX_particle_set;

struct LPTX_particle_init_set;
typedef struct LPTX_particle_init_set LPTX_particle_init_set;

struct LPTX_param;
typedef struct LPTX_param LPTX_param;

struct LPTX_vector;
typedef struct LPTX_vector LPTX_vector;

struct LPTX_rectilinear_grid;
typedef struct LPTX_rectilinear_grid LPTX_rectilinear_grid;

struct LPTX_rectilinear_scalar;
typedef struct LPTX_rectilinear_scalar LPTX_rectilinear_scalar;

struct LPTX_rectilinear_vector;
typedef struct LPTX_rectilinear_vector LPTX_rectilinear_vector;

struct LPTX_particle_stat;
typedef struct LPTX_particle_stat LPTX_particle_stat;

struct LPTX_particle_flags;
typedef struct LPTX_particle_flags LPTX_particle_flags;

struct LPTX_collision_list_data;
typedef struct LPTX_collision_list_data LPTX_collision_list_data;

struct LPTX_collision_list;
typedef struct LPTX_collision_list LPTX_collision_list;

struct LPTX_collision_list_set;
typedef struct LPTX_collision_list_set LPTX_collision_list_set;

/* empty token (internally used) */
#define LPTX_empty

/* comma (internally used) */
#define LPTX_comma ,

#ifdef JUPITER_LPTX_MPI
#define LPTX_MPI_data_types_2(mpi_type, ...) mpi_type
#define LPTX_MPI_data_types_3(mpi_type, ...) mpi_type
#define LPTX_MPI_data_types_4(mpi_type, ...) mpi_type
#define LPTX_MPI_data_displs_2(mpi_type, c_type) sizeof(c_type)
#define LPTX_MPI_data_displs_3(mpi_type, c_type, mem) offsetof(c_type, mem)
#define LPTX_MPI_data_displs_4(mpi_type, c_type, mem, l) offsetof(c_type, mem)
#define LPTX_MPI_data_len_2(mpi_type, ...) 0
#define LPTX_MPI_data_len_3(mpi_type, ...) 1
#define LPTX_MPI_data_len_4(mpi_type, c_type, mem, len) len
#define LPTX_MPI_data_p(_1, _2, _3, _4, N, ...) N
#define LPTX_MPI_data_n(...) LPTX_MPI_data_p(__VA_ARGS__, 4, 3, 2, 1, 0)
#define LPTX_MPI_data_v(n, t, ...) LPTX_MPI_data_##t##_##n(__VA_ARGS__)
#define LPTX_MPI_data_e(n, ...) LPTX_MPI_data_v(n, __VA_ARGS__)

#define LPTX_MPI_data_b(types_or_displs, ...) \
  LPTX_MPI_data_e(LPTX_MPI_data_n(__VA_ARGS__), types_or_displs, __VA_ARGS__)

/**
 * @brief Generates data for MPI_Type_create_struct
 * @param types_or_displs filter key
 * @param mpi_type Corresponding MPI type.
 * @param c_type Struct type name
 * @param ...[0] Member name
 * @param ...[1] Lengths of member (1 if omitted)
 *
 * The arguemnt of @p types_or_displs must be one of:
 *
 * - `types`: Get array of `MPI_Datatype`s.
 * - `displs`: Get array of displacements.
 * - `len`: Get array of block lengths.
 *
 * ```c
 * struct your_type {
 *   int member1;
 *   char member2[10];
 *   double member3;
 * };
 * #define your_type_MPI_data(t) \
 * { \
 *   LPTX_MPI_data(t, MPI_INT, struct your_type, member1), \
 *   LPTX_MPI_data(t, MPI_CHAR, struct your_type, member2, 10), \
 *   LPTX_MPI_data(t, MPI_DOUBLE, struct your_type, member3), \
 * }
 *
 * //...
 *
 * MPI_Datatype new_type;
 * MPI_Datatype types[] = your_type_MPI_data(types);
 * MPI_Aint displs[] = your_type_MPI_data(displs);
 * int block_lengths[] = your_type_MPI_data(len);
 * int count = sizeof(types) / sizeof(*types);
 *
 * MPI_Type_create_struct(count, block_lengths, displs, types, &new_type);
 *
 * // If you want to use the struct in arrays.
 * MPI_Type_create_resized(new_type, 0, sizeof(struct your_type), &array_type);
 * ```
 *
 * Recommend to use type generator functions for LPTX types.
 */
#define LPTX_MPI_data(types_or_displs, mpi_type, c_type, ...) \
  LPTX_MPI_data_b(types_or_displs, mpi_type, c_type, __VA_ARGS__)
#endif

typedef LPTX_bool
LPTX_cb_substep_loop(LPTX_param *param,
                     LPTX_single_time_integrate_data *integrate_data,
                     void *arg);

typedef LPTX_bool LPTX_cb_foreach_init_sets(LPTX_particle_init_set *set,
                                            void *arg);
typedef LPTX_bool LPTX_cb_foreach_particle_sets(LPTX_particle_set *set,
                                                void *arg);
typedef LPTX_bool LPTX_cb_foreach_particle_range(LPTX_particle_set *set,
                                                 LPTX_idtype start,
                                                 LPTX_idtype *last, void *arg);
typedef LPTX_bool LPTX_cb_foreach_particles(LPTX_particle_data *pt, void *arg);

typedef LPTX_bool LPTX_cb_foreach_collision_data(LPTX_collision_list_data *p,
                                                 void *arg);

typedef LPTX_bool LPTX_cb_particle_if(const LPTX_particle_data *pt, void *arg);

/**
 * @brief Callback for comparison of particles for sorting
 * @retval -1 (or negative) @p a leads @p b (a < b for sorting ascending order)
 * @retval 0                @p a is equal to @p b
 * @retval 1 (or positive)  @p b leads @p a (a > b for sorting ascending order)
 *
 * This function may be called twice or more for same pair.
 *
 * @sa qsort()
 */
typedef int LPTX_cb_particle_compn(const LPTX_particle_data *a,
                                   const LPTX_particle_data *b, void *args);


/**
 * @brief Callback for comparison of particles for (binary) search
 * @retval -1 (or negative) matching item is in after @p p
 * @retval 0                @p p is matching item
 * @retval 1  (or positive) matching item is in before @p p
 *
 * This function may be called twice or more for same @p p.
 *
 * @sa bsearch()
 */
typedef int LPTX_cb_particle_comp1(const LPTX_particle_data *p, void *args);

enum LPTX_cb_fluid_index_flags
{
  LPTX_FLUID_INDEX_OK = 0x0000,            ///< Everything ok
  LPTX_FLUID_INDEX_OUT_OF_DOMAIN = 0x0001, ///< Outside of domain
};
typedef enum LPTX_cb_fluid_index_flags LPTX_cb_fluid_index_flags;

/**
 *
 */
typedef LPTX_cb_fluid_index_flags
LPTX_cb_fluid_index(void *args, LPTX_vector position, LPTX_idtype *i,
                    LPTX_idtype *j, LPTX_idtype *k);

typedef int LPTX_cb_mpirank(void *args, const LPTX_particle *p);

typedef LPTX_type LPTX_cb_fluid_variable_f(void *args, LPTX_vector position,
                                           LPTX_idtype i, LPTX_idtype j,
                                           LPTX_idtype k);

typedef LPTX_vector LPTX_cb_fluid_variable_v(void *args, LPTX_vector position,
                                             LPTX_idtype i, LPTX_idtype j,
                                             LPTX_idtype k);

typedef LPTX_type LPTX_cb_get_scalar_of(const void *array, LPTX_idtype i);

/**
 * @brief Merge comparator
 *
 * @note @p pa or @p pb may points @p outp (you have to take care of
 *       self-assignment, while particle data is not plain of data)
 *
 * @note @p outp is filled with initial value on first call for an ID,
 *       so you usually want to copy @p pa or @p pb to @p outp at least.
 */
typedef void LPTX_cb_particle_set_merge(LPTX_particle_data *outp,
                                        const LPTX_particle_data *pa,
                                        const LPTX_particle_data *pb,
                                        void *arg);

/**
 * @param number_of_new_particles
 *        Number of new particles to generate on collision [out]
 * @param a A particle to test
 * @param b Another particle to test
 * @param arg User-defined extra parameter
 * @retval LPTX_true two particles are collided
 * @retval LPTX_false two particles are not collided
 *
 * @p number_of_new_particles is 0 by default.
 *
 * @note This function is **also** called for exited particles. Please check the
 *       flags and skip (return LPTX_false) them not to collide them.
 */
typedef LPTX_bool LPTX_cb_collision_if(LPTX_idtype *number_of_new_particles,
                                       const LPTX_particle_data *a,
                                       const LPTX_particle_data *b, void *arg);

/**
 * @brief Collision calculation interface.
 * @param number_of_partners Number of particles in @p partners
 * @param partners Collision partiners (array of pointer to particle)
 * @param partner_matrix Collision matrix of partners
 * @param number_of_new_particles Number of particles in @p new_particles
 * @param new_particles New collided particle (array of particle)
 *
 * To delete particle in @p partners, set `LPTX_PT_IS_USED` flags to false. Such
 * particles will be deleted by garbege collection or similar process which
 * ignores unused particles.
 *
 * To just stop tracking and exit particles, set `LPTX_PT_IS_EXITED` flags to
 * true.
 *
 * `LPTX_PT_IS_COLLIDED` flags is used for informational purpose (usually the
 * reason of exit). It does not affect to tracking processing.
 *
 * @p number_of_new_particles is the sum of returned particles that while
 * building collision list, involving partiners.
 */
typedef void LPTX_cb_collision_func(LPTX_idtype number_of_partners,
                                    LPTX_particle_data **partners,
                                    const LPTX_bool *partner_matrix,
                                    LPTX_idtype number_of_new_particles,
                                    LPTX_particle_data *new_particles,
                                    void *arg);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LPTX_STATIC_ASSERT(cond, key, msg) \
  _Static_assert((cond), msg);             \
  typedef char LPTX__assert_##key[1]
#else
#define LPTX_STATIC_ASSERT(cond, key, msg) \
  typedef char LPTX__assert_##key[(cond) ? 1 : -1]
#endif

#ifdef _OPENMP
/**
 * Usage:
 * ```c
 * #ifdef _OPENMP
 * #pragma omp parallel if (count > LPTX_omp_small_threshold)
 * #endif
 *  {
 * #ifdef _OPENMP
 * #pragma omp for
 * #endif
 *    for (LPTX_idtype jj = 0; jj < count; ++jj)
 *      ary[jj] = 0;
 *  }
 * ```
 *
 * @note The `omp for` loop will be workshared if already in parallel. If your
 * function may be expected to be called within an already parallel region, add
 * `&& !omp_in_parallel()` to the condition of `omp parallel if()`.
 */
enum LPTX_omp_constants
{
  LPTX_omp_small_threshold = 100,
  /*!< Minimum count to work in parallel for small loops. */
  LPTX_omp_unsafe_threshold = 10,
  /*!< Minimum count to work in parallel for thread-parallelizeable part of
       thread-unsafe functions (use LPTX_small_parallel_threshold for small
       loop) */
};
#endif

JUPITER_LPTX_DECL_END

#endif
