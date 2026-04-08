#ifndef JUPITER_RANDOM_H
#define JUPITER_RANDOM_H

/**
 * Based on xoshiro256+ written in 2018 by David Blackman and
 * Sebastiano Vigna (vigna@acm.org), and modified.
 *
 * Original source can be found in:
 * <https://prng.di.unimi.it/xoshiro256plus.c>
 *
 * This is xoshiro256+ 1.0, our best and fastest generator for
 * floating-point numbers. We suggest to use its upper bits for
 * floating-point generation, as it is slightly faster than
 * xoshiro256++/xoshiro256**. It passes all tests we are aware of
 * except for the lowest three bits, which might fail linearity tests
 * (and just those), so if low linear complexity is not considered an
 * issue (as it is usually the case) it can be used to generate 64-bit
 * outputs, too.
 *
 * We suggest to use a sign test to extract a random Boolean value,
 * and right shifts to extract subsets of bits.
 *
 * The state must be seeded so that it is not everywhere zero. If you
 * have a 64-bit seed, we suggest to seed a splitmix64 generator and
 * use its output to fill s
 */

/*
 * N.B. gfortran uses (maybe in recent versions) xoroshiro256** (256 bit seed).
 *      Intel Fortran uses Pierre L'ecuyer's method[1988] (64 bit seed)
 *      PGI Fortran uses unknown method (1088 bit seed)
 *      In C, rand() is usually implemented with linear congruential method.
 *      In C++ (C++11 or later), usually we may choose MT19937 (32 bit seed).
 *
 *      Also, the seed is initalized in random in gfortran's random_number().
 */

#include "random_base.h"
#include "defs.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#ifdef JUPITER_RANDOM_MPI
#include <mpi.h>
#endif

JUPITER_RANDOM_DECL_START

/**
 * @brief Fills seed with random value
 */
JUPITER_RANDOM_DECL
void jupiter_random_seed_fill_random(jupiter_random_seed *seed);

static inline uint64_t jupiter_random_rotl(const uint64_t x, int k)
{
  return (x << k) | (x >> (64 - k));
}

static inline uint64_t jupiter_random_nexti(jupiter_random_seed *seed)
{
  uint64_t *s = seed->seed;
  const uint64_t result = s[0] + s[3];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = jupiter_random_rotl(s[3], 45);

  return result;
}

static inline void jupiter_random_jump_common(jupiter_random_seed *seed,
                                              const uint64_t *jump_array,
                                              size_t njump_array)
{

  uint64_t s0 = 0;
  uint64_t s1 = 0;
  uint64_t s2 = 0;
  uint64_t s3 = 0;

  uint64_t *s = seed->seed;

  for (int i = 0; i < njump_array; ++i) {
    for (int b = 0; b < 64; b++) {
      if (jump_array[i] & UINT64_C(1) << b) {
        s0 ^= s[0];
        s1 ^= s[1];
        s2 ^= s[2];
        s3 ^= s[3];
      }
      jupiter_random_nexti(seed);
    }
  }

  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
}

/*
 * ref: http://peteroupc.github.io/jump.html
 */

/**
 * Jumps random seed ahead 2^32 steps.
 */
static inline void jupiter_random_jump32(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0x58120d583c112f69,
    0x7d8d0632bd08e6ac,
    0x214fafc0fbdbc208,
    0x0e055d3520fdb9d7,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^48 steps.
 */
static inline void jupiter_random_jump48(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0xf11fb4faea62c7f1,
    0xf825539dee5e4763,
    0x474579292f705634,
    0x5f728be2c97e9066,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^64 steps.
 */
static inline void jupiter_random_jump64(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0xb13c16e8096f0754,
    0xb60d6c5b8c78f106,
    0x34faff184785c20a,
    0x12e4a2fbfc19bff9,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^96 steps.
 */
static inline void jupiter_random_jump96(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0x148c356c3114b7a9,
    0xcdb45d7def42c317,
    0xb27c05962ea56a13,
    0x31eebb6c82a9615f,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^128 steps.
 */
static inline void jupiter_random_jump128(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0x180ec6d33cfd0aba,
    0xd5a61266f0c9392c,
    0xa9582618e03fc9aa,
    0x39abdc4529b1661c,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^160 steps.
 */
static inline void jupiter_random_jump160(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0xc04b4f9c5d26c200,
    0x69e6e6e431a2d40b,
    0x4823b45b89dc689c,
    0xf567382197055bf0,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^192 steps.
 */
static inline void jupiter_random_jump192(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0x76e15d3efefdcbbf,
    0xc5004e441c522fb3,
    0x77710069854ee241,
    0x39109bb02acbe635,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

/**
 * Jumps random seed ahead 2^224 steps
 */
static inline void jupiter_random_jump224(jupiter_random_seed *seed)
{
  static const uint64_t JUMP[] = {
    0x0c7840cbc3b121ad,
    0xd317530723ab526a,
    0xf31d2e03157bc387,
    0xa2b5d83a373c7ac2,
  };
  jupiter_random_jump_common(seed, JUMP, sizeof(JUMP) / sizeof(*JUMP));
}

#if !defined(FLT_RADIX) || FLT_RADIX != 2
#error Only supports floating-point values with radix 2
#endif

#if !defined(FLT_MANT_DIG) || FLT_MANT_DIG > 64
#error Big floating point number is not supported
#endif

#if !defined(DBL_MANT_DIG) || DBL_MANT_DIG > 64
#error Big floating point number is not supported
#endif

/**
 * @return random value
 *
 * Returns an uniformly distributed random value between [0, 1)
 */
static inline float jupiter_random_nextf(jupiter_random_seed *seed)
{
  uint64_t u = jupiter_random_nexti(seed);
  enum consts
  {
    mant_shift = 64 - FLT_MANT_DIG
  };
  float f = (float)(u >> mant_shift);
  return f / (float)(UINT64_C(1) << FLT_MANT_DIG);
}

/**
 * @return random value
 *
 * Returns an uniformly distributed random value between [0, 1)
 */
static inline double jupiter_random_nextd(jupiter_random_seed *seed)
{
  uint64_t u = jupiter_random_nexti(seed);
  enum consts
  {
    mant_shift = 64 - DBL_MANT_DIG
  };
  double f = (double)(u >> mant_shift);
  return f / (double)(UINT64_C(1) << DBL_MANT_DIG);
}

/**
 * @return random value
 *
 * Returns random integral value between [0, n)
 *
 * Uses upper 32bit and multiply n and the right shift (to floor down).
 */
static inline uint32_t jupiter_random_nextn(jupiter_random_seed *seed,
                                            uint32_t n)
{
  uint64_t u = jupiter_random_nexti(seed);
  u >>= 32;
  u *= n;
  u >>= 32;
  return u;
}

/**
 * returns [0, 2**8) random number. This function is used for implementing
 * McFarland's Ziggurat algorithm, but can be used for general purpose.
 */
static inline uint_fast8_t jupiter_random_nexti8(jupiter_random_seed *seed)
{
  uint64_t u = jupiter_random_nexti(seed);
  return (u >> 56) & UINT8_MAX;
}

/**
 * returns [0, 2**63) random number. This function is used for implementing
 * McFarland's Ziggurat algorithm, but can be used for general purpose.
 */
static inline int64_t jupiter_random_nexti63(jupiter_random_seed *seed)
{
  uint64_t u = jupiter_random_nexti(seed);
  return (u >> 1) & INT64_MAX;
}

/**
 * returns [-2**63, 2**63) random number. This function is used for implementing
 * McFarland's Ziggurat algorithm, but can be used for general purpose.
 *
 * Just reinterpret 64bit random number as (signed) int64_t.
 */
static inline int64_t jupiter_random_nextis(jupiter_random_seed *seed)
{
  union u
  {
    uint64_t u;
    int64_t s;
  } d;
  d.u = jupiter_random_nexti(seed);
  return d.s;
}

#include "random_vectors.h"

static inline double jupiter_random__sample_x(const double *x_j, double u)
{
  /* pow(2.0, 63.0) -> 0x1.0p+63 */
  return x_j[0] * 0x1.0p+63 + (x_j[-1] - x_j[0]) * u;
}

static inline double jupiter_random__sample_y(const double *y_i, double u)
{
  return y_i[-1] * 0x1.0p+63 + (y_i[0] - y_i[-1]) * u;
}

static inline double jupiter_random__exp_overhang(jupiter_random_seed *seed,
                                                  uint_fast8_t j)
{
  const double *X_j = jupiter_random__exp_X + j;
  const double *Y_i = jupiter_random__exp_Y + j;
  int64_t iE_max = 853965788476313639;
  int64_t U_x = jupiter_random_nexti63(seed);
  int64_t U_distance = jupiter_random_nexti63(seed) - U_x;
  double x;
  if (U_distance < 0) {
    U_distance = -U_distance;
    U_x -= U_distance;
  }
  x = jupiter_random__sample_x(X_j, U_x);
  if (U_distance >= iE_max)
    return x;
  if (jupiter_random__sample_y(Y_i, 0x1.0p+63 - (U_x + U_distance)) <= exp(-x))
    return x;
  return jupiter_random__exp_overhang(seed, j);
}

static inline uint_fast8_t
jupiter_random__exp_sample_A(jupiter_random_seed *seed)
{
  uint_fast8_t j = jupiter_random_nexti8(seed);
  int64_t s = jupiter_random_nextis(seed);
  if (s >= jupiter_random__exp_A_ipmf[j])
    return jupiter_random__exp_A_map[j];
  return j;
}

static inline uint_fast8_t
jupiter_random__norm_sample_A(jupiter_random_seed *seed)
{
  uint_fast8_t j = jupiter_random_nexti8(seed);
  int64_t s = jupiter_random_nextis(seed);
  if (s >= jupiter_random__norm_A_ipmf[j])
    return jupiter_random__norm_A_map[j];
  return j;
}

/**
 * @return random value
 *
 * Returns exponentially distributed random number:
 * https://github.com/cd-mcfarland/fast_prng
 *
 * Currently we provide double version only.
 */
static inline double jupiter_random_nextde(jupiter_random_seed *seed)
{
  uint_fast8_t i_max = jupiter_random__exp_layers;
  uint_fast8_t i = jupiter_random_nexti8(seed);
  uint_fast8_t j;
  double X_0;
  if (i < i_max)
    return jupiter_random__exp_X[i] * jupiter_random_nexti63(seed);

  j = jupiter_random__exp_sample_A(seed);
  X_0 = 7.56927469415;
  if (j > 0)
    return jupiter_random__exp_overhang(seed, j);
  return X_0 + jupiter_random_nextde(seed);
}

/**
 * @return random value
 *
 * Returns normally distributed random number using McFarland's Ziggurat
 * algorithm: https://github.com/cd-mcfarland/fast_prng
 *
 * Currently we provide double version only.
 */
static inline double jupiter_random_nextdn(jupiter_random_seed *seed)
{
  uint_fast8_t i_max = jupiter_random__norm_bins;
  uint_fast8_t i = jupiter_random_nexti8(seed);
  int64_t U_1;
  double sign_bit;
  uint_fast8_t j;
  int64_t U_diff;
  int64_t max_iE = 2269182951627975918, min_iE = 760463704284035181;
  double X_0 = 3.6360066255;
  uint_fast8_t j_inflection = 204;
  double x;
  const double *X_j, *Y_i;

  if (i < i_max)
    return jupiter_random__norm_X[i] * jupiter_random_nextis(seed);

  U_1 = jupiter_random_nexti63(seed);
  sign_bit = (U_1 & 0x100) ? 1.0 : -1.0;
  j = jupiter_random__norm_sample_A(seed);
  X_j = jupiter_random__norm_X + j;
  Y_i = jupiter_random__norm_Y + j;
  if (j > j_inflection) {
    for (;;) {
      x = jupiter_random__sample_x(X_j, U_1);
      U_diff = jupiter_random_nexti63(seed) - U_1;
      if (U_diff >= 0)
        break;
      if (U_diff >= -max_iE &&
          jupiter_random__sample_y(Y_i, 0x1.0p+63 - (U_1 + U_diff)) <
            exp(-0.5 * x * x))
        break;
      U_1 = jupiter_random_nexti63(seed);
    }
  } else if (j == 0) {
    do {
      x = pow(X_0, -1.0) * jupiter_random_nextde(seed);
    } while (jupiter_random_nextde(seed) < 0.5 * x * x);
    x += X_0;
  } else if (j < j_inflection) {
    for (;;) {
      U_diff = jupiter_random_nexti63(seed) - U_1;
      if (U_diff < 0) {
        U_diff = -U_diff;
        U_1 -= U_diff;
      }
      x = jupiter_random__sample_x(X_j, U_1);
      if (U_diff > min_iE)
        break;
      if (jupiter_random__sample_y(Y_i, 0x1.0p+63 - (U_1 + U_diff)) <
          exp(-0.5 * x * x))
        break;
      U_1 = jupiter_random_nexti63(seed);
    }
  } else {
    for (;;) {
      x = jupiter_random__sample_x(X_j, U_1);
      if (jupiter_random__sample_y(Y_i, jupiter_random_nextis(seed)) <
          exp(-0.5 * x * x))
        break;
      U_1 = jupiter_random_nexti63(seed);
    }
  }
  return sign_bit * x;
}

/**
 * Seed with counter for syncing between threads and/or MPI ranks
 */
struct jupiter_random_seed_counter
{
  jupiter_random_seed seed;
  uint64_t counter;
};
typedef struct jupiter_random_seed_counter jupiter_random_seed_counter;

static inline uint64_t jupiter_random_nextic(jupiter_random_seed_counter *sc)
{
  sc->counter += 1;
  return jupiter_random_nexti(&sc->seed);
}

static inline float jupiter_random_nextfc(jupiter_random_seed_counter *sc)
{
  sc->counter += 1;
  return jupiter_random_nextf(&sc->seed);
}

static inline double jupiter_random_nextdc(jupiter_random_seed_counter *sc)
{
  sc->counter += 1;
  return jupiter_random_nextd(&sc->seed);
}

static inline uint32_t jupiter_random_nextnc(jupiter_random_seed_counter *sc,
                                             uint32_t n)
{
  sc->counter += 1;
  return jupiter_random_nextn(&sc->seed, n);
}

static inline void jupiter_random_jump32c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump32(&sc->seed);
  sc->counter += UINT64_C(1) << 32;
}

static inline void jupiter_random_jump48c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump32(&sc->seed);
  sc->counter += UINT64_C(1) << 48;
}

static inline void jupiter_random_jump64c(jupiter_random_seed_counter *sc)
{
  /* The counter will overflow to same value */
  jupiter_random_jump64(&sc->seed);
}

static inline void jupiter_random_jump96c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump96(&sc->seed);
}

static inline void jupiter_random_jump128c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump128(&sc->seed);
}

static inline void jupiter_random_jump160c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump160(&sc->seed);
}

static inline void jupiter_random_jump192c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump192(&sc->seed);
}

static inline void jupiter_random_jump224c(jupiter_random_seed_counter *sc)
{
  jupiter_random_jump224(&sc->seed);
}

#define JUPITER_RANDOM_DEFINE_SEP_SEMICOLON ;
#define JUPITER_RANDOM_DEFINE_SEP_NONE
#define JUPITER_RANDOM_DEFINE_SEP(s) JUPITER_RANDOM_DEFINE_SEP_##s
#define JUPITER_RANDOM_DEFINE_JUMPS(f, s, ...)                       \
  f(32, __VA_ARGS__) JUPITER_RANDOM_DEFINE_SEP(s) f(48, __VA_ARGS__) \
    JUPITER_RANDOM_DEFINE_SEP(s) f(64, __VA_ARGS__)                  \
      JUPITER_RANDOM_DEFINE_SEP(s) f(96, __VA_ARGS__)                \
        JUPITER_RANDOM_DEFINE_SEP(s) f(128, __VA_ARGS__)             \
          JUPITER_RANDOM_DEFINE_SEP(s) f(160, __VA_ARGS__)           \
            JUPITER_RANDOM_DEFINE_SEP(s) f(192, __VA_ARGS__)         \
              JUPITER_RANDOM_DEFINE_SEP(s) f(224, __VA_ARGS__)

#ifdef _OPENMP
/**
 * Synchronize thread-local seed counter @p local via @p shared storage,
 * (make *shared == *local with maximum counter in local)
 *
 * Assumes they consumes less than 2^64 random numbers for all @p
 * local seeds. This fuction will never include the use of jumps.
 *
 * The counter will be reset to 0.
 */
static inline void jupiter_random_syncc_omp(jupiter_random_seed_counter *shared,
                                            jupiter_random_seed_counter *local)
{
  int copied = 0;
  uint64_t cnt;

#pragma omp single
  {
    shared->counter = local->counter;
    copied = 1;
  }
#pragma omp barrier

  if (!copied) {
#pragma omp atomic read
    cnt = shared->counter;
    if (cnt < local->counter) {
#pragma omp critical
      {
        if (shared->counter < local->counter)
          shared->counter = local->counter;
      }
    }
  }
#pragma omp barrier
#pragma omp atomic read
  cnt = shared->counter;

  if (cnt == 0 && copied) {
    *shared = *local;
  } else if (cnt == local->counter) {
#pragma omp critical
    {
      if (shared->counter == local->counter) {
        *shared = *local;
        shared->counter = UINT64_C(0);
      }
    }
  }
#pragma omp barrier

#pragma omp critical
  *local = *shared;
}

static inline void
jupiter_random_jump_omp_common(void (*const copy_jump_func)(int, void *arg),
                               void *arg)
{
  if (omp_get_num_threads() > 1) {
#pragma omp barrier
    for (int tid = 0; tid < omp_get_num_threads(); ++tid) {
      if (tid == omp_get_thread_num())
        copy_jump_func(tid, arg);
#pragma omp barrier
    }
  } else {
    copy_jump_func(-1, arg);
  }
}

struct jupiter_random_jump_omp_args
{
  jupiter_random_seed *shared;
  jupiter_random_seed *local;
  void (*const jumper)(jupiter_random_seed *);
};

static inline void jupiter_random_jump_omp_copy_jump(int n, void *args)
{
  struct jupiter_random_jump_omp_args *a = args;
  if (n > 0)
    a->jumper(a->shared);
  *a->local = *a->shared;
}

/**
 * @brief Using jump to generate different random sequence for each
 *        threads.
 * @param shared Thread-shared seed value (source)
 * @param local  Thread-local seed value (output)
 * @param jumper Jumping function (e.g. jupiter_random_jump64)
 *
 * Call this function in a parallelized section.
 */
static inline void
jupiter_random_jump_omp(jupiter_random_seed *shared, jupiter_random_seed *local,
                        void (*const jumper)(jupiter_random_seed *))
{
  struct jupiter_random_jump_omp_args args = {.shared = shared,
                                              .local = local,
                                              .jumper = jumper};
  jupiter_random_jump_omp_common(jupiter_random_jump_omp_copy_jump, &args);
}

#define JUPITER_RANDOM_JUMP_OMP(n, x)                                          \
  static inline void jupiter_random_jump##n##_omp(jupiter_random_seed *shared, \
                                                  jupiter_random_seed *local)  \
  {                                                                            \
    jupiter_random_jump_omp(shared, local, jupiter_random_jump##n);            \
  }

JUPITER_RANDOM_DEFINE_JUMPS(JUPITER_RANDOM_JUMP_OMP, NONE, 1)
#undef JUPITER_RANDOM_JUMP_OMP

struct jupiter_random_jumpc_omp_args
{
  jupiter_random_seed_counter *shared;
  jupiter_random_seed_counter *local;
  void (*const jumper)(jupiter_random_seed_counter *);
};

static inline void jupiter_random_jumpc_omp_copy_jump(int n, void *args)
{
  struct jupiter_random_jumpc_omp_args *a = args;
  if (n > 0)
    a->jumper(a->shared);
  *a->local = *a->shared;
}

/**
 * @brief Using jump to generate different random sequence for each
 *        threads.
 * @param shared Thread-shared seed value (source)
 * @param local  Thread-local seed value (output)
 * @param jumper Jumping function (e.g. jupiter_random_jump64c)
 *
 * Call this function in a parallelized section.
 */
static inline void
jupiter_random_jumpc_omp(jupiter_random_seed_counter *shared,
                         jupiter_random_seed_counter *local,
                         void (*const jumper)(jupiter_random_seed_counter *))
{
  struct jupiter_random_jumpc_omp_args args = {.shared = shared,
                                               .local = local,
                                               .jumper = jumper};
  jupiter_random_jump_omp_common(jupiter_random_jumpc_omp_copy_jump, &args);
}

#define JUPITER_RANDOM_JUMP_OMP(n, x)                                        \
  static inline void jupiter_random_jump##n##c_omp(                          \
    jupiter_random_seed_counter *shared, jupiter_random_seed_counter *local) \
  {                                                                          \
    jupiter_random_jumpc_omp(shared, local, jupiter_random_jump##n##c);      \
  }

JUPITER_RANDOM_DEFINE_JUMPS(JUPITER_RANDOM_JUMP_OMP, NONE, 1)
#undef JUPITER_RANDOM_JUMP_OMP

#endif

#ifdef JUPITER_RANDOM_MPI
static inline int jupiter_random_mpi_create_type(
  int count, const int block_lengths[], const MPI_Aint displacements[],
  const MPI_Datatype array_of_types[], size_t ub, MPI_Datatype *new_type)
{
  MPI_Datatype type;
  int r;
  r = MPI_Type_create_struct(count, block_lengths, displacements,
                             array_of_types, &type);
  if (r != MPI_SUCCESS)
    return r;

  r = MPI_Type_create_resized(type, 0, ub, new_type);

  MPI_Type_free(&type);
  if (r != MPI_SUCCESS)
    return r;

  r = MPI_Type_commit(new_type);
  if (r != MPI_SUCCESS) {
    MPI_Type_free(new_type);
    return r;
  }

  return r;
}

static inline int jupiter_random_mpi_seed_type(MPI_Datatype *new_type)
{
  const int block_lengths[] = {JUPITER_RANDOM_SEED_SIZE};
  const MPI_Aint displacements[] = {offsetof(jupiter_random_seed, seed)};
  const MPI_Datatype array_of_types[] = {MPI_UINT64_T};
  enum consts
  {
    block_lengths_size = sizeof(block_lengths) / sizeof(*block_lengths)
  };

  return jupiter_random_mpi_create_type(block_lengths_size, block_lengths,
                                        displacements, array_of_types,
                                        sizeof(jupiter_random_seed), new_type);
}

static inline int jupiter_random_mpi_seed_counter_type(MPI_Datatype *new_type)
{
  const int block_lengths[] = {JUPITER_RANDOM_SEED_SIZE, 1};
  const MPI_Aint displacements[] = {offsetof(jupiter_random_seed_counter,
                                             seed.seed),
                                    offsetof(jupiter_random_seed_counter,
                                             counter)};
  const MPI_Datatype array_of_types[] = {MPI_UINT64_T, MPI_UINT64_T};
  enum consts
  {
    block_lengths_size = sizeof(block_lengths) / sizeof(*block_lengths)
  };

  return jupiter_random_mpi_create_type(block_lengths_size, block_lengths,
                                        displacements, array_of_types,
                                        sizeof(jupiter_random_seed_counter),
                                        new_type);
}
#endif

JUPITER_RANDOM_DECL_END

#endif
