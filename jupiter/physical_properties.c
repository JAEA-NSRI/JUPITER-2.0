#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "struct.h"

type diff_coef(int compo, type t)
{
  type dc;
  if(compo == 0 || compo == 1){
    if(t <= 1473.0) {
      dc = 1.422e-2*exp(-30011.976/t);
      return dc;
    } else if(t > 1473.0 && t < 1498.0){
      dc = 1.094e+10*exp(-65349.669/t);
      return dc;
    } else {  
      return 1.3e-9;
    }
  }
  return 0.0;
}
// liquidus temperature [K]
type liq(int compo, type Y)
{
  type liq;
  liq = 0.0;
  if(compo == 0 || compo == 1){
    if(Y < 0.037) {
      liq = 1450.0 - 255.0*Y/0.037 + 273.0;
    } else if (Y >= 0.037 && Y < 0.162){
      liq = 1195.0 + 3640.0*(Y - 0.037) + 273.0;
    } else if (Y >= 0.162 && Y < 0.25) {
      liq = 1650.0 - 150.0*(Y - 0.162)/0.088 + 273.0;
    } else {
      liq = 1500.0 + 1263.0*(Y - 0.25)/0.75 + 273.0;
    }
  }
  return liq;
}

// density dens [kg.m-3], (150 < T < 2480)
type fe_dens(type t)
{
  type temp;
  if(t < 1200.0) {
    type t2=t*t, t3=t2*t, t4=t3*t;
    temp = 7.25163e-10*t4
         - 1.38345e-6*t3
         + 6.90517e-4*t2
         - 0.350672*t
         + 7942.51;
    return temp;
  } else if(t < 1667.0) {
    return 7650.0 - 0.51*(t - 1184.0);
  } else if(t < 1811.0) {
    return 7355.0 - 0.42*(t - 1667.0);
  } else if(t < 2480.0) {
    return 7034.96 - 0.926*(t - 1811.0);
  } else {
    return 6415.466;
  }
}


// viscosity mu [Pa.s], (1850 < T < 2500)
type fe_visc(type t)
{
  type visc, t2=t*t, t3=t2*t, t4=t3*t;

  if(t < 1850.0) {
    return 6.0e-03;
  } else if(t < 2500.0) {
    visc = 4.89716e-15*t4
         - 4.78839e-11*t3
         + 1.78247e-7*t2
         - 3.01676e-4*t
         + 0.19931;
    return visc;
  } else {
    return 2.1e-03;
  }
}


// surface tension [N.m-1], (1809 < T < 2073)
type fe_st(type t)
{
  if(t < 1809) {
    return 1.87;
  } else if(1809.0 < t && t < 2073.0) {
    return 1.87 - 4.3e-4*(t - 1811.0);
  } else {
    return 1.75;
  }
}


// thermal conductivity [W.m-1.K-1], (0 < T < 6500)
type fe_tc(type t)
{
  type tc, t2=t*t, t3=t2*t;
  if(t < 15.0) {
    tc = - 0.303806*t3
         + 0.488117*t2
         + 174.083*t
         - 3.5906;
    return tc;
  } else if(t < 250.0) {
    return 52605.0*pow(t, -1.229);
  } else if(t < 1000.0) {
    return - 0.0704*t + 99.827;
  } else if(t < 1810.0) {
    return 0.0077*t + 20.49;
  } else if(t < 6500.0) {
    tc = 2.26729e-10*t3
       - 5.9686e-6*t2
       + 0.0290115*t
       + 6.50042;
    return tc;
  } else {
    return 5.10;
  }
}

// specific heat capacity at constant pressure [J.kg-1.K-1], (5 < T < 3130)
type fe_shc(type t)
{
  type shc, t2=t*t, t3=t2*t, t4=t3*t, t5=t4*t, t6=t5*t;
  if(t < 1040.0) {
    shc = 8.80855e-14*t6
        - 2.49593e-10*t5
        + 2.60194e-7*t4
        - 1.18248e-4*t3
        + 0.0193687*t2
        + 1.24568*t
        - 26.8505;
    return shc;
  } else if(t < 1800.0) {
    shc = 6.2868e-8*t4
        - 3.56425e-4*t3
        + 0.75343*t2
        - 703.591*t
        + 245508.0;
    return shc;
  } else {
    return 824.0;
  }
}

/*
type muexp(type t, type tm_A_l, type mu_l)
{
  type Mexp,ttm;
  ttm = t-tm_A_l;
  if(abs(ttm) < 600.0){
    Mexp = 0.0206*exp(-0.0287*ttm);
    return Mexp;
  } else {
    return mu_l;
  }
}
*/
//==================================
//  materials
//----------------------------------
/*
void UO2(phase_value *f)
{
    f->tm_A_soli = 1273.0;// melting temperature[K]
    f->tm_A_liq = 2623.0;
    f->tb = 2792.0;// boiling temperature[K]
    f->tr =  500.0;// room    temperature[K]
    //f->lrr  = 0.78;// Laser refrection rate (1.0 um).
    f->lh_A   = 297000.0;// latent heat[J/kg]
    f->beta = 23.0;    // coefficient of volumetric expansion
    f->sigma_A = 0.5;    // coefficient of surface tension
    f->dsdt = -1.34e-4;
    //-- density[kg m-3]
    f->rho_s_A = 7140.0;
    f->rho_l_A = 4940.0;
    f->rho_g =   1.2;
    //-- viscosity[Pa s]
    f->mu_s_A = 4.5e-3;
    f->mu_l_A = 4.5e-3;
    f->mu_g = 4.0e-05;
    //-- specific heat at constant volume[J kg-1 K-1]
    f->specht_s_A = 350.0;
    f->specht_l_A = 448.0;
    f->specht_g = 1000.0;
    //-- thermal conductivity[W m-1 K-1]
    f->thc_s_A = 2.0;
    f->thc_l_A = 3.5e+0;
    f->thc_g =   0.026;
    //-- radiation factor
    f->radf = 0.05;
}
void SUS(phase_value *f) //SUS304
{
    f->tm_B_soli = 1780.0;// melting temperature[K]
    f->tm_B_liq = 2900.0;
    f->tb = 2792.0;// boiling temperature[K]
    f->tr =  293.0;// room   temperature[K]
    //f->lrr  = 0.78;// Laser refrection rate (1.0 um).
    f->lh_B  = 706000.0;// latent heat[J/kg]
    f->beta = 23.0;    // coefficient of volumetric expansion
    f->sigma_B = 0.86;    // coefficient of surface tension
    f->dsdt = -1.34e-4;
    //-- density[kg m-3]
    f->rho_s_B = 5300.0;
    f->rho_l_B = 7000.0; //???
    f->rho_g =   1.2;
    //-- viscosity[Pa s]
    f->mu_s_B = 5.44e-03;
    f->mu_l_B = 5.44e-03;
    f->mu_g = 4.0e-05;
    //-- specific heat at constant volume[J kg-1 K-1]
    f->specht_s_B = 575.0;
    f->specht_l_B = 807.0;
    f->specht_g = 1000.0;
    //-- thermal conductivity[W m-1 K-1]
    f->thc_s_B = 4.7;
    f->thc_l_B = 60.0;
    f->thc_g =   0.026;
    //-- radiation factor
    f->radf = 0.05;
}
*/


//*******************************************************************************
/*
void aluminium(phase_value *f)
{
    f->tm =  933.0;// melting temperature[K]
    f->tb = 2792.0;// boiling temperature[K]
    f->tr =  293.0;// room    temperature[K]
    f->lrr  = 0.78;// Laser refrection rate (1.0 um).
    f->lh   = 394000.0;// latent heat[J/kg]
    f->beta = 23.0;    // coefficient of volumetric expansion
    f->sigma= 0.86;    // coefficient of surface tension
    f->dsdt = -1.34e-4;
    // density[kg m-3]
    f->rho_s = 2700.0;
    f->rho_l = 2377.0;
    f->rho_g =    1.2;
    // viscosity[Pa s]
    f->mu_s = 1.0e-06;
    f->mu_l = 8.6e-04;
    f->mu_g = 1.8e-05;
    // specific heat at constant volume[J kg-1 K-1]
    f->specht_s =  901.0;
    f->specht_l = 1170.0;
    f->specht_g = 1000.0;
    // thermal conductivity[W m-1 K-1]
    f->thc_s = 237.0;
    f->thc_l =  93.0;
    f->thc_g =   0.026;
    // radiation factor
    f->radf = 0.05;
}

void Fe(phase_value *f)
{
    f->tm = 1803.0;// melting temperature[K]
    f->tb = 2792.0;// boiling temperature[K]
    f->tr =  293.0;// room    temperature[K]
    f->lh = 297000.0;// latent heat
    f->lrr  = 0.52;// Laser refrection rate (1.0 um).
    f->beta = 23.0;// coefficient of volumetric expansion
    f->sigma= 1.8;// coefficient of surface tension
    // density[kg m-3]
    f->rho_s = 7870.0;
    f->rho_l = 7034.0;
    f->rho_g =    1.2;
    //f->rho_g =    322.0;
    // viscosity[Pa s]
    f->mu_s = 6.00e-03;
    f->mu_l = 5.44e-03;
    f->mu_g = 1.862e-05;
    // specific heat at constant volume[J kg-1 K-1]
    f->specht_s =  448.0;
    f->specht_l =  824.0;
    f->specht_g = 1000.0;
    // thermal conductivity[W m-1 K-1]
    f->thc_s = 80.2; //Y
    //f->thc_l = 40.3;
    //f->thc_s = 0.001;
    f->thc_l = 0.0005;
    f->thc_g = 0.026;
    // radiation factor
    f->radf = 0.5;
}

void water(phase_value *f)
{
    f->tm = 273.15;// melting temperature[K]
    f->tb = 373.15;// boiling temperature[K]
    f->tr = 293.15;// room    temperature[K]
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!11!!!!!!
    f->lh   = 0.0;// latent heat
    f->beta = 0.0;// coefficient of volumetric expansion
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!11!!!!!!
    f->sigma= 0.072;// coefficient of surface tension
    // density[kg m-3]
    f->rho_s =  916.8;
    f->rho_l = 1000.0;
    f->rho_g =    1.2;
    // viscosity[Pa s]
    f->mu_s = 1.0e-03;
    f->mu_l = 1.0e-03;//<= 25 oC
    f->mu_g = 1.8e-05;
    // specific heat at constant volume[J kg-1 K-1]
    f->specht_s = 2100.0;//<= -1 oC
    f->specht_l = 4217.0;//<=  0 oC
    f->specht_g =  717.0;
    // thermal conductivity[W m-1 K-1]
    f->thc_s = 2.2;
    f->thc_l = 0.6;
    f->thc_g = 0.026;
    // radiation factor
    f->radf = 0.0;
}
*/
