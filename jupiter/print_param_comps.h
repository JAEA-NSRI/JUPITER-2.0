#ifndef JUPITER_PRINT_PARAM_COMPS_H
#define JUPITER_PRINT_PARAM_COMPS_H

/*
 * print param for ox_component_info and component_info.
 *
 * @note Do not confuse with print_param_comp.h, which is for printing
 * simpler composite types, including controllable_type.
 */

#include "common.h"
#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_frac.h"
#include "print_param_basic.h"
#include "component_info_defs.h"
#include "print_param_core.h"

#ifdef __cplusplus
extern "C" {
#endif

//--- component_info_data

typedef int
PP_component_info_data_ok_func(const struct component_info_data *value,
                               void *arg);

/**
 * This just prints ID. By default, it always return error when no
 * component_data is bound.
 */
struct PP_component_info_data_value_data
{
  struct pp_format_value_data data;
  struct PP_int_value_data idata;
  const struct component_info_data *value;
  PP_component_info_data_ok_func *okfunc;
  void *arg;
  const char *tmp;
};

JUPITER_DECL
void PP_component_info_data_value_format(int *argc, const char ***argv,
                                         void *a);
JUPITER_DECL
const char *PP_component_info_data_value_null_format(void *a);
JUPITER_DECL
void PP_component_info_data_value_delete(void *a);
JUPITER_DECL
int PP_component_info_data_value_def_custom_okfunc(void *a);
JUPITER_DECL
int PP_component_info_data_value_full_custom_okfunc(void *a);

static inline struct pp_format_value_data *PP_component_info_data_value__init(
  struct PP_component_info_data_value_data *p,
  const struct component_info_data *value, int (*tester)(void *a),
  PP_component_info_data_ok_func *okfunc, void *okdata)
{
  p->value = value;
  p->okfunc = okfunc;
  p->arg = okdata;
  PP_int_value_init(&p->idata, 0, NULL, NULL);
  return pp_format_value_init_vec(&p->data, PP_component_info_data_value_format,
                                  PP_component_info_data_value_null_format,
                                  tester, PP_component_info_data_value_delete,
                                  p);
}

/**
 * Prints ID of bound component, returns error when no component is bound
 */
static inline struct pp_format_value_data *
PP_component_info_data_value_init(struct PP_component_info_data_value_data *p,
                                  const struct component_info_data *value,
                                  PP_component_info_data_ok_func *okfunc,
                                  void *okdata)
{
  return PP_component_info_data_value__init(
    p, value, PP_component_info_data_value_def_custom_okfunc, okfunc, okdata);
}

/**
 * Prints ID of bound component or specified ID.
 */
static inline struct pp_format_value_data *
PP_component_info_data_fulltest_value_init(
  struct PP_component_info_data_value_data *p,
  const struct component_info_data *value,
  PP_component_info_data_ok_func *okfunc, void *okdata)
{
  return PP_component_info_data_value__init(
    p, value, PP_component_info_data_value_full_custom_okfunc, okfunc, okdata);
}

static inline void PP_component_info_data_f(
  flags *f, int indent, const char *name,
  const struct component_info_data *value, const char *unit,
  PP_component_info_data_ok_func *okfunc, void *okdata, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_component_info_data_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_component_info_data_value_init(&v, value, okfunc, okdata);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

static inline void PP_component_info_data_fulltest_f(
  flags *f, int indent, const char *name,
  const struct component_info_data *value, const char *unit,
  PP_component_info_data_ok_func *okfunc, void *okdata, int *nogood)
{
  struct PP_charp_name_data n;
  struct PP_component_info_data_value_data v;
  struct PP_charp_unit_data u;
  PP_charp_name_init(&n, name);
  PP_component_info_data_fulltest_value_init(&v, value, okfunc, okdata);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

struct PP_component_info_data_precalc_ok_data
{
  int ok;
};

static inline int
PP_component_info_data_precalc_ok(const struct component_info_data *data,
                                  void *arg)
{
  return ((struct PP_component_info_data_precalc_ok_data *)arg)->ok;
}

static inline void
PP_component_info_data(flags *f, int indent, const char *name,
                       const struct component_info_data *value,
                       const char *unit, int ok, int *nogood)
{
  struct PP_component_info_data_precalc_ok_data d = {.ok = ok};
  PP_component_info_data_f(f, indent, name, value, unit,
                           PP_component_info_data_precalc_ok, &d, nogood);
}

/**
 * Since including check for both component_data pointer and ID value is
 * hard to include in single @p ok, using this function is not recommended.
 */
static inline void
PP_component_info_data_fulltest(flags *f, int indent, const char *name,
                                const struct component_info_data *value,
                                const char *unit, int ok, int *nogood)
{
  struct PP_component_info_data_precalc_ok_data d = {.ok = ok};
  PP_component_info_data_fulltest_f(f, indent, name, value, unit,
                                    PP_component_info_data_precalc_ok, &d,
                                    nogood);
}

//---

struct PPcomponent_info_default_ok_data
{
  int include_m1;          ///< Allow ID -1
  component_phases phases; ///< Allowed phases
};

/**
 * Default Component ID tester for PP_component_info_data_f().
 *
 * Pass pointer to struct PP_component_info_default_ok_data as ok data.
 */
JUPITER_DECL
int PPcomponent_info_default_ok_func(const struct component_info_data *value,
                                     void *okdata);

static inline void
PPcomponent_info_default_ok_init(struct PPcomponent_info_default_ok_data *r,
                                 int m1, component_phases phases)
{
  r->include_m1 = m1;
  r->phases = phases;
}

//--- component_info

typedef struct pp_format_name_data *
PPcomponent_info_name_func(int index, const struct component_info_data *value,
                           void *arg);

typedef int
PPcomponent_info_for_each_id(flags *flg, int indent, int ncompo, int index,
                             const struct component_info_data *value,
                             int *nogood, void *arg);

/**
 * @brief Print list of component_info
 * @param flg Flag data
 * @param indent Indentation width
 * @param title Title of the list
 * @param head_char Header rule character (e.g. '.', '-', '=', ...)
 * @param info Component info array
 * @param ncompo_test Tester function for number of components
 * @param ncompo_ok_data Extra data passing for ncompo_test
 * @param name_func Label generator for each labels
 * @param name_data Extra data passing for name_func
 * @param compid_test Tester function for component IDs
 * @param compid_ok_data Extra data passing for compid_test
 * @param process_for_each_id Extra process for looping each IDs
 * @param foreach_data Extra data passing for process_for_each_id
 * @param nogood Set to ON if any values are not accepted.
 *
 * If any tester functions are NULL, any values are accepted.
 *
 * If there are no extra processing, @p process_for_each_id can be NULL.
 *
 * The print of number of components will be omitted when it is 1 and valid.
 */
void PPcomponent_info_f(flags *flg, int indent, const char *title,
                        char head_char, struct component_info *info,
                        PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
                        PPcomponent_info_name_func *name_func, void *name_data,
                        PP_component_info_data_ok_func *compid_test,
                        void *compid_ok_data,
                        PPcomponent_info_for_each_id *process_for_each_id,
                        void *foreach_data, int *nogood);

struct PPcomponent_info_default_name_data
{
  struct pp_format_name_data data;
  int index;
  char *buf;
};

/**
 * Use struct PPcomponent_info_default_name_data for @p arg, and pass to
 * @pname_func.
 *
 * This function initialize @p arg. No need to initialize manually.
 */
struct pp_format_name_data *PPcomponent_info_default_name_func(
  int index, const struct component_info_data *value, void *arg);

/**
 * PPcomponent_info_f() with the default component ID label.
 */
static inline void PPcomponent_info(
  flags *flg, int indent, const char *title, char head_char,
  struct component_info *info, PP_int_ok_func *ncompo_test,
  void *ncompo_ok_data, PP_component_info_data_ok_func *compid_test,
  void *compid_ok_data, PPcomponent_info_for_each_id *process_for_each_id,
  void *foreach_data, int *nogood)
{
  struct PPcomponent_info_default_name_data d;
  PPcomponent_info_f(flg, indent, title, head_char, info, ncompo_test,
                     ncompo_ok_data, PPcomponent_info_default_name_func, &d,
                     compid_test, compid_ok_data, process_for_each_id,
                     foreach_data, nogood);
}

//--- component_info_frac

typedef struct pp_format_name_data *PPcomponent_info_frac_name_func(
  int index, const struct component_info_frac_data *value, void *arg);

typedef int
PPcomponent_info_frac_for_each_id(flags *flg, int indent, int ncompo, int index,
                                  const struct component_info_frac_data *value,
                                  int *nogood, void *arg);

/**
 * @brief Print list of component_info_frac
 * @param flg Flag data
 * @param indent Indentation width
 * @param title Title of the list
 * @param head_char Header rule character (e.g. '.', '-', '=', ...)
 * @param print_fraction Whether fractions should be printed
 * @param print_fraction_even_if_single Print fraction even if single compoent
 * @param info Component info array
 * @param ncompo_test Tester function for number of components
 * @param ncompo_ok_data Extra data passing for ncompo_test
 * @param idname_func Label function for component IDs
 * @param idname_data Extra data passing for name_func
 * @param compid_test Tester function for component IDs
 * @param compid_ok_data Extra data passing for compid_test
 * @param fname_func Label function for fractions
 * @param fname_data Extra data passing for fname_func
 * @param fraction_test Tester function for fractions
 * @param fraction_ok_data Extra data passing for fraction_test
 * @param fraction_unit The unit of fraction
 * @param process_for_each_id Extra process for looping each IDs
 * @param foreach_data Extra data passing for process_for_each_id
 * @param nogood Set to ON if any values are not accepted.
 *
 * If any tester functions are NULL, any values are accepted.
 *
 * If there are no extra processing, @p process_for_each_id can be NULL.
 *
 * The print of number of components will be omitted when it is 1 and valid.
 *
 * Fractions will be omitted when the number of compoent is 1 (and it's valid),
 * but @p print_fraction_even_if_single allows to force print it.
 *
 * @p print_fraction_even_if_single has no effect if @p print_fraction is 0.
 */
JUPITER_DECL
void PPcomponent_info_frac_f(
  flags *flg, int indent, const char *title, char head_char, int print_fraction,
  int print_fraction_even_if_single, struct component_info_frac *info,
  PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
  PPcomponent_info_frac_name_func *idname_func, void *idname_data,
  PP_component_info_data_ok_func *compid_test, void *compid_ok_data,
  PPcomponent_info_frac_name_func *fname_func, void *fname_data,
  PP_double_ok_func *fraction_test, void *fraction_ok_data,
  const char *fraction_unit,
  PPcomponent_info_frac_for_each_id *process_for_each_id, void *foreach_data,
  int *nogood);

struct PPcomponent_info_frac_default_idname_data
{
  struct PPcomponent_info_default_name_data d;
};

/**
 * Use struct PPcomponent_info_frac_default_idname_data for @p arg, and pass to
 * @pname_func.
 *
 * This function initialize @p arg. No need to initialize manually.
 */
JUPITER_DECL
struct pp_format_name_data *PPcomponent_info_frac_default_idname_func(
  int index, const struct component_info_frac_data *value, void *arg);

struct PPcomponent_info_frac_default_fname_data
{
  struct PP_charp_name_data data;
};

/**
 * Use struct PPcomponent_info_frac_default_fname_data for @p arg, and pass to
 * @pname_func.
 *
 * Use PP_charp_name_init() to init the data. This function does nothing.
 */
struct pp_format_name_data *PPcomponent_info_frac_default_fname_func(
  int index, const struct component_info_frac_data *value, void *arg);

/**
 * PPcomponent_info_frac_f() with specified fraction name.
 */
static inline void PPcomponent_info_frac_fnf(
  flags *flg, int indent, const char *title, char head_char, int print_fraction,
  int print_fraction_even_if_single, struct component_info_frac *info,
  PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
  PPcomponent_info_frac_name_func *idname_func, void *idname_data,
  PP_component_info_data_ok_func *compid_test, void *compid_ok_data,
  const char *fraction_name, PP_double_ok_func *fraction_test,
  void *fraction_ok_data, const char *fraction_unit,
  PPcomponent_info_frac_for_each_id *process_for_each_id, void *foreach_data,
  int *nogood)
{
  struct PPcomponent_info_frac_default_fname_data fdd;
  PP_charp_name_init(&fdd.data, fraction_name);
  PPcomponent_info_frac_f(flg, indent, title, head_char, print_fraction,
                          print_fraction_even_if_single, info, ncompo_test,
                          ncompo_ok_data, idname_func, idname_data, compid_test,
                          compid_ok_data,
                          PPcomponent_info_frac_default_fname_func, &fdd,
                          fraction_test, fraction_ok_data, fraction_unit,
                          process_for_each_id, foreach_data, nogood);
}

/**
 * PPcomponent_info_frac_f() with the default component ID label and
 * specified fraction name.
 */
static inline void PPcomponent_info_frac_nf(
  flags *flg, int indent, const char *title, char head_char, int print_fraction,
  int print_fraction_even_if_single, struct component_info_frac *info,
  PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
  PP_component_info_data_ok_func *compid_test, void *compid_ok_data,
  const char *fraction_name, PP_double_ok_func *fraction_test,
  void *fraction_ok_data, const char *fraction_unit,
  PPcomponent_info_frac_for_each_id *process_for_each_id, void *foreach_data,
  int *nogood)
{
  struct PPcomponent_info_frac_default_idname_data idd;
  struct PPcomponent_info_frac_default_fname_data fdd;
  PP_charp_name_init(&fdd.data, fraction_name);
  PPcomponent_info_frac_f(flg, indent, title, head_char, print_fraction,
                          print_fraction_even_if_single, info, ncompo_test,
                          ncompo_ok_data,
                          PPcomponent_info_frac_default_idname_func, &idd,
                          compid_test, compid_ok_data,
                          PPcomponent_info_frac_default_fname_func, &fdd,
                          fraction_test, fraction_ok_data, fraction_unit,
                          process_for_each_id, foreach_data, nogood);
}

/**
 * PPcomponent_info_frac_f() with the default component ID label and
 * default fraction name.
 */
static inline void
PPcomponent_info_frac(flags *flg, int indent, const char *title, char head_char,
                      int print_fraction, int print_fraction_even_if_single,
                      struct component_info_frac *info,
                      PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
                      PP_component_info_data_ok_func *compid_test,
                      void *compid_ok_data, PP_double_ok_func *fraction_test,
                      void *fraction_ok_data, const char *fraction_unit,
                      PPcomponent_info_frac_for_each_id *process_for_each_id,
                      void *foreach_data, int *nogood)
{
  PPcomponent_info_frac_nf(flg, indent, title, head_char, print_fraction,
                           print_fraction_even_if_single, info, ncompo_test,
                           ncompo_ok_data, compid_test, compid_ok_data,
                           "Fraction", fraction_test, fraction_ok_data,
                           fraction_unit, process_for_each_id, foreach_data,
                           nogood);
}

//--- ox_component_info

/**
 * @brief Print list of component_info_frac
 * @param flg Flag data
 * @param indent Indentation width
 * @param title Title of the list
 * @param head_char Header rule character (e.g. '.', '-', '=', ...)
 * @param optional Allows ID -1 as not set
 * @param phases acceptable phases for this entry
 * @param need_fraction Whether fractions should be printed
 * @param info Component info array
 * @param ncompo_test Tester function for number of components
 * @param ncompo_ok_data Extra data passing for ncompo_test
 * @param compid_test Tester function for component IDs
 * @param compid_ok_data Extra data passing for compid_test
 * @param fraction_test Tester function for fractions
 * @param fraction_ok_data Extra data passing for fraction_test
 * @param process_for_each_id Extra process for looping each IDs
 * @param foreach_data Extra data passing for process_for_each_id
 * @param nogood Set to ON if any values are not accepted.
 */
JUPITER_DECL
void PPox_component_info_f(
  flags *flg, int indent, const char *title, char head_char,
  struct ox_component_info *info, int optional, component_phases phases,
  int need_fraction, PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
  PP_component_info_data_ok_func *compid_test, void *compid_ok_data,
  PP_double_ok_func *fraction_test, void *fraction_ok_data,
  PPcomponent_info_frac_for_each_id *process_for_each_id, void *foreach_data,
  int *nogood);

/**
 * PPox_component_info_f without any custom functions
 */
static inline void PPox_component_info(flags *flg, int indent,
                                       const char *title, char head_char,
                                       struct ox_component_info *info,
                                       int optional, component_phases phases,
                                       int need_fraction, int *nogood)
{
  PPox_component_info_f(flg, indent, title, head_char, info, optional, phases,
                        need_fraction, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                        NULL, nogood);
}

/**
 * PPox_component_info for solid phases entries
 */
static inline void PPox_component_info_solid(flags *flg, int indent,
                                             const char *title, char head_char,
                                             struct ox_component_info *info,
                                             int optional, int need_fraction,
                                             int *nogood)
{
  component_phases phases = component_phases_a(COMPONENT_PHASE_SOLID);
  PPox_component_info(flg, indent, title, head_char, info, optional, phases,
                      need_fraction, nogood);
}

/**
 * PPox_component_info for gas phases entries
 */
static inline void PPox_component_info_gas(flags *flg, int indent,
                                           const char *title, char head_char,
                                           struct ox_component_info *info,
                                           int optional, int need_fraction,
                                           int *nogood)
{
  component_phases phases = component_phases_a(COMPONENT_PHASE_GAS);
  PPox_component_info(flg, indent, title, head_char, info, optional, phases,
                      need_fraction, nogood);
}

#ifdef __cplusplus
}
#endif

#endif
