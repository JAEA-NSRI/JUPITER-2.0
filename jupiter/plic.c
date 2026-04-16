#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "struct.h"
#include "func.h"
#include "boundary_util.h"

type forward_3d_core(type m1, type m2, type m3, type a){

    type V, m12 = m1+m2, m = fmin(m12, m3);
    type V1, eps = 1e-10;

    if(a<m1){
        V = a*a*a/(6*m1*m2*m3);
    }else if(a<m2){
        V1 = m1*m1/(fmax(6*m2*m3, eps));
        V = a*(a-m1)/(2*m2*m3) + V1;
    }else if(a<m){
        V = (a*a*(3*m12-a) + m1*m1*(m1-3*a) + m2*m2*(m2-3*a))/(6*m1*m2*m3);
    }else{
        if(m3<m12){
            V = (a*a*(3-2*a) + m1*m1*(m1-3*a) + m2*m2*(m2-3*a) + m3*m3*(m3-3*a))/(6*m1*m2*m3);
        }else{
            V = (2*a-m12)/(2*m3);
        }
    }

    return V;
}

type forward_3d(type m1, type m2, type m3, type a, type dx, type dy, type dz){

    if(m1<0){
        a -= m1*dx;
        m1 = -m1;
    }

    if(m2<0){
        a -= m2*dy;
        m2 = -m2;
    }

    if(m3<0){
        a -= m3*dz;
        m3 = -m3;
    }

    //normalization
    m1 *= dx;
    m2 *= dy;
    m3 *= dz;

    type normalize = m1+m2+m3;
    m1 /= normalize;
    m2 /= normalize;
    m3 /= normalize;
    a  /= normalize;

    if(a>1.0){
        return dx*dy*dz;
    }else if(a<0.0){
        return 0.0;
    }

    type V;

    if(!(m1<=m2 && m2<=m3)){
        int i,j;
        type m[3], temp;
        m[0] = m1, m[1] = m2, m[2] =m3;
        for(i=0; i<3; i++){
            for(j=3-1; j>i; j--){
                if(m[j] < m[j-1]){
                    temp = m[j];
                    m[j] = m[j-1];
                    m[j-1] = temp;
                }
            }
        }
        m1 = m[0], m2 = m[1], m3 = m[2];
    }

    if(a<0.5){
        V = forward_3d_core(m1, m2, m3, a);
    }else{
        a = 1.0 - a;
        V = 1.0 - forward_3d_core(m1, m2, m3, a);
    }

    V *= dx*dy*dz;

    return V;

}

type solve_cubic_equation(type a3, type a2, type a1, type a0){

    type p0, q0, theta;

    a0 /= a3;
    a1 /= a3;
    a2 /= a3;
    a3 = 1.0;

    p0 = a1/3 - a2*a2/9;
    q0 = (a1*a2-3*a0)/6 - a2*a2*a2/27;

    theta = (acos(q0/(sqrt(-p0*p0*p0))))/3;

    return sqrt(-p0)*(sqrt(3)*sin(theta)-cos(theta))-a2/3;
}


type inverse_3d_core(type m1, type m2, type m3, type V){

    type a, m12 = m1+m2, m = fmin(m12, m3);
    type V1, V2, V3, V31, V32, eps = 1e-10;
    type a3_, a2_, a1_, a0_,
         a3__, a2__, a1__, a0__;

    V1 = m1*m1/(fmax(6*m2*m3, eps));
    V2 = V1 + (m2-m1)/(2*m3);
    V31 = (m3*m3*(3*m12-m3)+m1*m1*(m1-3*m3) + m2*m2*(m2-3*m3))/(6*m1*m2*m3);
    V32 = m12/(2*m3);

    if(m3<=m12){
        V31 = (m3*m3*(3*m12-m3)+m1*m1*(m1-3*m3) + m2*m2*(m2-3*m3))/(6*m1*m2*m3);
        V3 = V31;
    }else{
        V3 = V32;
    }

    if(0<=V && V<V1){
        a = cbrt(6*m1*m2*m3*V);
    }else if(V1<= V && V<V2){
        a = 0.5*(m1 + sqrt(m1*m1 + 8*m2*m3*(V-V1)));
    }else if(V2<=V && V<V3){
        a3_ = -1;
        a2_ = 3*m12;
        a1_ = -3*(m1*m1+m2*m2);
        a0_ = m1*m1*m1 + m2*m2*m2 - 6*m1*m2*m3*V;
        a = solve_cubic_equation(a3_, a2_, a1_, a0_);
    }else{
        if(m3<=m12){
            a3__ = -2;
            a2__ = 3;
            a1__ = -3*(m1*m1+m2*m2+m3*m3);
            a0__ = m1*m1*m1 + m2*m2*m2 + m3*m3*m3 - 6*m1*m2*m3*V;
            a = solve_cubic_equation(a3__, a2__, a1__, a0__);
        }else{
            a = m3*V + 0.5*m12;
        }
    }

    return a;

}

type inverse_3d(type m1, type m2, type m3, type V, type dx, type dy, type dz){

    type m1_original = m1,
         m2_original = m2,
         m3_original = m3;

    m1 = fabs(m1), m2 = fabs(m2), m3 = fabs(m3);

    //normalization
    m1 *= dx;
    m2 *= dy;
    m3 *= dz;
    V /= dx*dy*dz;

    type normalize = m1+m2+m3;
    m1 /= normalize;
    m2 /= normalize;
    m3 /= normalize;

    if(!(m1<=m2 && m2<=m3)){
        int i,j;
        type m[3], temp;
        m[0] = m1, m[1] = m2, m[2] =m3;
        for(i=0; i<3; i++){
            for(j=3-1; j>i; j--){
                if(m[j] < m[j-1]){
                    temp = m[j];
                    m[j] = m[j-1];
                    m[j-1] = temp;
                }
            }
        }
        m1 = m[0], m2 = m[1], m3 = m[2];
    }

    type a;

    if(V<0.5){
        a = inverse_3d_core(m1, m2, m3, V);
    }else{
        V = 1.0 - V;
        a = 1.0 - inverse_3d_core(m1, m2, m3, V);
    }

    a *= normalize;

    if(m1_original<0){
        a += m1_original*dx;
    }

    if(m2_original<0){
        a += m2_original*dy;
    }

    if(m3_original<0){
        a += m3_original*dz;
    }

    return a;

}

static void
calc_surface_boundary_plic_inlet(type *fm, type *fp,
                                  int icompo, int ilayer, struct surface_boundary_data *sb,
                                                                    type bnd_norm, int multi_layer_flag, int face_has_orifice)

{
  int ncompo;
  type fa;
  type f;

  f = 0.0;
  fa = 0.0;

  ncompo = inlet_component_data_ncomp(sb->comps);
  for (int ic = 0; ic < ncompo; ++ic) {
    type fi;
    int id;
    struct inlet_component_element *e;
    e = inlet_component_data_get(sb->comps, ic);
    fi = e->ratio.current_value;
    id = e->comp.d->comp_index;
        fa += fi;
        if (icompo == id || (icompo < -1 && id != -1)) {
            f += fi;
    }
  }

  if (fa > 0.0) {
    int to_m, to_p;
    to_m = 0;
    to_p = 0;
    if (bnd_norm > 0.0) {
      to_m = 1;
    } else if (bnd_norm < 0.0) {
      to_p = 1;
    }
    if ((to_m || to_p) && sb->normal_inlet_vel.current_value < 0.0) {
      to_m = !to_m;
      to_p = !to_p;
    }

    f = f / fa;
    if (to_m) {

        if(multi_layer_flag == OFF){

            if (fm) *fm = f;

        }else if(multi_layer_flag == ON){ 

            if (fm && fp){
                if(!face_has_orifice){
                    *fm = 1.0;
                    return;
                }
                *fm = f; 
            }

        }
        
    } else if (to_p) {

        if(multi_layer_flag == OFF){

            if (fp) *fp = f;

        }else if(multi_layer_flag == ON){ 

            if (fm && fp){
                if(!face_has_orifice){
                    *fp = 1.0;
                    return;
                }
                *fp = f;               
            }

        }

    }
  }
}

static void
calc_surface_boundary_plic_f(type *fm, type *fp,
                              int icompo, int ilayer, struct surface_boundary_data *sb,
                                                            type bnd_norm, int multi_layer_flag, int face_has_orifice)
{
  switch (sb->cond) {
  case INLET:
    switch(sb->inlet_dir) {
    case SURFACE_INLET_DIR_NORMAL:
            calc_surface_boundary_plic_inlet(fm, fp, icompo, ilayer, sb, bnd_norm,
                                                                             multi_layer_flag, face_has_orifice);
      break;
    case SURFACE_INLET_DIR_INVALID:
      break;
    }
    break;
  }
}

int plic(type *f, type *flx, type *fly, type *flz, type *nvx, type *nvy, type *nvz, type *alpha, type *u, type *v, type *w, type *lss, type *fs, type *fs_ibm, int icompo, int ilayer, type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, struct surface_boundary_data **surf_bnd_array, parameter *prm, variable *val, int direction_flag)
{

    flags  *flg=prm->flg;
    domain *cdo=prm->cdo;
    mpi_param *mpi=prm->mpi;
    int j, jx, jy, jz,
        nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mxy=cdo->mxy, stm=cdo->stm;
    type dt=cdo->dt, dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
         dti=1.0/dt, dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, ft, eps=1.0e-10;  


// inverse problem
#pragma omp parallel for private(jz, jy, jx, j)
    for(jz=-1; jz<nz+1; jz++){
        for(jy=-1; jy<ny+1; jy++){
            for(jx=-1; jx<nx+1; jx++){
                
                j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);

                if(f[j]>1.0-eps || f[j]<eps){
                    alpha[j] = 0.0;
                }else{
                    type m1 = nvx[j],
                         m2 = nvy[j],
                         m3 = nvz[j],
                         V = clip(f[j])*dx*dy*dz;

                    alpha[j] = inverse_3d(m1, m2, m3, V, dx, dy, dz);
                }
            }

        }
    }


// forward problem
    if(direction_flag==0){
#pragma omp parallel for private(jz, jy, jx, j)
        for(jz=0; jz<nz+1; jz++){
            for(jy=0; jy<ny+1; jy++){
                for(jx=0; jx<nx+1; jx++){
                    
                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);

                    type m1, m2, m3, delta, a, um, fw, fe;
                    int face_has_orifice;

                    um = u[j];
                    fw = clip(f[j-1]);
                    fe = clip(f[j]);

                    face_has_orifice =
                        flg->multi_layer == ON && val->is_orifice_layer &&
                        (val->is_orifice_layer[(j-1) + ilayer * cdo->m] == ON ||
                         val->is_orifice_layer[j + ilayer * cdo->m] == ON);
                    
                    if (surf_bnd_array) {

                        struct surface_boundary_data *spu;
                        type snx;
                        spu = surf_bnd_array[3 * j];
                        snx = bnd_norm_u[3 * j];
                        if (spu) {
                            um = calc_surface_boundary_vel(um, spu, snx);
                            calc_surface_boundary_plic_f(&fw, &fe, icompo, ilayer, spu, snx,
                                                         flg->multi_layer, face_has_orifice);
                        }
                    }

                    if(um>=0.0){
                        if(fw>1.0-eps || fw<eps || fabs(fe-fw)<eps){
                            flx[j] = clip(fw)*um*dt*dy*dz/(dx*dy*dz);
                        }else{
                            m1 = nvx[j-1],
                            m2 = nvy[j-1],
                            m3 = nvz[j-1],
                            delta = fabs(um*dt),
                            a = alpha[j-1]-m1*(dx-delta);
                            flx[j] = forward_3d(m1, m2, m3, a, delta, dy, dz)/(dx*dy*dz);
                        }
                    }else{
                        if(fe>1.0-eps || fe<eps || fabs(fe-fw)<eps){
                            flx[j] = clip(fe)*um*dt*dy*dz/(dx*dy*dz);
                        }else{
                            m1 = nvx[j],
                            m2 = nvy[j],
                            m3 = nvz[j],
                            delta = fabs(um*dt),
                            a = alpha[j];
                            flx[j] = -forward_3d(m1, m2, m3, a, delta, dy, dz)/(dx*dy*dz);
                        }

                    }

                    
                }
            }
        }
    }else if(direction_flag==1){
#pragma omp parallel for private(jz, jy, jx, j)
        for(jz=0; jz<nz+1; jz++){
            for(jy=0; jy<ny+1; jy++){
                for(jx=0; jx<nx+1; jx++){

                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);

                    type m1, m2, m3, delta, a, vm, fs_face, fn;
                    int face_has_orifice;

                    vm = v[j];
                    fs_face = clip(f[j-mx]);
                    fn = clip(f[j]);

                    face_has_orifice =
                        flg->multi_layer == ON && val->is_orifice_layer &&
                        (val->is_orifice_layer[(j-mx) + ilayer * cdo->m] == ON ||
                         val->is_orifice_layer[j + ilayer * cdo->m] == ON);

                    if (surf_bnd_array) {
                        
                        struct surface_boundary_data *spv;
                        type sny;
                        spv = surf_bnd_array[3 * j + 1];
                        sny = bnd_norm_v[3 * j + 1];
                        if (spv) {
                            vm = calc_surface_boundary_vel(vm, spv, sny);
                            calc_surface_boundary_plic_f(&fs_face, &fn, icompo, ilayer, spv, sny,
                                                         flg->multi_layer, face_has_orifice);
                        }
                    }


                    if(vm>=0.0){
                        if(fs_face>1.0-eps || fs_face<eps || fabs(fn-fs_face)<eps){
                            fly[j] = clip(fs_face)*vm*dt*dx*dz/(dx*dy*dz);
                        }else{
                            m1 = nvx[j-mx],
                            m2 = nvy[j-mx],
                            m3 = nvz[j-mx],
                            delta = fabs(vm*dt),
                            a = alpha[j-mx]-m2*(dy-delta);
                            fly[j] = forward_3d(m1, m2, m3, a, dx, delta, dz)/(dx*dy*dz);
                        }
                    }else{
                        if(fn>1.0-eps || fn<eps || fabs(fn-fs_face)<eps){
                            fly[j] = clip(fn)*vm*dt*dx*dz/(dx*dy*dz);
                        }else{
                            m1 = nvx[j],
                            m2 = nvy[j],
                            m3 = nvz[j],
                            delta = fabs(vm*dt),
                            a = alpha[j];
                            fly[j] = -forward_3d(m1, m2, m3, a, dx, delta, dz)/(dx*dy*dz);

                        }
                    }
                }
            }
        }
    }else if(direction_flag==2){
#pragma omp parallel for private(jz, jy, jx, j)
        for(jz=0; jz<nz+1; jz++){
            for(jy=0; jy<ny+1; jy++){
                for(jx=0; jx<nx+1; jx++){
                    
                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);

                    type m1, m2, m3, delta, a, wm, fb, ft;
                    int face_has_orifice;

                    wm = w[j];
                    fb = clip(f[j-mxy]);
                    ft = clip(f[j]);

                    face_has_orifice =
                        flg->multi_layer == ON && val->is_orifice_layer &&
                        (val->is_orifice_layer[(j-mxy) + ilayer * cdo->m] == ON ||
                         val->is_orifice_layer[j + ilayer * cdo->m] == ON);
                    
                    if (surf_bnd_array) {
                        
                        struct surface_boundary_data *spw;
                        type snz;
                        spw = surf_bnd_array[3 * j + 2];
                        snz = bnd_norm_w[3 * j + 2];
                        if (spw) {
                            wm = calc_surface_boundary_vel(wm, spw, snz);
                            calc_surface_boundary_plic_f(&fb, &ft, icompo, ilayer, spw, snz,
                                                         flg->multi_layer, face_has_orifice);
                        }
                    }

                    if(wm>=0.0){
                        if(fb>1.0-eps || fb<eps || fabs(ft-fb)<eps){
                            flz[j] = clip(fb)*wm*dt*dx*dy/(dx*dy*dz);
                        }else{
                            m1 = nvx[j-mxy],
                            m2 = nvy[j-mxy],
                            m3 = nvz[j-mxy],
                            delta = fabs(wm*dt),
                            a = alpha[j-mxy]-m3*(dz-delta);
                            flz[j] = forward_3d(m1, m2, m3, a, dx, dy, delta)/(dx*dy*dz);
                        }
                    }else{
                        if(ft>1.0-eps || ft<eps || fabs(ft-fb)<eps){
                            flz[j] = clip(ft)*wm*dt*dx*dy/(dx*dy*dz);
                        }else{
                            m1 = nvx[j],
                            m2 = nvy[j],
                            m3 = nvz[j],
                            delta = fabs(wm*dt),
                            a = alpha[j];
                            flz[j] = -forward_3d(m1, m2, m3, a, dx, dy, delta)/(dx*dy*dz);
                        }
                    }
                    
                }
            }
        }
    }

    // time integration
        
    type flw, fle, fls, fln, flb, flt;
    type up, um, vp, vm, wp, wm;
    if(direction_flag==0){
#pragma omp parallel for private(j, jz, jy, jx, flw, fle, um, up, ft)
        for(jz=0; jz<nz; jz++){
            for(jy=0; jy<ny; jy++){
                for(jx=0; jx<nx; jx++){

                    type cc;

                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);
                    cc = f[j]>0.5 ? 1: 0;

                    if(fs[j] > 0.5){
                        up = 0.0; um = 0.0;
                    }else{
                        up = u[j+1];   um = u[j];
                    }

                    
                    if (surf_bnd_array) {
                    
                    struct surface_boundary_data *spum, *spup;
                    type snxm, snxp;

                    spum = surf_bnd_array[3 * j];
                    snxm = bnd_norm_u[3 * j];
                    if (spum) {
                        um = calc_surface_boundary_vel(um, spum, snxm);
                    }

                    spup = surf_bnd_array[3 * (j+1)];
                    snxp = bnd_norm_u[3 * (j+1)];
                    if (spup) {
                        up = calc_surface_boundary_vel(up, spup, snxp);
                    }
                    }                    

                    //flw=flx[j];
                    //fle=flx[j+1]; 
                    flw = flx[j] - cc*um*dxi*dt;
                    fle = flx[j+1] - cc*up*dxi*dt; 

                    if(flg->IBM == ON) {
                        //IBM_flux_corr_vof_direction_split(&flw,&fle,j,fs_ibm,cdo,direction_flag);
                    }
                    outflow_flux_direction_split(&flw,&fle,jx,nx,mpi,direction_flag);

                    //ft=-(fle-flw)*dti + f[j]*(up-um)*dxi;
                    ft=-(fle-flw)*dti;

                    f[j]=f[j]+ft*dt;

                }
            }
        }
    }else if(direction_flag==1){
#pragma omp parallel for private(j, jz, jy, jx, fls, fln, vm, vp, ft)
        for(jz=0; jz<nz; jz++){
            for(jy=0; jy<ny; jy++){
                for(jx=0; jx<nx; jx++){

                    type cc;

                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);
                    cc = f[j]>0.5 ? 1: 0;

                    if(fs[j] > 0.5){
                        vp = 0.0; vm = 0.0;
                    }else{
                        vp = v[j+mx];  vm = v[j];
                    }


                    if (surf_bnd_array) {
                    
                    struct surface_boundary_data *spvm, *spvp;
                    type snym, snyp;

                    spvm = surf_bnd_array[3 * j + 1];
                    snym = bnd_norm_v[3 * j + 1];
                    if (spvm) {
                        vm = calc_surface_boundary_vel(vm, spvm, snym);
                    }

                    spvp = surf_bnd_array[3 * (j+mx) + 1];
                    snyp = bnd_norm_v[3 * (j+mx) + 1];
                    if (spvp) {
                        vp = calc_surface_boundary_vel(vp, spvp, snyp);
                    }
                    }



                    //fls=fly[j];
                    //fln=fly[j+mx]; 
                    fls = fly[j] - cc*vm*dyi*dt;
                    fln = fly[j+mx] - cc*vp*dyi*dt; 

                    if(flg->IBM == ON) {
                        //IBM_flux_corr_vof_direction_split(&fls,&fln, j,fs_ibm,cdo,direction_flag);
                    }
                    outflow_flux_direction_split(&fls,&fln,jy,ny, mpi,direction_flag);

                    //ft=-(fln-fls)*dti + f[j]*(vp-vm)*dyi;
                    ft=-(fln-fls)*dti;

                    f[j]=f[j]+ft*dt;

                }
            }
        }
    }else if(direction_flag==2){
#pragma omp parallel for private(j, jz, jy, jx, flb, flt, wm, wp, ft)
        for(jz=0; jz<nz; jz++){
            for(jy=0; jy<ny; jy++){
                for(jx=0; jx<nx; jx++){

                    type cc;

                    j=(jx+stm)+mx*(jy+stm)+mxy*(jz+stm);
                    cc = f[j]>0.5 ? 1: 0;

                    if(fs[j] > 0.5){
                        wp = 0.0; wm = 0.0;
                    }else{
                        wp = w[j+mxy]; wm = w[j];
                    }


                    if (surf_bnd_array) {
                    
                    struct surface_boundary_data *spwm, *spwp;
                    type snzm, snzp;

                    spwm = surf_bnd_array[3 * j + 2];
                    snzm = bnd_norm_w[3 * j + 2];
                    if (spwm) {
                        wm = calc_surface_boundary_vel(wm, spwm, snzm);
                    }

                    spwp = surf_bnd_array[3 * (j+mxy) + 2];
                    snzp = bnd_norm_w[3 * (j+mxy) + 2];
                    if (spwp) {
                        wp = calc_surface_boundary_vel(wp, spwp, snzp);
                    }
                    }

                    //flb=flz[j];
                    //flt=flz[j+mxy]; 
                    flb = flz[j] - cc*wm*dzi*dt;
                    flt = flz[j+mxy] - cc*wp*dzi*dt; 

                    if(flg->IBM == ON) {
                        //IBM_flux_corr_vof_direction_split(&flb,&flt, j,fs_ibm,cdo,direction_flag);
                    }
                    outflow_flux_direction_split(&flb,&flt,jz,nz, mpi,direction_flag);

                    //ft=-(flt-flb)*dti + f[j]*(wp-wm)*dzi;
                    ft=-(flt-flb)*dti;

                    f[j]=f[j]+ft*dt;

                }
            }
        } 
    }

    return 0;

}
