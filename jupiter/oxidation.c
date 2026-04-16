
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "component_data.h"
#include "component_data_defs.h"
#include "component_info.h"
#include "component_info_defs.h"
#include "component_info_frac.h"
#include "csv.h"
#include "struct.h"
#include "func.h"
#include "csvutil.h"
#include "oxidation.h"
#include "os/os.h"

#include "materials.h"
#include "common_util.h"
#include "tempdep_calc.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

int ox_set_component_info(struct ox_component_info *info,
                          component_data *comp_data_head, int max_num_compo,
                          int read_fraction, int optional, const char *keyname,
                          csv_data *csv, const char *fname, csv_row **row,
                          csv_column **col,
                          ox_set_component_info_id_check *id_check,
                          ox_set_component_info_frac_check *frac_check,
                          void *arg, int *stat)
{
  int nc;
  int i;
  int ret;
  csv_row *xrow;
  csv_column *xcol;
  SET_P_INIT(csv, fname, row ? row : &xrow, col ? col : &xcol);

  CSVASSERT(info);
  CSVASSERT(keyname);

  ret = 0;

  if (max_num_compo != 1) {
    SET_P(&nc, int, keyname, 1, -1);
    if (nc <= 0) {
      SET_P_PERROR(ERROR, "Number of component ids must be positive",
                   max_num_compo - 1);
      ret = 1;
    }
  } else {
    nc = 1;
  }

  if (ret) {
    if (stat)
      *stat = ON;
    return ret;
  }

  if (!ox_component_info_resize(info, nc, 0)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
    return 2;
  }

  for (int ic = 0; ic < nc; ++ic) {
    type frac;
    struct component_info_data *data = ox_component_info_getcp(info, ic);
    struct csv_to_component_info_data_data cid = {
      .comp_data_head = comp_data_head,
      .dest = data,
    };

    if (!SET_P_SOURCE_ROW()) {
      SET_P(&cid, component_info_data, keyname, 1, -1);
    } else {
      SET_P_NEXT(&cid, component_info_data, -1);
    }

    if (nc == 1 && optional && data->id == -1) {
      ox_component_info_clear(info);
      nc = 0;
      break;
    } else if (!data->d) {
      ret = 1;
    }
    if (data->d) {
      for (int ix = 0; ix < ic; ++ix) {
        if (ox_component_info_getc(info, ix) == data->d)
          SET_P_PERROR(WARN, "Duplicated ID found");
      }
    }
    if (data->d && id_check) {
      int r;
      r = id_check(ic, data->id, data->d, fname, SET_P_SOURCE_ROW(),
                   SET_P_SOURCE_COL(), arg);
      if (!r) {
        data->d = NULL;
        ret = 1;
      }
    }

    if (nc > 1 && read_fraction) {
      CSVASSERT(!!SET_P_SOURCE_ROW());
      SET_P_NEXT(&frac, double, HUGE_VAL);
      if (!SET_P_PERROR_GREATER(frac, 0.0, OFF, OFF, ERROR,
                                "Fraction must be positive")) {
        ret = 1;
      } else if (frac_check) {
        int r;
        r = frac_check(ic, data->id, data->d, frac, fname, SET_P_SOURCE_ROW(),
                       SET_P_SOURCE_COL(), arg);
        if (!r)
          ret = 1;
      }
    } else {
      if (read_fraction) {
        frac = 1.0;
      } else {
        frac = 0.0;
      }
    }

    ox_component_info_setf(info, ic, frac);
  }

  if (!ret && read_fraction) {
    type fsum;

    fsum = 0.0;
    for (i = 0; i < nc; ++i) {
      fsum += ox_component_info_getf(info, i);
      if (!isfinite(fsum)) {
        if (SET_P_SOURCE_ROW())
          csvperrorf_row(fname, SET_P_SOURCE_ROW(), 0, CSV_EL_ERROR,
                         "Sum of fractions overflowed");
        ret = 1;
        break;
      }
    }

    if (fsum <= 0.0) {
      if (SET_P_SOURCE_ROW())
        csvperrorf_row(fname, SET_P_SOURCE_ROW(), 0, CSV_EL_ERROR,
                       "Sum of fractions is zero");
      ret = 1;
    }

    if (!ret) {
      for (i = 0; i < nc; ++i) {
        type f = ox_component_info_getf(info, i);
        ox_component_info_setf(info, i, f / fsum);
      }
    }
  }

  component_info_frac_uniq(&info->comps);
  component_info_frac_sort(&info->comps);

  if (ret) {
    if (stat)
      *stat = ON;
  }
  return ret;
}

int ox_set_component_info_id_check_zry(int idx, int id, component_data *d,
                                       const char *fname, csv_row *row,
                                       csv_column *col, void *arg)
{
  if (!component_phases_has_solid(d->phases)) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "Component for Zircaloy must be available for solid");
    return 0;
  }
  return 1;
}

int ox_set_component_info_id_check_zro2(int idx, int id, component_data *d,
                                        const char *fname, csv_row *row,
                                        csv_column *col, void *arg)
{
  if (!component_phases_has_solid(d->phases)) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "Component for ZrO2 must be available for solid");
    return 0;
  }
  return 1;
}

int ox_set_component_info_id_check_h2(int idx, int id, component_data *d,
                                      const char *fname, csv_row *row,
                                      csv_column *col, void *arg)
{
  if (d->jupiter_id == -1) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "ID -1 cannot be used for Hydrogen");
    return 0;
  }

  if (!component_phases_has_gas(d->phases)) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "Component for Hydrogen must be available for gas");
    return 0;
  }
  return 1;
}

int ox_set_component_info_id_check_h2o(int idx, int id, component_data *d,
                                       const char *fname, csv_row *row,
                                       csv_column *col, void *arg)
{
  if (d->jupiter_id == -1) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "ID -1 cannot be used for Steam (H2O)");
    return 0;
  }

  if (!component_phases_has_gas(d->phases)) {
    if (col)
      csvperrorf_col(fname, col, CSV_EL_ERROR,
                     "Component for Steam (H2O) must be available for gas");
    return 0;
  }
  return 1;
}

int ox_component_make_Yarr(type *Yret, int *idret, int oncompo, type *Y,
                           ptrdiff_t m, ptrdiff_t index, int incompo,
                           struct ox_component_info *comp_info)
{
  int n;
  int i;
  int j;

  CSVASSERT(Y);
  CSVASSERT(m > 0);
  CSVASSERT(index >= 0);
  CSVASSERT(oncompo > 0);
  CSVASSERT(incompo > 0);
  CSVASSERT(comp_info);

  n = ox_component_info_ncompo(comp_info);
  if (n <= 0) {
    return -1;
  }

  if (!Yret && !idret) {
    if (incompo < n) {
      n = incompo;
    }
    return n;
  }

  j = 0;
  for (i = 0; i < n; ++i) {
    component_data *d;
    int ic;

    d = ox_component_info_getc(comp_info, i);
    if (!d)
      continue;

    ic = d->comp_index;
    if (ic < 0 || ic >= incompo)
      continue;

    if (j < oncompo) {
      if (Yret) {
        Yret[j] = clipx(Y[ic * m + index]);
      }
      if (idret) {
        idret[j] = ic;
      }
    }
    j++;
  }
  return j;
}

/**
 * @brief Calculate Effective Yzr and Yzro2
 * @param aYzry Concentration array of each component of Zircaloy
 * @param nYzry Number of elements in @p aYzry
 * @param aYzro Concentration array of each component of ZrO2
 * @param nYzro Number of elements in @p aYzro
 * @param ox_zry Zircaloy Compound Data
 * @param ox_zro2 ZrO2 Compound Data
 * @param Yzry Zircaloy Concentration (out)
 * @param Yzro2 ZrO2 Concentration (out)
 *
 * If component IDs of ox_zry and ox_zro2 do not share same IDs,
 * this function sets sum of aYzry and aYzro to Yzry and Yzro2
 * respectively.
 *
 * If component IDs of ox_zry and ox_zro2 partially share same IDs:
 *
 *   1. Find minimum concentration which does not IDs shared from aYzro.
 *      -- Assumes ox_zro2 has fractions. If not, this function
 *         sets Yzry to 1 and Yro2 to 0 to stop oxidation.
 *
 *   2. Calculate concentrations of each components at ratio of
 *      fractions of Yzro2 from minimal concentration.
 *
 *   3. Subtract concentrations calculated by 2. for duplicated IDs
 *      from aYzry to be corrected.
 *
 *   4. For corrected Yzro, use concentrations calculated by 2. for
 *      duplicated IDs, and, minimum of concentrations calculated by
 *      2. and original concentrations for others.
 *
 *   5. Set sum of corrected aYzry to Yzry and sum of corrected aYzro
 *      to Yzro.
 *
 * If component IDs of ox_zry and ox_zro2 are completely same, this
 * function sets Yzro2 to sum of aYzro2 and Yzry to zero, to stop
 * oxidation.
 *
 * Above explanation states modifies aYzry and aYzro, but in actual
 * specification aYzry and aYzro will keep original values; this
 * function allocates temporary arrays on the stack and use them.
 *
 * @note ID lists must be sorted.
 */
static void
ox_calc_Yzr_and_Yzro2(type *aYzry, int nYzry, type *aYzro, int nYzro,
                      struct ox_component_info *ox_zry,
                      struct ox_component_info *ox_zro2,
                      type *Yzry, type *Yzro2)
{
  int i;
  int j;
  type aYzry_corr[nYzry];
  type aYzro_corr[nYzro];
  type Ymin;
  type Ytmp;
  int nshared;

  CSVASSERT(nYzry == ox_component_info_ncompo(ox_zry));
  CSVASSERT(nYzro == ox_component_info_ncompo(ox_zro2));

  nshared = 0;
  i = 0;
  j = 0;
  while (i < nYzry && j < nYzro) {
    const struct component_info_data *di, *dj;
    int r;
    di = ox_component_info_getcp(ox_zry, i);
    dj = ox_component_info_getcp(ox_zro2, j);
    r = component_info_sort_comp1(di, dj, NULL);
    if (r < 0) { /* i < j */
      i++;
    } else if (r > 0) { /* i > j */
      j++;
    } else {
      nshared++;
      i++;
      j++;
    }
  }

  if (nshared <= 0) {
    *Yzry  = ox_type_asum(aYzry, nYzry);
    *Yzro2 = ox_type_asum(aYzro, nYzro);
    return;
  }

  if (nshared == nYzry && nshared == nYzro) {
    *Yzry  = 0.0;
    *Yzro2 = 1.0;
    return;
  }

  Ymin = ox_type_amin(aYzro, nYzro, &j);
  Ymin = Ymin / ox_component_info_getf(ox_zro2, j);

  i = 0;
  j = 0;
  while (i < nYzry && j < nYzro) {
    const struct component_info_data *di, *dj;
    int r;
    di = ox_component_info_getcp(ox_zry, i);
    dj = ox_component_info_getcp(ox_zro2, j);
    r = component_info_sort_comp1(di, dj, NULL);
    if (r < 0) {
      aYzry_corr[i] = aYzry[i];
      i++;
    } else if (r > 0) {
      Ytmp = Ymin * ox_component_info_getf(ox_zro2, j);
      aYzro_corr[j] = ox_f_clip(ox_type_min(aYzro[j], Ytmp));
      j++;
    } else {
      Ytmp = Ymin * ox_component_info_getf(ox_zro2, j);
      aYzry_corr[i] = ox_f_clip(aYzry[j] - Ytmp);
      aYzro_corr[j] = ox_f_clip(Ytmp);
      i++;
      j++;
    }
  }
  for (; i < nYzry; ++i) {
    aYzry_corr[i] = aYzry[i];
  }
  for (; j < nYzro; ++j) {
    Ytmp = Ymin * ox_component_info_getf(ox_zro2, j);
    aYzro_corr[j] = ox_f_clip(ox_type_min(aYzro[j], Ytmp));
  }

  *Yzry  = ox_type_asum(aYzry_corr, nYzry);
  *Yzro2 = ox_type_asum(aYzro_corr, nYzro);
  return;
}

/**
 * @brief Assign Ytotal * fraction to Y.
 * @param Y destination Y array
 * @param Ytotal Total amount which will occupied by @p comp
 * @param comp component information to assign
 * @param jj destination location
 * @param m  component offset of Y
 */
static void
ox_set_Y_to_fraction(type *Y, type Ytotal, struct ox_component_info *comp,
                     ptrdiff_t jj, int m)
{
  int nc;

  CSVASSERT(comp);
  CSVASSERT(Y);
  CSVASSERT(ox_component_info_ncompo(comp) >= 1);
  CSVASSERT(jj >= 0);
  CSVASSERT(m > 0);

  nc = ox_component_info_ncompo(comp);
  for (int ic = 0; ic < nc; ++ic) {
    component_data *d;
    int id;
    type frac;

    d = ox_component_info_getc(comp, ic);
    if (!d)
      continue;

    id = d->comp_index;
    frac = 1.0;
    if (nc > 1)
      frac = ox_component_info_getf(comp, ic);

    Y[id * m + jj] = Ytotal * frac;
  }
}

static void
ox_set_Y_to_zero(type *Y, struct ox_component_info *comp, ptrdiff_t jj, int m)
{
  ox_set_Y_to_fraction(Y, 0.0, comp, jj, m);
}

static void
ox_add_Y_with_fraction(type *Y, type Yadd, struct ox_component_info *comp,
                       ptrdiff_t jj, int m)
{
  int nc;

  CSVASSERT(comp);
  CSVASSERT(Y);
  CSVASSERT(jj >= 0);
  CSVASSERT(m > 0);

  nc = ox_component_info_ncompo(comp);
  for (int ic = 0; ic < nc; ++ic) {
    component_data *d;
    int id;
    type frac;

    d = ox_component_info_getc(comp, ic);
    if (!d)
      continue;

    id = d->comp_index;
    frac = 1.0;
    if (nc > 1)
      frac = ox_component_info_getf(comp, ic);

    frac = Y[id + m + jj] + Yadd * frac;
    Y[id * m + jj] = ox_f_clip(frac);
  }
}

/**
 * @brief Update oxidation state value
 * @param Yzry  Fraction of Zircaloy
 * @param Yzro2 Fraction of Zirconium Dioxide
 * @param state Status value to update (in/out)
 * @param phi   VOF value of oxidizable zircaloy region (in/out)
 *
 * When the amount of Zircaloy is decreasing and that of ZrO2 is
 * increasing, sets and VOF to 0 and state to FINISHED if it is not
 * RECESSING.
 *
 * When the amount of Zircaloy is decreasing and that of ZrO2 is
 * not increasing, sets and VOF to 1 and state to OUT_OF_BOUNDS.
 *
 * When the amount of Zircaloy is isncreasing and that of ZrO2 is
 * decreasing, sets and VOF to 0 and state to IN_BOUNDS if it is
 * OUT_OF_BOUNDS or FINISHED.
 */
static void ox_state_update(type Yzry, type Yzro2, int *state, type *phi)
{
  const type Ythres_inc = 0.5; /* Threshold for increasing Y */
  const type Ythres_dec = 0.5; /* Threshold for decreasing Y */
  int stat;

  stat = *state;
  if (Yzry < Ythres_dec) {
    if (Yzro2 > Ythres_inc) {
      if (stat != OX_STATE_RECESSING) {
        *state = OX_STATE_FINISHED;
      }
      *phi = 0.0;
    } else {
      *state = OX_STATE_OUT_OF_BOUNDS;
      *phi = 1.0;
    }
  }
  if ((stat == OX_STATE_OUT_OF_BOUNDS || stat == OX_STATE_FINISHED) &&
      Yzry > Ythres_inc && Yzro2 < Ythres_dec) {
    *state = OX_STATE_IN_BOUNDS;
    *phi = 1.0;
  }
}

/**
 * @brief Tests whether neighbor cell is GAS reginon and contains enough H2O
 * @param nrk MPI rank number to the neighbor for the same direction to check
 * @param cond_for_no_neighbor_mpi_rank extra condition that nrk is -1
 * @param state Array of oxidation state
 * @param ox_f_h2o Array of f value describes where contains H2O
 * @param jj Array index to check
 * @param ret_cnt if the neighbor cell is GAS, count up 1.
 * @retval 1 neighbor cell contains enough H2O.
 * @retval 0 otherwise (includes non-GAS cell, and not applicable cell
 *           because of boundary)
 *
 * @p jj is used as-is. So you should pass `jj - 1` for X- neighbor,
 * `jj + 1` for X+ neighbor, `jj + mx` for Y+ neighbor etc.
 *
 * @p cond_for_no_neighbor_mpi_rank exists for excluding
 * boundary. Only meaning full if @p nrk is not -1, the value will
 * never be evaluated.
 */
static int
ox_state_test_neighbor_h2o(int nrk, int cond_for_no_neighbor_mpi_rank,
                           const int *state, type *ox_f_h2o, ptrdiff_t jj,
                           int *ret_cnt)
{
  if (nrk == -1 || cond_for_no_neighbor_mpi_rank) {
    if (state[jj] == OX_STATE_GAS) {
      if (ret_cnt) (*ret_cnt)++;
      /*
       * - ox_f_h2o is assumed to be 0 or 1.
       * - ox_f_h2o stores 0 if the cell contains enough H2O, which
       *   makes positive level set function value for metal region.
       */
      if (ox_f_h2o[jj] < 0.5) {
        return 1;
      }
    }
  }
  return 0;
}

static int
ox_state_update_direct_neighbor_h2o(const int *state, type *ox_f_h2o,
                                    int nrk[6],
                                    int nx, int ny, int nz,
                                    int mx, int my, int mz, int stm,
                                    ptrdiff_t jj)
{
  int jx, jy, jz;
  int mxy = mx * my;
  int icnt;

  if (state[jj] != OX_STATE_IN_BOUNDS && state[jj] != OX_STATE_RECESSING) {
    return OX_STATE_UNCHANGED;
  }

  calc_struct_index(jj, mx, my, mz, &jx, &jy, &jz);
  jx -= stm;
  jy -= stm;
  jz -= stm;

  CSVASSERT(jx >= 0);
  CSVASSERT(jy >= 0);
  CSVASSERT(jz >= 0);
  CSVASSERT(jx < nx);
  CSVASSERT(jy < ny);
  CSVASSERT(jz < nz);

  icnt = 0;
  if (ox_state_test_neighbor_h2o(nrk[0], jz > 0, state, ox_f_h2o, jj - mxy,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }
  if (ox_state_test_neighbor_h2o(nrk[1], jz < nz - 1, state, ox_f_h2o, jj + mxy,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }
  if (ox_state_test_neighbor_h2o(nrk[2], jy > 0, state, ox_f_h2o, jj - mx,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }
  if (ox_state_test_neighbor_h2o(nrk[3], jy < ny - 1, state, ox_f_h2o, jj + mx,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }
  if (ox_state_test_neighbor_h2o(nrk[4], jx > 0, state, ox_f_h2o, jj - 1,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }
  if (ox_state_test_neighbor_h2o(nrk[5], jx < nx - 1, state, ox_f_h2o, jj + 1,
                                 &icnt)) {
    return OX_STATE_IN_BOUNDS;
  }

  /* stop oxidation if there are not enough H2O around the cell */
  if (icnt > 0) {
    return OX_STATE_RECESSING;
  }
  return OX_STATE_UNCHANGED;
}

static void
ox_update_f_h2o(parameter *prm, domain *cdo, type *Y, type *fs, type *fl,
                struct ox_component_info *ox_h2o, type *ox_f_h2o,
                type ox_h2o_threshold, type *ox_lset_h2o, int iter)
{
  CSVASSERT(ox_h2o);
  CSVASSERT(ox_f_h2o);
  CSVASSERT(Y);
  CSVASSERT(fs);
  CSVASSERT(fl);
  CSVASSERT(prm);
  CSVASSERT(cdo);

#pragma omp parallel
  {
    int nx, ny, nz, n;
    int mx, my, mz, m;
    int jx, jy, jz;
    int j;
    int stm;
    ptrdiff_t jj;

    n = cdo->n;
    m = cdo->m;
    nx = cdo->nx;
    ny = cdo->ny;
    nz = cdo->nz;
    mx = cdo->mx;
    my = cdo->my;
    mz = cdo->mz;
    stm = cdo->stm;

#pragma omp for
    for (j = 0; j < n; ++j) {
      type Yh2o;
      type fg;

      calc_struct_index(j, nx, ny, nz, &jx, &jy, &jz);
      jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);

      fg = ox_f_clip(1.0 - fs[jj] - fl[jj]);
      if (ox_is_enabled_component(ox_h2o)) {
        int nc = ox_component_info_ncompo(ox_h2o);
        Yh2o = 0.0;
        for (int ic = 0; ic < nc; ++ic) {
          component_data *d;
          int id;
          d = ox_component_info_getc(ox_h2o, ic);
          if (!d)
            continue;

          id = d->comp_index;
          CSVASSERT(id >= 0);

          Yh2o += Y[id * m + jj];
        }

        Yh2o *= fg;
      } else {
        Yh2o = fg;
      }

      if (Yh2o < ox_h2o_threshold) {
        ox_f_h2o[jj] = 1.0;
      } else {
        ox_f_h2o[jj] = 0.0;
      }
    }
  }

  bcf(ox_f_h2o, prm);

  if (iter <= 0) {
    iter = 300;
  }
  Level_Set(1, iter, ox_lset_h2o, ox_f_h2o, prm);

  bcf(ox_lset_h2o, prm);
}

int oxidation_init(parameter *prm, domain *cdo, type *Y, type *Yt, type *fs,
                   type *fl, int ncompo,
                   struct ox_component_info *ox_zry,
                   struct ox_component_info *ox_zro2,
                   struct ox_component_info *ox_h2o, type *ox_lset,
                   type *ox_vof, int *ox_flag, type *ox_f_h2o,
                   type ox_h2o_threshold, type *ox_lset_h2o,
                   type *ox_lset_h2o_s)
{
  int mx, my, mz;
  int f;
  ptrdiff_t m;
  ptrdiff_t jj;

  mx = cdo->mx;
  my = cdo->my;
  mz = cdo->mz;
  m = cdo->m;
  f = 0;

#pragma omp parallel for
  for (jj = 0; jj < m; ++jj) {
    int nYzo, nYzr;
    type aYzr[ncompo];
    type aYzo[ncompo];
    type Yzr;
    type Yzo;

    nYzr = ox_component_make_Yarr(aYzr, NULL, ncompo, Y, m, jj,
                                  ncompo, ox_zry);
    nYzo = ox_component_make_Yarr(aYzo, NULL, ncompo, Y, m, jj,
                                  ncompo, ox_zro2);

    ox_calc_Yzr_and_Yzro2(aYzr, nYzr, aYzo, nYzo, ox_zry, ox_zro2, &Yzr, &Yzo);

    if (Yt[jj] < 1.0) {
      ox_flag[jj] = OX_STATE_GAS;
      ox_vof[jj] = 0.0;
    } else {
      ox_vof[jj] = 1.0;
      ox_flag[jj] = OX_STATE_OUT_OF_BOUNDS;
      ox_state_update(Yzr, Yzo, &ox_flag[jj], &ox_vof[jj]);
    }
  }

  bcf(ox_vof, prm);

  Level_Set(1, 300, ox_lset, ox_vof, prm);
  bcf(ox_lset, prm);

  if (ox_lset_h2o_s) {
#pragma omp parallel for
    for (jj = 0; jj < m; ++jj) {
      ox_lset_h2o_s[jj] = -ox_huge_val();
    }
  }

  if (ox_f_h2o) {
    int *ox_flag_next;

    ox_update_f_h2o(prm, cdo, Y, fs, fl, ox_h2o, ox_f_h2o, ox_h2o_threshold,
                    ox_lset_h2o, 300);

    ox_flag_next = (int *)calloc(cdo->m, sizeof(int));
    if (!ox_flag_next) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      return 1;
    }

#pragma omp parallel
    {
      int n, nx, ny, nz;
      int mx, my, mz;
      int jx, jy, jz;
      int stm;
      ptrdiff_t j, jj;

      n = cdo->n;
      nx = cdo->nx;
      ny = cdo->ny;
      nz = cdo->nz;
      mx = cdo->mx;
      my = cdo->my;
      mz = cdo->mz;
      stm = cdo->stm;

#pragma omp for
      for (j = 0; j < n; ++j) {
        int nf;
        calc_struct_index(j, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);
        nf = ox_state_update_direct_neighbor_h2o(ox_flag, ox_f_h2o,
                                                 prm->mpi->nrk,
                                                 nx, ny, nz, mx, my, mz, stm,
                                                 jj);
        if (nf == OX_STATE_UNCHANGED) {
          ox_flag_next[jj] = ox_flag[jj];
        } else {
          ox_flag_next[jj] = nf;
        }
      }

#pragma omp for
      for (j = 0; j < n; ++j) {
        calc_struct_index(j, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);
        ox_flag[jj] = ox_flag_next[jj];
      }
    }

    free(ox_flag_next);
  }

  return 0;
}

static int
make_matrix_array_Yh2(int topo_flag, type *A, type *b,
                      int stmx, int stmy, int stmz,
                      int stpx, int stpy, int stpz,
                      int mx, int my, int mz,
                      parameter *prm, void *arg);

static void ox_flag_communicate(parameter *prm, int *ox_flag);

type oxidation_calc(struct oxidation_calc_args *p)
{
  type cputime_start;
  int *ox_flag_next;

  CSVASSERT(p);
  CSVASSERT(p->cdo);
  CSVASSERT(p->Y);
  CSVASSERT(p->t);
  CSVASSERT(p->ox_dt);
  CSVASSERT(p->ox_dt_local);
  CSVASSERT(p->ox_flag);
  CSVASSERT(p->ox_lset);
  CSVASSERT(p->ox_vof);
  /* ox_q is optional */
  CSVASSERT(p->ls);
  /* ox_kp is optional */
  /* ox_dens is optional */
  CSVASSERT(p->ox_zry);
  CSVASSERT(p->ox_zro2);
  CSVASSERT(p->ox_kp_func);
  CSVASSERT(p->ox_zry);
  CSVASSERT(p->ox_zro2);

  cputime_start = cpu_time();

  if (p->ox_kp) {
    zero_clear(p->ox_kp, p->cdo->m);
  }
  if (p->ox_q) {
    zero_clear(p->ox_q, p->cdo->m);
  }
  if (p->ox_dens) {
    zero_clear(p->ox_dens, p->cdo->m);
  }
  if (p->ox_recess_k) {
    zero_clear(p->ox_recess_k, p->cdo->m);
  }

  if (p->ox_f_h2o) {
    ox_update_f_h2o(p->prm, p->cdo, p->Y, p->fs, p->fl, p->ox_h2o, p->ox_f_h2o,
                    p->ox_h2o_threshold, p->ox_lset_h2o, 30);
  }

  ox_flag_next = (int *)calloc(p->cdo->m, sizeof(int));
  if (!ox_flag_next) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    p->prm->status = ON;
    return 0.0;
  }

#pragma omp parallel shared(p)
  {
    int ncompo = p->cdo->NBaseComponent;
    ptrdiff_t jn;
    ptrdiff_t n = p->cdo->n;
    ptrdiff_t m = p->cdo->m;
    int nx = p->cdo->nx;
    int ny = p->cdo->ny;
    int nz = p->cdo->nz;
    int mx = p->cdo->mx;
    int my = p->cdo->my;
    int mz = p->cdo->mz;
    int stm = p->cdo->stm;
    struct ox_component_info *ox_zry = p->ox_zry;
    struct ox_component_info *ox_zro2 = p->ox_zro2;

#pragma omp for
    for (jn = 0; jn < n; ++jn) {
      type delta;
      type dx, dy, dz;
      int ii, ij, ik;
      int mi, mj, mk;
      ptrdiff_t jj;
      type Yzry_a[ncompo];
      type Yzro_a[ncompo];
      int Izry_a[ncompo];
      int nzry;
      int nzro;
      int i;
      int stat;
      int calc_oxidation;
      int calc_recession;
      type Yzry, Yzro;
      int nf;

      calc_struct_index(jn, nx, ny, nz, &ii, &ij, &ik);

      mi = ii + stm;
      mj = ij + stm;
      mk = ik + stm;
      jj = calc_address(mi, mj, mk, mx, my, mz);

      stat = p->ox_flag[jj];
      if (stat == OX_STATE_GAS) continue;

      dx = p->cdo->x[mi + 1] - p->cdo->x[mi];
      dy = p->cdo->y[mj + 1] - p->cdo->y[mj];
      dz = p->cdo->z[mk + 1] - p->cdo->z[mk];

      nzry = ox_component_make_Yarr(Yzry_a, Izry_a, ncompo,
                                    p->Y, m, jj, ncompo, ox_zry);
      nzro = ox_component_make_Yarr(Yzro_a, NULL, ncompo,
                                    p->Y, m, jj, ncompo, ox_zro2);

      ox_calc_Yzr_and_Yzro2(Yzry_a, nzry, Yzro_a, nzro, ox_zry, ox_zro2,
                            &Yzry, &Yzro);

      /* Characteristic cell length */
      delta = sqrt((dx * dx + dy * dy + dz * dz) / 3.0);

      calc_oxidation = 0;
      calc_recession = 0;

      nf = ox_state_update_direct_neighbor_h2o(p->ox_flag, p->ox_f_h2o,
                                               p->prm->mpi->nrk, nx, ny, nz,
                                               mx, my, mz, stm, jj);
      if (nf == OX_STATE_UNCHANGED) {
        ox_flag_next[jj] = p->ox_flag[jj];
      } else {
        ox_flag_next[jj] = nf;
      }
      ox_state_update(Yzry, Yzro, &ox_flag_next[jj], &p->ox_vof[jj]);

      if (stat != OX_STATE_RECESSING) {
        if (ox_flag_next[jj] == OX_STATE_RECESSING) {
          calc_recession = 1;
        } else if (p->ox_lset[jj] > 0.0 && p->ox_lset[jj] < delta &&
                   (stat == OX_STATE_STARTED || stat == OX_STATE_IN_BOUNDS)) {
          if (stat == OX_STATE_STARTED &&
              p->ox_lset_h2o[jj] >
              p->ox_lset_h2o_s[jj] + p->ox_h2o_lset_min_to_recess) {
            ox_flag_next[jj] = OX_STATE_RECESSING;
            stat = OX_STATE_RECESSING;
            calc_recession = 1;
          } else {
            calc_oxidation = 1;
          }
        }
      } else {
        if (p->ox_lset_h2o[jj] <= p->ox_lset_h2o_s[jj]) {
          if (p->ox_dt_local[jj] <= delta) {
            if (p->ox_dt_local[jj] <= 0.0) {
              stat = OX_STATE_IN_BOUNDS;
            } else {
              stat = OX_STATE_STARTED;
            }

            ox_flag_next[jj] = stat;
            p->ox_vof[jj] = 1.0;
            ox_set_Y_to_fraction(p->Y, Yzry + Yzro, ox_zry, jj, m);
            Yzry = 1.0;
            Yzro = 0.0;

            /* calculation will perform if levelset meets condition */
            if (p->ox_lset[jj] > 0.0 && p->ox_lset[jj] < delta) {
              calc_oxidation = 1;
            }
          } else {
            stat = OX_STATE_FINISHED;
            ox_flag_next[jj] = stat;
          }
        } else {
          calc_recession = 1;
        }
      }
      /* Assume that none or only one of calculations are flagged */
      CSVASSERT(!(calc_recession && calc_oxidation));

      if (calc_recession) { /* recession */
        /*
         * When start recessing stat != RECESSING.
         * When continues recessing both stat and p->ox_flag[jj] == RECESSING
         */
        if (p->ox_recess_init > 0.0 && p->ox_dt_local[jj] > 0.0) {
          type rate;
          type temp;
          type recess;
          type ddt, dt, dt_local;

#pragma omp atomic update
          p->cdo->ox_nre_calc += 1;

          temp = p->t[jj];
          rate = tempdep_calc(p->ox_recess_rate, temp);
          if (p->ox_recess_k) {
            p->ox_recess_k[jj] = rate;
          }

          dt = p->ox_dt[jj];
          dt_local = p->ox_dt_local[jj];

          recess = p->ox_recess_init - dt;
          if (recess < p->ox_recess_min) {
            recess = p->ox_recess_min;
          }
          ddt = 2.0 * p->delta_t * sqrt(rate * recess);
          dt -= ddt;
          dt_local -= ddt;

          /* No more oxidation to recess */
          if (dt_local <= 0.0) {
            dt_local = 0.0;
            dt = 0.0;

            ox_flag_next[jj] = OX_STATE_IN_BOUNDS;
            Yzry = Yzry + Yzro;
            ox_set_Y_to_zero(p->Y, ox_zro2, jj, m);
            ox_set_Y_to_fraction(p->Y, Yzry, ox_zry, jj, m);
            p->ox_vof[jj] = 1.0;
          }
          p->ox_dt[jj] = dt;
          p->ox_dt_local[jj] = dt_local;
        } else {
          /* reject setting to RECESSING. */
          ox_flag_next[jj] = OX_STATE_IN_BOUNDS;
          ox_state_update(Yzry, Yzro, &ox_flag_next[jj], &p->ox_vof[jj]);
        }
      }

      if (calc_oxidation) {
        type rho_a[ncompo];
        type temp;
        type kp;
        type dt;
        type dtd;
        type rho;
        type rho_sq;
        type thick_inc_sq;

        temp = p->t[jj];

#pragma omp atomic update
        p->cdo->ox_nox_calc += 1;

        for (i = 0; i < nzry; ++i) {
          rho_a[i] = tempdep_calc(&p->comps[Izry_a[i]].rho_s, temp);
        }
        rho = 0.3509 * YNPHV1w(nzry, Yzry_a, rho_a);
        rho_sq = rho * rho;

        if (p->ox_dens) {
          p->ox_dens[jj] = rho;
        }

        kp = tempdep_calc(p->ox_kp_func, temp);
        if (p->ox_kp) {
          p->ox_kp[jj] = kp;
        }

        dt = p->ox_dt[jj];
        dtd = p->ox_dt_local[jj];

        thick_inc_sq = kp / rho_sq * p->delta_t;

        dt = dt * dt + thick_inc_sq;
        if (dt < 0.0) dt = 0.0;
        dt = sqrt(dt);

        dtd = dtd * dtd + thick_inc_sq;
        if (dtd < 0.0) dtd = 0.0;
        dtd = sqrt(dtd);

        if (stat != OX_STATE_STARTED) {
          p->ox_lset_h2o_s[jj] = p->ox_lset_h2o[jj];
        } else {
          if (p->ox_lset_h2o_s[jj] > p->ox_lset_h2o[jj]) {
            p->ox_lset_h2o_s[jj] = p->ox_lset_h2o[jj];
          }
        }

        if (p->ls[jj] < delta) {
          ox_flag_next[jj] = OX_STATE_STARTED;
        } else if (stat == OX_STATE_IN_BOUNDS) {
          dt = dt + p->ls[jj];
          ox_flag_next[jj] = OX_STATE_STARTED;
        } else {
          CSVASSERT(stat == OX_STATE_STARTED);
        }
        p->ox_dt[jj] = dt;
        p->ox_dt_local[jj] = dtd;

        if (dt > 0.0) {
          if (p->ox_q) {
            type q;
            q = p->ox_q_fac * 4.22e+10 * kp / (2.0 * rho_sq * dt * delta);
            p->ox_q[jj] = q;
          }
          if (ox_is_enabled_component(p->ox_h2) && p->ox_Yh2) {
            type y;
            /* mul delta_t */
            y = 2.895e+2 * kp / (2.0 * rho_sq * dt * delta) * p->delta_t;
            p->ox_Yh2[jj] += y;
          }
        }
        if (dtd > delta) {
          p->ox_vof[jj] = 0.0;
          ox_flag_next[jj] = OX_STATE_FINISHED;
          ox_set_Y_to_zero(p->Y, ox_zry, jj, m);
          ox_set_Y_to_fraction(p->Y, 1.0, ox_zro2, jj, m);
        }
      }
    }
  }
  bcf(p->ox_vof, p->prm);

  Level_Set(1, p->prm->cdo->ls_iteration, p->ox_lset, p->ox_vof, p->prm);
  bcf(p->ox_lset, p->prm);

#pragma omp parallel
  {
    ptrdiff_t jj;

#pragma omp for
    for (jj = 0; jj < p->cdo->m; ++jj) {
      p->ox_flag[jj] = ox_flag_next[jj];
    }
  }
  free(ox_flag_next);

  ox_flag_communicate(p->prm, p->ox_flag);

  if (ox_is_enabled_component(p->ox_h2) && p->ox_Yh2) {
    CSVASSERT(p->ox_Yh2_div);

    /* Transport Yh2 in solid with diffusion */
#pragma omp parallel
    {
      parameter *prm = p->prm;
      domain *cdo = prm->cdo;
      ptrdiff_t n = cdo->n;
      int nx = cdo->nx;
      int ny = cdo->ny;
      int nz = cdo->nz;
      int mx = cdo->mx;
      int my = cdo->my;
      int mz = cdo->mz;
      int stm = cdo->stm;
      type dt = cdo->dt;
      ptrdiff_t i;
      ptrdiff_t jj;
      int jx, jy, jz;

#pragma omp for
      for (i = 0; i < n; ++i) {
        calc_struct_index(i, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);

        p->ox_Yh2_div[i] = - p->ox_Yh2[jj] / dt;
      }
    }

#ifdef CCSE
    ccse_poisson_f(0, "Yh2", p->ox_Yh2, p->ox_Yh2_div, p->prm,
                   30000, 1.0e-6, 1.0e-50, make_matrix_array_Yh2, p);
#endif

    /* Translate Yh2 released into gas-phase */
#pragma omp parallel
    {
      parameter *prm = p->prm;
      domain *cdo = prm->cdo;
      ptrdiff_t n = cdo->n;
      ptrdiff_t m = cdo->m;
      int nx = cdo->nx;
      int ny = cdo->ny;
      int nz = cdo->nz;
      int mx = cdo->mx;
      int my = cdo->my;
      int mz = cdo->mz;
      int stm = cdo->stm;
      ptrdiff_t i;
      int jx, jy, jz;
      struct ox_component_info *ox_h2 = p->ox_h2;
      struct ox_component_info *ox_h2o = p->ox_h2o;
      type mmass_h2o;
      type mmass_h2;

#pragma omp for
      for (i = 0; i < n; ++i) {
        ptrdiff_t jj;

        calc_struct_index(i, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);

        if (p->ox_flag[jj] != OX_STATE_GAS) continue;

        if (ox_is_enabled_component(ox_h2)) {
          type t = p->t[jj];
          int nc = ox_component_info_ncompo(ox_h2);
          int ic;
          type rho_h2;
          type rho_h2f[nc];
          type rho_h2a[nc];
          type Yh2;

          ic = 0;
          for (int iic = 0; iic < nc; ++iic) {
            component_data *d;
            int id;
            d = ox_component_info_getc(ox_h2, iic);
            if (!d)
              continue;

            CSVASSERT(d->phase_comps_index >= 0);
            id = d->phase_comps_index;
            rho_h2a[ic] = tempdep_calc(&p->comps[id].rho_g, t);
            rho_h2f[ic] = ox_component_info_getf(ox_h2, iic);
            ++ic;
          }
          if (ic > 1) {
            rho_h2 = YNPHV1w(ic, rho_h2f, rho_h2a);
          } else {
            rho_h2 = rho_h2a[0];
          }

          Yh2 = p->ox_Yh2[jj] / rho_h2;

          if (p->fluid_dynamics == ON) {
            ox_add_Y_with_fraction(p->Y, Yh2, ox_h2, jj, m);
          }
        }

        if (ox_is_enabled_component(ox_h2) &&
            ox_is_enabled_component(ox_h2o)) {
          int nch2 = ox_component_info_ncompo(ox_h2);
          int nch2o = ox_component_info_ncompo(ox_h2o);
          int ic;
          type t;
          type mmass_h2;
          type mmass_h2o;
          type mmass_a[(nch2 > nch2o) ? nch2 : nch2o];
          type mmass_f[(nch2 > nch2o) ? nch2 : nch2o];
          type rho_h2oa[nch2o];
          type rho_h2o;
          type Yh2o;

          t = p->t[jj];

          ic = 0;
          for (int iic = 0; iic < nch2; ++iic) {
            component_data *d;
            int id;
            d = ox_component_info_getc(ox_h2, iic);
            if (!d)
              continue;

            CSVASSERT(d->phase_comps_index >= 0);
            id = d->phase_comps_index;
            mmass_a[ic] = p->comps[id].molar_mass;
            mmass_f[ic] = ox_component_info_getf(ox_h2, iic);
            ++ic;
          }
          if (ic > 1) {
            mmass_h2 = YNPHV1w(ic, mmass_f, mmass_a);
          } else {
            mmass_h2 = mmass_a[0];
          }

          ic = 0;
          for (int iic = 0; iic < nch2o; ++iic) {
            component_data *d;
            int id;
            d = ox_component_info_getc(ox_h2o, iic);
            if (!d)
              continue;

            CSVASSERT(d->phase_comps_index >= 0);
            id = d->phase_comps_index;
            mmass_a[ic] = p->comps[id].molar_mass;
            mmass_f[ic] = ox_component_info_getf(ox_h2o, iic);
            rho_h2oa[ic] = tempdep_calc(&p->comps[id].rho_g, t);
          }
          if (ic > 1) {
            mmass_h2o = YNPHV1w(ic, mmass_f, mmass_a);
            rho_h2o = YNPHV1w(ic, mmass_f, rho_h2oa);
          } else {
            mmass_h2o = mmass_a[0];
            rho_h2o = rho_h2oa[0];
          }

          Yh2o = p->ox_Yh2[jj] * mmass_h2o / mmass_h2;
          Yh2o /= rho_h2o;

          if (p->fluid_dynamics == ON) {
            ox_add_Y_with_fraction(p->Y, -Yh2o, ox_h2o, jj, m);
          }
        }

        p->ox_Yh2[jj] = 0.0;
      }
    }
  }

  //--- H2 absorption
  if (p->h2_absorp_eval) {
    mpi_param *mpi;
    domain *cdo;
    int *naveraged;
    type *aP;
    type *aT;
    type *aZr;
    type *aKs;
    type *P;
    type *T;
    type *Zr;
    type *Ks;

    cdo = p->cdo;
    mpi = p->prm->mpi;
    CSVASSERT(cdo);
    CSVASSERT(mpi);
    CSVASSERT(ox_is_enabled_component(p->ox_h2));

    P = p->h2_absorp_P;
    T = p->h2_absorp_T;
    Zr = p->h2_absorp_Zr;
    Ks = p->h2_absorp_Ks;
    aP = NULL;
    aT = NULL;
    aZr = NULL;
    aKs = NULL;

    if (!P) {
      aP = (type *)calloc(p->cdo->nz, sizeof(type));
      P = aP;
    }
    if (!T) {
      aT = (type *)calloc(p->cdo->nz, sizeof(type));
      T = aT;
    }
    if (!Zr) {
      aZr = (type *)calloc(p->cdo->nz, sizeof(type));
      Zr = aZr;
    }
    if (!Ks) {
      aKs = (type *)calloc(p->cdo->nz, sizeof(type));
      Ks = aKs;
    }
    naveraged = (int *)calloc(p->cdo->nz, sizeof(int));
    if (!P || !T || !Zr || !Ks || !naveraged) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      p->prm->status = ON;
      free(aP);
      free(aT);
      free(aZr);
      free(aKs);
      free(naveraged);
      return 0.0;
    }

#pragma omp parallel
    {
      int i;
      ptrdiff_t jj;
      int jx, jy, jz;
      int n, m;
      int nx, ny, nz;
      int mx, my, mz, mxy;
      int stm;

      cdo = p->cdo;
      n = cdo->n;
      m = cdo->m;
      nx = cdo->nx;
      ny = cdo->ny;
      nz = cdo->nz;
      mx = cdo->mx;
      my = cdo->my;
      mz = cdo->mz;
      mxy = mx * my;
      stm = cdo->stm;

#pragma omp for
      for (jz = 0; jz < nz; ++jz) {
        P[jz] = 0.0;
        T[jz] = 0.0;
        Zr[jz] = 0.0;
        Ks[jz] = 0.0;
        p->h2_absorp_eval[jz] = 0.0;
      }

#pragma omp for
      for (i = 0; i < n; ++i) {
        type fg, fs, fl;

        calc_struct_index(i, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + stm, jy + stm, jz + stm, mx, my, mz);

        fs = p->fs[jj];
        fl = p->fl[jj];
        fg = ox_f_clip(1.0 - fs - fl);

        if (fg > 0.0) {
          type p_h2;
          type Yh2;
          int nc = ox_component_info_ncompo(p->ox_h2);

          p_h2 = p->h2_absorp_base_p;
          if (p->h2_absorp_eval_P_change == ON) {
            p_h2 += p->p[jj];
          }

          Yh2 = 0.0;
          for (int ic = 0; ic < nc; ++ic) {
            component_data *d;
            int id;
            d = ox_component_info_getc(p->ox_h2, ic);
            if (!d)
              continue;

            id = d->comp_index;
            CSVASSERT(id >= 0);

            Yh2 += p->Y[id * m + jj];
          }
          Yh2 = ox_f_clip(Yh2);
          p_h2 *= Yh2;

#pragma omp atomic update
          P[jz] += p_h2;
#pragma omp atomic update
          T[jz] += p->t[jj];
#pragma omp atomic update
          naveraged[jz] += 1;
        }

        if (p->ox_flag[jj] == OX_STATE_IN_BOUNDS) {
          int f;
          f = 0;
          if ((mpi->nrk[4] >= 0 || jx > 0) &&
              p->ox_flag[jj - 1] == OX_STATE_GAS) {
            f = 1;
          }
          if ((mpi->nrk[5] >= 0 || jx < nx - 1) &&
              p->ox_flag[jj + 1] == OX_STATE_GAS) {
            f = 1;
          }
          if ((mpi->nrk[2] >= 0 || jy > 0) &&
              p->ox_flag[jj - mx] == OX_STATE_GAS) {
            f = 1;
          }
          if ((mpi->nrk[3] >= 0 || jy < ny - 1) &&
              p->ox_flag[jj + mx] == OX_STATE_GAS) {
            f = 1;
          }
          if (f) {
#pragma omp atomic write
            Zr[jz] = 1.0;
          }
        }
      }
    }

#ifdef JUPITER_MPI
    {
      MPI_Comm xy_comm;
      int nz_chk;

      MPI_Comm_split(mpi->CommJUPITER, mpi->rank_z, mpi->rank_xy, &xy_comm);

      MPI_Allreduce(&cdo->nz, &nz_chk, 1, MPI_INT, MPI_MAX, xy_comm);
      CSVASSERT(nz_chk == cdo->nz);

      MPI_Allreduce(MPI_IN_PLACE, P, cdo->nz, MPI_TYPE, MPI_SUM, xy_comm);
      MPI_Allreduce(MPI_IN_PLACE, T, cdo->nz, MPI_TYPE, MPI_SUM, xy_comm);
      MPI_Allreduce(MPI_IN_PLACE, Zr, cdo->nz, MPI_TYPE, MPI_MAX, xy_comm);
      MPI_Allreduce(MPI_IN_PLACE, naveraged, cdo->nz, MPI_INT, MPI_SUM, xy_comm);
      MPI_Comm_disconnect(&xy_comm);
    }
#endif

#pragma omp parallel
    {
      int jz;
      int nz;
      const type R = 8.314;
      const type DH = -65015.0;
      const type DS = -101.0;

      cdo = p->cdo;
      nz = cdo->nz;

#pragma omp for
      for (jz = 0; jz < nz; ++jz) {
        P[jz] /= naveraged[jz];
        T[jz] /= naveraged[jz];
        if (T[jz] > 0.0) {
          Ks[jz] = exp(DS / R - DH / (R * T[jz]));
        } else {
          Ks[jz] = 0.0;
        }

        if (Zr[jz] > 0.0) {
          p->h2_absorp_eval[jz] = Ks[jz] * sqrt(P[jz]);
        } else {
          p->h2_absorp_eval[jz] = 0.0;
        }
      }
    }

    free(aP);
    free(aT);
    free(aZr);
    free(aKs);
    free(naveraged);
  }

  return cpu_time() - cputime_start;
}

type oxidation(variable *val, material *mtl, parameter *prm)
{
  flags *flg;
  domain *cdo;
  phase_value *phv;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->phv);

  flg = prm->flg;
  cdo = prm->cdo;
  phv = prm->phv;

  if (flg->oxidation == ON) {
    type *Y;
    type *fs;
    type *fl;
    type *t;

    if (flg->solute_diff == ON) {
      Y = val->Y;
      fs = val->fs;
      fl = val->fl;
    } else {
      Y = val->fs;
      fs = val->fs_sum;
      fl = val->fl_sum;
    }
    if (flg->two_energy == ON) {
      t = val->ts;
    } else {
      t = val->t;
    }

    struct oxidation_calc_args args = {
      .prm = prm,
      .cdo = cdo,
      .fluid_dynamics = flg->fluid_dynamics,
      .delta_t = cdo->dt,
      .ox_q_fac = cdo->ox_q_fac,
      .Y = Y,
      .fs = fs,
      .fl = fl,
      .t = t,
      .ox_dt = val->ox_dt,
      .ox_dt_local = val->ox_dt_local,
      .ox_flag = val->ox_flag,
      .ox_lset = val->ox_lset,
      .ox_vof = val->ox_vof,
      .ox_q = val->ox_q,
      .ox_Yh2 = val->ox_h2,
      .ls = val->ls,
      .ox_kp = mtl->ox_kp,
      .ox_dens = mtl->ox_dens,
      .ox_diff_h2_sol = cdo->ox_diff_h2_sol,
      .ox_diff_h2_srf = cdo->ox_diff_h2_srf,
      .ox_Yh2_div = mtl->div_b,
      .ox_kp_func = &cdo->ox_kp,
      .comps = phv->comps,
      .ox_zry = &cdo->ox_zry,
      .ox_zro2 = &cdo->ox_zro2,
      .ox_h2 = &cdo->ox_h2,
      .ox_h2o = &cdo->ox_h2o,
      .ox_h2o_threshold = cdo->ox_h2o_threshold,
      .ox_h2o_lset_min_to_recess = cdo->ox_h2o_lset_min_to_recess,
      .ox_f_h2o = val->ox_f_h2o,
      .ox_lset_h2o = val->ox_lset_h2o,
      .ox_lset_h2o_s = val->ox_lset_h2o_s,
      .ox_recess_init = cdo->ox_recess_init,
      .ox_recess_rate = &cdo->ox_recess_rate,
      .ox_recess_min = cdo->ox_recess_min,
      .ox_recess_k = mtl->ox_recess_k,

      .p = val->p,
      .h2_absorp_eval = (flg->h2_absorp_eval == ON) ? val->h2_absorp_eval : NULL,
      .h2_absorp_Ks = val->h2_absorp_Ks,
      .h2_absorp_P = val->h2_absorp_P,
      .h2_absorp_T = val->h2_absorp_T,
      .h2_absorp_Zr = val->h2_absorp_Zr,
      .h2_absorp_base_p = cdo->h2_absorp_base_p,
      .h2_absorp_eval_P_change = flg->h2_absorp_eval_p_change,
    };

    return oxidation_calc(&args);
  }
  return 0.0;
}

static inline type
ox_diff_sel(int flagA, int flagB, type diff_sol, type diff_srf)
{
  switch(flagB) {
  case OX_STATE_FINISHED:
  case OX_STATE_STARTED:
    if (flagA == OX_STATE_FINISHED || flagA == OX_STATE_STARTED) {
      return diff_sol;
    }
    if (flagA == OX_STATE_GAS) return diff_srf;
    return 0.0;
  case OX_STATE_GAS:
    if (flagA == OX_STATE_FINISHED || flagA == OX_STATE_STARTED) {
      return diff_srf;
    }
    return 0.0;
  }
  return 0.0;
}

static int
make_matrix_array_Yh2(int topo_flag, type *A, type *b,
                      int stmx, int stmy, int stmz,
                      int stpx, int stpy, int stpz,
                      int mx, int my, int mz,
                      parameter *prm, void *arg)
{
  struct oxidation_calc_args *p;
  domain *cdo = prm->cdo;
  int nx;
  int ny;
  int nz;
  ptrdiff_t nxy;
  ptrdiff_t n;
  ptrdiff_t ii;
  int m_ = mx * my * mz;
  int  *ox_flag;
  type  diff_sol;
  type  diff_srf;

  type dxi2=cdo->dxi*cdo->dxi,
       dyi2=cdo->dyi*cdo->dyi,
       dzi2=cdo->dzi*cdo->dzi;
  type dti;

  nx = mx - stmx - stpx;
  ny = my - stmy - stpy;
  nz = mz - stmz - stpz;
  nxy = nx * ny;
  n = nxy * nz;

  p = (struct oxidation_calc_args *)arg;
  dti = 1.0 / cdo->dt;

  ox_flag = p->ox_flag;
  diff_sol = p->ox_diff_h2_sol;
  diff_srf = p->ox_diff_h2_srf;

#pragma omp parallel for
  for (ii = 0; ii < n; ++ii) {
    type cc, cw, ce, cs, cn, cb, ct;
    int jx, jy, jz;
    ptrdiff_t j_;
    ptrdiff_t jj;
    ptrdiff_t jc, jw, je, js, jn, jb, jt;
    int ifc, ifw, ife, ifs, ifn, ifb, ift;

    calc_struct_index(ii, nx, ny, nz, &jx, &jy, &jz);
    j_ = calc_address(jx + stmx, jy + stmy, jz + stmz, mx, my, mz);
    jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                      cdo->mx, cdo->my, cdo->mz);

    jc = jj;
    jw = jj - 1;
    je = jj + 1;
    js = jj - cdo->mx;
    jn = jj + cdo->mx;
    jb = jj - cdo->mxy;
    jt = jj + cdo->mxy;

    ifc = ox_flag[jc];
    ifw = ox_flag[jw];
    ife = ox_flag[je];
    ifs = ox_flag[js];
    ifn = ox_flag[jn];
    ifb = ox_flag[jb];
    ift = ox_flag[jt];

    cw = ox_diff_sel(ifc, ifw, diff_sol, diff_srf) * dxi2;
    ce = ox_diff_sel(ifc, ife, diff_sol, diff_srf) * dxi2;
    cs = ox_diff_sel(ifc, ifs, diff_sol, diff_srf) * dyi2;
    cn = ox_diff_sel(ifc, ifn, diff_sol, diff_srf) * dyi2;
    cb = ox_diff_sel(ifc, ifb, diff_sol, diff_srf) * dzi2;
    ct = ox_diff_sel(ifc, ift, diff_sol, diff_srf) * dzi2;
    cc = - (cw + ce + cs + cn + cb + ct + dti + 1.0e-8);

    // index of matrix
    ptrdiff_t icc = j_;
    ptrdiff_t icw = j_ - 1;
    ptrdiff_t ice = j_ + 1;
    ptrdiff_t ics = j_ - nx;
    ptrdiff_t icn = j_ + nx;
    ptrdiff_t icb = j_ - nxy;
    ptrdiff_t ict = j_ + nxy;
    // Neumann-boundary
    if(prm->mpi->nrk[4] == -1 && jx == 0   ) { cc += cw;  icw = -1; }
    if(prm->mpi->nrk[5] == -1 && jx == nx-1) { cc += ce;  ice = -1; }
    if(prm->mpi->nrk[2] == -1 && jy == 0   ) { cc += cs;  ics = -1; }
    if(prm->mpi->nrk[3] == -1 && jy == ny-1) { cc += cn;  icn = -1; }
    if(prm->mpi->nrk[0] == -1 && jz == 0   ) { cc += cb;  icb = -1; }
    if(prm->mpi->nrk[1] == -1 && jz == nz-1) { cc += ct;  ict = -1; }

    if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
    if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
    if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
    if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
    if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
    if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
    if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
  }

  return 0;
}

#ifdef JUPITER_MPI
static MPI_Datatype ox_create_subarray(int mx, int my, int mz,
                                       int is, int ie, int js, int je,
                                       int ks, int ke,
                                       MPI_Datatype element_type)
{
  int gd[3];
  int nd[3];
  int st[3];
  int nx = ie - is + 1;
  int ny = je - js + 1;
  int nz = ke - ks + 1;
  MPI_Datatype ret;

  gd[0] = mx;
  gd[1] = my;
  gd[2] = mz;
  nd[0] = nx;
  nd[1] = ny;
  nd[2] = nz;
  st[0] = is;
  st[1] = js;
  st[2] = ks;

  ret = MPI_DATATYPE_NULL;
  MPI_Type_create_subarray(3, gd, nd, st, MPI_ORDER_FORTRAN, element_type, &ret);
  if (ret == MPI_DATATYPE_NULL) {
    csvperrorf(__func__, 0, 0, CSV_EL_FATAL, NULL,
               "Failed to create subarray type");
    MPI_Abort(MPI_COMM_WORLD, 1);
    return ret;
  }
  MPI_Type_commit(&ret);

  return ret;
}

static void
ox_communicate(void *buf, MPI_Datatype subm[2], MPI_Datatype subp[2],
               int rankm, int rankp, MPI_Comm comm)
{
  if (rankm != -1 && rankp != -1) {
    MPI_Sendrecv(buf, 1, subm[0], rankm, 0, buf, 1, subp[1], rankp, 0,
                 comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(buf, 1, subp[0], rankp, 1, buf, 1, subm[1], rankm, 1,
                 comm, MPI_STATUS_IGNORE);
  } else if (rankm != -1) {
    MPI_Send(buf, 1, subm[0], rankm, 0, comm);
    MPI_Recv(buf, 1, subm[1], rankm, 1, comm, MPI_STATUS_IGNORE);
  } else if (rankp != -1) {
    MPI_Recv(buf, 1, subp[1], rankp, 0, comm, MPI_STATUS_IGNORE);
    MPI_Send(buf, 1, subp[0], rankp, 1, comm);
  }
}
#endif

static void ox_flag_communicate(parameter *prm, int *ox_flag)
{
#ifdef JUPITER_MPI
  MPI_Datatype subtype[6][2];
  mpi_param *mpi = prm->mpi;
  domain *cdo = prm->cdo;
  int i, j;
  int mx = cdo->mx;
  int my = cdo->my;
  int mz = cdo->mz;
  int stm = cdo->stm;
  int stp = cdo->stp;
  int nx = cdo->nx;
  int ny = cdo->ny;
  int nz = cdo->nz;
  int is, ie, js, je, ks, ke;
  int wid = 3;

  enum {
    s/* end */ = 0, r/* ecv */ = 1,
    zm = 0, zp = 1, ym = 2, yp = 3, xm = 4, xp = 5
  };

  for (i = 0; i < 6; ++i) {
    for (j = 0; j < 2; ++j) {
      subtype[i][j] = MPI_DATATYPE_NULL;
    }
  }

  is = stm;
  ie = stm + nx - 1;
  js = stm;
  je = stm + ny - 1;
  ks = stm;
  ke = stm + nz - 1;

  if (mpi->nrk[zm] != -1) {
    subtype[zm][s] = ox_create_subarray(mx, my, mz,
                                        is, ie, js, je, ks, ks + wid - 1,
                                        MPI_INT);
    subtype[zm][r] = ox_create_subarray(mx, my, mz,
                                        is, ie, js, je, ks - wid, ks - 1,
                                        MPI_INT);
  }
  if (mpi->nrk[zp] != -1) {
    subtype[zp][s] = ox_create_subarray(mx, my, mz,
                                        is, ie, js, je, ke - wid + 1, ke,
                                        MPI_INT);
    subtype[zp][r] = ox_create_subarray(mx, my, mz,
                                        is, ie, js, je, ke + 1, ke + wid,
                                        MPI_INT);
  }
  if (mpi->nrk[ym] != -1) {
    subtype[ym][s] = ox_create_subarray(mx, my, mz,
                                        is, ie, js, js + wid - 1, ks, ke,
                                        MPI_INT);
    subtype[ym][r] = ox_create_subarray(mx, my, mz,
                                        is, ie, js - wid, js - 1, ks, ke,
                                        MPI_INT);
  }
  if (mpi->nrk[yp] != -1) {
    subtype[yp][s] = ox_create_subarray(mx, my, mz,
                                        is, ie, je - wid + 1, je, ks, ke,
                                        MPI_INT);
    subtype[yp][r] = ox_create_subarray(mx, my, mz,
                                        is, ie, je + 1, je + wid, ks, ke,
                                        MPI_INT);
  }
  if (mpi->nrk[xm] != -1) {
    subtype[xm][s] = ox_create_subarray(mx, my, mz,
                                        is, is + wid - 1, js, je, ks, ke,
                                        MPI_INT);
    subtype[xm][r] = ox_create_subarray(mx, my, mz,
                                        is - wid, is - 1, js, je, ks, ke,
                                        MPI_INT);
  }
  if (mpi->nrk[xp] != -1) {
    subtype[xp][s] = ox_create_subarray(mx, my, mz,
                                        ie - wid + 1, ie, js, je, ks, ke,
                                        MPI_INT);
    subtype[xp][r] = ox_create_subarray(mx, my, mz,
                                        ie + 1, ie + wid, js, je, ks, ke,
                                        MPI_INT);
  }

  ox_communicate(ox_flag, subtype[zm], subtype[zp],
                 mpi->nrk[zm], mpi->nrk[zp], mpi->CommJUPITER);
  ox_communicate(ox_flag, subtype[ym], subtype[yp],
                 mpi->nrk[ym], mpi->nrk[yp], mpi->CommJUPITER);
  ox_communicate(ox_flag, subtype[xm], subtype[xp],
                 mpi->nrk[xm], mpi->nrk[xp], mpi->CommJUPITER);

  for (i = 0; i < 6; ++i) {
    for (j = 0; j < 2; ++j) {
      if (subtype[i][j] != MPI_DATATYPE_NULL) {
        MPI_Type_free(&subtype[i][j]);
      }
    }
  }
#endif
}
