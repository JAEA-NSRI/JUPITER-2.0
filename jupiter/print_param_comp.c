#include "print_param_comp.h"
#include "csvutil.h"
#include "control/manager.h"
#include "strlist.h"
#include "os/asprintf.h"

#include <limits.h>
#include <stdlib.h>

const char *PP_controllable_type_value_format(void *a)
{
  int r;
  jupiter_strlist *lp;
  struct PP_controllable_type_value_data *p;
  p = (struct PP_controllable_type_value_data *)a;

  PP_controllable_type_value_data_delete(a);

  if (p->value->exec) {
    jupiter_strlist *ln;
    const char *fvname;
    int lnfv;
    int lnval;
    int lnt;

    fvname = jcntrl_executive_manager_entry_name(p->value->exec);
    lp = jupiter_strlist_dup_s(fvname);
    if (!lp)
      return NULL;

    jupiter_strlist_append(&p->lh, lp);

    ln = jupiter_strlist_asprintf("(current value: %.6g)",
                                  p->value->current_value);
    if (!ln)
      return NULL;

    jupiter_strlist_append(&p->lh, ln);

    CSVASSERT(lp->node.len <= INT_MAX);
    CSVASSERT(ln->node.len <= INT_MAX);

    lnfv = lp->node.len;
    lnval = ln->node.len;

    lnt = 0;
    if (lnfv < lnval) {
      lnt = lnval - lnfv;
      ln = lp;
    } else if (lnfv > lnval) {
      lnt = lnfv - lnval;
      ln = ln;
    }
    if (lnt > 0) {
      jupiter_strlist *lt;
      lt = jupiter_strlist_asprintf("%*s", lnt, "");
      if (lt) {
        jupiter_strlist *lx;
        jupiter_strlist_insert_prev(ln, lt);
        lx = jupiter_strlist_join_list(lt, ln, NULL);
        jupiter_strlist_free(lt);
        if (lx) {
          jupiter_strlist_insert_prev(ln, lx);
          jupiter_strlist_free(ln);
        }
      }
    }

    lp = jupiter_strlist_join_all(&p->lh, "\n");
    if (!lp)
      return NULL;

    jupiter_strlist_free_all(&p->lh);
    jupiter_strlist_append(&p->lh, lp);
  } else {
    lp = jupiter_strlist_asprintf("%.6e", p->value->current_value);
    jupiter_strlist_append(&p->lh, lp);
  }
  if (lp)
    return lp->buf;
  return NULL;
}

const char *PP_controllable_type_value_null_format(void *a)
{
  return "(error)";
}

void PP_controllable_type_value_data_delete(void *a)
{
  struct PP_controllable_type_value_data *p;
  p = (struct PP_controllable_type_value_data *)a;

  jupiter_strlist_free_all(&p->lh);
}

int PP_controllable_type_value_custom_okfunc(void *a)
{
  struct PP_controllable_type_value_data *p;
  p = (struct PP_controllable_type_value_data *)a;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

const char *PP_int_pair_value_format(void *arg)
{
  int r;
  struct PP_int_pair_value_data *p;
  p = (struct PP_int_pair_value_data *)arg;

  PP_int_pair_value_delete(arg);

  r = jupiter_asprintf(&p->buf, "(%d%s%d)", p->a, p->sep ? p->sep : " ", p->b);
  if (r < 0) {
    p->buf = NULL;
    return NULL;
  }

  return p->buf;
}

const char *PP_int_pair_value_null_format(void *arg) { return "(error)"; }

int PP_int_pair_value_custom_okfunc(void *arg)
{
  struct PP_int_pair_value_data *p;
  p = (struct PP_int_pair_value_data *)arg;

  if (p->okfunc)
    return p->okfunc(p->a, p->b, p->okdata);

  return 1;
}

void PP_int_pair_value_delete(void *arg)
{
  struct PP_int_pair_value_data *p;
  p = (struct PP_int_pair_value_data *)arg;

  free(p->buf);
  p->buf = NULL;
}
