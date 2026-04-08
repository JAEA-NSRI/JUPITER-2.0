#include "print_param_core.h"
#include "csvutil.h"
#include "geometry/list.h"
#include "strlist.h"
#include "struct.h"
#include "os/asprintf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

int pp_for_all_mpi(flags *flg, int cond)
{
#ifdef JUPITER_MPI
  if (flg->list_fp_comm == MPI_COMM_NULL)
    return cond;

  MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LAND, flg->list_fp_comm);
#endif

  return cond;
}

int pp_for_any_mpi(flags *flg, int cond)
{
#ifdef JUPITER_MPI
  if (flg->list_fp_comm == MPI_COMM_NULL)
    return cond;

  MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, flg->list_fp_comm);
#endif

  return cond;
}

void print_param_supportn(flags *flg, const char *text)
{
  int r;
  const char *mpi_cc = "[RANK?] ";
  const char *mpi_ss = "[*****] ";
  char *mpi_rc;
  FILE *fp;
  const char *fpn;
  char *l;
  char *dp;
  const char *cp;
  const char *np;
  size_t n;
  size_t nmpi_cc;
  int nnl;
  int all_rank_text_same;
  int print_rank;
#ifdef JUPITER_MPI
  int rank;
  MPI_File mfp;
#endif

  if (!flg->list_fp_name || strcmp(flg->list_fp_name, "") == 0) {
    return;
  }
  fp = flg->list_fp;
  fpn = flg->list_fp_name;
#ifdef JUPITER_MPI
  mfp = flg->list_fp_mpi;
#endif
  mpi_rc = NULL;
  l = NULL;
  all_rank_text_same = 0;

#ifdef JUPITER_MPI
  rank = -1;
  MPI_Initialized(&r);
  if (r) {
    MPI_Finalized(&r);
    r = !r;
  }
  if (r) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  }
#endif

#ifdef JUPITER_MPI
  if (mfp != MPI_FILE_NULL) {
    int nltxt, i;
    int crank, cproc;
    int cond;
    MPI_Comm comm;
    CSVASSERT(flg->list_fp_comm != MPI_COMM_NULL);

    comm = flg->list_fp_comm;
    MPI_Comm_rank(comm, &crank);
    MPI_Comm_size(comm, &cproc);

    nltxt = strlen(text) + 1;
    i = nltxt;
    cond = 1;
    if (crank == 0) {
      print_rank = rank;
    }
    MPI_Bcast(&print_rank, 1, MPI_INT, 0, comm);
    if (cproc > 1) {
      if (crank == 0) {
        MPI_Recv(&i, 1, MPI_INT, 1, 1, comm, MPI_STATUS_IGNORE);
      } else if (crank == cproc - 1) {
        MPI_Send(&nltxt, 1, MPI_INT, crank - 1, crank, comm);
      } else {
        MPI_Sendrecv(&nltxt, 1, MPI_INT, crank - 1, crank, &i, 1, MPI_INT,
                     crank + 1, crank + 1, comm, MPI_STATUS_IGNORE);
      }
      cond = (nltxt != i);
      MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, comm);
    }
    if (!cond) {
      char *buf;
      buf = (char *)malloc(sizeof(char) * nltxt);
      cond = (buf == NULL);
      MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, comm);
      if (!cond) {
        if (crank == 0) {
          MPI_Recv(buf, nltxt, MPI_CHAR, 1, 1, comm, MPI_STATUS_IGNORE);
        } else if (crank == cproc - 1) {
          MPI_Send(text, nltxt, MPI_CHAR, crank - 1, crank, comm);
        } else {
          MPI_Sendrecv(text, nltxt, MPI_CHAR, crank - 1, crank, buf, nltxt,
                       MPI_CHAR, crank + 1, crank + 1, comm, MPI_STATUS_IGNORE);
        }
        nltxt = 0;
        if (crank < cproc - 1) {
          nltxt = strcmp(text, buf);
        }
        all_rank_text_same = (nltxt == 0);
        MPI_Allreduce(MPI_IN_PLACE, &all_rank_text_same, 1, MPI_INT, MPI_LAND,
                      comm);
      } else {
        cond = 0;
      }
      free(buf);
    }
  }
#endif

#ifdef JUPITER_MPI
  if (all_rank_text_same) {
    mpi_cc = mpi_ss;
    if (rank != print_rank) {
      mpi_cc = NULL;
    }
  } else {
    if (rank >= 0) {
      r = jupiter_asprintf(&mpi_rc, "[%5d] ", rank);
      if (r < 0) {
        mpi_rc = NULL;
      } else {
        mpi_cc = mpi_rc;
      }
    }
  }
#else
  mpi_cc = NULL;
#endif

  if (mpi_cc) {
    nnl = 0;
    cp = text;
    while ((np = strchr(cp, '\n'))) {
      nnl++;
      cp = np + 1;
    }
    if (*cp != '\0') {
      nnl++;
    }

    nmpi_cc = strlen(mpi_cc);
    n = strlen(text) + nnl * nmpi_cc + 2; /* 2 is for '\n' and '\0'. */
    l = (char *)malloc(sizeof(char) * n);
    if (l) {
      dp = l;
      cp = text;
      while (1) {
        np = strchr(cp, '\n');
        if (np) {
          memcpy(dp, mpi_cc, nmpi_cc);
          dp += nmpi_cc;
          n = np - cp + 1;
          memcpy(dp, cp, n);
          dp += n;
          cp = np + 1;
        } else {
          if (*cp != '\0') {
            memcpy(dp, mpi_cc, nmpi_cc);
            dp += nmpi_cc;
            strcpy(dp, cp);
            dp += strlen(cp);
            *dp++ = '\n';
          }
          *dp++ = '\0';
          break;
        }
      }

      text = l;
    }
  } else {
    n = strlen(text);
    if (n == 0 || text[n - 1] != '\n') {
      r = jupiter_asprintf(&l, "%s\n", text);
      if (r < 0) {
        l = NULL;
        text = "!!! Allocation error while printing out mesage\n";
      } else {
        text = l;
      }
    }
  }

  n = strlen(text);
#ifdef JUPITER_MPI
  if (mfp != MPI_FILE_NULL) {
    if (all_rank_text_same) {
      if (print_rank != rank) {
        text = "";
        n = 0;
      }
    }
    MPI_File_write_ordered(flg->list_fp_mpi, text, n, MPI_CHAR,
                           MPI_STATUS_IGNORE);
  }
#endif
  if (fp) {
    fwrite(text, sizeof(char), n, flg->list_fp);
  }

  free(mpi_rc);
  free(l);
}

void print_param_supportv(flags *flg, const char *format, va_list ap)
{
  char *tc;
  int r;

  CSVASSERT(flg);

  r = jupiter_vasprintf(&tc, format, ap);
  if (r >= 0) {
    print_param_supportn(flg, tc);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
  }

  free(tc);
}

void print_param_support(flags *flg, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  print_param_supportv(flg, format, ap);
  va_end(ap);
}

void print_param_header(flags *flg, char border_char, int indent, int pos,
                        int len, const char *format, ...)
{
  int c, r;
  char *b;
  char *t;
  va_list ap, aq;

  CSVASSERT(pos >= 0);
  CSVASSERT(len >= 0);

  va_start(ap, format);
  va_copy(aq, ap);
  r = jupiter_vasprintf(&t, format, ap);
  va_end(ap);

  if (r < 0)
    goto print_param_header_rescue;

  c = strlen(t);

  /* + 2 is for space */
  if ((pos + c + 2) > len) {
    len = pos + c + 2;
  }

  b = (char *)malloc(sizeof(char) * (len + 1));
  if (!b) {
    free(t);
    goto print_param_header_rescue;
  }

  memset(b, border_char, len);
  strncpy(b + pos + 1, t, c); /* do not copy NUL. */
  b[pos] = ' ';
  b[pos + c + 1] = ' ';
  b[len] = '\0';

  print_param_support(flg, "%*s%s", indent, "", b);

  free(t);
  free(b);
  va_end(aq);
  return;

print_param_header_rescue:
  print_param_supportv(flg, format, aq);
  va_end(aq);
  return;
}

/* YSE: Added value_null which is printing text if value is NULL. */
/**
 * @brief Print multiple line value
 *
 * @param f        Flag data (used to get output destination)
 * @param indent   indent width
 * @param baselen  Base length (in chars, including indent)
 * @param name     Decription of the value
 * @param argc     Number of texts
 * @param argv     Value texts
 * @param value_null Value text when value is NULL.
 * @param unit     Unit text
 * @param ok       Whether this parameter fits for the calculation
 * @param nogood   if \p ok is 0 sets non-0 value.
 *
 * Values are right-justified at @p baselen, if @p baselen > @p indent. (Actual
 * column will be shifted righted by rank-number indication and space for
 * bullets)
 *
 * Each text should not contain control characters except '\n'.
 *
 * If \p value_null is NULL, prints name only.
 */
static void pp_m_line(flags *f, int indent, int baselen, const char *name,
                      int argc, const char **argv, const char *value_null,
                      const char *unit, int ok, int *nogood)
{
  struct jupiter_strlist_head lhn, lhd;
  struct jupiter_strlist *llp, *lln, *llt;
  struct geom_list *lp, *ln, *lh;
  const char *value;
  const char *bullet = " * ";
  const char *sepr = ": ";
  int i;
  int nw;

  CSVASSERT(argc >= 0);
  CSVASSERT(argv || (argc == 0 && !argv));

  jupiter_strlist_head_init(&lhn);
  jupiter_strlist_head_init(&lhd);

  if (indent < 0)
    indent = 0;
  if (baselen > indent)
    baselen -= indent;

  llp = jupiter_strlist_dup_s(name);
  if (!llp)
    goto error;

  jupiter_strlist_append(&lhn, llp);

  if (!jupiter_strlist_split_ch(llp, '\n'))
    goto error;

  jupiter_strlist_free_all(&lhd);

  value = NULL;
  if (argc > 0) {
    value = argv[0];
    for (int i = 1; i < argc; ++i)
      if (!argv[i])
        value = NULL;
  }

  if (value || value_null) {
    int vallen;   ///< Max number of columns
    int lenfst;   ///< Length of first line
    int nlines;   ///< Number of lines
    int cuttable; ///< Number of leading spaces at first line
    int cut;      ///< Number of spaces to cut at first line

    /* Count number of lines and value width */
    if (value) {
      for (i = 0; i < argc; ++i) {
        llp = jupiter_strlist_dup_s(argv[i]);
        if (!llp)
          goto error;

        jupiter_strlist_append(&lhd, llp);
      }

      llp = jupiter_strlist_join_all(&lhd, "\n");
      jupiter_strlist_free_all(&lhd);
      if (!llp)
        goto error;
    } else {
      llp = jupiter_strlist_dup_s(value_null);
      if (!llp)
        goto error;

      unit = NULL;
    }
    jupiter_strlist_append(&lhd, llp);

    if (!jupiter_strlist_split_ch(llp, '\n'))
      goto error;

    nlines = 0;
    vallen = 0;
    lenfst = 0;

    lh = &lhd.list;
    geom_list_foreach (lp, lh) {
      int n;

      llp = jupiter_strlist_entry(lp);
      if (!llp)
        continue;

      nlines++;
      n = strlen(llp->buf);
      if (n > vallen)
        vallen = n;

      if (nlines == 1)
        lenfst = n;
    }

    /* Count leading space can be removable */
    cuttable = 0;
    if (lenfst > 0 && vallen > 0) {
      int n;
      llp = jupiter_strlist_entry(geom_list_next(lh));
      for (i = 0; i < lenfst; ++i) {
        if (llp->buf[i] != ' ' || llp->buf[i] == '\0')
          break;
        cuttable++;
      }
      if (llp->buf[i] == '\0') /* consists space only */
        cuttable = vallen;
    }

    llp = jupiter_strlist_entry(geom_list_prev(&lhn.list));
    if (!llp)
      goto error;

    /* Add value-label separator */
    lln = jupiter_strlist_dup_s(sepr);
    if (!lln)
      goto error;

    jupiter_strlist_insert_next(llp, lln);

    if (!(llt = jupiter_strlist_join_list(llp, lln, NULL)))
      goto error;

    jupiter_strlist_insert_prev(llp, llt);
    jupiter_strlist_free(llp);
    jupiter_strlist_free(lln);

    /* Check value fits the last line of label */
    cut = 0;
    nw = strlen(llt->buf);
    if (baselen < 0) { /* Justify left */
      baselen = nw + vallen;
      if (cuttable > 0) {
        cut = (nw > cuttable) ? cuttable : nw;
        baselen -= cut;
      }
    }
    if (nw + vallen - cuttable <= baselen) {
      if (nw + vallen > baselen)
        cut = nw + vallen - baselen;
      nw = baselen - nw;
    } else {
      nw = baselen;
      llt = NULL;
    }

    i = 0;
    lh = &lhd.list;
    geom_list_foreach_safe (lp, ln, lh) {
      struct jupiter_strlist *llx;
      struct geom_list *lxp, *lxn, *lxs, *lxe;
      int n;

      llp = jupiter_strlist_entry(lp);
      if (!llp)
        continue;

      if (cut > 0) {
        if (cut >= lenfst) {
          llp->buf[0] = '\0';
        } else {
          memmove(llp->buf, llp->buf + cut, lenfst - cut + 1);
        }
      }

      n = nw - vallen - cut;
      if (n > 0) {
        llx = jupiter_strlist_asprintf("%*s", n, "");
        if (!llx)
          goto error;

        jupiter_strlist_append(&lhn, llx);
        if (!llt)
          llt = llx;
      }

      jupiter_strlist_delete(llp);
      jupiter_strlist_append(&lhn, llp);
      if (!llt)
        llt = llp;
      llx = llp;

      if (i == (nlines - 1) / 2) {
        int print;
        int n;

        n = strlen(llp->buf);
        n = vallen - n - cut;
        n += 1;

        print = unit && *unit != '\0';
        print = print || !ok;

        CSVASSERT(n > 0);

        if (print) {
          llx = jupiter_strlist_asprintf("%*s", n, "");
          if (!llx)
            goto error;

          jupiter_strlist_append(&lhn, llx);

          if (unit && *unit != '\0') {
            llx = jupiter_strlist_dup_s(unit);
            if (!llx)
              goto error;

            jupiter_strlist_append(&lhn, llx);
          } else {
            unit = NULL;
          }

          if (!ok) {
            if (unit) {
              llx = jupiter_strlist_dup_s(" " PP_INVALID_MARK);
            } else {
              llx = jupiter_strlist_dup_s(PP_INVALID_MARK);
            }
            if (!llx)
              goto error;

            jupiter_strlist_append(&lhn, llx);
          }
        }
      }

      if (!(llp = jupiter_strlist_join_list(llt, llx, NULL)))
        goto error;

      jupiter_strlist_insert_prev(llt, llp);
      lxs = &llt->node.list;
      lxe = geom_list_next(&llx->node.list);
      geom_list_foreach_range_safe (lxp, lxn, lxs, lxe) {
        llp = jupiter_strlist_entry(lxp);
        if (llp)
          jupiter_strlist_free(llp);
      }

      llt = NULL;
      cut = 0;
      nw = baselen;
      ++i;
    }
  }

  lln = jupiter_strlist_entry(geom_list_next(&lhn.list));
  if (!lln)
    goto error;

  llp = jupiter_strlist_asprintf("%*s%s", indent, "", bullet);
  if (!llp)
    goto error;

  jupiter_strlist_prepend(&lhn, llp);

  llt = jupiter_strlist_join_list(llp, lln, NULL);
  if (!llt)
    goto error;

  jupiter_strlist_prepend(&lhn, llt);
  jupiter_strlist_free(llp);
  jupiter_strlist_free(lln);

  nw = strlen(bullet);
  lln = jupiter_strlist_asprintf("\n%*s%*s", indent, "", nw, "");
  if (!lln)
    goto error;

  jupiter_strlist_append(&lhd, lln);

  if (!(llp = jupiter_strlist_join_all(&lhn, lln->buf)))
    goto error;

  jupiter_strlist_free_all(&lhn);
  jupiter_strlist_append(&lhn, llp);

  print_param_supportn(f, llp->buf);

  if (!ok)
    *nogood = ON;

clean:
  jupiter_strlist_free_all(&lhn);
  jupiter_strlist_free_all(&lhd);
  return;

error:
  print_param_support(f, "%s%s: (error while format)", bullet, name);
  goto clean;
}

/**
 * @brief Print one line value
 *
 * @param f        Flag data (used to get output destination)
 * @param indent   indent width
 * @param namelen  maximum name length (in chars)
 * @param valuelen minimum value length (in chars)
 * @param cut      cut out name text if name is longer than \p namelen
 * @param name     Description of the value
 * @param value    Value text
 * @param value_null Value text when value is NULL
 * @param unit     Unit text
 * @param ok       Whether this parameter fits for the calculation
 * @param nogood   if \p ok is 0 sets non-0 value
 *
 * Each text must not contain control characters (especially '\n').
 */
static void pp_1_line(flags *f, int indent, int baselen, const char *name,
                      const char *value, const char *value_null,
                      const char *unit, int ok, int *nogood)
{
  pp_m_line(f, indent, baselen, name, 1, &value, value_null, unit, ok, nogood);
}

//-------------------------

struct pp_format_name_data_x
{
  flags *flg;
  struct pp_format_name_data *p;
  const char *ret;
};

static void pp_format_name_impl(void *a)
{
  struct pp_format_name_data_x *x;
  x = (struct pp_format_name_data_x *)a;
  x->ret = x->p->name(x->p->data);
}

static const char *pp_format_name_f(flags *flg, struct pp_format_name_data *n)
{
  struct pp_format_name_data_x d = {.flg = flg, .p = n, .ret = NULL};
  pp_format_name_impl(&d);
  return d.ret;
}

const char *pp_format_name(struct pp_format_name_data *n)
{
  return pp_format_name_f(NULL, n);
}

//--------

struct pp_format_value_data_x
{
  flags *flg;
  struct pp_format_value_data *p;
  pp_format_func *f;
  pp_formatv_func *v;
  pp_format_okfunc *okf;
  const char **retp;
  int *argc;
  const char ***argv;
  int ok;
};

static void pp_format_value_impl(void *a)
{
  struct pp_format_value_data_x *x;
  x = (struct pp_format_value_data_x *)a;

  if (x->v) {
    x->v(x->argc, x->argv, x->p->data);
    *x->retp = NULL;
  } else {
    *x->retp = x->f(x->p->data);
  }

  x->ok = 1;
  if (x->okf) {
    x->ok = x->okf(x->p->data);
  }
}

static int pp_format_value_f(flags *flg, struct pp_format_value_data *p,
                             const char **retp, int *argc, const char ***argv)
{
  struct pp_format_value_data_x d = {
    .flg = flg,
    .argc = argc,
    .argv = argv,
    .f = p->value,
    .v = p->mline_value,
    .p = p,
    .retp = retp,
    .okf = p->okfunc,
  };
  pp_format_value_impl(&d);
  return d.ok;
}

int pp_format_value(struct pp_format_value_data *v, const char **retp,
                    int *argc, const char ***argv)
{
  return pp_format_value_f(NULL, v, retp, argc, argv);
}

static const char *pp_format_value_null_f(flags *flg,
                                          struct pp_format_value_data *p)
{
  const char *ret;
  struct pp_format_value_data_x d = {
    .flg = flg,
    .argc = NULL,
    .argv = NULL,
    .f = p->value_null,
    .v = NULL,
    .p = p,
    .retp = &ret,
    .okf = NULL,
  };
  pp_format_value_impl(&d);
  return ret;
}

const char *pp_format_value_null(struct pp_format_value_data *v)
{
  return pp_format_value_null_f(NULL, v);
}

//-------

struct pp_format_unit_data_x
{
  flags *flg;
  struct pp_format_unit_data *p;
  const char *ret;
};

static void pp_format_unit_impl(void *a)
{
  struct pp_format_unit_data_x *x;
  x = (struct pp_format_unit_data_x *)a;
  x->ret = x->p->unit(x->p->data);
}

static const char *pp_format_unit_f(flags *flg, struct pp_format_unit_data *n)
{
  struct pp_format_unit_data_x d = {.flg = flg, .p = n, .ret = NULL};
  pp_format_unit_impl(&d);
  return d.ret;
}

const char *pp_format_unit(struct pp_format_unit_data *u)
{
  return pp_format_unit_f(NULL, u);
}

//-------

static void pp_format_print(struct pp_m_line_g_data *d)
{
  const char *name_str;
  const char *value_str;
  const char *unit_str;
  const char *null_str;
  int value_argc = 0;
  const char **value_argv = NULL;
  int ok;

  name_str = pp_format_name_f(d->f, d->name);
  if (!name_str)
    name_str = "**name format error**";

  unit_str = pp_format_unit_f(d->f, d->unit);
  if (!unit_str)
    unit_str = "*err*";

  ok = pp_format_value_f(d->f, d->value, &value_str, &value_argc, &value_argv);

  null_str = pp_format_value_null_f(d->f, d->value);

  if (value_str) {
    pp_1_line(d->f, d->indent, d->baselen, name_str, value_str, null_str,
              unit_str, ok, d->nogood);
  } else {
    pp_m_line(d->f, d->indent, d->baselen, name_str, value_argc, value_argv,
              null_str, unit_str, ok, d->nogood);
  }
}

void pp_format_name_clean(struct pp_format_name_data *n)
{
  if (n->name_delete)
    n->name_delete(n->data);
}

void pp_format_value_clean(struct pp_format_value_data *v)
{
  if (v->value_delete)
    v->value_delete(v->data);
}

void pp_format_unit_clean(struct pp_format_unit_data *u)
{
  if (u->unit_delete)
    u->unit_delete(u->data);
}

static void pp_m_line_data_clean(struct pp_m_line_g_data *d)
{
  pp_format_name_clean(d->name);
  pp_format_value_clean(d->value);
  pp_format_unit_clean(d->unit);
}

static void pp_m_line_g_impl(void *a)
{
  int ok;
  struct pp_m_line_g_data *d;
  d = (struct pp_m_line_g_data *)a;

  pp_format_print(d);
  pp_m_line_data_clean(d);
}

void pp_m_line_g(struct pp_m_line_g_data *data) { pp_m_line_g_impl(data); }
