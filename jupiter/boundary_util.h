/**
 * @flie boundary_util.h
 * @brief Boundary structure utility.
 */

#ifndef BOUNDARY_UTIL_H
#define BOUNDARY_UTIL_H

#include "common.h"
#include "common_util.h"
#include "component_data_defs.h"
#include "inlet_component_defs.h"

#include <stdio.h> /* for FILE* */

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "control/defs.h"
#include "serializer/defs.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
struct inlet_component_data *inlet_component_data_new(int ncomp);
JUPITER_DECL
void inlet_component_data_delete(struct inlet_component_data *data);

static inline int
inlet_component_data_ncomp(const struct inlet_component_data *data)
{
  if (!data || data->ncomp <= 0)
    return 0;
  return data->ncomp;
}

static inline struct inlet_component_element *
inlet_component_data_get(struct inlet_component_data *data, int index)
{
  if (index < 0 || index >= inlet_component_data_ncomp(data))
    return NULL;
  return &data->array[index];
}

JUPITER_DECL
struct inlet_component_data *
inlet_component_data_dup(struct inlet_component_data *src);

/**
 * @brief Remove each controllable entries from their update list
 */
JUPITER_DECL
void inlet_component_data_remove_from_list(struct inlet_component_data *list);

/**
 * @brief Add each controllable entries to given update list
 */
JUPITER_DECL
void inlet_component_data_add_to_list(struct inlet_component_data *list,
                                      controllable_type *control_head);

/**
 * @brief Make map entry for inlet_component_element.
 *
 * Returning pointer is MSGPACKX_MAP_HEAD.
 */
JUPITER_DECL
msgpackx_map_node *
inlet_component_element_to_msgpackx(struct inlet_component_element *element);

/**
 * @brief Make array entry for inlet_component_data.
 *
 * Returning pointer is MSGPACKX_ARRAY_HEAD.
 */
JUPITER_DECL
msgpackx_array_node *
inlet_component_data_to_msgpackx(struct inlet_component_data *data);

/**
 * @brief Read map data and set to @p dest
 *
 * controllable_type element needs to be added to the process list.
 */
JUPITER_DECL
int inlet_component_element_from_msgpackx(struct inlet_component_element *dest,
                                          msgpackx_map_node *mhead,
                                          component_data *comp_data_head,
                                          jcntrl_executive_manager *manager);

/**
 * @brief Read array data, allocate new inlet_component_data and set
 *
 * controllable_type element needs to be added to the process list.
 */
JUPITER_DECL
struct inlet_component_data *
inlet_component_data_from_msgpackx(msgpackx_array_node *ahead,
                                   component_data *comp_data_head,
                                   jcntrl_executive_manager *manager);

/**
 * @brief Normalize boundary direction bits
 * @param dir Parameter to dir
 * @return result.
 *
 * This function sets individual direction bits according
 * BOUNDARY_DIR_ALL, BOUNDARY_DIR_X, BOUNDARY_DIR_Y, or BOUNDARY_DIR_Z
 * are set.
 */
JUPITER_DECL
boundary_direction boundary_direction_normalize(boundary_direction dir);

/* == fluid == */
/**
 * @brief Initialize fluid boundary data
 * @param data data to initialize.
 *
 * @warning This function is for initialize head data. Plase create
 *          new one instead of reusing allocated data.
 */
JUPITER_DECL
void fluid_boundary_data_init(fluid_boundary_data *data);

/**
 * @brief New fluid boundary data
 * @param head Head of fluid_boundary_data
 * @return Created data, or NULL if allocation failed.
 *
 * Allocated data is bound to given domain data.
 */
JUPITER_DECL
fluid_boundary_data *fluid_boundary_data_new(fluid_boundary_data *head);

/**
 * @brief Delete single fluid boundary data
 * @param data Data to delete
 *
 * If NULL given, this function does nothing.
 */
JUPITER_DECL
void fluid_boundary_data_delete(fluid_boundary_data *data);

/**
 * @brief Delete all allocated fluid boundary data.
 * @param head Head entry of fluid_boundary_data.
 *
 * Given pointer will no be freed. If you really want to do so,
 * use fluid_boundary_data_delete().
 */
JUPITER_DECL
void fluid_boundary_data_delete_all(fluid_boundary_data *head);

JUPITER_DECL
fluid_boundary_data *fluid_boundary_data_next(fluid_boundary_data *data);
JUPITER_DECL
fluid_boundary_data *fluid_boundary_data_prev(fluid_boundary_data *data);

void fluid_boundary_data_move(fluid_boundary_data *data,
                              fluid_boundary_data *new_head);

/**
 * @brief Set fluid boundary condition to cell array
 * @param nx        X size of array dest and vof
 * @param ny        Y size of array dest and vof
 * @param nz        Z size of array dest and vof
 * @param fl_data   Boundary data to set
 * @param threshold VOF threshold value
 * @param dest Destination cell array
 * @param vof VOF value for masking
 * @return @p fl_data
 *
 * dest is array of pointer to fluid_boundary_data.
 *
 * For VOF thoresold value, this function uses the value from the
 * parameter, and not stored value in init_data if given.
 */
JUPITER_DECL
fluid_boundary_data *fluid_boundary_set_by_vof(int nx, int ny, int nz,
                                               fluid_boundary_data *fl_data,
                                               type threshold,
                                               fluid_boundary_data **dest,
                                               type *vof);

static inline fluid_boundary_data *
fluid_boundary_Xcc(domain *cdo, fluid_boundary_data **ary, int jy, int jz)
{
  ptrdiff_t p;
  p = calc_address(jy + cdo->stmb, jz + cdo->stmb, 0, cdo->nby, cdo->nbz, 1);
  return ary[p];
}

static inline fluid_boundary_data *
fluid_boundary_Ycc(domain *cdo, fluid_boundary_data **ary, int jx, int jz)
{
  ptrdiff_t p;
  p = calc_address(jx + cdo->stmb, jz + cdo->stmb, 0, cdo->nbx, cdo->nbz, 1);
  return ary[p];
}

static inline fluid_boundary_data *
fluid_boundary_Zcc(domain *cdo, fluid_boundary_data **ary, int jx, int jy)
{
  ptrdiff_t p;
  p = calc_address(jx + cdo->stmb, jy + cdo->stmb, 0, cdo->nbx, cdo->nby, 1);
  return ary[p];
}

/**
 * @returns boundary condition should be applied for center of edge
 *          between @p a and @p b.
 */
static inline fluid_boundary_data *fluid_boundary_v2(fluid_boundary_data *a,
                                                     fluid_boundary_data *b)
{
  if (a == b)
    return a;
  if (!a || a->cond == -1)
    return b;
  if (!b || b->cond == -1)
    return a;

  /*
   * a \ b   W S O I i
   * WALL    a a a a a
   * SLIP    b a a a a
   * OUT     b b b a N
   * IN      b b b a N
   * invalid b b b b N
   *
   * Currently, the edge between OUT and OUT conditions does not differ, but can
   * be differ in future versions (e.g. with different pressure value).
   *
   * INLET and INLET conditions are different conditions, so we have to argue
   * that which we should take precedence.
   *
   * 'invalid' here means for values other than valid values and -1. This
   * includes BOUNDARY_MPI, but should not be passed here.
   */
  switch (a->cond) {
  case WALL:
    return a;
  case SLIP:
    if (b->cond == WALL)
      return b;
    return a;
  default:
    switch (b->cond) {
    case WALL:
      return b;
    case SLIP:
      return b;
    case OUT:
    case INLET:
      if (a->cond == INLET)
        return a;
      return b;
    default:
      break;
    }
    break;
  }
  /* should be unreachable */
  return NULL;
}

/**
 * @returns boundary condition should be applied for vertex surrounded by
 *          @p a, @p b, @p c and @p d.
 */
static inline fluid_boundary_data *fluid_boundary_v4(fluid_boundary_data *a,
                                                     fluid_boundary_data *b,
                                                     fluid_boundary_data *c,
                                                     fluid_boundary_data *d)
{
  a = fluid_boundary_v2(a, b);
  b = fluid_boundary_v2(c, d);
  return fluid_boundary_v2(a, b);
}

static inline fluid_boundary_data *
fluid_boundary__vc(domain *cdo, variable *val, int ju, int jv,
                   fluid_boundary_data *(*cc)(domain *, variable *, int, int))
{
  return fluid_boundary_v2(cc(cdo, val, ju, jv), cc(cdo, val, ju - 1, jv));
}

static inline fluid_boundary_data *
fluid_boundary__cv(domain *cdo, variable *val, int ju, int jv,
                   fluid_boundary_data *(*cc)(domain *, variable *, int, int))
{
  return fluid_boundary_v2(cc(cdo, val, ju, jv), cc(cdo, val, ju, jv - 1));
}

static inline fluid_boundary_data *
fluid_boundary__vv(domain *cdo, variable *val, int ju, int jv,
                   fluid_boundary_data *(*cc)(domain *, variable *, int, int))
{
  fluid_boundary_data *pp, *pn, *np, *nn;
  pp = cc(cdo, val, ju, jv);
  pn = cc(cdo, val, ju, jv - 1);
  np = cc(cdo, val, ju - 1, jv);
  nn = cc(cdo, val, ju - 1, jv - 1);
  return fluid_boundary_v4(pp, np, pn, nn);
}

#define DEFINE_FLUID_BOUNDARY_CC(dir, axis, uvar, vvar)                      \
  static inline fluid_boundary_data *fluid_boundary_##dir##cc(domain *cdo,   \
                                                              variable *val, \
                                                              int uvar,      \
                                                              int vvar)      \
  {                                                                          \
    return fluid_boundary_##axis##cc(cdo, val->bnd_##dir.fl, uvar, vvar);    \
  }

#define DEFINE_FLUID_BOUNDARY_CO(dir, co, uvar, vvar)                        \
  static inline fluid_boundary_data *fluid_boundary_##dir##co(domain *cdo,   \
                                                              variable *val, \
                                                              int uvar,      \
                                                              int vvar)      \
  {                                                                          \
    return fluid_boundary__##co(cdo, val, uvar, vvar,                        \
                                fluid_boundary_##dir##cc);                   \
  }

DEFINE_FLUID_BOUNDARY_CC(B, Z, jx, jy)
DEFINE_FLUID_BOUNDARY_CC(T, Z, jx, jy)
DEFINE_FLUID_BOUNDARY_CC(N, Y, jx, jz)
DEFINE_FLUID_BOUNDARY_CC(S, Y, jx, jz)
DEFINE_FLUID_BOUNDARY_CC(E, X, jy, jz)
DEFINE_FLUID_BOUNDARY_CC(W, X, jy, jz)

DEFINE_FLUID_BOUNDARY_CO(B, cv, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(B, vc, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(B, vv, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(T, cv, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(T, vc, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(T, vv, jx, jy)
DEFINE_FLUID_BOUNDARY_CO(S, cv, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(S, vc, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(S, vv, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(N, cv, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(N, vc, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(N, vv, jx, jz)
DEFINE_FLUID_BOUNDARY_CO(W, cv, jy, jz)
DEFINE_FLUID_BOUNDARY_CO(W, vc, jy, jz)
DEFINE_FLUID_BOUNDARY_CO(W, vv, jy, jz)
DEFINE_FLUID_BOUNDARY_CO(E, cv, jy, jz)
DEFINE_FLUID_BOUNDARY_CO(E, vc, jy, jz)
DEFINE_FLUID_BOUNDARY_CO(E, vv, jy, jz)

#undef DEFINE_FLUID_BOUNDARY_CC
#undef DEFINE_FLUID_BOUNDARY_CO

/* ===== cell center ====== */

/**
 * @brief Obtain boundary condition on bottom at cell (jx, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jy)
 */
fluid_boundary_data *fluid_boundary_Bcc(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on top at cell (jx, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jy)
 */
fluid_boundary_data *fluid_boundary_Tcc(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on south at cell (jx, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jz)
 */
fluid_boundary_data *fluid_boundary_Scc(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on north at cell (jx, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jz)
 */
fluid_boundary_data *fluid_boundary_Ncc(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on west at cell (jy, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy, jz)
 */
fluid_boundary_data *fluid_boundary_Wcc(domain *cdo, variable *val, int jy,
                                        int jz);

/**
 * @brief Obtain boundary condition on east at cell (jy, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy, jz)
 */
fluid_boundary_data *fluid_boundary_Ecc(domain *cdo, variable *val, int jy,
                                        int jz);

/* ==== U- (y, x or x direction in EW, NS or TB boundary) edge ==== */
/**
 * @brief Obtain boundary condition on bottom at edge (jx-1/2, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jy)
 */
fluid_boundary_data *fluid_boundary_Bvc(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on top at edge (jx-1/2, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jy)
 */
fluid_boundary_data *fluid_boundary_Tvc(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on south at edge (jx-1/2, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jz)
 */
fluid_boundary_data *fluid_boundary_Svc(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on north at edge (jx-1/2, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jz)
 */
fluid_boundary_data *fluid_boundary_Nvc(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on west at edge (jy-1/2, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy-1/2, jz)
 */
fluid_boundary_data *fluid_boundary_Wvc(domain *cdo, variable *val, int jy,
                                        int jz);

/**
 * @brief Obtain boundary condition on west at edge (jy-1/2, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy-1/2, jz)
 */
fluid_boundary_data *fluid_boundary_Evc(domain *cdo, variable *val, int jy,
                                        int jz);

/* ==== V- (z, z or y direction in EW, NS or TB boundary) edge ==== */
/**
 * @brief Obtain boundary condition on bottom at edge (jx, jy-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jy-1/2)
 */
fluid_boundary_data *fluid_boundary_Bcv(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on top at edge (jx, jy-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jy-1/2)
 */
fluid_boundary_data *fluid_boundary_Tcv(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on south at edge (jx, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Scv(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on north at edge (jx, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Ncv(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on west at edge (jy, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Wcv(domain *cdo, variable *val, int jy,
                                        int jz);

/**
 * @brief Obtain boundary condition on east at edge (jy, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Ecv(domain *cdo, variable *val, int jy,
                                        int jz);

/* ==== lower left corner ==== */
/**
 * @brief Obtain boundary condition on bottom at vertex (jx-1/2, jy-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jy-1/2)
 */
fluid_boundary_data *fluid_boundary_Bvv(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on top at vertex (jx-1/2, jy-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jy-1/2)
 */
fluid_boundary_data *fluid_boundary_Tvv(domain *cdo, variable *val, int jx,
                                        int jy);

/**
 * @brief Obtain boundary condition on south at vertex (jx-1/2, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Svv(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on north at vertex (jx-1/2, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Nvv(domain *cdo, variable *val, int jx,
                                        int jz);

/**
 * @brief Obtain boundary condition on west at vertex (jy-1/2, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jy-1/2, jz-1/2)
 */
fluid_boundary_data *fluid_boundary_Wvv(domain *cdo, variable *val, int jy,
                                        int jz);

/**
 * @brief Obtain boundary condition on east at vertex (jy-1/2, jz-1/2)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return fluid boundary condition at (jx-1/2, jy-1/2)
 */
fluid_boundary_data *fluid_boundary_Evv(domain *cdo, variable *val, int jy,
                                        int jz);

/* == thermal == */
/**
 * @brief Initialize thermal boundary data
 * @param data data to initialize.
 *
 * @warning This function is for initialize head data. Plase create
 *          new one instead of reusing allocated data.
 */
void thermal_boundary_data_init(thermal_boundary_data *data);

/**
 * @brief New thermal boundary data
 * @param head Thermal boundary head
 * @return Created data, or NULL if allocation failed.
 *
 * Allocated data is bound to given domain data.
 */
thermal_boundary_data *thermal_boundary_data_new(thermal_boundary_data *head);

/**
 * @brief Delete single thermal boundary data
 * @param data Data to delete
 *
 * If NULL given, this function does nothing.
 */
void thermal_boundary_data_delete(thermal_boundary_data *data);

/**
 * @brief Delete all allocated thermal boundary data.
 * @param head Head entry of thermal_boundary_data.
 *
 * Given pointer will no be freed. If you really want to do so,
 * use thermal_boundary_data_delete().
 */
void thermal_boundary_data_delete_all(thermal_boundary_data *head);

thermal_boundary_data *thermal_boundary_data_next(thermal_boundary_data *data);
thermal_boundary_data *thermal_boundary_data_prev(thermal_boundary_data *data);

void thermal_boundary_data_move(thermal_boundary_data *data,
                                thermal_boundary_data *new_head);

/**
 * @brief Set thermal boundary condition to cell array
 * @param nx        X size of array dest and vof
 * @param ny        Y size of array dest and vof
 * @param nz        Z size of array dest and vof
 * @param th_data   Boundary data to set
 * @param threshold VOF threshold value
 * @param dest Destination cell array
 * @param vof VOF value for masking
 * @return @p th_data
 *
 * dest is array of pointer to thermal_boundary_data.
 *
 * cdo is used for getting head of thermal_boundary_data.
 *
 * For VOF thoresold value, this function uses the value from the
 * parameter, and not stored value in init_data if given.
 */
thermal_boundary_data *
thermal_boundary_set_by_vof(int nx, int ny, int nz,
                            thermal_boundary_data *th_data, type threshold,
                            thermal_boundary_data **dest, type *vof);

/* == surface == */
void surface_boundary_data_init(surface_boundary_data *data);

surface_boundary_data *surface_boundary_data_new(surface_boundary_data *head);

void surface_boundary_data_delete(surface_boundary_data *data);
void surface_boundary_data_delete_all(surface_boundary_data *head);

surface_boundary_data *surface_boundary_data_next(surface_boundary_data *p);
surface_boundary_data *surface_boundary_data_prev(surface_boundary_data *p);

void surface_boundary_data_move(surface_boundary_data *data,
                                surface_boundary_data *new_head);

/**
 * @brief Serialize boundary array data
 * @param fl_head Head element of fluid_boundary_data list.
 * @param th_head Head element of thermal_boundary_data list.
 * @param nbx X size of boundary array
 * @param nby Y size of boundary array
 * @param nbz Z size of boundary array
 * @param bnd_W West side (X-) boundary data (size = nby * nbz)
 * @param bnd_E East side (X+) boundary data (size = nby * nbz)
 * @param bnd_S South side (Y-) boundary data (size = nbx * nbz)
 * @param bnd_N North side (Y+) boundary data (size = nbx * nbz)
 * @param bnd_B Bottom side (Z-) boundary data (size = nbx * nby)
 * @param bnd_T Top side (Z+) boundary data (size = nbx * nby)
 * @return Serialized data or NULL if failed.
 *
 * The all data used in each boundary arrays must be accessible from
 * @p fl_head and @p th_head chain. Otherwise, the output will be
 * corrupted.
 *
 * Unused data in @p fl_head and @p th_head will not be included
 * in the output.
 *
 * The given parameter of @p fl_head and @p th_head is treated as
 * boundary head and should be 'MPI-connected' boundary.
 *
 * If the NULL is given for @p bnd_W, @p bnd_E, @p bnd_S, @p bnd_N, @p
 * bnd_B and/or @p bnd_T, the corresponding will data not be included.
 *
 * Result data is not packed.
 *
 * The arguments @p bnd_W, @p bnd_E, @p bnd_S, @p bnd_N, @p bnd_B and
 * @p bnd_T themselves are not an array. Its members, `fl` and `th`,
 * are array of the pointer to `fluid_boundary_data` or
 * `thermal_boundary_data` respectively.
 */
msgpackx_data *boundary_data_array_dump(
  fluid_boundary_data *fl_head, thermal_boundary_data *th_head, int nbx,
  int nby, int nbz, struct boundary_array *bnd_W, struct boundary_array *bnd_E,
  struct boundary_array *bnd_S, struct boundary_array *bnd_N,
  struct boundary_array *bnd_B, struct boundary_array *bnd_T);

/**
 * @brief De-serialize boundary array data
 * @param fl_head Head element of fluid_boundary_data list.
 * @param th_head Head element of thermal_boundary_data list.
 * @param manager Controlling executive manager to get from
 * @param control_head Head element of controlled parameter chain
 * @param nbx X size of boundary array
 * @param nby Y size of boundary array
 * @param nbz Z size of boundary array
 * @param ncompo Number of components
 * @param bnd_W West side (X-) boundary data (size = nby * nbz)
 * @param bnd_E East side (X+) boundary data (size = nby * nbz)
 * @param bnd_S South side (Y-) boundary data (size = nbx * nbz)
 * @param bnd_N North side (Y+) boundary data (size = nbx * nbz)
 * @param bnd_B Bottom side (Z-) boundary data (size = nbx * nby)
 * @param bnd_T Top side (Z+) boundary data (size = nbx * nby)
 * @return Serialized data or NULL if failed.
 *
 * The given parameter of @p fl_head and @p th_head is treated as
 * boundary head and should be 'MPI-connected' boundary.
 *
 * @p ncompo is used for checking material IDs in INLET conditions.
 * To disable this check (but it's dangerous), give negative value
 * (ex. -1) for @p ncompo.
 *
 * The arguments @p bnd_W, @p bnd_E, @p bnd_S, @p bnd_N, @p bnd_B and
 * @p bnd_T themselves are not an array. Its members, `fl` and `th`,
 * are array of the pointer to `fluid_boundary_data` or
 * `thermal_boundary_data` respectively.
 */
int boundary_data_array_construct(
  msgpackx_data *data, fluid_boundary_data *fl_head,
  thermal_boundary_data *th_head, component_data *comp_data_head,
  jcntrl_executive_manager *manager, controllable_type *control_head, //
  int nbx, int nby, int nbz, struct boundary_array *bnd_W,
  struct boundary_array *bnd_E, struct boundary_array *bnd_S,
  struct boundary_array *bnd_N, struct boundary_array *bnd_B,
  struct boundary_array *bnd_T);

int boundary_data_array_read_from_file(
  FILE *fp, const char *file, fluid_boundary_data *fl_head,
  thermal_boundary_data *th_head, component_data *comp_data_head,
  jcntrl_executive_manager *manager, controllable_type *control_head, //
  int nbx, int nby, int nbz,                                          //
  struct boundary_array *bnd_W, struct boundary_array *bnd_E,
  struct boundary_array *bnd_S, struct boundary_array *bnd_N,
  struct boundary_array *bnd_B, struct boundary_array *bnd_T, //
  csv_error *cserr, msgpackx_error *merr);

static inline thermal_boundary_data *
thermal_boundary_Xcc(domain *cdo, thermal_boundary_data **ary, int jy, int jz)
{
  ptrdiff_t p;
  p = calc_address(jy + cdo->stmb, jz + cdo->stmb, 0, cdo->nby, cdo->nbz, 1);
  return ary[p];
}

static inline thermal_boundary_data *
thermal_boundary_Ycc(domain *cdo, thermal_boundary_data **ary, int jx, int jz)
{
  ptrdiff_t p;
  p = calc_address(jx + cdo->stmb, jz + cdo->stmb, 0, cdo->nbx, cdo->nbz, 1);
  return ary[p];
}

static inline thermal_boundary_data *
thermal_boundary_Zcc(domain *cdo, thermal_boundary_data **ary, int jx, int jy)
{
  ptrdiff_t p;
  p = calc_address(jx + cdo->stmb, jy + cdo->stmb, 0, cdo->nbx, cdo->nby, 1);
  return ary[p];
}

#define DEFINE_THERMAL_BOUNDARY_CC(dir, axis, uvar, vvar)                    \
  static inline thermal_boundary_data *                                      \
  thermal_boundary_##dir##cc(domain *cdo, variable *val, int uvar, int vvar) \
  {                                                                          \
    return thermal_boundary_##axis##cc(cdo, val->bnd_##dir.th, uvar, vvar);  \
  }

DEFINE_THERMAL_BOUNDARY_CC(B, Z, jx, jy)
DEFINE_THERMAL_BOUNDARY_CC(T, Z, jx, jy)
DEFINE_THERMAL_BOUNDARY_CC(N, Y, jx, jz)
DEFINE_THERMAL_BOUNDARY_CC(S, Y, jx, jz)
DEFINE_THERMAL_BOUNDARY_CC(E, X, jy, jz)
DEFINE_THERMAL_BOUNDARY_CC(W, X, jy, jz)

#undef DEFINE_THERMAL_BOUNDARY_CC

/**
 * @brief Obtain boundary condition on bottom at cell (jx, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jx, jy)
 */
thermal_boundary_data *thermal_boundary_Bcc(domain *cdo, variable *val, int jx,
                                            int jy);

/**
 * @brief Obtain boundary condition on top at cell (jx, jy)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jx, jy)
 */
thermal_boundary_data *thermal_boundary_Tcc(domain *cdo, variable *val, int jx,
                                            int jy);

/**
 * @brief Obtain boundary condition on south at cell (jx, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jx, jz)
 */
thermal_boundary_data *thermal_boundary_Scc(domain *cdo, variable *val, int jx,
                                            int jz);

/**
 * @brief Obtain boundary condition on north at cell (jx, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jx X-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jx, jz)
 */
thermal_boundary_data *thermal_boundary_Ncc(domain *cdo, variable *val, int jx,
                                            int jz);

/**
 * @brief Obtain boundary condition on west at cell (jy, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jx, jz)
 */
thermal_boundary_data *thermal_boundary_Wcc(domain *cdo, variable *val, int jy,
                                            int jz);

/**
 * @brief Obtain boundary condition on east at cell (jy, jz)
 * @param cdo Doamin info
 * @param val Variable info
 * @param jy Y-axis index (starting with 0 for internal domain)
 * @param jz Z-axis index (starting with 0 for internal domain)
 * @return thermal boundary condition at (jy, jz)
 */
thermal_boundary_data *thermal_boundary_Ecc(domain *cdo, variable *val, int jy,
                                            int jz);

#ifdef JUPITER_MPI
#endif

#ifdef __cplusplus
}
#endif

#endif
