/* This is a -*- c -*- header (not Fortran) */
/**
 * @file LPTdefs.h
 * @brief Macro and type definitions for LPT calculations
 */

#ifndef JUPITER_LPT_DEFS_H
#define JUPITER_LPT_DEFS_H

#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_LPT_EXPORT)
#define JUPITER_LPT_DECL __declspec(dllexport)
#elif defined(JUPITER_LPT_IMPORT)
#define JUPITER_LPT_DECL __declspec(dllimport)
#else
#define JUPITER_LPT_DECL
#endif
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_LPT_EXPORT) || defined(JUPITER_LPT_IMPORT)
#define JUPITER_LPT_DECL __attribute__ ((visibility ("default")))
#else
#define JUPITER_LPT_DECL
#endif
#else
#define JUPITER_LPT_DECL
#endif
#endif

#ifdef JUPITER_LPT_API_SINGLE
typedef float LPT_type;
#else
typedef double LPT_type;
#endif

/**
 * @brief Prototype for LPT error callback function
 * @param arg Passes @p arg argument passed for cLPTset_error_callback().
 * @param ierr Internal error code
 * @param len  Length of @p msg
 * @param msg  Descriptive message of an error
 *
 * @warning @p msg is *not* NUL-termianated. Make sure to use @p len argument
 * for string length.
 */
typedef void LPT_error_callback(void *arg, int ierr, int len, const char *msg);

/**
 * @brief Prototype for LPT user-defind wall treatment callback function
 * @param arg Passes @p arg argument passed for cLPTset_udwbc_callback().
 * @param ip Particle ID
 * @param pt Particle position (in order of X, Y and Z)
 * @param pt Particle velocity (in order of X, Y and Z)
 * @param pdia Particle diameter
 * @param icf Particle cell position (in order of X, Y and Z)
 * @param iwp Whether
 *
 * To implement a function, you must set `iwp` and `dl` at
 * least. Optionally, you can set `pt`, and `put`. `icf` will be
 * updated automatically in the LPT routine.
 *
 * The particle data interacting via `cLPTgetpts` or `cLPTsetpts` with
 * specified ID `ip` is *not* guaranteed to point same particle,
 * because the upstream subroutine uses arguments to passing these
 * data, not accessing module directly.
 */
typedef void LPT_udwbc_callback(void *arg, int ip, LPT_type pt[3],
                                LPT_type put[3], LPT_type pdia,
                                const int icf[3], int *iwp, LPT_type *dl);

enum LPT_time_scheme
{
  LPT_TIME_SCHEME_INVALID = 0,
  LPT_TIME_SCHEME_ADAMS_BASHFORTH_2 = 1, ///< Second-order Adams-Bashforth
  LPT_TIME_SCHEME_RUNGE_KUTTA_2 = 2, ///< Second-order Runge-Kutta
  LPT_TIME_SCHEME_RUNGE_KUTTA_3 = 3, ///< Third-order low-storage Runge-Kutta
};
typedef enum LPT_time_scheme LPT_time_scheme;

struct LPT_range
{
  LPT_type start; ///< Start point of range
  LPT_type end;   ///< End point of range
};
typedef struct LPT_range LPT_range;

struct LPT_vector
{
  LPT_type x; ///< X-axis value
  LPT_type y; ///< Y-axis value
  LPT_type z; ///< Z-axis value
};
typedef struct LPT_vector LPT_vector;

struct LPT_pset
{
  int nistpt; ///< Number of particles in this set
  int itrdm;  ///< Whether emits particles in randomly in time-domain
  LPT_range x; ///< X-axis range of generation [m]
  LPT_range y; ///< Y-axis range of generation [m]
  LPT_range z; ///< Z-axis range of generation [m]
  LPT_range tm; ///< Time range of generation [s]
  LPT_type di;  ///< Diameter of particle [m]
  LPT_type ri;  ///< Density of particle [kg/m3]
  LPT_vector u; ///< Initial velocity [m/s]
};
typedef struct LPT_pset LPT_pset;

/**
 * @brief Stores LPT particle data in SOA (Structure of Arrays)
 *
 * This represents the internal of LPT (just copies from/to LPT
 * module, cannot skip copy because the type may differ), and can be
 * good for visualization output.
 */
struct LPT_particles
{
  ptrdiff_t npt;  ///< Number of particle
  LPT_type *uxf;  ///< X velocity of fluid [m/s]
  LPT_type *uyf;  ///< Y velocity of fluid [m/s]
  LPT_type *uzf;  ///< Z velocity of fluid [m/s]
  LPT_type *xpt;  ///< X position [m]
  LPT_type *ypt;  ///< Y position [m]
  LPT_type *zpt;  ///< Z position [m]
  LPT_type *uxpt; ///< X velocity of particle [m/s]
  LPT_type *uypt; ///< Y velocity of particle [m/s]
  LPT_type *uzpt; ///< Z velocity of particle [m/s]
  LPT_type *timpt; ///< Activation time [s]
  LPT_type *rhopt; ///< Density of particle [kg/m3]
  LPT_type *diapt; ///< Diameter of particle [m]
  LPT_type *fuxpt;
  LPT_type *fuypt;
  LPT_type *fuzpt;
  LPT_type *fduxt;
  LPT_type *fduyt;
  LPT_type *fduzt;
  int *icfpt; ///< X-axis cell index of current particle position
  int *jcfpt; ///< Y-axis cell index of current particle position
  int *kcfpt; ///< Z-axis cell index of current particle position
  int *ifpsrd; ///< Phase (liquid/gas) of each particles located
  int *iwpsrd; ///< Whether particle is on (or in) the wall
};
typedef struct LPT_particles LPT_particles;

/**
 * @brief Stores single LPT particle data or for AOS (Array of
 *        Structure) storage
 *
 * This is good for operating on a single particle or for data
 * conversion.
 */
struct LPT_particle
{
  LPT_vector uf;  ///< Velocity of fluid [m/s]
  LPT_vector pt;  ///< Position [m]
  LPT_vector upt; ///< Velocity of particle [m/s]
  LPT_type timpt; ///< Activation time [s]
  LPT_type rhopt; ///< Density of particle [kg/m3]
  LPT_type diapt; ///< Diameter of particle [m]
  LPT_vector fupt;
  LPT_vector fdut;
  int icfpt; ///< X-axis cell index of current particle position
  int jcfpt; ///< Y-axis cell index of current particle position
  int kcfpt; ///< Z-axis cell index of current particle position
  int ifpsrd; ///< Phase (solid/liquid/gas) of each particles located
  int iwpsrd; ///< (unknown)
};
typedef struct LPT_particle LPT_particle;

enum LPT_realfield
{
  LPT_FV_INVALID =  0,
  LPT_FV_PEWALL  =  1,
  LPT_FV_PLMS1   =  2,
  LPT_FV_PLMS2   =  3,
  LPT_FV_PLMS3   =  4,
  LPT_FV_PLALS   =  5,
  LPT_FV_YVIS    =  6,
  LPT_FV_VXFCOR1 =  7,
  LPT_FV_VXFCOR2 =  8,
  LPT_FV_VYFCOR1 =  9,
  LPT_FV_VYFCOR2 = 10,
  LPT_FV_VZFCOR1 = 11,
  LPT_FV_VZFCOR2 = 12,
};

enum LPT_intfield
{
  LPT_IV_INVALID =   0,
  LPT_IV_NDIMPLS = 101,
  LPT_IV_IPLMS1  = 102,
  LPT_IV_IPLMS2  = 103,
  LPT_IV_IPLMS3  = 104,
  LPT_IV_NUMW0   = 105,
};

enum LPT_realptval
{
  LPT_FP_INVALID = 0,
  LPT_FP_UXF     = 50,
  LPT_FP_UYF     = 51,
  LPT_FP_UZF     = 52,
  LPT_FP_XPT     = 53,
  LPT_FP_YPT     = 54,
  LPT_FP_ZPT     = 55,
  LPT_FP_UXPT    = 56,
  LPT_FP_UYPT    = 57,
  LPT_FP_UZPT    = 58,
  LPT_FP_TIMPT   = 59,
  LPT_FP_RHOPT   = 60,
  LPT_FP_DIAPT   = 61,
  LPT_FP_FUXPT   = 62,
  LPT_FP_FUYPT   = 63,
  LPT_FP_FUZPT   = 64,
  LPT_FP_FDUXT   = 65,
  LPT_FP_FDUYT   = 66,
  LPT_FP_FDUZT   = 67,
};

enum LPT_intptval
{
  LPT_IP_INVALID = 0,
  LPT_IP_ICFPT   = 150,
  LPT_IP_JCFPT   = 151,
  LPT_IP_KCFPT   = 152,
  LPT_IP_IFPSRD  = 153,
  LPT_IP_IWPSRD  = 154,
};

#endif
