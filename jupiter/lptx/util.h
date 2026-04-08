/**
 * @file lptx/util.h
 */

#ifndef JUPITER_LPTX_UTIL_H
#define JUPITER_LPTX_UTIL_H

#include "defs.h"
#include "error.h"
#include "pvector.h"
#include "vector.h"

#include <jupiter/random/random.h>

#ifdef _OPENMP
#include <omp.h>
#endif

static inline LPTX_type LPTX_jupiter_random_nextt(jupiter_random_seed *seed)
{
#if defined(JUPITER_LPTX_DOUBLE)
  return jupiter_random_nextd(seed);
#else
  return jupiter_random_nextf(seed);
#endif
}

static inline LPTX_type
LPTX_jupiter_random_nexttc(jupiter_random_seed_counter *seed)
{
#if defined(JUPITER_LPTX_DOUBLE)
  return jupiter_random_nextdc(seed);
#else
  return jupiter_random_nextfc(seed);
#endif
}

/* sphere particle surface area volume and mass */

static inline LPTX_type LPTX_particle_surface_area_d(LPTX_type diameter)
{
  return LPTX_M_PI * diameter * diameter;
}

static inline LPTX_type LPTX_particle_surface_area_r(LPTX_type radius)
{
  return LPTX_C(4.) * LPTX_M_PI * radius * radius;
}

static inline LPTX_type LPTX_particle_surface_area(const LPTX_particle *p)
{
  return LPTX_particle_surface_area_d(p->diameter);
}

static inline LPTX_type LPTX_particle_volume_d(LPTX_type diameter)
{
  LPTX_type diacb = diameter * diameter * diameter;
  return LPTX_M_PI * diacb / LPTX_C(6.);
}

static inline LPTX_type LPTX_particle_volume_r(LPTX_type radius)
{
  LPTX_type radcb = radius * radius * radius;
  return LPTX_M_PI * radcb * LPTX_C(4.) / LPTX_C(3.);
}

static inline LPTX_type LPTX_particle_volume(const LPTX_particle *p)
{
  return LPTX_particle_volume_d(p->diameter);
}

static inline LPTX_type LPTX_particle_mass_d(LPTX_type density,
                                             LPTX_type diameter)
{
  return density * LPTX_particle_volume_d(diameter);
}

static inline LPTX_type LPTX_particle_mass(const LPTX_particle *p)
{
  return LPTX_particle_mass_d(p->density, p->diameter);
}

/* unitless numbers for particle */

static inline LPTX_type LPTX_particle_reynolds_number(const LPTX_particle *p)
{
  LPTX_type plufup;
  plufup = LPTX_vector_abs(LPTX_vector_sub(p->fluid_velocity, p->velocity));
  return p->fluid_density * plufup * p->diameter / p->fluid_viscosity;
}

static inline LPTX_type LPTX_particle_prandtl_number(const LPTX_particle *p)
{
  LPTX_type cpmu;
  cpmu = p->fluid_specific_heat * p->fluid_viscosity;
  return cpmu / p->fluid_thermal_conductivity;
}

static inline LPTX_type LPTX_particle_nusselt_number(const LPTX_particle *p)
{
  return p->heat_transfer_rate * p->diameter / p->fluid_thermal_conductivity;
}

/* rectilinear grid access helper utility */

static inline LPTX_type LPTX_get_scalar_of_type(const void *array,
                                                LPTX_idtype i)
{
  return *((const LPTX_type *)array + i);
}

static inline LPTX_type LPTX_get_scalar_of_double(const void *array,
                                                  LPTX_idtype i)
{
  return *((const double *)array + i);
}

static inline LPTX_type LPTX_get_scalar_of_float(const void *array,
                                                 LPTX_idtype i)
{
  return *((const float *)array + i);
}

/**
 * Init LPTX_rectilinear_grid for all parameters specified
 *
 * @p choords_i, @p choords_j and @p choords_k is allowed to be different
 * type, but usually may not be.
 */
static inline LPTX_rectilinear_grid LPTX_rectilinear_grid_c(
  const void *coords_i, const void *coords_j, const void *coords_k,
  LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx,
  LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
  LPTX_idtype stmz, LPTX_cb_get_scalar_of *getter_i,
  LPTX_cb_get_scalar_of *getter_j, LPTX_cb_get_scalar_of *getter_k)
{
  return (LPTX_rectilinear_grid){
    .coords_i = coords_i,
    .coords_j = coords_j,
    .coords_k = coords_k,
    .nx = nx,
    .ny = ny,
    .nz = nz,
    .mx = mx,
    .my = my,
    .mz = mz,
    .stmx = stmx,
    .stmy = stmy,
    .stmz = stmz,
    .get_coord_i = getter_i,
    .get_coord_j = getter_j,
    .get_coord_k = getter_k,
  };
}

/**
 * Init LPTX_rectilinear_grid for any type of coordinate
 */
static inline LPTX_rectilinear_grid
LPTX_rectilinear_grid_g(const void *coords_i, const void *coords_j,
                        const void *coords_k, LPTX_idtype nx, LPTX_idtype ny,
                        LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                        LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                        LPTX_idtype stmz, LPTX_cb_get_scalar_of *getter)
{
  return LPTX_rectilinear_grid_c(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                 my, mz, stmx, stmy, stmz, getter, getter,
                                 getter);
}

/**
 * Init LPTX_rectilinear_grid for coordinte with double array
 */
static inline LPTX_rectilinear_grid
LPTX_rectilinear_grid_d(const double *coords_i, const double *coords_j,
                        const double *coords_k, LPTX_idtype nx, LPTX_idtype ny,
                        LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                        LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                        LPTX_idtype stmz)
{
  return LPTX_rectilinear_grid_g(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                 my, mz, stmx, stmy, stmz,
                                 LPTX_get_scalar_of_double);
}

static inline LPTX_rectilinear_grid
LPTX_rectilinear_grid_f(const float *coords_i, const float *coords_j,
                        const float *coords_k, LPTX_idtype nx, LPTX_idtype ny,
                        LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                        LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                        LPTX_idtype stmz)
{
  return LPTX_rectilinear_grid_g(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                 my, mz, stmx, stmy, stmz,
                                 LPTX_get_scalar_of_float);
}

static inline LPTX_rectilinear_grid
LPTX_rectilinear_grid_t(const LPTX_type *coords_i, const LPTX_type *coords_j,
                        const LPTX_type *coords_k, LPTX_idtype nx,
                        LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx,
                        LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
                        LPTX_idtype stmy, LPTX_idtype stmz)
{
  return LPTX_rectilinear_grid_g(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                 my, mz, stmx, stmy, stmz,
                                 LPTX_get_scalar_of_type);
}

/**
 * Init LPTX_rectilinear_vector for any type of vector element types.
 *
 * Usually, you would pass the different arrays for vector components (Struct of
 * Array), but you can pass shifted getter function to use a vector in Array of
 * Struct format.
 */
static inline LPTX_rectilinear_vector LPTX_rectilinear_vector_c(
  const void *vector_x, const void *vector_y, const void *vector_z, //
  LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,                   //
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz,                   //
  LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz,             //
  LPTX_cb_get_scalar_of *get_func_x, LPTX_cb_get_scalar_of *get_func_y,
  LPTX_cb_get_scalar_of *get_func_z)
{
  return (LPTX_rectilinear_vector){
    .vector_x = vector_x,
    .vector_y = vector_y,
    .vector_z = vector_z,
    .nx = nx,
    .ny = ny,
    .nz = nz,
    .mx = mx,
    .my = my,
    .mz = mz,
    .stmx = stmx,
    .stmy = stmy,
    .stmz = stmz,
    .get_func_x = get_func_x,
    .get_func_y = get_func_y,
    .get_func_z = get_func_z,
  };
}

static inline LPTX_rectilinear_vector LPTX_rectilinear_vector_g(
  const void *vector_x, const void *vector_y, const void *vector_z, //
  LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,                   //
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz,                   //
  LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz,             //
  LPTX_cb_get_scalar_of *get_func)
{
  return LPTX_rectilinear_vector_c(vector_x, vector_y, vector_z, nx, ny, nz, mx,
                                   my, mz, stmx, stmy, stmz, get_func, get_func,
                                   get_func);
}

static inline LPTX_rectilinear_vector
LPTX_rectilinear_vector_d(const double *vector_x, const double *vector_y,
                          const double *vector_z,                         //
                          LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz, //
                          LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, //
                          LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  return LPTX_rectilinear_vector_g(vector_x, vector_y, vector_z, nx, ny, nz, mx,
                                   my, mz, stmx, stmy, stmz,
                                   LPTX_get_scalar_of_double);
}

static inline LPTX_rectilinear_vector
LPTX_rectilinear_vector_f(const float *vector_x, const float *vector_y,
                          const float *vector_z,                          //
                          LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz, //
                          LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, //
                          LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  return LPTX_rectilinear_vector_g(vector_x, vector_y, vector_z, nx, ny, nz, mx,
                                   my, mz, stmx, stmy, stmz,
                                   LPTX_get_scalar_of_float);
}

static inline LPTX_rectilinear_vector
LPTX_rectilinear_vector_t(const LPTX_type *vector_x, const LPTX_type *vector_y,
                          const LPTX_type *vector_z,                      //
                          LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz, //
                          LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, //
                          LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  return LPTX_rectilinear_vector_g(vector_x, vector_y, vector_z, nx, ny, nz, mx,
                                   my, mz, stmx, stmy, stmz,
                                   LPTX_get_scalar_of_type);
}

static inline LPTX_rectilinear_scalar
LPTX_rectilinear_scalar_c(const void *scalar, LPTX_idtype nx, LPTX_idtype ny,
                          LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                          LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                          LPTX_idtype stmz, LPTX_cb_get_scalar_of *getter)
{
  return (LPTX_rectilinear_scalar){
    .scalar = scalar,
    .nx = nx,
    .ny = ny,
    .nz = nz,
    .mx = mx,
    .my = my,
    .mz = mz,
    .stmx = stmx,
    .stmy = stmy,
    .stmz = stmz,
    .get_func = getter,
  };
}

static inline LPTX_rectilinear_scalar
LPTX_rectilinear_scalar_d(const double *scalar, LPTX_idtype nx, LPTX_idtype ny,
                          LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                          LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                          LPTX_idtype stmz)
{
  return LPTX_rectilinear_scalar_c(scalar, nx, ny, nz, mx, my, mz, stmx, stmy,
                                   stmz, LPTX_get_scalar_of_double);
}

static inline LPTX_rectilinear_scalar
LPTX_rectilinear_scalar_f(const float *scalar, LPTX_idtype nx, LPTX_idtype ny,
                          LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
                          LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy,
                          LPTX_idtype stmz)
{
  return LPTX_rectilinear_scalar_c(scalar, nx, ny, nz, mx, my, mz, stmx, stmy,
                                   stmz, LPTX_get_scalar_of_float);
}

static inline LPTX_rectilinear_scalar
LPTX_rectilinear_scalar_t(const LPTX_type *scalar, LPTX_idtype nx,
                          LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx,
                          LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
                          LPTX_idtype stmy, LPTX_idtype stmz)
{
  return LPTX_rectilinear_scalar_c(scalar, nx, ny, nz, mx, my, mz, stmx, stmy,
                                   stmz, LPTX_get_scalar_of_type);
}

static inline LPTX_idtype LPTX_rectilinear_addr(LPTX_idtype i, LPTX_idtype j,
                                                LPTX_idtype k, LPTX_idtype nx,
                                                LPTX_idtype ny, LPTX_idtype nz)
{
  LPTX_idtype jj;
  jj = k;
  jj *= ny;
  jj += j;
  jj *= nx;
  jj += i;
  return jj;
}

static inline LPTX_type
LPTX_rectilinear_scalar_at(const LPTX_rectilinear_scalar *s, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_idtype jj;
  jj = LPTX_rectilinear_addr(i + s->stmx, j + s->stmy, k + s->stmz, //
                             s->mx, s->my, s->mz);
  return s->get_func(s->scalar, jj);
}

static inline LPTX_vector
LPTX_rectilinear_vector_at(const LPTX_rectilinear_vector *v, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_idtype jj;
  jj = LPTX_rectilinear_addr(i + v->stmx, j + v->stmy, k + v->stmz, //
                             v->mx, v->my, v->mz);
  return LPTX_vector_c(v->get_func_x(v->vector_x, jj),
                       v->get_func_y(v->vector_y, jj),
                       v->get_func_z(v->vector_z, jj));
}

static inline LPTX_type
LPTX_rectilinear_vector_x(const LPTX_rectilinear_vector *v, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_idtype jj;
  jj = LPTX_rectilinear_addr(i + v->stmx, j + v->stmy, k + v->stmz, //
                             v->mx, v->my, v->mz);
  return v->get_func_x(v->vector_x, jj);
}

static inline LPTX_type
LPTX_rectilinear_vector_y(const LPTX_rectilinear_vector *v, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_idtype jj;
  jj = LPTX_rectilinear_addr(i + v->stmx, j + v->stmy, k + v->stmz, //
                             v->mx, v->my, v->mz);
  return v->get_func_y(v->vector_y, jj);
}
static inline LPTX_type
LPTX_rectilinear_vector_z(const LPTX_rectilinear_vector *v, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_idtype jj;
  jj = LPTX_rectilinear_addr(i + v->stmx, j + v->stmy, k + v->stmz, //
                             v->mx, v->my, v->mz);
  return v->get_func_z(v->vector_z, jj);
}

/**
 * return cell WSB corner
 */
static inline LPTX_vector
LPTX_rectilinear_grid_v000(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_vector_c(g->get_coord_i(g->coords_i, i + g->stmx),
                       g->get_coord_j(g->coords_j, j + g->stmy),
                       g->get_coord_k(g->coords_k, k + g->stmz));
}

static inline LPTX_vector
LPTX_rectilinear_grid_v100(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i + 1, j, k);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v010(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i, j + 1, k);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v110(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i + 1, j + 1, k);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v001(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i, j, k + 1);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v101(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i + 1, j, k + 1);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v011(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i, j + 1, k + 1);
}

static inline LPTX_vector
LPTX_rectilinear_grid_v111(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  return LPTX_rectilinear_grid_v000(g, i + 1, j + 1, k + 1);
}

/**
 * return west face center
 */
static inline LPTX_vector
LPTX_rectilinear_grid_fcW(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v011(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_fcE(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v100(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_fcS(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v101(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_fcN(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v010(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_fcB(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v110(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_fcT(const LPTX_rectilinear_grid *g, //
                          LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v001(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

/**
 * return center of egde between B and S face
 */
static inline LPTX_vector
LPTX_rectilinear_grid_ecSB(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v100(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecNB(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v010(g, i, j, k);
  b = LPTX_rectilinear_grid_v110(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecST(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v001(g, i, j, k);
  b = LPTX_rectilinear_grid_v101(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecNT(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v011(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecWB(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v010(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecEB(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v100(g, i, j, k);
  b = LPTX_rectilinear_grid_v110(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecWT(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v001(g, i, j, k);
  b = LPTX_rectilinear_grid_v011(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecET(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v101(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecWS(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v001(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecES(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v100(g, i, j, k);
  b = LPTX_rectilinear_grid_v101(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecWN(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v010(g, i, j, k);
  b = LPTX_rectilinear_grid_v011(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

static inline LPTX_vector
LPTX_rectilinear_grid_ecEN(const LPTX_rectilinear_grid *g, //
                           LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v110(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

/**
 * return cell center
 */
static inline LPTX_vector
LPTX_rectilinear_grid_cc(const LPTX_rectilinear_grid *g, //
                         LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  LPTX_vector a, b;
  a = LPTX_rectilinear_grid_v000(g, i, j, k);
  b = LPTX_rectilinear_grid_v111(g, i, j, k);
  return LPTX_vector_mulf(0.5, LPTX_vector_add(a, b));
}

/**
 * Copy base particle data
 *
 * This function is only for making textually meaningful.
 * This function does not care about self-assignment.
 */
static inline void LPTX_particle_copy_base(LPTX_particle_data *to,
                                           const LPTX_particle_data *from)
{
  to->base = from->base;
}

/**
 * Copy particle vectors
 *
 * This function does not care about self-assignment.
 */
static inline void LPTX_particle_copy_vectors(LPTX_particle_data *to,
                                              const LPTX_particle_data *from)
{
  LPTX_assert(to->number_of_vectors == from->number_of_vectors);
  for (LPTX_idtype i = 0; i < to->number_of_vectors; ++i)
    LPTX_particle_vector_copy((LPTX_particle_vector *)&to->vectors[i],
                              &from->vectors[i]);
}

/**
 * Copy particle vectors with filling missing elements
 */
static inline void
LPTX_particle_copy_vectors_fill(LPTX_particle_data *to,
                                const LPTX_particle_data *from, LPTX_type fill)
{
  LPTX_assert(to->number_of_vectors >= from->number_of_vectors);
  for (LPTX_idtype i = 0; i < to->number_of_vectors; ++i) {
    const LPTX_particle_vector *src;
    src = (i < from->number_of_vectors) ? &from->vectors[i] : NULL;
    LPTX_particle_vector_copy_fill((LPTX_particle_vector *)&to->vectors[i], src,
                                   fill);
  }
}

/**
 * Copy particle vectors with filling missing elements, different fills for
 * each vectors
 */
static inline void
LPTX_particle_copy_vectors_fillv(LPTX_particle_data *to,
                                 const LPTX_particle_data *from,
                                 const LPTX_particle_vector *fills)
{
  LPTX_assert(to->number_of_vectors >= from->number_of_vectors);
  LPTX_assert(fills->length >= to->number_of_vectors);

  for (LPTX_idtype i = 0; i < to->number_of_vectors; ++i) {
    const LPTX_particle_vector *src;
    LPTX_type fill;
    src = (i < from->number_of_vectors) ? &from->vectors[i] : NULL;
    fill = LPTX_particle_vector_getv(fills, i);
    LPTX_particle_vector_copy_fill((LPTX_particle_vector *)&to->vectors[i], src,
                                   fill);
  }
}

/**
 * Copy particles including vectors
 *
 * This function does not care about self-assignment.
 *
 * Vector sizes in both particle must be equally-sized.
 */
static inline void LPTX_particle_copy(LPTX_particle_data *to,
                                      const LPTX_particle_data *from)
{
  LPTX_particle_copy_vectors(to, from);
  LPTX_particle_copy_base(to, from);
}

/**
 * Copy particles including vectors, with filling missing elements in @p from
 *
 * This function does not care about self-assignment
 */
static inline void LPTX_particle_copy_fill(LPTX_particle_data *to,
                                           const LPTX_particle_data *from,
                                           LPTX_type fill)
{
  LPTX_particle_copy_vectors_fill(to, from, fill);
  LPTX_particle_copy_base(to, from);
}

/**
 * Copy particles including vectors, with filling missing elements in @p from
 *
 * This function does not care about self-assignment
 *
 * You can specifiy different missing value for each vectors, so @p fills must
 * have elements with number of vectors in @p to at least.
 */
static inline void LPTX_particle_copy_fillv(LPTX_particle_data *to,
                                            const LPTX_particle_data *from,
                                            const LPTX_particle_vector *fills)
{
  LPTX_particle_copy_vectors_fillv(to, from, fills);
  LPTX_particle_copy_base(to, from);
}

/**
 * @brief Implementation of LPTX_omp_distribute
 * @param is_out Writes local start
 * @param ie_out Writes local end
 * @param nt Number of parallelization slots (e.g. threads)
 * @param it Index of parallelization slot for this call (e.g. thread id)
 * @apram is Start index to distribute (inclusive)
 * @param ie Last index to distribute (exclusive)
 *
 * This function is available even if OpenMP is not enabled.
 */
static inline void LPTX_omp_distribute_calc(LPTX_idtype *is_out,
                                            LPTX_idtype *ie_out, int nt, int it,
                                            LPTX_idtype is, LPTX_idtype ie)
{
  LPTX_idtype ne, nel, rem;
  LPTX_bool bovr;

  LPTX_assert(ie >= is);
  LPTX_assert(nt > 0);
  LPTX_assert(it >= 0);
  LPTX_assert(it < nt);

  ne = ie - is;
  nel = ne / nt;
  rem = ne % nt;
  is += it * nel;
  if (it < rem) {
    is += it;
    nel += 1;
  } else {
    is += rem;
  }
  ie = is + nel;

  *is_out = is;
  *ie_out = ie;
}

/**
 * @brief Manual distribute for OpenMP thread parallelization in range of [is,
 *        ie)
 * @param is_out Writes local start
 * @param ie_out Writes local end
 * @param nt_out Writes number of threads
 * @param it_out Writes thread id
 * @apram is Start index to distribute (inclusive)
 * @param ie Last index to distribute (exclusive)
 *
 * This function is available even if OpenMP is not enabled. In this case,
 * this function sets *is_out = is, *ie_out = ie, *nt_out = 1 and *it_out = 0
 */
static inline void LPTX_omp_distribute(LPTX_idtype *is_out, LPTX_idtype *ie_out,
                                       int *nt_out, int *it_out, LPTX_idtype is,
                                       LPTX_idtype ie)
{
#if _OPENMP
  int nt = omp_get_num_threads();
  int it = omp_get_thread_num();

  LPTX_omp_distribute_calc(is_out, ie_out, nt, it, is, ie);
  if (nt_out)
    *nt_out = nt;
  if (it_out)
    *it_out = it;
#else
  *is_out = is;
  *ie_out = ie;
  if (nt_out)
    *nt_out = 1;
  if (it_out)
    *it_out = 0;
#endif
}

#endif
