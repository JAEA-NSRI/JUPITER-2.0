#include "component_info.h"
#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "csv.h"
#include "csvutil.h"
#include "strlist.h"
#include "struct.h"
#include "os/asprintf.h"

#include <string.h>
#include <stdlib.h>

void *component_info__alloc(struct general_vector_node *a, int n)
{
  struct component_info_data *d;
  component_info *c;

  c = component_info__getter(a);
  d = (struct component_info_data *)malloc(sizeof(struct component_info_data) *
                                           n);
  *(struct component_info_data **)&c->d = d;
  return d;
}

void component_info__delete(struct general_vector_node *a)
{
  component_info *c;
  c = component_info__getter(a);
  free(c->d);
  *(struct component_info_data **)&c->d = NULL;
}

void component_info__copy(struct general_vector_node *to, int ts,
                          struct general_vector_node *from, int fs, int n)
{
  component_info *tc, *fc;
  tc = component_info__getter(to);
  fc = component_info__getter(from);
  memmove(tc->d + ts, fc->d + fs, sizeof(struct component_info_data) * n);
}

static int
set_component_info_f_is_required(int index,
                                 const struct set_component_info_funcs_data *f)
{
  if (f->is_required_func)
    return f->is_required_func(index, f->arg);
  return f->is_required;
}

static int
set_component_info_f_default(int index,
                             const struct set_component_info_funcs_data *f)
{
  if (f->default_id_func)
    return f->default_id_func(index, f->arg);
  return f->default_id;
}

int set_component_info(struct component_info *info, csv_data *csv,
                       const char *fname, csv_row **row, csv_column **column,
                       struct component_data *comp_data_head,
                       int number_of_comp, int append, int allow_dups,
                       int need_sorted,
                       const struct set_component_info_funcs_data *funcs,
                       int *stat)
{
  component_info tmp;
  csv_column **dup_columns;
  int i, j, prev_id;
  int ncompo;
  int astat;
  SET_P_INIT(csv, fname, row, column);

  CSVASSERT(number_of_comp >= 0);

  component_info_init(&tmp);

  ncompo = 0;
  if (append) {
    ncompo = component_info_ncompo(info);
    component_info_share(&tmp, info);
  }

  astat = !!component_info_resize(&tmp, ncompo + number_of_comp, 1);
  if (!astat) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
  }

  if (number_of_comp == 0) {
    if (astat)
      component_info_share(info, &tmp);
    component_info_clear(&tmp);
    return astat;
  }

  dup_columns = NULL;
  if (!allow_dups)
    dup_columns = (csv_column **)calloc(number_of_comp, sizeof(csv_column *));

  for (i = 0; i < number_of_comp; ++i) {
    int is_req = set_component_info_f_is_required(i, funcs);
    struct component_info_data data;
    struct csv_to_component_info_data_data cdata = {
      .comp_data_head = comp_data_head,
      .dest = &data,
    };

    if (is_req) {
      SET_P_NEXT_REQUIRED(&cdata, component_info_data, JUPITER_ID_INVALID,
                          stat);
    } else {
      int defval = set_component_info_f_default(i, funcs);
      SET_P_NEXT(&cdata, component_info_data, defval);
    }

    if (funcs->validator) {
      char *msg;
      const char *cmsg;
      int ret;

      msg = NULL;
      cmsg = NULL;
      ret = funcs->validator(&msg, &cmsg, &data, i, funcs->arg);
      if (!ret) {
        if (msg)
          cmsg = msg;
        if (!cmsg)
          cmsg = "Unusable component ID here";
        SET_P_PERROR(ERROR, "%s", cmsg);
        if (stat)
          *stat = ON;
      }
      if (msg)
        free(msg);
    }

    if (dup_columns)
      dup_columns[i] = SET_P_SOURCE_COL();

    if (astat) {
      tmp.d[ncompo + i] = data;

      if (!allow_dups && data.d) {
        for (int j = 0; j < i; ++j) {
          if (tmp.d[ncompo + j].d != data.d)
            continue;

          if (dup_columns) {
            SET_P_PERROR(ERROR,
                         "This ID is already defined at line %ld, column %ld",
                         getCSVTextLineOrigin(dup_columns[j]),
                         getCSVTextColumnOrigin(dup_columns[j]));
          } else {
            SET_P_PERROR(ERROR, "This ID is already defined");
          }
          if (stat)
            *stat = ON;
          break;
        }
      }
    }

    if (i > 0) {
      if (need_sorted > 0 && data.id < prev_id) {
        SET_P_PERROR(ERROR, "IDs must be inputted in ascending order");
        if (stat)
          *stat = ON;
      }
      if (need_sorted < 0 && data.id > prev_id) {
        SET_P_PERROR(ERROR, "IDs must be inputted in descending order");
        if (stat)
          *stat = ON;
      }
    }
    prev_id = data.id;

    if (funcs->extra_proc) {
      funcs->extra_proc(&data, i, funcs->arg);
    }
  }

  if (astat)
    component_info_share(info, &tmp);
  component_info_clear(&tmp);

  free(dup_columns);
  return astat;
}

static int set_component_info_make_message(
  char **msgout, const struct component_info_data *d, int index,
  struct set_component_info_default_validator_data *arg,
  jupiter_strlist_head *lst)
{
  jupiter_strlist *lsp;
  int n;

  n = 0;
  if (arg->accept_n1_gas) {
    ++n;
    lsp = jupiter_strlist_dup_s("-1");
    if (!lsp)
      return -1;

    jupiter_strlist_append(lst, lsp);
  }
  if (component_phases_has_solid(arg->phases)) {
    ++n;
    lsp = jupiter_strlist_dup_s("Solid");
    if (!lsp)
      return -1;

    jupiter_strlist_append(lst, lsp);
  }
  if (component_phases_has_liquid(arg->phases)) {
    ++n;
    lsp = jupiter_strlist_dup_s("Liquid");
    if (!lsp)
      return -1;

    jupiter_strlist_append(lst, lsp);
  }
  if (component_phases_has_gas(arg->phases)) {
    ++n;
    lsp = jupiter_strlist_dup_s("Gas");
    if (!lsp)
      return -1;

    jupiter_strlist_append(lst, lsp);
  }
  if (n <= 0)
    return -1;

  if (n == 2) {
    lsp = jupiter_strlist_join_all(lst, " or ");
    if (!lsp)
      return -1;

    jupiter_strlist_free_all(lst);
    jupiter_strlist_append(lst, lsp);

  } else if (n > 2) {
    jupiter_strlist *lsn, *lsm;
    lsn = jupiter_strlist_dup_s("or ");
    if (!lsn)
      return -1;

    jupiter_strlist_insert_prev(lsp, lsn);
    lsm = jupiter_strlist_join_list(lsn, lsp, "");
    if (!lsm)
      return -1;

    jupiter_strlist_insert_prev(lsp, lsm);
    jupiter_strlist_delete(lsp);
    jupiter_strlist_delete(lsn);

    lsp = jupiter_strlist_join_all(lst, ", ");
    if (!lsp)
      return -1;

    jupiter_strlist_free_all(lst);
    jupiter_strlist_append(lst, lsp);
  }

  lsp = jupiter_strlist_dup_s("Invalid Component ID: It must be one of ");
  if (!lsp)
    return -1;

  jupiter_strlist_prepend(lst, lsp);

  lsp = jupiter_strlist_dup_s(" components");
  if (!lsp)
    return -1;

  jupiter_strlist_append(lst, lsp);

  lsp = jupiter_strlist_join_all(lst, "");
  if (!lsp)
    return -1;

  jupiter_strlist_append(lst, lsp);
  return jupiter_asprintf(msgout, "%s", lsp->buf);
}

int set_component_info_default_validator(
  char **msgout, const char **cmsgout, const struct component_info_data *d,
  int index, struct set_component_info_default_validator_data *arg)
{
  int valid = 0;
  int r;
  jupiter_strlist_head lst;

  if (!arg)
    return 1;

  CSVASSERT(arg->accept_n1_gas || component_phases_is_valid(arg->phases));

  if (d->id == -1) {
    CSVASSERT(d->d);
    if (arg->accept_n1_gas)
      valid = 1;
  } else if (d->d) {
    component_phases p;
    p = component_phases_band(d->d->phases, arg->phases);
    if (component_phases_any(p))
      valid = 1;
  }

  if (valid)
    return valid;

  jupiter_strlist_head_init(&lst);
  r = set_component_info_make_message(msgout, d, index, arg, &lst);
  jupiter_strlist_free_all(&lst);

  if (r < 0) {
    *msgout = NULL;
    *cmsgout = "Invalid Component ID";
  }
  return valid;
}
