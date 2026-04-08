#include "component_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "field_control.h"
#include "geometry/list.h"
#include "trip_control.h"
#include "component_info.h"
#include "component_data_defs.h"
#include "heat_source.h"
#include "struct.h"
#include "func.h"
#include "common_util.h"
#include "os/os.h"

// Add laser irradiation
type heat_source_iav(type xw, type xe, type ys, type yn, type zb, type zt,
        type lsr_x, type lsr_y, type lsr_z, type qq0, type r0, type alpha, type R)
{
    type q0;
    q0 = (1.0 - R)*qq0*0.25*M_PI*r0*r0
        *( erf((xe - lsr_x)/r0)
         - erf((xw - lsr_x)/r0) )/( xe - xw )
        *( erf((yn - lsr_y)/r0)
         - erf((ys - lsr_y)/r0) )/( yn - ys );
    return - q0*(  exp( alpha*(zb - lsr_z) ) - exp( alpha*(zt - lsr_z) ) )/(zt - zb);
}

/* YSE: Move oxidation function to oxidation.c */

/* YSE: Rename argument dom */
int set_heat_source(type *q, type *fs, type *fl, type *Y, type *flg_obst_A, int n_ext_q, type *ext_q[n_ext_q], parameter *prm)
{
  mpi_param *mpi = prm->mpi;
  domain *cdo = prm->cdo;
  flags  *flg = prm->flg;
  laser *lsr = prm->lsr;
  int  i, j, jx, jy, jz, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, mx=cdo->mx, my=cdo->my, mz=cdo->mz,
       mxy=mx*my, m=cdo->m, stm=cdo->stm;

  // Add laser irradiation
  if(flg->laser == ON){
    type *x=cdo->x, *y=cdo->y, *z=cdo->z, lsr_z_max=0.0,
          lsr_x=lsr->lsr_x, lsr_y=lsr->lsr_y, lsr_z,
          surf=0.5;

    lsr_x = lsr->lsr_x + (lsr->swp_vel)*(cdo->time);//sweep laser position (x-direction)
  
#pragma omp parallel for private(jx,jy,jz, j, lsr_z)
    for(jy = 0; jy < my; ++jy) {
      for(jx = 0; jx < mx; ++jx) {
      // search laser position (z-direction)
        lsr_z=0.0;
        for(jz = 0; jz < mz; ++jz) {
        j = jx + mx*jy + mxy*jz;
        // search liquid or solid
        if((fl[j]-surf)*(fl[j+mxy]-surf) < 0.0) lsr_z = z[jz+1];
        if((fs[j]-surf)*(fs[j+mxy]-surf) < 0.0) lsr_z = z[jz+1];
        }
#ifdef MPI
      MPI_Allreduce(&lsr_z, &lsr_z, 1, MPI_TYPE, MPI_MAX, mpi->CommLz);
      MPI_Barrier(mpi->CommLz);
#endif
        for(jz = 0; jz < mz; ++jz) {
          j = jx + mx*jy + mxy*jz;
          // set heat source
          q[j] = heat_source_iav(x[jx],x[jx+1],y[jy],y[jy+1],z[jz],z[jz+1], 
                                     lsr_x,lsr_y,lsr_z, lsr->qm, lsr->r0, lsr->alpha, lsr->R);
          if( fs[j] < surf && fl[j] < surf ) q[j] = 0.0;
        }
      }
    }
  }
  // Laser irradiation end

  /* YSE: Implemented heat source set from input data */
  if (heat_source_param_has_any(&cdo->heat_sources_head)) {
#pragma omp parallel for private(jx,jy,jz), collapse(3)
    for (jz = 0; jz < mz; ++jz) {
      for (jy = 0; jy < my; ++jy) {
        for (jx = 0; jx < mx; ++jx) {
          ptrdiff_t j;
          heat_source_param *hp;
          type fli, fsi, Yi, fg;
          type fl_sum, fs_sum, Y_sum, Yg_sum;
          int ic;
          type qwsum;
          struct geom_list *lp, *lh;

          qwsum = 0.0;
          fl_sum = 0.0;
          fs_sum = 0.0;
          Y_sum = 0.0;
          Yg_sum = 0.0;

          j = calc_address(jx, jy, jz, mx, my, mz);
          if (Y) {
            for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
              Y_sum += clip(Y[j + ic * m]);
            }
            for (; ic < cdo->NumberOfComponent; ++ic) {
              Yg_sum += clip(Y[j + ic * m]);
            }
            fs_sum = clip(fs[j]);
            fl_sum = clip(fl[j]);
          } else {
            Y_sum = 1.0;
            Yg_sum = 0.0;
            for (ic = 0; ic < cdo->NumberOfComponent; ++ic) {
              fs_sum += clip(fs[j + ic * m]);
              fl_sum += clip(fl[j + ic * m]);
            }
          }
          fg = clip(1.0 - fs_sum - fl_sum);

          lh = &cdo->heat_sources_head.list;
          geom_list_foreach(lp, lh) {
            component_phases ph;
            int icY, icfs, icfl;
            hp = heat_source_param_entry(lp);
            if (!hp->comp.d)
              continue;

            ic = hp->comp.d->jupiter_id;
            icY = component_data_index(hp->comp.d, cdo, flg,
                                       COMPONENT_VARIABLE_MOLAR_FRACTION);
            icfs = component_data_index(hp->comp.d, cdo, flg,
                                        COMPONENT_VARIABLE_SOLID_VOF);
            icfl = component_data_index(hp->comp.d, cdo, flg,
                                        COMPONENT_VARIABLE_LIQUID_VOF);
            ph = hp->comp.d->phases;
            if (icY >= 0 || icfs >= 0 || icfl >= 0) {
              if (component_phases_has_solid_or_liquid(ph)) {
                Yi = 0.0;
                fsi = 0.0;
                fli = 0.0;

                if (Y && icY >= 0) {
                  if (Y_sum == 0.0) {
                    Yi = 0.0;
                  } else {
                    Yi = clip(Y[j + icY * m]) / Y_sum;
                  }
                  fsi = clip(fs[j]) * Yi;
                  fli = clip(fl[j]) * Yi;
                } else {
                  if (icfs >= 0)
                    fsi = clip(fs[j + icfs * m]);
                  if (icfl >= 0)
                    fli = clip(fl[j + icfl * m]);
                }

                qwsum += fsi * hp->q_s.current_value;
                qwsum += fli * hp->q_l.current_value;
              } else if (component_phases_has_gas(ph)) {
                if (Y) {
                  Yi = clip(Y[j + ic * m]);
                } else {
                  Yi = 0.0;
                }

                qwsum += fg * Yi * hp->q_s.current_value;
              }
            } else if (ic == -1) {
              if (component_phases_has_gas(ph)) {
                if (Y) {
                  Yi = clip(1.0 - Yg_sum);
                } else {
                  Yi = 1.0;
                }

                qwsum += fg * Yi * hp->q_s.current_value;
              }
            }
          }

          q[j] += qwsum;
        }
      }
    }
  }

  if (n_ext_q > 0) {
#pragma omp parallel for private(jx,jy,jz), collapse(3)
    for (jz = 0; jz < mz; ++jz) {
      for (jy = 0; jy < my; ++jy) {
        for (jx = 0; jx < mx; ++jx) {
          int iq;
          ptrdiff_t j;
          j = calc_address(jx, jy, jz, mx, my, mz);

          for (iq = 0; iq < n_ext_q; ++iq)
            q[j] += ext_q[iq][j];
        }
      }
    }
  }
  return 0;
}

type heat_source(variable *val, material *mtl, parameter *prm)
{
  flags  *flg = prm->flg;
  type   time0 = cpu_time();
  type  *Y;
  type  *ox_q;
  int n_ext_q;
  enum { MAX_EXTERNAL_Q = 7 };
  type *ext_q[MAX_EXTERNAL_Q];
  if(flg->heat_eq != ON) return 0;

  zero_clear(mtl->q, prm->cdo->m);
// radiation(val, mtl, prm);
// oxidation(val, mtl, prm);
  if (prm->flg->solute_diff == ON) {
    Y = val->Y;
  } else {
    Y = NULL;
  }

  n_ext_q = 0;
  if (val->qgeom)
    ext_q[n_ext_q++] = val->qgeom;

  if (prm->flg->oxidation == ON && val->ox_q)
    ext_q[n_ext_q++] = val->ox_q;

  if (val->qpt)
    ext_q[n_ext_q++] = val->qpt;

  CSVASSERT(n_ext_q <= MAX_EXTERNAL_Q);
  set_heat_source(mtl->q, val->fs, val->fl, Y, val->flg_obst_A, n_ext_q, ext_q, prm);

  return  cpu_time() - time0;
}

//---- utils for heat_source_param

void heat_source_param_init(heat_source_param *p)
{
  geom_list_init(&p->list);
  component_info_data_init(&p->comp);
  p->control = TRIP_CONTROL_INVALID;
  controllable_type_init(&p->q_l);
  controllable_type_init(&p->q_s);
}

heat_source_param *heat_source_param_new(void)
{
  heat_source_param *p;
  p = (heat_source_param *)malloc(sizeof(heat_source_param));
  if (!p)
    return NULL;

  heat_source_param_init(p);
  return p;
}

void heat_source_param_clean(heat_source_param *p)
{
  geom_list_delete(&p->list);
  controllable_type_remove_from_list(&p->q_l);
  controllable_type_remove_from_list(&p->q_s);
}

void heat_source_param_delete(heat_source_param *p)
{
  heat_source_param_clean(p);
  free(p);
}

void heat_source_param_delete_all(heat_source_param *head)
{
  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe(lp, ln, lh) {
    heat_source_param *p;
    p = heat_source_param_entry(lp);
    heat_source_param_delete(p);
  }
}

heat_source_param *
heat_source_param_find_base(heat_source_param *head,
                            heat_source_param_find_func *func, void *arg)
{
  struct geom_list *lp, *lh;
  lh = &head->list;
  geom_list_foreach (lp, lh) {
    heat_source_param *p;
    p = heat_source_param_entry(lp);
    if (func(p, arg))
      return p;
  }
  return NULL;
}

static int heat_source_param_find_impl(heat_source_param *p, void *arg)
{
  int comp_index = *(int *)arg;
  if (!p->comp.d)
    return 0;
  return p->comp.d->comp_index == comp_index;
}

heat_source_param *heat_source_param_find(heat_source_param *head,
                                          int comp_index)
{
  return heat_source_param_find_base(head, heat_source_param_find_impl,
                                     &comp_index);
}

static int heat_source_param_find_by_id_impl(heat_source_param *p, void *arg)
{
  int id = *(int *)arg;
  if (!p->comp.d)
    return p->comp.id == id;
  return p->comp.d->jupiter_id == id;
}

heat_source_param *heat_source_param_find_by_id(heat_source_param *head,
                                                int jupiter_id)
{
  return heat_source_param_find_base(head, heat_source_param_find_by_id_impl,
                                     &jupiter_id);
}

static int heat_source_param_find_by_d_impl(heat_source_param *p, void *arg)
{
  component_data *d = (component_data *)arg;
  if (!p->comp.d)
    return 0;
  return p->comp.d == d;
}

heat_source_param *heat_source_param_find_by_data(heat_source_param *head,
                                                  component_data *data)
{
  return heat_source_param_find_base(head, heat_source_param_find_by_d_impl,
                                     data);
}
