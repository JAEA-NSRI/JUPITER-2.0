#include "print_param_infomap.h"

#include "control/defs.h"
#include "control/manager.h"
#include "field_control.h"
#include "geometry/defs.h"
#include "geometry/infomap.h"
#include "geometry/list.h"
#include "geometry/variant.h"
#include "print_param_core.h"
#include "print_param_variant.h"
#include "strlist.h"
#include "struct.h"

#include <string.h>
#include <stdlib.h>

const char *PP_info_map_name_format(void *a)
{
  struct PP_info_map_name_data *p;
  p = (struct PP_info_map_name_data *)a;

  if (!p->node)
    return NULL;
  return geom_info_map_get_description(p->node);
}

//----

static jupiter_strlist *
PP_info_map_value_control_exec(jupiter_strlist_head *lh, const char *label,
                               jcntrl_executive_manager_entry *name)
{
  struct geom_list *llp, *lln, *lls, *lle;
  jupiter_strlist *lp, *lt;

  lp = jupiter_strlist_dup_s("(");
  if (!lp)
    return NULL;

  jupiter_strlist_append(lh, lp);

  lt = jupiter_strlist_dup_s(label);
  if (!lt)
    return NULL;

  jupiter_strlist_append(lh, lt);

  lt = jupiter_strlist_dup_s(": ");
  if (!lt)
    return NULL;

  jupiter_strlist_append(lh, lt);

  lt = jupiter_strlist_dup_s(jcntrl_executive_manager_entry_name(name));
  if (!lt)
    return NULL;

  jupiter_strlist_append(lh, lt);

  lt = jupiter_strlist_dup_s(")");
  if (!lt)
    return NULL;

  jupiter_strlist_append(lh, lt);

  lls = &lp->node.list;
  lle = &lt->node.list;

  lt = jupiter_strlist_join_list(lp, lt, NULL);
  if (!lt)
    return NULL;

  jupiter_strlist_insert_prev(lp, lt);

  geom_list_foreach_range_safe (llp, lln, lls, lle) {
    lp = jupiter_strlist_entry(llp);
    if (!lp)
      continue;

    jupiter_strlist_free(lp);
  }

  return lt;
}

static int
PP_info_map_value_control_format(jupiter_strlist_head *lh,
                                 controllable_geometry_entry *controls)
{
  jcntrl_executive_manager_entry *e;
  int icnt = 0;

  switch (controls->type) {
  case GEOM_VARTYPE_DOUBLE:
    if ((e = controls->control[0].exec)) {
      if (!PP_info_map_value_control_exec(lh, "Controlled by", e))
        return -1;
      ++icnt;
    }
    break;
  case GEOM_VARTYPE_VECTOR2:
  case GEOM_VARTYPE_VECTOR3:
    if ((e = controls->control[0].exec)) {
      if (!PP_info_map_value_control_exec(lh, "X component controlled by", e))
        return -1;
      ++icnt;
    }
    if ((e = controls->control[1].exec)) {
      if (!PP_info_map_value_control_exec(lh, "Y component controlled by", e))
        return -1;
      ++icnt;
    }
    if (controls->type == GEOM_VARTYPE_VECTOR2)
      break;
    if ((e = controls->control[2].exec)) {
      if (!PP_info_map_value_control_exec(lh, "Z component controlled by", e))
        return -1;
      ++icnt;
    }
    break;
  default:
    break;
  }
  return icnt;
}

void PP_info_map_value_format(int *argc, const char ***argv, void *a)
{
  struct pp_format_value_data *v;
  struct PP_info_map_value_data *p;
  const geom_variant *var;
  struct geom_list *llp, *lln, *llh;
  jupiter_strlist *lt;
  int xargc;
  int ncontrol;
  const char **xargv;
  int maxlen;

  PP_info_map_value_delete(a);

  p = (struct PP_info_map_value_data *)a;
  if (!p->node) {
    p->cbuf = NULL;
    *argc = 0;
    *argv = &p->cbuf;
    return;
  }

  var = geom_info_map_get_value(p->node);
  if (!p->node) {
    p->cbuf = NULL;
    *argc = 0;
    *argv = &p->cbuf;
    return;
  }

  /* Populate field controlled parameters */
  ncontrol = 0;
  if (p->control) {
    ncontrol = PP_info_map_value_control_format(&p->lsh, p->control);
    if (ncontrol < 0) {
      p->cbuf = NULL;
      *argc = 0;
      *argv = &p->cbuf;
      return;
    }
  }

  v =
    PP_geom_variant_value_init(&p->var, var, NULL, NULL, p->var.extenum_format,
                               p->var.extenum_delete, p->var.extenum_arg);

  if (ncontrol <= 0) {
    p->cbuf = NULL;
    pp_format_value(v, &p->cbuf, argc, argv);
    if (p->cbuf) {
      *argc = 1;
      *argv = &p->cbuf;
    }
    return;
  }

  p->cbuf = NULL;
  pp_format_value(v, &p->cbuf, &xargc, &xargv);
  if (p->cbuf) {
    xargc = 1;
    xargv = &p->cbuf;
  }

  lt = NULL;
  for (int i = 0; i < xargc; ++i) {
    jupiter_strlist *lp;

    if (!xargv[i])
      continue;

    lp = jupiter_strlist_dup_s(xargv[i]);
    if (!lp) {
      *argc = 1;
      *argv = &p->cbuf;
      return;
    }

    if (!lt) {
      jupiter_strlist_append(&p->lsh, lp);
    } else {
      jupiter_strlist_insert_next(lt, lp);
    }
    lt = lp;
  }

  maxlen = 0;
  llh = &p->lsh.list;
  geom_list_foreach (llp, llh) {
    jupiter_strlist *lp;
    int len;

    lp = jupiter_strlist_entry(llp);
    if (!lp)
      continue;

    len = strlen(lp->buf);
    if (len > maxlen)
      maxlen = len;
  }

  geom_list_foreach_safe (llp, lln, llh) {
    jupiter_strlist *lp, *lt, *ln;
    int len;

    lp = jupiter_strlist_entry(llp);
    if (!lp)
      continue;

    len = strlen(lp->buf);
    if (len >= maxlen)
      continue;

    len = maxlen - len;
    lt = jupiter_strlist_asprintf("%*s", len, "");
    if (!lt) {
      p->cbuf = NULL;
      *argc = 0;
      *argv = &p->cbuf;
      return;
    }

    jupiter_strlist_insert_prev(lp, lt);
    ln = jupiter_strlist_join_list(lt, lp, NULL);
    if (!ln) {
      p->cbuf = NULL;
      *argc = 0;
      *argv = &p->cbuf;
    }

    jupiter_strlist_insert_prev(lt, ln);
    jupiter_strlist_free(lt);
    jupiter_strlist_free(lp);
  }

  lt = jupiter_strlist_join_all(&p->lsh, "\n");
  if (!lt) {
    p->cbuf = NULL;
    *argc = 0;
    *argv = &p->cbuf;
    return;
  }

  jupiter_strlist_free_all(&p->lsh);
  jupiter_strlist_append(&p->lsh, lt);

  p->cbuf = lt->buf;
  *argc = 1;
  *argv = &p->cbuf;
}

const char *PP_info_map_value_null_format(void *a)
{
  struct PP_info_map_value_data *p;
  p = (struct PP_info_map_value_data *)a;

  if (!p->var.value || geom_variant_get_type(p->var.value) == GEOM_VARTYPE_NULL)
    return "(not set)";
  return "(error)";
}

int PP_info_map_value_custom_ok_func(void *a)
{
  struct PP_info_map_value_data *p;
  p = (struct PP_info_map_value_data *)a;

  if (!p->node)
    return 0;

  if (p->okfunc)
    return p->okfunc(p->node, p->okarg);
  return 1;
}

void PP_info_map_value_delete(void *a)
{
  struct PP_info_map_value_data *p;
  p = (struct PP_info_map_value_data *)a;

  if (p->var.value)
    PP_geom_variant_value_delete(&p->var);

  jupiter_strlist_free_all(&p->lsh);
}

//----

const char *PP_info_map_unit_format(void *a)
{
  const char *unit_expr;
  struct PP_info_map_unit_data *p;
  p = (struct PP_info_map_unit_data *)a;

  PP_info_map_unit_delete(a);

  if (!p->node)
    return "";

  unit_expr = geom_info_map_get_unit(p->node);
  if (!unit_expr)
    return "";

  if (p->base_unit || p->length_unit) {
    int r;
    r = geom_info_map_convert_unit(&p->buf, p->base_unit, p->length_unit,
                                   unit_expr);
    if (r < 0) {
      p->buf = NULL;
      return NULL;
    }

    if (*p->buf == '\0' || strcmp(p->buf, "1") == 0) {
      free(p->buf);
      p->buf = NULL;
      return "";
    }

    return p->buf;
  }

  return unit_expr;
}

void PP_info_map_unit_delete(void *a)
{
  struct PP_info_map_unit_data *p;
  p = (struct PP_info_map_unit_data *)a;

  free(p->buf);
  p->buf = NULL;
}

//----

struct PPgeom_info_map_ok_data
{
  geom_size_type index;
  PPgeom_info_map_ok_func *func;
  void *arg;
};

static int PPgeom_info_map_ok_func_impl(geom_info_map *node, void *a)
{
  struct PPgeom_info_map_ok_data *p;
  p = (struct PPgeom_info_map_ok_data *)a;

  if (!node)
    return 0;

  if (p->func)
    return p->func(p->index, geom_info_map_get_value(node), p->arg);
  return 1;
}

void PPgeom_info_map(flags *flg, int indent, const char *base_unit,
                     const char *length_unit, geom_info_map *map_head,
                     controllable_geometry_entry *start, int *nogood,
                     PP_geom_variant_extenumd_format *ext_format,
                     PP_geom_variant_extenumd_delete *ext_delete,
                     void *ext_handler_data,
                     PPgeom_info_map_ok_func *ok_handler, void *ok_handler_data)
{
  struct PP_info_map_name_data name;
  struct PP_info_map_unit_data unit;
  struct PP_info_map_value_data value;
  struct PPgeom_info_map_ok_data ok = {
    .index = 0,
    .func = ok_handler,
    .arg = ok_handler_data,
  };

  geom_info_map *cur;
  for (cur = geom_info_map_next(map_head); cur != map_head;
       cur = geom_info_map_next(cur), ok.index++) {
    struct pp_format_name_data *n;
    struct pp_format_value_data *v;
    struct pp_format_unit_data *u;
    controllable_geometry_entry *e;

    e = NULL;
    if (start) {
      controllable_geometry_entry *ei;
      ei = controllable_geometry_entry_next(start);
      for (; ei != start; ei = controllable_geometry_entry_next(ei)) {
        if (ei->index != ok.index)
          continue;
        e = ei;
        break;
      }
    }

    n = PP_info_map_name_init(&name, cur);
    v = PP_info_map_value_init(&value, cur, e, PPgeom_info_map_ok_func_impl,
                               &ok, ext_format, ext_delete, ext_handler_data);
    u = PP_info_map_unit_init(&unit, cur, base_unit, length_unit);

    pp_m_line_f(flg, indent, PP_BASELEN, n, v, u, nogood);
  }
}
