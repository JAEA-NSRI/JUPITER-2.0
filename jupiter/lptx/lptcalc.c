#include "lptcalc.h"
#include "collision_list.h"
#include "constants.h"
#include "defs.h"
#include "error.h"
#include "jupiter/geometry/list.h"
#include "jupiter/random/random.h"
#include "overflow.h"
#include "param.h"
#include "particle.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include "pvector.h"
#include "struct_defs.h"
#include "type_math.h"
#include "util.h"
#include "vector.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

static LPTX_pcoef lptx_pcoef_c(LPTX_type a, LPTX_type b, LPTX_type g)
{
  return (LPTX_pcoef){.a = a, .b = b, .g = g};
}

static LPTX_pcoef lptx_pcoef_z(void)
{
  return lptx_pcoef_c(LPTX_C(0.), LPTX_C(0.), LPTX_C(0.));
}

static LPTX_pcoef lptx_pcoef_i(LPTX_int iptirk, LPTX_pcoef c1, LPTX_pcoef c2,
                               LPTX_pcoef c3)
{
  switch (iptirk) {
  case 1:
    return c1;
  case 2:
    return c2;
  case 3:
    return c3;
  }
  return lptx_pcoef_z();
}

static LPTX_pcoef lptx_pcoef(LPTX_time_scheme scheme, LPTX_int iptirk)
{
  switch (scheme) {
  case LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2:
    return lptx_pcoef_i(iptirk,
                        (LPTX_pcoef){
                          .a = LPTX_C(1.0),
                          .b = LPTX_C(3.0) / LPTX_C(2.0),
                          .g = LPTX_C(-1.0) / LPTX_C(2.0),
                        },
                        (LPTX_pcoef){
                          .a = LPTX_C(1.0) / LPTX_C(2.0),
                          .b = LPTX_C(1.0),
                          .g = LPTX_C(0.0),
                        },
                        (LPTX_pcoef){
                          .a = LPTX_C(1.0) / LPTX_C(2.0),
                          .b = LPTX_C(2.0),
                          .g = LPTX_C(-1.0),
                        });
  case LPTX_TIME_SCHEME_RUNGE_KUTTA_2:
    return lptx_pcoef_i(iptirk,
                        (LPTX_pcoef){
                          .a = LPTX_C(1.0) / LPTX_C(2.0),
                          .b = LPTX_C(1.0),
                          .g = LPTX_C(0.0),
                        },
                        (LPTX_pcoef){
                          .a = LPTX_C(1.0) / LPTX_C(2.0),
                          .b = LPTX_C(2.0),
                          .g = LPTX_C(-1.0),
                        },
                        //--- not specified
                        (LPTX_pcoef){
                          .a = LPTX_C(0.0),
                          .b = LPTX_C(0.0),
                          .g = LPTX_C(0.0),
                        });
  case LPTX_TIME_SCHEME_RUNGE_KUTTA_3:
    return lptx_pcoef_i(iptirk,
                        (LPTX_pcoef){
                          .a = LPTX_C(8.0) / LPTX_C(15.0),
                          .b = LPTX_C(1.0),
                          .g = LPTX_C(0.0),
                        },
                        (LPTX_pcoef){
                          .a = LPTX_C(2.0) / LPTX_C(15.0),
                          .b = LPTX_C(75.0) / LPTX_C(24.0),
                          .g = LPTX_C(-17.0) / LPTX_C(8.0),
                        },
                        (LPTX_pcoef){
                          .a = LPTX_C(5.0) / LPTX_C(15.0),
                          .b = LPTX_C(9.0) / LPTX_C(4.0),
                          .g = LPTX_C(-5.0) / LPTX_C(4.0),
                        });
  case LPTX_TIME_SCHEME_INVALID:
    break;
  }
  return lptx_pcoef_z();
}

static LPTX_type LPTX_cunningham_correction(LPTX_type mean_free_path,
                                            LPTX_type diameter, LPTX_type A1,
                                            LPTX_type A2, LPTX_type A3)
{
  LPTX_type knudsen;
  LPTX_type e;

  if (diameter <= LPTX_C(0.0) || mean_free_path <= LPTX_C(0.0))
    return LPTX_C(1.0);

  knudsen = mean_free_path / diameter;
  e = LPTX_type_exp(-A3 / knudsen);
  return LPTX_C(1.0) + LPTX_C(2.0) * knudsen * (A1 + A2 * e);
}

static LPTX_type LPTX_cunningham_correction_pt(const LPTX_param *param,
                                               const LPTX_particle *p)
{
  if (param->use_constant_Cc)
    return param->cunningham_correction;

  return LPTX_cunningham_correction(p->mean_free_path, p->diameter,
                                    param->cunningham_corr_const_A1,
                                    param->cunningham_corr_const_A2,
                                    param->cunningham_corr_const_A3);
}

static LPTX_type LPTX_drag_coeffecient(LPTX_type reynolds_number)
{
  /*
   * By Schiller-Naumann, Re < 800.
   */
  return (LPTX_C(24.) / reynolds_number) *
         (LPTX_C(1.) +
          LPTX_C(0.15) * LPTX_math(pow)(reynolds_number, LPTX_C(0.687)));
}

static LPTX_type LPTX_calc_taup(const LPTX_particle *p)
{
  LPTX_type prepf, pcdpt, pdiasq, ptaup;
  prepf = LPTX_particle_reynolds_number(p);
  if (prepf <= LPTX_C(0.)) {
    pcdpt = LPTX_TYPE_HUGE_VAL;
  } else {
    pcdpt = LPTX_drag_coeffecient(prepf);
  }

  pdiasq = p->diameter * p->diameter;
  if (p->density * pdiasq > LPTX_C(0.)) {
    if (prepf <= LPTX_C(0.) || !isfinite(pcdpt)) {
      /*
       * Limit for (prepf -> +0) on Schiller-Naumann, Re < 800 formula
       */
      ptaup = LPTX_C(18.) * p->fluid_viscosity / (p->density * pdiasq);
    } else {
      ptaup = LPTX_C(3.) * pcdpt * p->fluid_viscosity * prepf /
              (LPTX_C(4.) * p->density * pdiasq);
    }
  } else {
    ptaup = LPTX_C(0.);
  }
  return ptaup;
}

static LPTX_vector LPTX_calc_brownian_force(LPTX_particle *p,
                                            LPTX_type time_step_width,
                                            LPTX_type cunningham_correction)
{
  LPTX_type dt, s0dt, pi2, dp, dp5, rp2, mu, intensity, T, zx, zy, zz, mass;
  LPTX_vector f;

  mu = p->fluid_viscosity;
  dp = p->diameter;
  dp5 = dp * dp * dp * dp * dp;
  rp2 = p->density * p->density;
  dt = time_step_width;
  T = p->fluid_temperature;

  /*
   * s0 =
   *     (216 * nu * kB * T) / (pi^2 * rho * dp^5 * (rho_p/rho)^2 * Cc)
   * <=> (216 * (mu/rho) * kB * T) / (pi^2 * dp^5 * (rho_p^2/rho) * Cc)
   * <=> (216 * mu * kB * T) / (pi^2 * dp^5 * rho_p^2 * Cc)
   *
   * s0 * pi / dt => (216 * mu * kB * T) / (pi * dp^5 * rho_p^2 * Cc * dt);
   */
  s0dt = LPTX_C(216.0) * mu * LPTX_boltzmann_constant * T;
  s0dt /= LPTX_M_PI * dp5 * rp2 * cunningham_correction * dt;
  intensity = LPTX_type_sqrt(s0dt);
  mass = p->density * LPTX_M_PI * dp * dp * dp / LPTX_C(6.);
  zx = jupiter_random_nextdn(&p->ptseed);
  zy = jupiter_random_nextdn(&p->ptseed);
  zz = jupiter_random_nextdn(&p->ptseed);
  return LPTX_vector_mulf(mass * intensity, LPTX_vector_c(zx, zy, zz));
}

static LPTX_vector LPTX_calc_thermophoretic_force(LPTX_particle *p,
                                                  LPTX_type Cs, LPTX_type Ct,
                                                  LPTX_type Cm)
{
  LPTX_type T, dp, mu, K, R, Kn, rho, Dtp;
  LPTX_vector Tgrad;

  T = p->fluid_temperature;
  Tgrad = p->fluid_temperature_grad;
  mu = p->fluid_viscosity;
  rho = p->fluid_density;
  dp = p->diameter;
  if (p->fluid_molecular_weight > 0.0) {
    R = LPTX_gas_constant / p->fluid_molecular_weight;
  } else {
    R = 0.0; /* exclude thermal conductivity effect */
  }
  K = LPTX_C(15.) / LPTX_C(4.) * mu * R;
  K /= p->thermal_conductivity;
  Kn = LPTX_C(2.) * p->mean_free_path / dp;

  Dtp = LPTX_C(6.0) * LPTX_M_PI * dp * mu * mu * Cs * (K + Ct * Kn);
  Dtp /= rho * (LPTX_C(1.0) + LPTX_C(3.0) * Cm * Kn) *
         (LPTX_C(1.0) + LPTX_C(2.0) * K + LPTX_C(2.0) * Ct * Kn);
  return LPTX_vector_mulf(-Dtp / T, Tgrad);
}

/**
 * Integrate time scheme of heat equation by p->heat_transfer_rate.
 */
static void LPTX_single_heat_integrate(LPTX_particle *p,
                                       const LPTX_single_time_integrate_data *d)
{
  LPTX_type h, ht, mp, cpp, tf, tp, dTdt, Ap, Np, delT;
  tp = p->temperature;
  tf = p->fluid_temperature;
  h = p->heat_transfer_rate;
  mp = LPTX_particle_mass(p);
  Ap = LPTX_particle_surface_area(p);
  cpp = p->specific_heat;
  ht = h * Ap * (tf - tp);
  dTdt = ht / (mp * cpp);

  delT = d->pcoef.b * dTdt + d->pcoef.g * p->dTdt;
  delT = d->pcoef.a * d->time_step_width * delT;
  p->temperature += delT;
  p->dTdt = dTdt;
  p->total_heat_transfer = ht * p->parceln;
}

static void
LPTX_single_heat_follow_fluid(LPTX_particle *p,
                              const LPTX_single_time_integrate_data *d)
{
  LPTX_type dTdt, tp, tf, h, ht, Ap, mp, cpp;
  tp = p->temperature;
  tf = p->fluid_temperature;
  Ap = LPTX_particle_surface_area(p);
  mp = LPTX_particle_mass(p);
  cpp = p->specific_heat;
  dTdt = /* (tf - tp) *  */ LPTX_C(1.) / d->time_step_width;
  ht = mp * cpp * dTdt;
  h = ht / Ap /* / (tf - tp) */;
  p->heat_transfer_rate = h;
  p->total_heat_transfer = ht * (tf - tp) * p->parceln;
  p->temperature = tf;
}

static void
LPTX_single_heat_ranz_marshall(LPTX_particle *p,
                               const LPTX_single_time_integrate_data *d)
{
  LPTX_type prepf, prandtl, nusselt, k, dp;
  prepf = LPTX_particle_reynolds_number(p);
  prandtl = LPTX_particle_prandtl_number(p);
  prepf = LPTX_type_sqrt(prepf);
  prandtl = LPTX_type_cbrt(prandtl);
  nusselt = LPTX_C(2.) + LPTX_C(0.6) * prepf * prandtl;
  k = p->fluid_thermal_conductivity;
  dp = p->diameter;
  p->heat_transfer_rate = nusselt * k / dp;
  LPTX_single_heat_integrate(p, d);
}

static void LPTX_calc_heat_exchange(LPTX_particle *p, LPTX_param *param,
                                    const LPTX_single_time_integrate_data *d)
{
  switch (param->heat_scheme) {
  case LPTX_HEAT_FOLLOW_FLUID:
    LPTX_single_heat_follow_fluid(p, d);
    break;
  case LPTX_HEAT_RANZ_MARSHALL:
    LPTX_single_heat_ranz_marshall(p, d);
    break;

  case LPTX_HEAT_OFF:
  case LPTX_HEAT_INVALID:
    break;
  }
}

static void
LPTX_single_time_integrate_p(LPTX_particle_data *data, LPTX_param *param,
                             const LPTX_single_time_integrate_data *d)
{
  LPTX_particle *p;
  LPTX_vector dpos, pdut, dvel;
  LPTX_type ptaup, ptaup_mxlim, rrho;
  LPTX_type pmass;
  p = &data->base;

  if (!LPTX_particle_is_used(p))
    return;

  if (LPTX_particle_is_exited(p))
    return;

  p->current_time = d->current_time;
  if (d->current_time < p->start_time)
    return;

  if (d->iptist == 0 && d->substepno == 0) {
    LPTX_param_count_tracked(param, 1);

    p->fxpt = LPTX_vector_zero();
  }

  /* Update particle position */
  dpos = LPTX_vector_add(LPTX_vector_mulf(d->pcoef.b, p->velocity),
                         LPTX_vector_mulf(d->pcoef.g, p->fupt));
  dpos = LPTX_vector_mulf(d->pcoef.a * d->time_step_width, dpos);
  p->position = LPTX_vector_add(p->position, dpos);

  p->fupt = p->velocity;

  /* Update particle velocity */
  ptaup = LPTX_calc_taup(p);

  ptaup_mxlim = LPTX_C(1.) / d->time_step_width;
  if (ptaup > ptaup_mxlim)
    ptaup = ptaup_mxlim;

  rrho = (p->density - p->fluid_density) / p->density;
  pdut = LPTX_vector_sub(p->fluid_velocity, p->velocity);
  pdut = LPTX_vector_add(LPTX_vector_mulf(ptaup, pdut),
                         LPTX_vector_mulf(rrho, param->gravity));

  pmass = LPTX_particle_mass(p);

  if (param->brownian_force) {
    LPTX_vector abm;

    if (d->iptist == 0 && d->substepno == 0) {
      LPTX_type dt;
      LPTX_type cc;
      dt = d->time_step_width * d->number_of_substeps;
      cc = LPTX_cunningham_correction_pt(param, p);
      p->fbpt = LPTX_calc_brownian_force(p, dt, cc);
    }

    abm = LPTX_vector_mulf(1.0 / pmass, p->fbpt);
    pdut = LPTX_vector_add(pdut, abm);
  }

  if (param->thermophoretic_force) {
    LPTX_vector abm;

    if (d->iptist == 0 && d->substepno == 0) {
      LPTX_type Cs = param->thermophoretic_const_Cs;
      LPTX_type Ct = param->thermophoretic_const_Ct;
      LPTX_type Cm = param->thermophoretic_const_Cm;

      p->fTpt = LPTX_calc_thermophoretic_force(p, Cs, Ct, Cm);
    }

    abm = LPTX_vector_mulf(1.0 / pmass, p->fTpt);
    pdut = LPTX_vector_add(pdut, abm);
  }

  dvel = LPTX_vector_add(LPTX_vector_mulf(d->pcoef.b, pdut),
                         LPTX_vector_mulf(d->pcoef.g, p->fdut));
  dvel = LPTX_vector_mulf(d->pcoef.a * d->time_step_width, dvel);
  p->velocity = LPTX_vector_add(p->velocity, dvel);

  p->fdut = pdut;

  pdut = LPTX_vector_sub(pdut, param->gravity);
  pdut = LPTX_vector_mulf(pmass / (LPTX_type)d->number_of_substeps, pdut);
  p->fxpt = LPTX_vector_sub(p->fxpt, pdut);

  /* Update particle temperature */
  if (LPTX_param_heat_exchange(param))
    LPTX_calc_heat_exchange(p, param, d);
}

static LPTX_bool LPTX_single_time_integrate_set(LPTX_particle_set *set,
                                                void *arg)
{
  LPTX_idtype np;
  const LPTX_single_time_integrate_data *d;

  d = (const LPTX_single_time_integrate_data *)arg;
  np = LPTX_particle_set_number_of_particles(set);

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype ip = 0; ip < np; ++ip)
    LPTX_single_time_integrate_p(&set->particles[ip], set->list.param, d);

  return LPTX_false;
}

void LPTX_single_time_integrate(
  LPTX_param *param, const LPTX_single_time_integrate_data *integrate_data)
{
  LPTX_param_foreach_particle_sets_i(param, LPTX_single_time_integrate_set,
                                     (void *)integrate_data);
}

LPTX_bool LPTX_substep_loop_n(LPTX_param *param, LPTX_idtype stepno,
                              LPTX_idtype number_of_substeps,
                              LPTX_type current_time, LPTX_type time_step_width,
                              LPTX_cb_substep_loop *func, void *args)
{
  LPTX_int iptirk;
  LPTX_type subtime, subtsw;
  LPTX_int iptsta, iptend;

  if (number_of_substeps < 1)
    return LPTX_false;

  param->cumulative_stat.nstep += 1;

  subtime = current_time;
  subtsw = time_step_width / (LPTX_type)number_of_substeps;

  switch (param->time_scheme) {
  case LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2:
    iptsta = 1;
    iptend = 1;
    break;
  case LPTX_TIME_SCHEME_RUNGE_KUTTA_2:
    iptsta = 1;
    iptend = 2;
    break;
  case LPTX_TIME_SCHEME_RUNGE_KUTTA_3:
    iptsta = 1;
    iptend = 3;
    break;
  case LPTX_TIME_SCHEME_INVALID:
  default:
    return LPTX_false;
  }

  for (LPTX_idtype isub = 0; isub < number_of_substeps; ++isub) {
    if (param->time_scheme == LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2 &&
        stepno == 1 && isub == 0) {
      iptsta = 2;
      iptend = 3;
    }

    for (LPTX_int iptirk = iptsta; iptirk <= iptend; ++iptirk) {
      struct LPTX_single_time_integrate_data d = {
        .pcoef = lptx_pcoef(param->time_scheme, iptirk),
        .iptirk = iptirk,
        .iptist = iptirk - iptsta,
        .current_time = current_time,
        .time_step_width = subtsw,
        .number_of_substeps = number_of_substeps,
        .substepno = isub,
        .stepno = stepno,
      };

      if (!func(param, &d, args))
        return LPTX_false;
    }

    subtime += subtsw;
  }

  return LPTX_true;
}

LPTX_bool LPTX_substep_loop_f(LPTX_param *param, LPTX_idtype stepno,
                              LPTX_type current_time, LPTX_type time_step_width,
                              LPTX_cb_substep_loop *func, void *args)
{
  LPTX_idtype n = param->number_of_substep;

  if (n < 1)
    n = 1;
  if (n > param->max_number_of_substep)
    n = param->max_number_of_substep;

  return LPTX_substep_loop_n(param, stepno, n, current_time, time_step_width,
                             func, args);
}

static LPTX_bool LPTX_update_number_of_substep_impl(LPTX_particle_set *set,
                                                    void *arg)
{
  LPTX_param *param = arg;
  LPTX_idtype n = LPTX_particle_set_number_of_particles(set);
  LPTX_type ptaup_max = LPTX_C(0.);

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype i = 0; i < n; ++i) {
    LPTX_type ptaup;
    const LPTX_particle *p = &set->particles[i].base;
    if (!LPTX_particle_is_used(p))
      continue;

    if (LPTX_particle_is_exited(p))
      continue;

    ptaup = LPTX_calc_taup(p);
    if (ptaup > ptaup_max)
      ptaup_max = ptaup;
  }

  LPTX_type_update_max(&param->ptaup_max, ptaup_max);
  return LPTX_false;
}

int LPTX_update_number_of_substep(LPTX_param *param, LPTX_type time_step_width)
{
  int r;
  param->ptaup_max = LPTX_C(0.);

#ifdef _OPENMP
#pragma omp parallel
#endif
  LPTX_param_foreach_particle_sets_i(param, LPTX_update_number_of_substep_impl,
                                     param);

  r = 0;
#ifdef JUPITER_LPTX_MPI
  if (param->mpi_comm != MPI_COMM_NULL) {
    r = MPI_Allreduce(MPI_IN_PLACE, &param->ptaup_max, 1, LPTX_MPI_TYPE,
                      MPI_MAX, param->mpi_comm);
    if (r != MPI_SUCCESS)
      return r;
  }
#endif

  param->number_of_substep =
    LPTX_math(floor)(time_step_width * param->ptaup_max);
  if (param->number_of_substep < 1)
    param->number_of_substep = 1;

  return r;
}

struct LPTX_count_collision_newpts_args
{
  LPTX_idtype n; ///< Number of new particles to allocate
};

static LPTX_bool LPTX_count_collision_newpts(LPTX_collision_list_data *d,
                                             void *args)
{
  struct LPTX_count_collision_newpts_args *p = args;
  if (LPTX_s_add_overflow(p->n, d->new_p, &p->n)) {
    p->n = -1;
    return LPTX_true;
  }

  d->processed = LPTX_false;
  return LPTX_false;
}

struct LPTX_count_max_collision_partners_args
{
  LPTX_idtype n;                    ///< Maximum number of partners
  LPTX_collision_list_data **roots; ///< Collision list roots
};

static LPTX_bool LPTX_count_max_collision_partners(LPTX_collision_list_data *d,
                                                   void *args)
{
  LPTX_idtype nary = 2;
  struct geom_list *lp, *lh;

  struct LPTX_count_max_collision_partners_args *p = args;
  if (d->processed)
    return LPTX_false;

  lh = &d->list;
  geom_list_foreach (lp, lh) {
    LPTX_collision_list_data *x;
    x = LPTX_collision_list_data_entry(lp);
    x->processed = LPTX_true;
    nary += 1;
  }
  d->processed = LPTX_true;

  if (p->n < nary)
    p->n = nary;

  *p->roots++ = d;
  return LPTX_false;
}

LPTX_idtype LPTX_calc_collision(LPTX_param *param, LPTX_cb_collision_func *func,
                                void *arg)
{
  LPTX_idtype nnewpt, nmaxpartners;
  LPTX_particle_set *nset = NULL;
  LPTX_particle_data **partners;
  LPTX_collision_list_data **roots;
  LPTX_idtype nroots;
  LPTX_bool *partner_matrix;
  LPTX_idtype ret;

  if (param->number_of_collisions <= 0)
    return 0;

  nset = NULL;
  partners = NULL;
  partner_matrix = NULL;
  roots = NULL;
  ret = 0;
  nroots = param->number_of_collisions;

  {
    struct LPTX_count_collision_newpts_args a = {.n = 0};
    LPTX_param_foreach_collision_data_i(param, LPTX_count_collision_newpts, &a);
    nnewpt = a.n;
  }

  if (nnewpt < 0)
    return -1;

  do {
    LPTX_particle_data *gnewp = NULL;

    roots = (LPTX_collision_list_data **)malloc(
      sizeof(LPTX_collision_list_data *) * nroots);
    if (!roots) {
      ret = -1;
      break;
    }

    if (nnewpt > 0) {
      nset = LPTX_param_new_particle_set(param, nnewpt);
      if (!nset) {
        ret = -1;
        break;
      }

      gnewp = nset->particles;
    }

    {
      struct LPTX_count_max_collision_partners_args a = {.n = 2,
                                                         .roots = roots};
      LPTX_param_foreach_collision_data_i(param,
                                          LPTX_count_max_collision_partners,
                                          &a);
      nmaxpartners = a.n;
      nroots = a.roots - roots;
    }

    if (nroots <= 0) {
      ret = 0;
      break;
    }

#ifdef _OPENMP
#pragma omp parallel if (nroots > LPTX_omp_small_threshold)
#endif
    do {
      LPTX_particle_data **lpartners;
      LPTX_bool *lmatrix;
      int nt = 1;
      int ti = 0;
      LPTX_idtype sz;

#ifdef _OPENMP
      nt = omp_get_num_threads();
      ti = omp_get_thread_num();
#endif

#ifdef _OPENMP
#pragma omp master
#endif
      do {
        sz = sizeof(LPTX_particle_data *);
        if (LPTX_s_mul_overflow(sz, nmaxpartners, &sz))
          break;
        if (LPTX_s_mul_overflow(sz, nt, &sz))
          break;

        partners = (LPTX_particle_data **)malloc(sz);
        if (!partners) {
          ret = -1;
          break;
        }

        sz = sizeof(LPTX_bool);
        if (LPTX_s_mul_overflow(sz, nmaxpartners, &sz))
          break;
        if (LPTX_s_mul_overflow(sz, nmaxpartners, &sz))
          break;
        if (LPTX_s_mul_overflow(sz, nt, &sz))
          break;

        partner_matrix = (LPTX_bool *)malloc(sz);
        if (!partner_matrix) {
          ret = -1;
          break;
        }
      } while (0);

#ifdef _OPENMP
#pragma omp barrier
#endif
      if (!partners || !partner_matrix)
        break;

      lpartners = partners + nmaxpartners * ti;
      lmatrix = partner_matrix + nmaxpartners * nmaxpartners * ti;

#ifdef _OPENMP
#pragma omp for
#endif
      for (LPTX_idtype jj = 0; jj < nroots; ++jj) {
        struct geom_list *lp, *lh;
        LPTX_idtype iptn;
        LPTX_collision_list_data *root = roots[jj];
        LPTX_idtype kk, ll;
        LPTX_idtype np;
        LPTX_particle_data *newp;

        lh = &root->list;

        newp = NULL;
        np = 0;
        iptn = 0;
        lpartners[iptn++] = root->a;
        lpartners[iptn++] = root->b;

        geom_list_foreach (lp, lh) {
          LPTX_collision_list_data *p;

          p = LPTX_collision_list_data_entry(lp);
          for (kk = 0; kk < iptn; ++kk) {
            if (lpartners[kk] == p->a)
              break;
          }
          if (kk == iptn)
            lpartners[iptn++] = p->a;

          for (kk = 0; kk < iptn; ++kk) {
            if (lpartners[kk] == p->b)
              break;
          }
          if (kk == iptn)
            lpartners[iptn++] = p->b;

          np += p->new_p;
        }

        for (kk = 0; kk < iptn; ++kk) {
          for (ll = 0; ll < iptn; ++ll)
            lmatrix[ll * iptn + kk] = LPTX_false;
        }

        lmatrix[1] = LPTX_true;
        lmatrix[iptn] = LPTX_true;
        geom_list_foreach (lp, lh) {
          LPTX_collision_list_data *p;

          p = LPTX_collision_list_data_entry(lp);
          for (kk = 0; kk < iptn; ++kk) {
            if (lpartners[kk] == p->a)
              break;
          }

          for (ll = 0; ll < iptn; ++ll) {
            if (lpartners[ll] == p->b)
              break;
          }

          lmatrix[kk * iptn + ll] = LPTX_true;
          lmatrix[ll * iptn + kk] = LPTX_true;
        }

        if (np > 0) {
#ifdef _OPENMP
#pragma omp atomic capture
#endif
          {
            newp = gnewp;
            gnewp += np;
          }
        }

        func(iptn, lpartners, lmatrix, np, newp, arg);
      }
    } while (0);
    if (ret < 0)
      break;

  } while (0);

  if (ret >= 0) {
    if (nset)
      LPTX_particle_set_append(param, nset);
    nset = NULL;

    param->last_stat.collided += ret;
    param->cumulative_stat.collided += ret;
  }

  if (nset)
    LPTX_particle_set_delete(nset);
  if (partners)
    free(partners);
  if (partner_matrix)
    free(partner_matrix);
  if (roots)
    free(roots);

  return ret;
}
