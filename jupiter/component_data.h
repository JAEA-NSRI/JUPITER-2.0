#ifndef JUPITER_COMPONENT_MAP_H
#define JUPITER_COMPONENT_MAP_H

#include "common.h"
#include "component_info.h"
#include "geometry/bitarray.h"
#include "geometry/list.h"
#include "component_data_defs.h"
#include "struct.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//--- component_phases

static inline component_phases component_phases_none(void)
{
  component_phases p;
  geom_bitarray_element_setall(p.phases, COMPONENT_PHASE_MAX, 0);
  return p;
}

static inline component_phases component_phases_all(void)
{
  component_phases p;
  geom_bitarray_element_setall(p.phases, COMPONENT_PHASE_MAX, 1);
  return p;
}

static inline component_phases component_phases__a(size_t np,
                                                   component_phase_name *n)
{
  component_phases p = component_phases_none();
  for (size_t i = 0; i < np; ++i)
    geom_bitarray_element_set(p.phases, n[i], 1);
  return p;
}

#define component_phases__ary(...) \
  (component_phase_name[]) { __VA_ARGS__ }
#define component_phases__asz(...) \
  (sizeof(component_phases__ary(__VA_ARGS__)) / sizeof(component_phase_name))

#define component_phases_a(...)                           \
  component_phases__a(component_phases__asz(__VA_ARGS__), \
                      component_phases__ary(__VA_ARGS__))

static inline int component_phases_eql(component_phases p, component_phases q)
{
  return geom_bitarray_element_eql(p.phases, q.phases, COMPONENT_PHASE_MAX);
}

static inline int component_phases_get(component_phases p,
                                       component_phase_name n)
{
  return geom_bitarray_element_get(p.phases, n);
}

static inline void component_phases_set(component_phases *p,
                                        component_phase_name n, int f)
{
  geom_bitarray_element_set(p->phases, n, f);
}

static inline int component_phases_has_solid(component_phases p)
{
  return component_phases_get(p, COMPONENT_PHASE_SOLID);
}

static inline int component_phases_has_liquid(component_phases p)
{
  return component_phases_get(p, COMPONENT_PHASE_LIQUID);
}

static inline int component_phases_has_solid_and_liquid(component_phases p)
{
  component_phases u, t;
  /* p & (SOLID | LIQUID) == (SOLID | LIQUID) */
  t = component_phases_a(COMPONENT_PHASE_SOLID, COMPONENT_PHASE_LIQUID);
  geom_bitarray_element_band(u.phases, t.phases, p.phases, COMPONENT_PHASE_MAX);
  return component_phases_eql(u, t);
}

static inline int component_phases_has_solid_or_liquid(component_phases p)
{
  component_phases u, t;
  t = component_phases_a(COMPONENT_PHASE_SOLID, COMPONENT_PHASE_LIQUID);
  geom_bitarray_element_band(u.phases, t.phases, p.phases, COMPONENT_PHASE_MAX);
  return geom_bitarray_element_getany(u.phases, COMPONENT_PHASE_MAX);
}

static inline int component_phases_has_gas(component_phases p)
{
  return component_phases_get(p, COMPONENT_PHASE_GAS);
}

static inline int component_phases_is_gas_only(component_phases p)
{
  return component_phases_eql(p, component_phases_a(COMPONENT_PHASE_GAS));
}

static inline int component_phases_any(component_phases p)
{
  return geom_bitarray_element_getany(p.phases, COMPONENT_PHASE_MAX);
}

/**
 * @brief check for possible phase combination for JUPITER
 * @retval 1 p has solid or liquid and not gas.
 * @retval 1 p has gas only
 * @retval 0 otherwise.
 */
static inline int component_phases_is_valid(component_phases p)
{
  if (component_phases_has_solid_or_liquid(p))
    return !component_phases_has_gas(p);
  return component_phases_is_gas_only(p);
}

static inline component_phases component_phases_band(component_phases a,
                                                     component_phases b)
{
  component_phases p;
  geom_bitarray_element_band(p.phases, a.phases, b.phases, COMPONENT_PHASE_MAX);
  return p;
}

static inline component_phases component_phases_bor(component_phases a,
                                                    component_phases b)
{
  component_phases p;
  geom_bitarray_element_bor(p.phases, a.phases, b.phases, COMPONENT_PHASE_MAX);
  return p;
}

//--- component_variables

static inline component_variables component_variables_none(void)
{
  component_variables v;
  geom_bitarray_element_setall(v.variables, COMPONENT_VARIABLE_MAX, 0);
  return v;
}

static inline component_variables component_variables_all(void)
{
  component_variables v;
  geom_bitarray_element_setall(v.variables, COMPONENT_VARIABLE_MAX, 1);
  return v;
}

static inline int component_variables_get(component_variables v,
                                          component_variable_name vn)
{
  return geom_bitarray_element_get(v.variables, vn);
}

static inline void component_variables_set(component_variables *v,
                                           component_variable_name vn, int f)
{
  geom_bitarray_element_set(v->variables, vn, f);
}

//--- component_data

static inline void component_data_init(component_data *c)
{
  geom_list_init(&c->list);
  c->jupiter_id = JUPITER_ID_INVALID;
  c->comp_index = -1;
  c->phase_comps_index = -1;
  c->mass_source_g_index = -1;
  c->lpt_mass_fraction_index = -1;
  c->csv = NULL;
  c->fname = NULL;
  c->generated = 1;
  c->phases = component_phases_none();
}

JUPITER_DECL
component_data *component_data_new(void);
JUPITER_DECL
void component_data_delete(component_data *p);

JUPITER_DECL
void component_data_delete_all(component_data *head);

/**
 * Return available variables for this component.
 *
 * @p p->phase needs to be set.
 *
 * Since @p p->comp_index is shared for all variables (except for
 * phase_value.comps and variable.mass_source_g) currently, the variable itself
 * may be available even if this function returns no. For example, solid VOF,
 * phase change variables where solid form is UNUSED.
 */
static inline component_variables component_data_vars(component_data *p,
                                                      domain *cdo, flags *flg)
{
  component_variables v = component_variables_none();

  if (p->comp_index < 0 || p->comp_index >= cdo->NumberOfComponent)
    return v;

  if (p->phase_comps_index >= 0 && p->phase_comps_index < cdo->NIComponent)
    component_variables_set(&v, COMPONENT_VARIABLE_PHASE_COMP, 1);

  if (flg->solute_diff == ON) {
    if (component_phases_has_solid(p->phases) ||
        component_phases_has_liquid(p->phases)) {
      component_variables_set(&v, COMPONENT_VARIABLE_VOLUME_FRACTION, 1);
    }

    /* Acutually Mass fraction for gases */
    component_variables_set(&v, COMPONENT_VARIABLE_MOLAR_FRACTION, 1);

    if (component_phases_has_gas(p->phases)) {
      if (component_info_any(p->jupiter_id, &cdo->mass_source_g_comps))
        component_variables_set(&v, COMPONENT_VARIABLE_MASS_SOURCE_GAS, 1);
    }
    if (component_info_any(p->jupiter_id, &cdo->lpt_mass_fractions))
      component_variables_set(&v, COMPONENT_VARIABLE_LPT_MASS_FRACTION, 1);

  } else {
    if (component_phases_has_solid(p->phases))
      component_variables_set(&v, COMPONENT_VARIABLE_SOLID_VOF, 1);
    if (component_phases_has_liquid(p->phases))
      component_variables_set(&v, COMPONENT_VARIABLE_LIQUID_VOF, 1);

    if (flg->phase_change == ON &&
        component_phases_has_solid_and_liquid(p->phases)) {
      if (flg->melting == ON)
        component_variables_set(&v, COMPONENT_VARIABLE_DELTA_MELT, 1);
      if (flg->solidification == ON)
        component_variables_set(&v, COMPONENT_VARIABLE_DELTA_SOLID, 1);
    }
  }
  return v;
}

/**
 * @brief Get whether given variable @p n is available for the component @p p
 *
 * @note component_data_index() can still return -1 even if this function
 *       returned true (1).
 */
static inline int component_data_has_var(component_data *p, domain *cdo,
                                         flags *flg, component_variable_name n)
{
  component_variables v = component_data_vars(p, cdo, flg);
  return component_variables_get(v, n);
}

/**
 * @brief Get array index for given variable @p n for component @p p
 * @retval -1 Invalid variable or not available.
 */
static inline int component_data_index(component_data *p, domain *cdo,
                                       flags *flg, component_variable_name n)
{
  switch (n) {
  case COMPONENT_VARIABLE_PHASE_COMP:
    return p->phase_comps_index;
  case COMPONENT_VARIABLE_MASS_SOURCE_GAS:
    return p->mass_source_g_index;
  case COMPONENT_VARIABLE_LPT_MASS_FRACTION:
    return p->lpt_mass_fraction_index;

  case COMPONENT_VARIABLE_SOLID_VOF:
  case COMPONENT_VARIABLE_LIQUID_VOF:
  case COMPONENT_VARIABLE_DELTA_MELT:
  case COMPONENT_VARIABLE_DELTA_SOLID:
  case COMPONENT_VARIABLE_VOLUME_FRACTION:
  case COMPONENT_VARIABLE_MOLAR_FRACTION:
    if (component_data_has_var(p, cdo, flg, n))
      return p->comp_index;
    return -1;

  case COMPONENT_VARIABLE_MAX:
    break;
  }
  return -1;
}

typedef int component_data_find_func(const component_data *d, void *arg);

/**
 * @brief Finds first entry that func() returns non-zero value.
 * @retval NULL No entry matches the condition
 */
JUPITER_DECL
component_data *component_data_find(component_data *head,
                                    component_data_find_func *func, void *arg);

/**
 * @brief Shorthand for finding entry by JUPITER ID
 * @retval NULL No entry matches
 *
 * @note Finding JUPITER_ID_INVALID may still return a non-NULL,
 * since no one rejects adding such an entry.
 */
JUPITER_DECL
component_data *component_data_find_by_jupiter_id(component_data *head,
                                                  int jupiter_id);

/**
 * @brief Shorthand for finding entry by component index
 * @retval NULL No entry matches
 */
JUPITER_DECL
component_data *component_data_find_by_comp_index(component_data *head,
                                                  int comp_index);

/**
 * @brief Update and sort comp_index in list to match JUPITER calculation spec.
 *
 * cdo->NumberOfComponent, cdo->NBaseComponent and cdo->NGasComponent
 * will also be updated.
 *
 * List will be sorted accordingly phases and jupiter_id (for inputted entries)
 * or list order (for generated entries).
 *
 * The phases member in each entries must be set before call this function.
 *
 * @warning Do not call this function after arrays are allocated
 *          (malloc_variables()).
 *
 * @p *stat will be set to ON when any of following conditions are met:
 *
 * - JUPITER ID -1 is duplicated (regardless of inputted or generated)
 * - JUPITER ID -1 is not gas-only
 */
JUPITER_DECL
void component_data_update_index(component_data *head, domain *cdo, int *stat);

/**
 * Extra metadata to store in file
 */
struct component_data_metadata
{
  int solute_diff;                ///< flg->solute_diff
  int qgeom;                      ///< val->qgeom
  int bnd_norm_u;                 ///< val->bnd_norm_u
  int bnd_norm_v;                 ///< val->bnd_norm_v
  int bnd_norm_w;                 ///< val->bnd_norm_w
  int NumberOfComponent;          ///< cdo->NumberOfComponent
  int NBaseComponent;             ///< cdo->NBaseComponent
  int NGasComponent;              ///< cdo->NGasComponent
  int NIComponent;                ///< cdo->NIComponent
  int NIBaseCopmonent;            ///< cdo->NIBaseComponent
  int NIGasCopmonent;             ///< cdo->NIGasComponent
  int gnx, gny, gnz;              ///< cdo->gnx, gny, gnz
  int nx, ny, nz;                 ///< cdo->nx, ny, nz
  int pex, pey, pez;              ///< flg->pex, pey, pez
  binary_output_mode output_mode; ///< flg->output_mode
};

JUPITER_DECL
int component_data_read(const char *file, component_data *head,
                        struct component_data_metadata *metaout);

JUPITER_DECL
msgpackx_error component_data_parse(msgpackx_data *data, component_data *head,
                                    struct component_data_metadata *metaout);

JUPITER_DECL
msgpackx_error component_data_pack(component_data *head,
                                   struct component_data_metadata *metadata,
                                   msgpackx_data *data);

int component_data_write(const char *file, component_data *head,
                         struct component_data_metadata *metadata);

/**
 * Collect output metadata from domain and flags
 */
JUPITER_DECL
void component_data_metadata_get(struct component_data_metadata *outp,
                                 domain *cdo, flags *flg, variable *val,
                                 binary_output_mode output_mode);

#ifdef __cplusplus
}
#endif

#endif
