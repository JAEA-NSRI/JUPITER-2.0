#include "print_param_comps.h"
#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "component_info_frac.h"
#include "csvutil.h"
#include "oxidation.h"
#include "oxidation_defs.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "component_info.h"
#include "os/asprintf.h"

#include <stdlib.h>

void PP_component_info_data_value_format(int *argc, const char ***argv, void *a)
{
  struct pp_format_value_data *p;
  struct PP_component_info_data_value_data *v;
  v = (struct PP_component_info_data_value_data *)a;

  if (!v->value) {
    *argc = 0;
    *argv = NULL;
  } else {
    int id;
    const char *retp = NULL;
    pp_format_value_clean(&v->idata.data);

    id = v->value->d ? v->value->d->jupiter_id : v->value->id;
    p = PP_int_value_init(&v->idata, id, NULL, NULL);
    pp_format_value(p, &retp, argc, argv);
    if (retp) {
      v->tmp = retp;
      *argc = 1;
      *argv = &v->tmp;
    }
  }
}

const char *PP_component_info_data_value_null_format(void *a)
{
  return "(null)";
}

void PP_component_info_data_value_delete(void *a)
{
  struct PP_component_info_data_value_data *v;
  v = (struct PP_component_info_data_value_data *)a;

  pp_format_value_clean(&v->idata.data);
  v->tmp = NULL;
}

int PP_component_info_data_value_full_custom_okfunc(void *a)
{
  struct PP_component_info_data_value_data *v;
  v = (struct PP_component_info_data_value_data *)a;

  if (!v->value)
    return 0;

  if (v->okfunc)
    return v->okfunc(v->value, v->arg);

  return 1;
}

int PP_component_info_data_value_def_custom_okfunc(void *a)
{
  struct PP_component_info_data_value_data *v;
  v = (struct PP_component_info_data_value_data *)a;

  if (!v->value || !v->value->d)
    return 0;

  return PP_component_info_data_value_full_custom_okfunc(a);
}

//----

int PPcomponent_info_default_ok_func(const struct component_info_data *value,
                                     void *okdata)
{
  component_phases phases;
  struct PPcomponent_info_default_ok_data *p;
  p = (struct PPcomponent_info_default_ok_data *)okdata;

  if (!value->d)
    return 0;

  if (value->id == -1) {
    if (p->include_m1)
      return 1;
    return 0;
  }

  phases = component_phases_band(value->d->phases, p->phases);
  if (component_phases_any(phases))
    return 1;
  return 0;
}

static void PPcomponent_info_default_name_delete(void *arg)
{
  struct PPcomponent_info_default_name_data *p;
  p = (struct PPcomponent_info_default_name_data *)arg;

  free(p->buf);
  p->buf = NULL;
}

static const char *PPcomponent_info_default_name_format(void *arg)
{
  int r;
  struct PPcomponent_info_default_name_data *p;
  p = (struct PPcomponent_info_default_name_data *)arg;

  PPcomponent_info_default_name_delete(arg);

  r = jupiter_asprintf(&p->buf, "Component ID %d", p->index);
  if (r < 0) {
    p->buf = NULL;
    return "Comopnent ID (error)";
  }

  return p->buf;
}

static struct pp_format_name_data *
PPcomponent_info_default_name_init(struct PPcomponent_info_default_name_data *p,
                                   int index)
{
  p->buf = NULL;
  p->index = index;
  return pp_format_name_init(&p->data, PPcomponent_info_default_name_format,
                             PPcomponent_info_default_name_delete, p);
}

static void PP_component_info_idxid(flags *f, int indent, int index,
                                    const struct component_info_data *value,
                                    const char *unit,
                                    PPcomponent_info_name_func *name_func,
                                    void *name_data,
                                    PP_component_info_data_ok_func *compid_test,
                                    void *compid_ok_data, int *nogood)
{
  struct pp_format_name_data *n;
  struct PP_component_info_data_value_data v;
  struct PP_charp_unit_data u;
  n = name_func(index, value, name_data);
  PP_component_info_data_value_init(&v, value, compid_test, compid_ok_data);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, n, &v.data, &u.data, nogood);
}

struct PP_component_info_one_name_data
{
  struct pp_format_name_data data;
  const char *title;
  char *buf;
};

static void PP_component_info_one_name_delete(void *arg)
{
  struct PP_component_info_one_name_data *p;
  p = (struct PP_component_info_one_name_data *)arg;

  free(p->buf);
  p->buf = NULL;
}

static const char *PP_component_info_one_name_format(void *arg)
{
  int r;
  struct PP_component_info_one_name_data *p;
  p = (struct PP_component_info_one_name_data *)arg;

  PP_component_info_one_name_delete(arg);

  r = jupiter_asprintf(&p->buf, "Component ID for %s", p->title);
  if (r < 0) {
    p->buf = NULL;
    return p->title;
  }

  return p->buf;
}

static struct pp_format_name_data *
PP_component_info_one_name_init(struct PP_component_info_one_name_data *p,
                                const char *title)
{
  p->buf = NULL;
  p->title = title;
  return pp_format_name_init(&p->data, PP_component_info_one_name_format,
                             PP_component_info_one_name_delete, p);
}

static void PP_component_info_oneid(flags *f, int indent, const char *title,
                                    const struct component_info_data *value,
                                    const char *unit,
                                    PP_component_info_data_ok_func *compid_test,
                                    void *compid_ok_data, int *nogood)
{
  struct PP_component_info_one_name_data n;
  struct PP_component_info_data_value_data v;
  struct PP_charp_unit_data u;
  PP_component_info_one_name_init(&n, title);
  PP_component_info_data_value_init(&v, value, compid_test, compid_ok_data);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, &v.data, &u.data, nogood);
}

/**
 * @brief Whether print data in multi-component output
 * @retval 1 Multi-component or invalid
 * @retval 0 Single-component and valid
 */
static int PPcomponent_info_multicomp(int ncompo, PP_int_ok_func *ncompo_test,
                                      void *ncompo_ok_data)
{
  if (ncompo != 1) /* include 0 or negative */
    return 1;

  if (!ncompo_test) /* Assumes it's valid */
    return 0;

  return !ncompo_test(ncompo, ncompo_ok_data);
}

void PPcomponent_info_f(flags *flg, int indent, const char *title,
                        char head_char, struct component_info *info,
                        PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
                        PPcomponent_info_name_func *name_func, void *name_data,
                        PP_component_info_data_ok_func *compid_test,
                        void *compid_ok_data,
                        PPcomponent_info_for_each_id *process_for_each_id,
                        void *foreach_data, int *nogood)
{
  int ncompo = component_info_ncompo(info);

  if (PPcomponent_info_multicomp(ncompo, ncompo_test, ncompo_ok_data)) {
    print_param_header(flg, head_char, indent, 3, 3, "%s compound data", title);

    PP_int_f(flg, indent, "Number of comopnents", ncompo, "", ncompo_test,
             ncompo_ok_data, nogood);

    for (int i = 0; i < ncompo; ++i) {
      const struct component_info_data *v;

      v = &info->d[i];
      PP_component_info_idxid(flg, indent, i, v, "", name_func, name_data,
                              compid_test, compid_ok_data, nogood);

      if (process_for_each_id)
        process_for_each_id(flg, indent, ncompo, i, v, nogood, foreach_data);
    }
    print_param_header(flg, head_char, indent, 3, 3, "End of %s compound data",
                       title);

  } else {
    const struct component_info_data *v;
    v = &info->d[0];

    PP_component_info_oneid(flg, indent, title, v, "", compid_test,
                            compid_ok_data, nogood);

    if (process_for_each_id)
      process_for_each_id(flg, indent, ncompo, 0, v, nogood, foreach_data);
  }
}

struct pp_format_name_data *PPcomponent_info_default_name_func(
  int index, const struct component_info_data *value, void *arg)
{
  struct PPcomponent_info_default_name_data *d;
  d = (struct PPcomponent_info_default_name_data *)arg;

  return PPcomponent_info_default_name_init(d, index);
}

//----

static void PP_component_info_frac_idxid(
  flags *flg, int indent, int index,
  const struct component_info_frac_data *value, const char *unit,
  PPcomponent_info_frac_name_func *name_func, void *name_data,
  PP_component_info_data_ok_func *okfunc, void *okdata, int *nogood)
{
  struct pp_format_name_data *n;
  struct PP_component_info_data_value_data v;
  struct PP_charp_unit_data u;
  n = name_func(index, value, name_data);
  PP_component_info_data_value_init(&v, &value->comp, okfunc, okdata);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(flg, indent, PP_BASELEN, n, &v.data, &u.data, nogood);
}

static void PP_component_info_frac_frac(
  flags *flg, int indent, int index,
  const struct component_info_frac_data *value, const char *unit,
  PPcomponent_info_frac_name_func *name_func, void *name_data,
  PP_double_ok_func *okfunc, void *okdata, int *nogood)
{
  struct pp_format_name_data *n;
  struct PP_double_value_data v;
  struct PP_charp_unit_data u;
  n = name_func(index, value, name_data);
  PP_double_value_init(&v, value->fraction, okfunc, okdata);
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(flg, indent, PP_BASELEN, n, &v.data, &u.data, nogood);
}

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
  int *nogood)
{
  int ncompo = component_info_frac_ncompo(info);

  if (!print_fraction)
    print_fraction_even_if_single = 0;

  if (print_fraction_even_if_single ||
      PPcomponent_info_multicomp(ncompo, ncompo_test, ncompo_ok_data)) {
    print_param_header(flg, head_char, indent, 3, 3, "%s compound data", title);

    PP_int_f(flg, indent, "Number of components", ncompo, "", ncompo_test,
             ncompo_ok_data, nogood);

    for (int i = 0; i < ncompo; ++i) {
      const struct component_info_frac_data *v;

      v = &info->d[i];
      PP_component_info_frac_idxid(flg, indent, i, v, "", idname_func,
                                   idname_data, compid_test, compid_ok_data,
                                   nogood);

      if (print_fraction) {
        PP_component_info_frac_frac(flg, indent + 2, i, v, "", fname_func,
                                    fname_data, fraction_test, fraction_ok_data,
                                    nogood);
      }

      if (process_for_each_id)
        process_for_each_id(flg, indent, ncompo, i, v, nogood, foreach_data);
    }
    print_param_header(flg, head_char, indent, 3, 3, "End of %s compound data",
                       title);
  } else {
    const struct component_info_frac_data *v;
    v = &info->d[0];

    PP_component_info_oneid(flg, indent, title, &v->comp, "", compid_test,
                            compid_ok_data, nogood);
  }
}

struct pp_format_name_data *PPcomponent_info_frac_default_idname_func(
  int index, const struct component_info_frac_data *value, void *arg)
{
  struct PPcomponent_info_frac_default_idname_data *d;
  d = (struct PPcomponent_info_frac_default_idname_data *)arg;
  return PPcomponent_info_default_name_func(index, &value->comp, &d->d);
}

struct pp_format_name_data *PPcomponent_info_frac_default_fname_func(
  int index, const struct component_info_frac_data *value, void *arg)
{
  struct PPcomponent_info_frac_default_fname_data *d;
  d = (struct PPcomponent_info_frac_default_fname_data *)arg;
  return &d->data.data;
}

//----

struct PPox_comp_compid_test_data
{
  struct PPcomponent_info_default_ok_data dd;
  PP_component_info_data_ok_func *extok;
  void *extdata;
};

static int PPox_comp_compid_ok_func(const struct component_info_data *value,
                                    void *arg)
{
  struct PPox_comp_compid_test_data *p;
  p = (struct PPox_comp_compid_test_data *)arg;

  if (PPcomponent_info_default_ok_func(value, &p->dd)) {
    if (p->extok)
      return p->extok(value, p->extdata);
    return 1;
  }

  return 0;
}

static void PPox_comp_compid_test_data_init(
  struct PPox_comp_compid_test_data *d, int optional, int ncompo,
  component_phases phases, PP_component_info_data_ok_func *extok, void *extdata)
{
  if (/* optional && */ ncompo != 1)
    optional = 0;
  PPcomponent_info_default_ok_init(&d->dd, optional, phases);
  d->extok = extok;
  d->extdata = extdata;
}

struct PPox_comp_ncompo_ok_test_data
{
  PP_int_ok_func *extok;
  void *extdata;
};

static int PPox_comp_ncompo_ok_func(int value, void *arg)
{
  struct PPox_comp_ncompo_ok_test_data *p;
  p = (struct PPox_comp_ncompo_ok_test_data *)arg;

  if (value > 0) {
    if (p->extok)
      return p->extok(value, p->extdata);
    return 1;
  }
  return 0;
}

static void
PPox_comp_ncompo_ok_test_data_init(struct PPox_comp_ncompo_ok_test_data *d,
                                   PP_int_ok_func *extok, void *extdata)
{
  d->extok = extok;
  d->extdata = extdata;
}

struct PPox_comp_frac_ok_test_data
{
  PP_double_ok_func *extok;
  void *extdata;
};

static int PPox_comp_fraction_ok_func(double value, void *arg)
{
  struct PPox_comp_frac_ok_test_data *p;
  p = (struct PPox_comp_frac_ok_test_data *)arg;

  if (value > 0.0) {
    if (p->extok)
      return p->extok(value, p->extdata);
    return 1;
  }
  return 0;
}

static void
PPox_comp_frac_ok_test_data_init(struct PPox_comp_frac_ok_test_data *d,
                                 PP_double_ok_func *extok, void *extdata)
{
  d->extok = extok;
  d->extdata = extdata;
}

void PPox_component_info_f(
  flags *flg, int indent, const char *title, char head_char,
  struct ox_component_info *info, int optional, component_phases phases,
  int need_fraction, PP_int_ok_func *ncompo_test, void *ncompo_ok_data,
  PP_component_info_data_ok_func *compid_test, void *compid_ok_data,
  PP_double_ok_func *fraction_test, void *fraction_ok_data,
  PPcomponent_info_frac_for_each_id *process_for_each_id, void *foreach_data,
  int *nogood)
{
  int ncompo;
  struct PPox_comp_ncompo_ok_test_data ncompo_data;
  struct PPox_comp_compid_test_data compid_data;
  struct PPox_comp_frac_ok_test_data frac_data;

  ncompo = ox_component_info_ncompo(info);
  PPox_comp_ncompo_ok_test_data_init(&ncompo_data, ncompo_test, ncompo_ok_data);
  PPox_comp_compid_test_data_init(&compid_data, optional, ncompo, phases,
                                  compid_test, compid_ok_data);
  PPox_comp_frac_ok_test_data_init(&frac_data, fraction_test, fraction_ok_data);

  PPcomponent_info_frac_nf(flg, indent, title, head_char, need_fraction, 0,
                           &info->comps, //
                           PPox_comp_ncompo_ok_func, &ncompo_data,
                           PPox_comp_compid_ok_func, &compid_data,
                           "Fraction at generated (normalized)",
                           PPox_comp_fraction_ok_func, &frac_data, "",
                           process_for_each_id, foreach_data, nogood);
}
