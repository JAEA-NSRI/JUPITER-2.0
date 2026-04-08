! *************
#ifdef LPT
      subroutine LPTcal0(xfgrid,yfgrid,zfgrid,time,
     &                   nx,ny,nz)
!     Initial Calculation of Lagrangian Particle Tracking method by one-way
!      coupling for evaluating behavior of fine particles
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.0 Created by H. Yoshida 13.Mar.2018
!        1. 1st release
!
!      arguments
!        istep : current step no. for time evaluation
!        xfgrid : Eulerian grid position at cell edge in x-direction [m]
!        yfgrid : Eulerian grid position at cell edge in y-direction [m]
!        zfgrid : Eulerian grid position at cell edge in z-direction [m]
!        time : current time [s]
!        npt  : number of particles
!        nx   : number of Eulerian grid points in x-direction
!        ny   : number of Eulerian grid points in y-direction
!        nz   : number of Eulerian grid points in z-direction
!
! *************
!
      use LPTval,       only: DeallocLPT
      use LPTval,       only: TipLPT, Timprn,
     &                        pCoefa, pCoefb, pCoefg,
     &                        psetXs, psetXe, psetYs, psetYe,
     &                        psetZs, psetZe, psetTms, psetTme,
     &                        psetDi, psetRi,
     &                        psetUx, psetUy, psetUz,
     &                        pXwbcm,pXwbcp,pYwbcm,pYwbcp,pZwbcm,pZwbcp,
     &                        pXpt, pYpt, pZpt, pUxpt, pUypt, pUzpt,
     &                        pTimpt, pRhopt, pDiapt
      use LPTval,       only: ipttim, npt, npset, nistpt, LPTwbcal,
     &                        icfpt, jcfpt, kcfpt, iunLPT, itrdm, npt0
#ifdef mpi
      use MPI, only: MPI_REAL8, MPI_COMM_WORLD
      use MT_PARALLEL_ATTRIBUTE0, only: myrank
#ifdef LPT_mpi_dbg
      use LPTval, only: nptlvc, iptlvc
#endif
#endif
!
      implicit none
!
      integer(kind=4) :: nx,ny,nz,is,idxpt
      real(kind=8) :: time
      real(kind=8) :: xfgrid(-1:nx+1),yfgrid(-1:ny+1),zfgrid(-1:nz+1)
#ifdef mpi
      integer(kind=4) :: myroot,ierr
#endif
#ifdef LPT_mpi_dbg
      integer(kind=4) :: ipp, ip
      real(kind=8), save :: pMrank(100)
      pMrank(:) = 0.0d0
#endif
!
! --------------------------------------------------------------------
!
!  initial set
!
!    set coefficient of time integration
!
         if ( ipttim==1 )  then ! second-order Adams-Bashform method
            pCoefa(1) =  1.0d0
            pCoefb(1) =  3.0d0/2.0d0
            pCoefg(1) = -1.0d0/2.0d0
            pCoefa(2) =  1.0d0/2.0d0
            pCoefa(3) =  1.0d0/2.0d0
            pCoefb(2) =  1.0d0
            pCoefb(3) =  2.0d0
            pCoefg(2) =  0.0d0
            pCoefg(3) = -1.0d0
         elseif ( ipttim==2 )  then ! second-order Runge-Kutta method
            pCoefa(1) =  1.0d0/2.0d0
            pCoefa(2) =  1.0d0/2.0d0
            pCoefb(1) =  1.0d0
            pCoefb(2) =  2.0d0
            pCoefg(1) =  0.0d0
            pCoefg(2) = -1.0d0
         elseif ( ipttim==3 )  then ! third-order low-storage Runge-Kutta method
            pCoefa(1) =  8.0d0/15.0d0
            pCoefa(2) =  2.0d0/15.0d0
            pCoefa(3) =  5.0d0/15.0d0
            pCoefb(1) =  1.0d0
            pCoefb(2) =  75.0d0/24.0d0
            pCoefb(3) =  9.0d0/4.0d0
            pCoefg(1) =  0.0d0
            pCoefg(2) = -17.0d0/8.0d0
            pCoefg(3) = -5.0d0/4.0d0
         endif
!
!    set grid point at boundary
!
         pXwbcm=xfgrid(0) ; pXwbcp=xfgrid(nx)
         pYwbcm=yfgrid(0) ; pYwbcp=yfgrid(ny)
         pZwbcm=zfgrid(0) ; pZwbcp=zfgrid(nz)
!
!    set initial particle conditions
!
#ifdef mpi
         myroot = 0
         if ( myrank==myroot ) then
#endif
         call LPTint0
!
         idxpt = 1
         do is = 1,npset
            call LPTint( psetXs(is),psetXe(is),psetYs(is),psetYe(is),
     &                   psetZs(is),psetZe(is),
     &                   psetTms(is),psetTme(is),psetDi(is),psetRi(is),
     &                   psetUx(is),psetUy(is),psetUz(is),
     &                   pXpt,pYpt,pZpt,pTimpt,pDiapt,pRhopt,
     &                   pUxpt,pUypt,pUzpt,
     &                   nistpt(is),idxpt,npt,npt0,itrdm(is) )
            idxpt = idxpt+nistpt(is)
         enddo
#ifdef mpi
         endif
         call MPI_Bcast(pXpt,  npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pYpt,  npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pZpt,  npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pTimpt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pDiapt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pRhopt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pUxpt, npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pUypt, npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pUzpt, npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
#endif
!
!    set cell number on Eulerian grid at particle position
!
         call LPTcst( pXpt,pYpt,pZpt,pTimpt,xfgrid,yfgrid,zfgrid,time,
     &                icfpt,jcfpt,kcfpt,npt,nx,ny,nz,0 )
!
!    print particle positions and velocities
!
#ifdef mpi
#ifdef LPT_mpi_dbg
         do ipp = 1,nptlvc(myrank)
            ip = iptlvc(ipp)
            pMrank(ip) = dble(myrank)
         enddo
         call LPTgat(pMrank,npt)
#endif
         myroot = 0
         if ( myrank==myroot ) then
#endif
         if ( TipLPT > 0.0 ) then
         call LPTprt(iunLPT,time,pXpt,pYpt,pZpt,pUxpt,pUypt,pUzpt,
#ifndef LPT_mpi_dbg
     &               pDiapt,pTimpt,npt)
#else
     &               pDiapt,pTimpt,pMrank,npt)
#endif
         endif
#ifdef mpi
         endif
#endif
         Timprn = Timprn + TipLPT
!
        if ( LPTwbcal==1 ) call LPTnwall(nx,ny,nz)
!
      end subroutine LPTcal0
!! *************
      subroutine LPTcal(istep,vxf1,vyf1,vzf1,vxf2,vyf2,vzf2,rhof1,rhof2,
     &                  vmuf1,vmuf2,xfgrid,yfgrid,zfgrid,gx,gy,gz,dt,
     &                  time,nx,ny,nz)
!     Calculation of Lagrangian Particle Tracking method by one-way
!      coupling for evaluating entrainment behavior of fine particles
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.1 Modified by H. Yoshida 16.Oct.2018
!        1. Introducing Sub time step for LPT (alpha version)
!     Ver.1.02 Modified by H. Yoshida 13.Mar.2018
!        1. Separate initial and final stage of calculation
!        2. Consider relative position of particle (surrounding fluid properties)
!     Ver.1.01 Modified by H. Yoshida 03.Mar.2018
!        1. Fit to modification of LPTcst Ver.1.01
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        istep : current step no. for time evaluation
!        vxf  : fluid velocity in x-direction at Eulerian staggered
!                                                    grid point[m/s]
!        vyf  : fluid velocity in y-direction at Eulerian staggered
!                                                   grid point [m/s]
!        vzf  : fluid velocity in z-direction at Eulerian staggered
!                                                   grid point [m/s]
!        rhof1: fluid 1 density at Eulerian grid point [kg/m3]
!        rhof2: fluid 2 density at Eulerian grid point [kg/m3]
!        vmuf1: fluid 1 static viscosity at Eulerian grid point [Pas]
!        vmuf2: fluid 2 static viscosity at Eulerian grid point [Pas]
!        xfgrid : Eulerian grid position at cell edge in x-direction [m]
!        yfgrid : Eulerian grid position at cell edge in y-direction [m]
!        zfgrid : Eulerian grid position at cell edge in z-direction [m]
!        gx   : gravity acceleration in x-direction [m/s2]
!        gy   : gravity acceleration in y-direction [m/s2]
!        gz   : gravity acceleration in z-direction [m/s2]
!        dt   : time step increment [s]
!        time : current time [s]
!        npt  : number of particles
!        nx  : number of Eulerian grid points in x-direction
!        ny  : number of Eulerian grid points in y-direction
!        nz  : number of Eulerian grid points in z-direction
!
! *************
!
      use LPTval,       only: AllocLPT, DeallocLPT
      use LPTval,       only: TipLPT, Timprn,
     &                        pCoefa, pCoefb, pCoefg,
     &                        pUxf, pUyf, pUzf,
     &                        pXpt, pYpt, pZpt, pUxpt, pUypt, pUzpt,
     &                        pTimpt, pRhopt, pDiapt,
     &                        pFUxpt, pFUypt, pFUzpt,
     &                        pFdUxt, pFdUyt, pFdUzt,
     &                        pEwall,dLpin
      use LPTval,       only: ipttim, npt,
     &                        icfpt, jcfpt, kcfpt, iunLPT,iFPsrd
      use LPTval,       only: fFxpt,fFypt,fFzpt
      use numcon,       only: pi
#ifdef mpi
#if defined(LPT_USE_MPI_F08_INTERFACE)
      use MPI_F08, only: MPI_COMM_WORLD,MPI_REAL8,MPI_MAX
      use MPI_F08, only: MPI_Allreduce
#else
      use MPI, only: MPI_COMM_WORLD,MPI_REAL8,MPI_MAX
#if defined(LPT_USE_MPI_INTERFACE)
      use MPI, only: MPI_Allreduce
#endif
#endif
      use MT_Parallel_Attribute0, only: myrank, lpt_comm
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
      save
!
      integer(kind=4) :: istep,nx,ny,nz,
     &                   iptst,ipten,iptirk,ip,iptstp
!
      real(kind=8) :: pCdfnc, gx, gy, gz, dt, time,
     &                plUfUp,pRhof,pVmuf,pRepf,pCdpt,pTaup,
     &                pdUxdt,pdUydt,pdUzdt
      real(kind=8) :: pmass

      real(kind=8) :: vxf1(-1:nx+1, 0:ny+1, 0:nz+1),
     &                vyf1( 0:nx+1,-1:ny+1, 0:nz+1),
     &                vzf1( 0:nx+1, 0:ny+1,-1:nz+1),
     &                vxf2(-1:nx+1, 0:ny+1, 0:nz+1),
     &                vyf2( 0:nx+1,-1:ny+1, 0:nz+1),
     &                vzf2( 0:nx+1, 0:ny+1,-1:nz+1),
     &                rhof1(0:nx+1,0:ny+1,0:nz+1),
     &                rhof2(0:nx+1,0:ny+1,0:nz+1),
     &                vmuf1(0:nx+1,0:ny+1,0:nz+1),
     &                vmuf2(0:nx+1,0:ny+1,0:nz+1),
     &                xfgrid(-1:nx+1),yfgrid(-1:ny+1),zfgrid(-1:nz+1)
      real(kind=8) :: yfsp(1:npt),rhof(1:npt),vmuf(1:npt)
!
!     for sub time step
      real(kind=8) :: dtLPT,pTaup_max,pTaup_mxlim
      integer(kind=4) :: n_max_dt,n_dt,ncsdt
#ifdef mpi
      integer(kind=4) :: ipp, myroot, ierr
      real(kind=8) :: pTaup_maxt
#ifdef LPT_DEFINE_MPI_INTERFACE
!
!     Explicitly define interface of MPI_Allreduce
!
      interface
         subroutine MPI_Allreduce(sendbuf,recvbuf,count,datatype,op,
     &        comm,ierror)
           type(*) :: sendbuf, recvbuf
           integer :: count, datatype, op, comm, ierror
         end subroutine MPI_Allreduce
      end interface
#endif
#endif
#ifdef LPT_mpi_dbg
      real(kind=8) :: pMrank(100)
      pMrank(:) = 0.0d0
#endif
!
! --------------------------------------------------------------------
!
!  sub cycle for time integration
!     ipttim =1 : second-order Adams-Bashform method
!            =2 : second-order Runge-Kutta method
!            =3 : third-order low-storage Runge-Kutta method
!
      iptst = 1
!
      ipten = ipttim
!
      if ( ipttim==1 .and. istep==1 )  then
!
        iptst = 2
        ipten = 3
!
        n_dt = 1
        dtLPT = dt
!
      endif
!
      pTaup_max = 0.0d0
!
      n_max_dt = 100
!
      n_dt = min0(max0(n_dt,1),n_max_dt)
!
      dtLPT = dt/dble(n_dt) ! evaluate sub time step
      pTaup_mxlim = 1.0d0/dtLPT
!
#ifndef mpi
      do ip=1,npt
#else
      do ipp = 1,nptlvc(myrank)
        ip = iptlvc(ipp)
#endif
!
        fFxpt(ip) = 0.0d0
        fFypt(ip) = 0.0d0
        fFzpt(ip) = 0.0d0
!
      enddo
!
      do ncsdt = 1,n_dt
!
        do iptirk = iptst,ipten
!
          iptstp = iptirk-iptst+1
!
!         evaluate relative position of particles for interface
!
          call LPTdpi( dLpin,iFPsrd,yfsp,pXpt,pYpt,pZpt,
     &                 xfgrid,yfgrid,zfgrid,pTimpt,pDiapt,
     &                 time,icfpt,jcfpt,kcfpt,npt,nx,ny,nz)
!
!         evaluate fluid properties around particle
!
#ifndef mpi
          do ip=1,npt
#else
          do ipp = 1,nptlvc(myrank)
            ip = iptlvc(ipp)
#endif
!
            rhof(ip) = rhof1(icfpt(ip),jcfpt(ip),kcfpt(ip))*yfsp(ip)
     &               + rhof2(icfpt(ip),jcfpt(ip),kcfpt(ip))
     &                 *(1.0d0-yfsp(ip))
            vmuf(ip) = vmuf1(icfpt(ip),jcfpt(ip),kcfpt(ip))*yfsp(ip)
     &               + vmuf2(icfpt(ip),jcfpt(ip),kcfpt(ip))
     &                 *(1.0d0-yfsp(ip))
!
          enddo
!
!         set fluid velocities at particle position
!         using Lagrangian interpolation
!
          call LPTvfs( iptstp,pUxf,pUyf,pUzf,vxf1,vyf1,vzf1,
     &                 vxf2,vyf2,vzf2,pXpt,pYpt,pZpt,pTimpt,
     &                 xfgrid,yfgrid,zfgrid,time,
     &                 icfpt,jcfpt,kcfpt,npt,nx,ny,nz )
!
#ifndef mpi
          do ip = 1,npt
#else
          do ipp = 1,nptlvc(myrank)
            ip = iptlvc(ipp)
#endif
!
            if ( time>=pTimpt(ip) )  then ! active particle
!
!             calculate particle positions in x-, y- and z-direction
!
              pXpt(ip) = pXpt(ip) + pCoefa(iptirk)*dtLPT*(
     &                              pCoefb(iptirk)*pUxpt(ip) +
     &                              pCoefg(iptirk)*pFUxpt(ip) )
              pYpt(ip) = pYpt(ip) + pCoefa(iptirk)*dtLPT*(
     &                              pCoefb(iptirk)*pUypt(ip) +
     &                              pCoefg(iptirk)*pFUypt(ip) )
              pZpt(ip) = pZpt(ip) + pCoefa(iptirk)*dtLPT*(
     &                              pCoefb(iptirk)*pUzpt(ip) +
     &                              pCoefg(iptirk)*pFUzpt(ip) )
!
              pFUxpt(ip) = pUxpt(ip)
              pFUypt(ip) = pUypt(ip)
              pFUzpt(ip) = pUzpt(ip)
!
!             calculate particle velocities in x-, y- and z-direction
!
              plUfUp = dsqrt( (pUxf(ip)-pUxpt(ip))**2
     &                       +(pUyf(ip)-pUypt(ip))**2
     &                       +(pUzf(ip)-pUzpt(ip))**2 )
              pRhof = rhof(ip)
              pVmuf = vmuf(ip)
              pRepf = pRhof*plUfUp*pDiapt(ip)/pVmuf
              pCdpt = pCdfnc( pRepf )
              if ( pRhopt(ip)*pDiapt(ip)**2>0.0d0 ) then
                pTaup = (3.0d0*pCdpt*pVmuf*pRepf) / ! inverse of relaxation time
     &                  (4.0d0*pRhopt(ip)*pDiapt(ip)**2)
              else
                pTaup = 0.0d0
              endif
              pTaup_max = dmax1(pTaup_max,pTaup)
!
!             dt*pTaup must be less than 1 to keep same sigh of velocity difference.
!             then, maximum value of pTaup equals to 1/dt
!
              pTaup = dmin1(pTaup,pTaup_mxlim)
!
              pdUxdt = ( (pUxf(ip)-pUxpt(ip))*pTaup )
     &               + (pRhopt(ip)-pRhof)/pRhopt(ip)*gx ! add buoyancy
              pdUydt = ( (pUyf(ip)-pUypt(ip))*pTaup )
     &               + (pRhopt(ip)-pRhof)/pRhopt(ip)*gy ! add buoyancy
              pdUzdt = ( (pUzf(ip)-pUzpt(ip))*pTaup )
     &               + (pRhopt(ip)-pRhof)/pRhopt(ip)*gz ! add buoyancy
!
              pUxpt(ip) = pUxpt(ip) + pCoefa(iptirk)*dtLPT*(
     &                                pCoefb(iptirk)*pdUxdt +
     &                                pCoefg(iptirk)*pFdUxt(ip) )
              pUypt(ip) = pUypt(ip) + pCoefa(iptirk)*dtLPT*(
     &                                pCoefb(iptirk)*pdUydt +
     &                                pCoefg(iptirk)*pFdUyt(ip) )
              pUzpt(ip) = pUzpt(ip) + pCoefa(iptirk)*dtLPT*(
     &                                pCoefb(iptirk)*pdUzdt +
     &                                pCoefg(iptirk)*pFdUzt(ip) )
!
              pmass = pRhopt(ip)*pi*pDiapt(ip)**3/6.0d0
              fFxpt(ip) =fFxpt(ip)-pmass*(pdUxdt-gx)/dble(n_dt)
              fFypt(ip) =fFypt(ip)-pmass*(pdUydt-gy)/dble(n_dt)
              fFzpt(ip) =fFzpt(ip)-pmass*(pdUzdt-gz)/dble(n_dt)
!
              pFdUxt(ip) = pdUxdt
              pFdUyt(ip) = pdUydt
              pFdUzt(ip) = pdUzdt
!
            endif
!
#ifdef LPT_mpi_dbg
            pMrank(ip) = dble(myrank)
#endif
!!!
          enddo
#ifdef mpi
!
!         gather valiable datas from all ranks
!
          call LPTgat(pXpt,npt)
          call LPTgat(pYpt,npt)
          call LPTgat(pZpt,npt)
          call LPTgat(pUxpt,npt)
          call LPTgat(pUypt,npt)
          call LPTgat(pUzpt,npt)
          call LPTgat(pTimpt,npt)
          call LPTgat(pFUxpt,npt)
          call LPTgat(pFUypt,npt)
          call LPTgat(pFUzpt,npt)
          call LPTgat(pFdUxt,npt)
          call LPTgat(pFdUyt,npt)
          call LPTgat(pFdUzt,npt)
#ifdef LPT_mpi_dbg
          call LPTgat(pMrank,npt)
#endif
#endif
!
!         set cell number on Eulerian grid at particle position
!
          call LPTcst( pXpt,pYpt,pZpt,pTimpt,xfgrid,yfgrid,zfgrid,
     &                 time,icfpt,jcfpt,kcfpt,npt,nx,ny,nz,1 )
!
        enddo
!
      enddo
!
#ifdef mpi
      pTaup_maxt = pTaup_max
      call MPI_Allreduce(pTaup_maxt,pTaup_max,1,MPI_REAL8,MPI_MAX,
     &                   lpt_comm,ierr)
#endif
      n_dt = int(dt*pTaup_max)
!
!    set wall boundary condition
!
      call LPTwbc( pUxpt,pUypt,pUzpt,pXpt,pYpt,pZpt,xfgrid,yfgrid,
     &             zfgrid,pTimpt,pDiapt, pEwall,icfpt,jcfpt,kcfpt,
     &             npt,nx,ny,nz,time )
#ifdef mpi
!
!     gather valiable datas from all ranks
!
      call LPTgat(pXpt,npt)
      call LPTgat(pYpt,npt)
      call LPTgat(pZpt,npt)
      call LPTgat(pUxpt,npt)
      call LPTgat(pUypt,npt)
      call LPTgat(pUzpt,npt)
      call LPTgat(pTimpt,npt)
      call LPTgat(pFUxpt,npt)
      call LPTgat(pFUypt,npt)
      call LPTgat(pFUzpt,npt)
      call LPTgat(pFdUxt,npt)
      call LPTgat(pFdUyt,npt)
      call LPTgat(pFdUzt,npt)
#ifdef LPT_mpi_dbg
      call LPTgat(pMrank,npt)
#endif
#endif
!
!    set cell number on Eulerian grid at particle position
!
      call LPTcst( pXpt,pYpt,pZpt,pTimpt,xfgrid,yfgrid,zfgrid,time,
     &             icfpt,jcfpt,kcfpt,npt,nx,ny,nz,1 )
!
!    print particle positions and velocities
!
      if ( time>=Timprn .and. TipLPT > 0.0d0 )  then
#ifdef mpi
      myroot = 0
      if ( myrank==myroot ) then
#endif
         call LPTprt(iunLPT,time,pXpt,pYpt,pZpt,pUxpt,pUypt,pUzpt,
#ifndef LPT_mpi_dbg
     &               pDiapt,pTimpt,npt)
#else
     &               pDiapt,pTimpt,pMrank,npt)
#endif
#ifdef mpi
      endif
#endif
         Timprn = Timprn + TipLPT
      endif
!
! --------------------------------------------------------------------
!
      end subroutine LPTcal
!
! *************
      real(8) function pCdfnc(pRept)
!     Calculation of drag coefficient for particle
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        pRept  : Reynolds number for particle
!
! *************
!
      implicit none
!
      real(kind=8), intent(in) :: pRept
!
! --------------------------------------------------------------------
!
!  drag coefficient for particle
!
!     Cd = (24/Re)(1+0.15*Re^0.687) by Sciller-Namann
!
      pCdfnc = 0.0d0
      if ( pRept>0.0d0 )
     &     pCdfnc = ( 24.0d0/pRept )*( 1.0d0 + (0.15d0*pRept**0.687) )
!
      end function pCdfnc
!
! *************
      subroutine LPTcst(pXpt,pYpt,pZpt,pTimpt,xfgrid,yfgrid,zfgrid,time,
     &                  icfpt,jcfpt,kcfpt,npt,nx,ny,nz,inif)
!     Set of cell number on Eulerian grid at particle position
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.01 Modified by H. Yoshida 03.Mar.2018
!        1. Add initialization
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pTimpt : active time for particle [s]
!        xfgrid : Eulerian grid position in x-direction [m]
!        yfgrid : Eulerian grid position in y-direction [m]
!        zfgrid : Eulerian grid position in z-direction [m]
!        time   : current time [s]
!        icfpt  : fluid cell number in x-direction at particle position
!        jcfpt  : fluid cell number in y-direction at particle position
!        kcfpt  : fluid cell number in z-direction at particle position
!        npt    : number of particles
!        nx    : number of Eulerian grid points in x-direction
!        ny    : number of Eulerian grid points in y-direction
!        nz    : number of Eulerian grid points in z-direction
!        inif   : initialization flag
!
! *************
!
      use LPTval, only: pXwbcm,pXwbcp,pYwbcm,pYwbcp,pZwbcm,pZwbcp
#ifdef mpi
      use MT_Parallel_Attribute0, only: myrank,numrank,nrk
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
!
      integer(kind=4) :: npt,nx,ny,nz,
     &                   ip,ic,jc,kc
      integer(kind=4) :: icfpt(npt),jcfpt(npt),kcfpt(npt)
      integer(kind=4), intent(in) :: inif
!
      real(kind=8) :: pXpt(npt),pYpt(npt),pZpt(npt),pTimpt(npt),
     &                xfgrid(-1:nx+1),yfgrid(-1:ny+1),
     &                zfgrid(-1:nz+1),time
#ifdef mpi
      integer(kind=4) :: ipfind, ipp
      logical :: lbnd(6)
      nptlvc(:) = 0
      iptlvc(:) = 0
!
! --------------------------------------------------------------------
!   make list-vector of target partices on myrank
!
      do ip = 1,npt
         ipfind = 0
#ifdef mpi_dbg
         write(60+myrank,*) 'ip=',ip,',pZpt=',pZpt(ip)
#endif
!
         lbnd = nrk .eq. -1
         if (.not. lbnd(1)) lbnd(1) = pZpt(ip) .ge. pZwbcm
         if (.not. lbnd(2)) lbnd(2) = pZpt(ip) .lt. pZwbcp
         if (.not. lbnd(3)) lbnd(3) = pYpt(ip) .ge. pYwbcm
         if (.not. lbnd(4)) lbnd(4) = pYpt(ip) .lt. pYwbcp
         if (.not. lbnd(5)) lbnd(5) = pXpt(ip) .ge. pXwbcm
         if (.not. lbnd(6)) lbnd(6) = pXpt(ip) .lt. pXwbcp
         if ( all(lbnd) ) then
            ipfind = ip
         endif
         if ( ipfind/=0 )  then
            nptlvc(myrank) = nptlvc(myrank) + 1
            iptlvc(nptlvc(myrank)) = ipfind
#ifdef mpi_dbg
            write(60+myrank,*)
     &           '  myrank=',myrank,',nptlvc=',nptlvc(myrank),
     &                 ',iptlvc=',iptlvc(nptlvc(myrank))
#endif
         endif
      enddo
#endif
!
! --------------------------------------------------------------------
!
!  search of fluid cell number at particle position
!
#ifndef mpi
      do ip = 1,npt
#else
      icfpt = 0
      jcfpt = 0
      kcfpt = 0
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
!
         if ( time>=pTimpt(ip) .or. inif==0 )  then ! active particle
!
!   x-direction
!
            if ( pXpt(ip)<=pXwbcm )  then
               icfpt(ip) = 0
!               write(6,*) "error in particle position x-",ip,pXpt(ip)
!               write(7,*) "error in particle position x-",ip,pXpt(ip)
            elseif ( pXpt(ip)>=pXwbcp )  then
               icfpt(ip) = nx+1
!               write(6,*) "error in particle position x+",ip,pXpt(ip)
!               write(7,*) "error in particle position x+",ip,pXpt(ip)
            else
               do ic = 1,nx
                  if ( xfgrid(ic-1)<=pXpt(ip) .and.
     &                 xfgrid(ic)>=pXpt(ip) )  then
                     icfpt(ip) = ic
                     exit
                  endif
               enddo
            endif
!
!   y-direction
!
            if ( pYpt(ip)<=pYwbcm )  then
               jcfpt(ip) = 0
!               write(6,*) "error in particle position y-",ip,pYpt(ip)
!               write(7,*) "error in particle position y-",ip,pYpt(ip)
            elseif ( pYpt(ip)>=pYwbcp )  then
               jcfpt(ip) = ny+1
!               write(6,*) "error in particle position y+",ip,pYpt(ip)
!               write(7,*) "error in particle position y+",ip,pYpt(ip)
            else
               do jc = 1,ny
                  if ( yfgrid(jc-1)<=pYpt(ip) .and.
     &                 yfgrid(jc)>=pYpt(ip) )  then
                     jcfpt(ip) = jc
                     exit
                  endif
               enddo
            endif
!
!   z-direction
!
            if ( pZpt(ip)<=pZwbcm )  then
               kcfpt(ip) = 0
!               write(6,*) "error in particle position z-",ip,pZpt(ip)
!               write(7,*) "error in particle position z-",ip,pZpt(ip)
            elseif ( pZpt(ip)>=pZwbcp )  then
               kcfpt(ip) = nz+1
!               write(6,*) "error in particle position z+",ip,pZpt(ip)
!               write(7,*) "error in particle position z+",ip,pZpt(ip)
            else
               do kc = 1,nz
                  if ( zfgrid(kc-1)<=pZpt(ip) .and.
     &                 zfgrid(kc)>=pZpt(ip) )  then
                     kcfpt(ip) = kc
                     exit
                  endif
               enddo
            endif
         endif
!
      enddo
!
      end subroutine LPTcst
!
! *************
      subroutine LPTdpi(dLpin,iFPsrd,yfsp,pXpt,pYpt,pZpt,
     &                  xfgrid,yfgrid,zfgrid,pTimpt,pDiapt,time,
     &                  icfpt,jcfpt,kcfpt,npt,nx,ny,nz)
!     evaluate relative position of particles for interface
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.0  Created by H. Yoshida (JAEA) 13.Mar.2018
!        1. 1st release
!
!      arguments
!        iptstp : step no. of time integration sub cycle
!        dLpin  : distance between particle and interface
!        iFPsrd : flag for surrounding fluid around particle
!                 (=1 liquid, =-1 gas, =0 on interface)
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pTimpt : active time for particle [s]
!        pDiapt : particle diameter [m]
!        time   : current time [s]
!        icfpt  : fluid cell number in x-direction at particle position
!        jcfpt  : fluid cell number in y-direction at particle position
!        kcfpt  : fluid cell number in z-direction at particle position
!        npt    : number of particles
!
! *************
!
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
      use cmval,        only: yfn
      use plic1,        only: plm1,plal1
      use plic4,        only: ndimpl1
#endif
#ifdef mpi
      use MT_Parallel_Attribute0, only: myrank
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
      integer(kind=4) :: npt,ip,nx,ny,nz
      integer(kind=4) :: icfpt(npt),jcfpt(npt),kcfpt(npt),iFPsrd(npt)
      integer(kind=4) :: kcfp,jcfp,icfp
!
      real(kind=8) :: dLpin(npt),pDiapt(npt),yfsp(1:npt),
     &                pXpt(npt),pYpt(npt),pZpt(npt),pTimpt(npt),
     &                xfgrid(-1:nx+1),yfgrid(-1:ny+1),
     &                zfgrid(-1:nz+1),time
#ifdef mpi
      integer(kind=4) :: ipp
#endif
!
! --------------------------------------------------------------------
!
      iFPsrd(:) = 0
      dLpin(:) = 1.0d30
!
!     evaluate distance between particle and interface
!
#ifndef mpi
      do ip = 1,npt
#else
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
!
        if ( time>=pTimpt(ip) )  then ! active particle
!
          kcfp = kcfpt(ip)
          jcfp = jcfpt(ip)
          icfp = icfpt(ip)
!
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
          if ( ndimpl1(icfp,jcfp,kcfp)/=0 ) then
!
            dLpin(ip) =plm1(icfp,jcfp,kcfp,1)*(pXpt(ip)-xfgrid(icfp-1))
     &                +plm1(icfp,jcfp,kcfp,2)*(pYpt(ip)-yfgrid(jcfp-1))
     &                +plm1(icfp,jcfp,kcfp,3)*(pZpt(ip)-zfgrid(kcfp-1))
     &                -plal1(icfp,jcfp,kcfp)
!
            if ( dabs(dLpin(ip))<=pDiapt(ip)*0.5d0 ) then
!
              iFPsrd(ip) = 0
              yfsp(ip) = ( 1.0d0-dLpin(ip)/(pDiapt(ip)*0.5d00) )*0.5d0
!
            elseif ( dLpin(ip)> 0.0d0 ) then
!
              iFPsrd(ip) = -1
              yfsp(ip) = 0.0d0
!
            else
!
              iFPsrd(ip) = 1
              yfsp(ip) = 1.0d0
!
            endif
!
          else
!
            dlpin(ip) = 1.0d30

!
            if ( yfn(icfp,jcfp,kcfp)==1.0d0 ) then
!
              iFPsrd(ip) = 1
              yfsp(ip) = 1.0d0
!
            else
!
              iFPsrd(ip) = -1
              yfsp(ip) = 0.0d0
!
            endif
!
          endif
#else
!! TODO: Please re-implement for JUPITER
          iFPsrd(ip) = -1
          yfsp(ip) = 0.0d0
#endif
!
        endif
!
      enddo
!
      end subroutine LPTdpi
!
! *************
      subroutine LPTvfs(iptstp,pUxf,pUyf,pUzf,vxf1,vyf1,vzf1,
     &                  vxf2,vyf2,vzf2,pXpt,pYpt,pZpt,pTimpt,
     &                  xfgrid,yfgrid,zfgrid,time,
     &                  icfpt,jcfpt,kcfpt,npt,nx,ny,nz)
!     Set of fluid velocities at particle position
!                                         using Lagrangian interpolation
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.01 Modified by H. Yoshida (JAEA) 03.Mar.2018
!        1. Add error correction
!        2. Consider non-fluid cell
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        iptstp : step no. of time integration sub cycle
!        pUxf   : fluid velocity in x-direction at particle position [m/s]
!        pUyf   : fluid velocity in y-direction at particle position [m/s]
!        pUzf   : fluid velocity in z-direction at particle position [m/s]
!        vxf    : fluid velocity in x-direction
!                                    at Eulerian staggered grid point [m/s]
!        vyf    : fluid velocity in y-direction
!                                    at Eulerian staggered grid point [m/s]
!        vzf    : fluid velocity in z-direction
!                                    at Eulerian staggered grid point [m/s]
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pTimpt : active time for particle [s]
!        xfgrid : Eulerian grid position in x-direction [m]
!        yfgrid : Eulerian grid position in y-direction [m]
!        zfgrid : Eulerian grid position in z-direction [m]
!        time   : current time [s]
!        icfpt  : fluid cell number in x-direction at particle position
!        jcfpt  : fluid cell number in y-direction at particle position
!        kcfpt  : fluid cell number in z-direction at particle position
!        npt    : number of particles
!        nx    : number of Eulerian grid points in x-direction
!        ny    : number of Eulerian grid points in y-direction
!        nz    : number of Eulerian grid points in z-direction
!
! *************
!
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
      use cmcs0,        only: icflg,iwe,iww,iwn,iws,iwt,iwb
      use jval,         only: ciw
#endif
      use LPTval,       only: vxfcor, vyfcor, vzfcor,iFPsrd,iWPsrd
#ifdef mpi
      use MT_Parallel_Attribute0, only: myrank,numrank
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
      integer(kind=4) :: iptstp,npt,nx,ny,nz,
     &                   ip,ic,jc,kc,ifp
      integer(kind=4) :: icfpt(npt),jcfpt(npt),kcfpt(npt)
      integer(kind=4) :: kcfm,kcfp,jcfm,jcfp,icfm,icfp
      integer(kind=4) :: im,il,jm,jl,km,kl
!
      real(kind=8) :: flxm,flxp,flym,flyp,flzm,flzp,
     &                pLzm,pLzp,pLym,pLyp,pLxm,pLxp
      real(kind=8) :: pUxf(npt),pUyf(npt),pUzf(npt),
     &                pXpt(npt),pYpt(npt),pZpt(npt),pTimpt(npt),
     &                vxf1(-1:nx+1, 0:ny+1, 0:nz+1),
     &                vyf1( 0:nx+1,-1:ny+1, 0:nz+1),
     &                vzf1( 0:nx+1, 0:ny+1,-1:nz+1),
     &                vxf2(-1:nx+1, 0:ny+1, 0:nz+1),
     &                vyf2( 0:nx+1,-1:ny+1, 0:nz+1),
     &                vzf2( 0:nx+1, 0:ny+1,-1:nz+1),
     &                xfgrid(-1:nx+1),yfgrid(-1:ny+1),
     &                zfgrid(-1:nz+1),time
      real(kind=8) :: cw1,cw2,cw3,cw4,cw
#ifdef mpi
      integer(kind=4) :: ipp
#endif
!
! --------------------------------------------------------------------
!
!  linear interpolate fluid velocities at cell corner (i+1/2,j+1/2,k+1/2)
!
      if ( iptstp==1 )  then
!
         do kc = 0,nz
!
           flzm = (zfgrid(kc)-zfgrid(kc-1))/(zfgrid(kc+1)-zfgrid(kc-1))
           flzp = 1.0d0-flzm
           do jc = 0,ny
             flym = (yfgrid(jc)-yfgrid(jc-1))/
     &              (yfgrid(jc+1)-yfgrid(jc-1))
             flyp = 1.0d0-flym
             do ic = -1,nx+1
!
               im = max0(ic,0)
               il = min0(ic+1,nx+1)
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
               cw1 = 1.0d0-ciw(iwe(im,jc,kc,1),iww(il,jc,kc,1))
               cw2 = 1.0d0-ciw(iwe(im,jc+1,kc,1),iww(il,jc+1,kc,1))
               cw3 = 1.0d0-ciw(iwe(im,jc,kc+1,1),iww(il,jc,kc+1,1))
               cw4 = 1.0d0-ciw(iwe(im,jc+1,kc+1,1),iww(il,jc+1,kc+1,1))
               cw = cw1+cw2+cw3+cw4
#else
               cw = 0.0d0
#endif
               if ( cw==0.0d0 ) then
                 cw1 = 1.0d0
                 cw2 = 1.0d0
                 cw3 = 1.0d0
                 cw4 = 1.0d0
               else
                 cw1 = cw1/cw
                 cw2 = cw2/cw
                 cw3 = cw3/cw
                 cw4 = cw4/cw
               endif
!
               vxfcor(ic,jc,kc,1) = cw1*flym*flzm*vxf1(ic,jc,kc)+
     &                              cw2*flyp*flzm*vxf1(ic,jc+1,kc)+
     &                              cw3*flym*flzp*vxf1(ic,jc,kc+1)+
     &                              cw4*flyp*flzp*vxf1(ic,jc+1,kc+1)
               vxfcor(ic,jc,kc,2) = cw1*flym*flzm*vxf2(ic,jc,kc)+
     &                              cw2*flyp*flzm*vxf2(ic,jc+1,kc)+
     &                              cw3*flym*flzp*vxf2(ic,jc,kc+1)+
     &                              cw4*flyp*flzp*vxf2(ic,jc+1,kc+1)
             enddo
           enddo
         enddo
!
         do kc = 0,nz
           flzm = (zfgrid(kc)-zfgrid(kc-1))/(zfgrid(kc+1)-zfgrid(kc-1))
           flzp = 1.0d0-flzm
           do ic = 0,nx
             flxm = (xfgrid(ic)-xfgrid(ic-1))/
     &              (xfgrid(ic+1)-xfgrid(ic-1))
             flxp = 1.0d0-flxm
!
             do jc = -1,ny+1
!
               jm = max0(jc,0)
               jl = min0(jc+1,ny+1)
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
               cw1 = 1.0d0-ciw(iwn(ic,jm,kc,2),iws(ic+1,jl,kc,2))
               cw2 = 1.0d0-ciw(iwn(ic+1,jm,kc,2),iws(ic+1,jl,kc,2))
               cw3 = 1.0d0-ciw(iwn(ic,jm,kc+1,2),iws(ic,jl,kc+1,2))
               cw4 = 1.0d0-ciw(iwn(ic+1,jm,kc+1,2),iws(ic+1,jl,kc+1,2))
               cw = cw1+cw2+cw3+cw4
#else
               cw = 0.0d0
#endif
               if ( cw==0.0d0 ) then
                 cw1 = 1.0d0
                 cw2 = 1.0d0
                 cw3 = 1.0d0
                 cw4 = 1.0d0
               else
                 cw1 = cw1/cw
                 cw2 = cw2/cw
                 cw3 = cw3/cw
                 cw4 = cw4/cw
               endif
!
               vyfcor(ic,jc,kc,1) = cw1*flxm*flzm*vyf1(ic,jc,kc)+
     &                              cw2*flxp*flzm*vyf1(ic+1,jc,kc)+
     &                              cw3*flxm*flzp*vyf1(ic,jc,kc+1)+
     &                              cw4*flxp*flzp*vyf1(ic+1,jc,kc+1)
               vyfcor(ic,jc,kc,2) = cw1*flxm*flzm*vyf2(ic,jc,kc)+
     &                              cw2*flxp*flzm*vyf2(ic+1,jc,kc)+
     &                              cw3*flxm*flzp*vyf2(ic,jc,kc+1)+
     &                              cw4*flxp*flzp*vyf2(ic+1,jc,kc+1)
             enddo
           enddo
         enddo
!
         do jc = 0,ny
           flym = (yfgrid(jc)-yfgrid(jc-1))/(yfgrid(jc+1)-yfgrid(jc-1))
           flyp = 1.0d0-flym
           do ic = 0,nx
             flxm = (xfgrid(ic)-xfgrid(ic-1))/
     &              (xfgrid(ic+1)-xfgrid(ic-1))
             flxp = 1.0d0-flxm
             do kc = -1,nz+1
!
               km = max0(kc,0)
               kl = min0(kc+1,nz+1)
!! TODO: Please re-implement for JUPITER
#ifdef TPFIT
               cw1 = 1.0d0-ciw(iwt(ic,jc,km,3),iwb(ic,jc,kl,3))
               cw2 = 1.0d0-ciw(iwt(ic+1,jc,km,3),iwb(ic+1,jc,kl,3))
               cw3 = 1.0d0-ciw(iwt(ic,jc+1,km,3),iwb(ic,jc+1,kl,3))
               cw4 = 1.0d0-ciw(iwt(ic+1,jc+1,km,3),iwb(ic+1,jc+1,kl,3))
               cw = cw1+cw2+cw3+cw4
#else
               cw = 0.0d0
#endif
               if ( cw==0.0d0 ) then
                 cw1 = 1.0d0
                 cw2 = 1.0d0
                 cw3 = 1.0d0
                 cw4 = 1.0d0
               else
                 cw1 = cw1/cw
                 cw2 = cw2/cw
                 cw3 = cw3/cw
                 cw4 = cw4/cw
               endif
!
               vzfcor(ic,jc,kc,1) = flxm*flym*vzf1(ic,jc,kc)+
     &                              flxp*flym*vzf1(ic+1,jc,kc)+
     &                              flxm*flyp*vzf1(ic,jc+1,kc)+
     &                              flxp*flyp*vzf1(ic+1,jc+1,kc)
               vzfcor(ic,jc,kc,2) = flxm*flym*vzf2(ic,jc,kc)+
     &                              flxp*flym*vzf2(ic+1,jc,kc)+
     &                              flxm*flyp*vzf2(ic,jc+1,kc)+
     &                              flxp*flyp*vzf2(ic+1,jc+1,kc)
             enddo
           enddo
         enddo
!
      endif
!
! --------------------------------------------------------------------
!
!  set of fluid velocities using 3D second-order Lagrangian interpolation
!
#ifndef mpi
      do ip = 1,npt
#else
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
!
        if ( time>=pTimpt(ip) )  then ! active particle
!
          kcfm = max0(kcfpt(ip)-1,0)
          kcfp = kcfpt(ip)
          jcfm = max0(jcfpt(ip)-1,0)
          jcfp = jcfpt(ip)
          icfm = max0(icfpt(ip)-1,0)
          icfp = icfpt(ip)
!
!         x-direction
!
          if ( iWPsrd(ip)==1 ) then ! particle is in wall.
!
!            if ( iwe(icfm,jcfm,kcfm),iwe )
            pUxf(ip) = ( vxf1(icfm,jcfm,kcfm) + vxf1(icfm-1,jcfm,kcfm)
     &                 ) *0.5d0
            pUyf(ip) = ( vyf1(icfm,jcfm,kcfm) + vyf1(icfm,jcfm-1,kcfm)
     &                 ) *0.5d0
            pUzf(ip) = ( vzf1(icfm,jcfm,kcfm) + vzf1(icfm,jcfm,kcfm-1)
     &                 ) *0.5d0
          else
!
!! TODO: Re-implement for JUPITER
#ifdef TPFIT
            if ( kcfp==0 .or. icflg(icfp,jcfp,max0(kcfm,0))==4 ) then
!             kfcm is not fluid region
              pLzm = 0.0d0
              pLzp = 1.0d0
            elseif ( kcfp==nz+1 .or. icflg(icfp,jcfp,min0(kcfp+1,nz+1))
     &             ==4 ) then
!             kfcp is not fluid region
              pLzm = 1.0d0
              pLzp = 0.0d0
            else
              pLzm = dmin1(dmax1(
     &               (pZpt(ip)-zfgrid(kcfp)
     &               )/(zfgrid(kcfm)-zfgrid(kcfp)),0.0d0),1.0d0)
              pLzp = 1.0d0-pLzm
            endif
!
            if ( jcfp==0 .or. icflg(icfp,max0(jcfm,0),kcfp)==4 ) then
!             jfcm is not fluid region
              pLym = 0.0d0
              pLyp = 1.0d0
            elseif ( jcfp==ny+1 .or. icflg(icfp,min0(jcfp+1,ny+1),kcfp)
     &             ==4 ) then
!             jfcp is not fluid region
              pLym = 1.0d0
              pLyp = 0.0d0
            else
              pLym = dmin1(dmax1(
     &               (pYpt(ip)-yfgrid(jcfp)
     &               )/(yfgrid(jcfm)-yfgrid(jcfp)),0.0d0),1.0d0)
              pLyp = 1.0d0-pLym
            endif
!
            if ( icfp==0 .or. icflg(max0(icfp,0),jcfp,kcfp)==0 ) then
!             ifcm is not fluid region
              pLxm = 0.0d0
              pLxp = 1.0d0
            elseif ( icfp==nx+1 .or. icflg(min0(icfp+1,nx+1),jcfp,kcfp)
     &             ==4 ) then
!             ifcp is not fluid region
              pLxm = 1.0d0
              pLxp = 0.0d0
            else
              pLxm = dmin1(dmax1(
     &               (pXpt(ip)-xfgrid(icfp)
     &               )/(xfgrid(icfm)-xfgrid(icfp)),0.0d0),1.0d0)
              pLxp = 1.0d0-pLxm
            endif
#else
            pLxm = 1.0d0
            pLxp = 0.0d0
            pLym = 1.0d0
            pLyp = 0.0d0
            pLzm = 1.0d0
            pLzp = 0.0d0
#endif
!
            if (iFPsrd(ip)/=-1 ) then
              ifp = 1
            else
              ifp = 2
            endif
!!
            pUxf(ip) = pLxm*pLym*pLzm*vxfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vxfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vxfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vxfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vxfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vxfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vxfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vxfcor(icfp,jcfp,kcfp,ifp)
!
            pUyf(ip) = pLxm*pLym*pLzm*vyfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vyfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vyfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vyfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vyfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vyfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vyfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vyfcor(icfp,jcfp,kcfp,ifp)
!
            pUzf(ip) = pLxm*pLym*pLzm*vzfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vzfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vzfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vzfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vzfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vzfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vzfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vzfcor(icfp,jcfp,kcfp,ifp)
!
          endif
!
        endif
!
      enddo
!
#ifndef mpi
!  set of fluid velocities using 3D second-order Lagrangian interpolation
!
      do ip = 1,npt
!
        if ( time>=pTimpt(ip) )  then ! active particle
!
          kcfm = max0(kcfpt(ip)-1,0)
          kcfp = kcfpt(ip)
          jcfm = max0(jcfpt(ip)-1,0)
          jcfp = jcfpt(ip)
          icfm = max0(icfpt(ip)-1,0)
          icfp = icfpt(ip)
!
          if ( iWPsrd(ip)==1 ) then
            pUxf(ip) = ( vxf1(icfm,jcfm,kcfm) + vxf1(icfm-1,jcfm,kcfm)
     &                 ) *0.5d0
            pUyf(ip) = ( vyf1(icfm,jcfm,kcfm) + vyf1(icfm,jcfm-1,kcfm)
     &                 ) *0.5d0
            pUzf(ip) = ( vzf1(icfm,jcfm,kcfm) + vzf1(icfm,jcfm,kcfm-1)
     &                 ) *0.5d0
          else
!
!! TODO: Re-implement for JUPITER
#ifdef TPFIT
            if ( kcfp==0 .or. icflg(icfp,jcfp,max0(kcfm,0))==4 ) then
!             kfcm is not fluid region
              pLzm = 0.0d0
              pLzp = 1.0d0
            elseif ( kcfp==nz+1 .or. icflg(icfp,jcfp,min0(kcfp+1,nz+1))
     &             ==4 ) then
!             kfcp is not fluid region
              pLzm = 1.0d0
              pLzp = 0.0d0
            else
              pLzm = dmin1(dmax1(
     &               (pZpt(ip)-zfgrid(kcfp)
     &               )/(zfgrid(kcfm)-zfgrid(kcfp)),0.0d0),1.0d0)
              pLzp = 1.0d0-pLzm
            endif
!
            if ( jcfp==0 .or. icflg(icfp,max0(jcfm,0),kcfp)==4 ) then
!             jfcm is not fluid region
              pLym = 0.0d0
              pLyp = 1.0d0
            elseif ( jcfp==ny+1 .or. icflg(icfp,min0(jcfp+1,ny+1),kcfp)
     &             ==4 ) then
!             jfcp is not fluid region
              pLym = 1.0d0
              pLyp = 0.0d0
            else
              pLym = dmin1(dmax1(
     &               (pYpt(ip)-yfgrid(jcfp)
     &               )/(yfgrid(jcfm)-yfgrid(jcfp)),0.0d0),1.0d0)
              pLyp = 1.0d0-pLym
            endif
!
            if ( icfp==0 .or. icflg(max0(icfp,0),jcfp,kcfp)==0 ) then
!             ifcm is not fluid region
              pLxm = 0.0d0
              pLxp = 1.0d0
            elseif ( icfp==nx+1 .or. icflg(min0(icfp+1,nx+1),jcfp,kcfp)
     &             ==4 ) then
!             ifcp is not fluid region
              pLxm = 1.0d0
              pLxp = 0.0d0
            else
              pLxm = dmin1(dmax1(
     &               (pXpt(ip)-xfgrid(icfp)
     &               )/(xfgrid(icfm)-xfgrid(icfp)),0.0d0),1.0d0)
              pLxp = 1.0d0-pLxm
            endif
#else
            pLxm = 1.0d0
            pLxp = 0.0d0
            pLym = 1.0d0
            pLyp = 0.0d0
            pLzm = 1.0d0
            pLzp = 0.0d0
#endif
!
            if (iFPsrd(ip)/=-1 ) then
              ifp = 1
            else
              ifp = 2
            endif
!!
            pUxf(ip) = pLxm*pLym*pLzm*vxfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vxfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vxfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vxfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vxfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vxfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vxfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vxfcor(icfp,jcfp,kcfp,ifp)
!
            pUyf(ip) = pLxm*pLym*pLzm*vyfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vyfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vyfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vyfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vyfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vyfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vyfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vyfcor(icfp,jcfp,kcfp,ifp)
!
            pUzf(ip) = pLxm*pLym*pLzm*vzfcor(icfm,jcfm,kcfm,ifp) +
     &                 pLxp*pLym*pLzm*vzfcor(icfp,jcfm,kcfm,ifp) +
     &                 pLxm*pLyp*pLzm*vzfcor(icfm,jcfp,kcfm,ifp) +
     &                 pLxp*pLyp*pLzm*vzfcor(icfp,jcfp,kcfm,ifp) +
     &                 pLxm*pLym*pLzp*vzfcor(icfm,jcfm,kcfp,ifp) +
     &                 pLxp*pLym*pLzp*vzfcor(icfp,jcfm,kcfp,ifp) +
     &                 pLxm*pLyp*pLzp*vzfcor(icfm,jcfp,kcfp,ifp) +
     &                 pLxp*pLyp*pLzp*vzfcor(icfp,jcfp,kcfp,ifp)
!
          endif
!
        endif
!
      enddo
#endif
!
      end subroutine LPTvfs
!
! *************
      subroutine LPTnwall(nx,ny,nz)
!     calculation of normal unit vector of wall
!     for Lagrangian particle transportation
!     Ver.0.1 Man, 14 May, 2018
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Coded by H. Yoshida @ Japan Atomic Energy Agency
! *************
!
!      use ngrid,        only: nx,ny,nz
      use LPTval,       only: plms,plals,ndimpls,iplms,numw0
#ifdef TPFIT
      use surcagl1,     only: yfs
#endif
!
      implicit none
!
      integer(kind=4), intent(in) :: nx, ny, nz
      integer(kind=4) :: i,j,k,ii(1:6),jj(1:6),kk(1:6),nm
! --------------------------------------------------------------------
!
      ii(:)=0
      jj(:)=0
      kk(:)=0
!
      ii(1) = -1
      ii(2) = 1
      jj(3) = -1
      jj(4) = 1
      kk(5) = -1
      kk(6) = 1
!
!     initialize parameters for wall position and gradient
!
      plms(:,:,:,:)  = 0.0d0
      plals(:,:,:)  = 0.0d0
      iplms(:,:,:,:) = 0
!
#ifdef TPFIT
      do k=0,nz+1
        do j=0,ny+1
          do i=0,nx+1
!
            if ( yfs(i,j,k)/=0.0d0 .and. yfs(i,j,k)/=1.0d0 ) then
              ndimpls(i,j,k) = 1
            else
              ndimpls(i,j,k) = 0
            endif
!
          enddo
        enddo
      enddo
#else
      ndimpls(:,:,:) = 0
#endif
!
!     detect wall surface position and gradient
!
#ifdef TPFIT
      call itevl(yfs,plms,ndimpls)
      call itinv1(plms,iplms,ndimpls)
      call itinv2(yfs,plms,iplms,ndimpls,plals)
#endif
!
!     detect wall surface between cells
!
      numw0(:,:,:) = 0
!
#ifdef TPFIT
      do k=1,nz
        do j=1,ny
          do i=1,nx
!
            if ( ndimpls(i,j,k)==0 ) then ! not detect surface in cell
!
              numw0(i,j,k) = 0
!
              do nm = 1,6
!
                if ( dabs(yfs(i,j,k)-yfs(i+ii(nm),j+jj(nm),k+kk(nm)))
     &              ==1.0d0 ) then
!
!                 existing surface between cells
!
!                 count number of wall surface between cells
!
                  numw0(i,j,k) = numw0(i,j,k) + 1
!
                endif
!
              enddo
!
            endif
!
          enddo
        enddo
      enddo
#endif
!
      end subroutine LPTnwall
!
! *************
      subroutine LPTwbc(pUxpt,pUypt,pUzpt,pXpt,pYpt,pZpt,xfgrid,yfgrid,
     &                  zfgrid,pTimpt,pDiapt,pEwall,icfpt,jcfpt,kcfpt,
     &                  npt,nx,ny,nz,time)
!     Set of wall boundary condition
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.2.01
!        1. Current time is passed by argument.
!     Ver.2.0  Modified by H. Yoshida (JAEA) 14.May.2018
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        pUxpt  : particle velocity in x-direction [m/s]
!        pUypt  : particle velocity in y-direction [m/s]
!        pUzpt  : particle velocity in z-direction [m/s]
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pTimpt : active time for particle [s]
!        pDiapt : particle diameter [m]
!        pEwall : coefficient of restitution in wall cell
!        icfpt  : fluid cell number in x-direction at particle position
!        jcfpt  : fluid cell number in y-direction at particle position
!        kcfpt  : fluid cell number in z-direction at particle position
!        npt    : number of particles
!        nx     : number of Eulerian grid points in x-direction
!        ny     : number of Eulerian grid points in y-direction
!        nz     : number of Eulerian grid points in z-direction
!        time   : current time [s]
!
! *************
!
      use LPTval,       only: pXwbcm,pXwbcp,pYwbcm,pYwbcp,pZwbcm,pZwbcp
      use LPTval,       only: plms,plals,ndimpls,LPTwbcal,numw0,iWPsrd
#ifdef TPFIT
      use surcagl1,     only:yfs
#else
      use cLPTbnd,      only: LPTudwbc
#endif
#ifdef mpi
      use MT_Parallel_Attribute0, only: myrank,numrank,nrk
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
      integer(kind=4) :: npt,nx,ny,nz,
     &                   ip,icfp,jcfp,kcfp,ii(1:6),jj(1:6),kk(1:6)
      integer(kind=4) :: icfpt(npt),jcfpt(npt),kcfpt(npt),nLpwmn,nm
!
      real(kind=8) :: time
      real(kind=8) :: xfgrid(-1:nx+1),yfgrid(-1:ny+1),zfgrid(-1:nz+1)
      real(kind=8) :: pUxpt(npt),pUypt(npt),pUzpt(npt),
     &                pXpt(npt),pYpt(npt),pZpt(npt),pTimpt(npt),
     &                pDiapt(npt),pEwall(0:nx+1,0:ny+1,0:nz+1)
      real(kind=8) :: dLpwll(1:npt),xpc,ypc,zpc,plmw0(1:6),
     &                dLpw0(1:6),dLpwmn,pEwall0
!
      logical :: lbnd(6)
!
#ifdef TPFIT
#else
      real(kind=8) :: pXw, pExw, pYw, pEyw, pZw, pEzw
#endif
#ifdef mpi
      integer(kind=4) :: ipp
#endif
!
! --------------------------------------------------------------------
!
!     initialize
!
      iWPsrd(:) = 0
      dLpwll(:) =-1.0d6
!
      ii(:)=0
      jj(:)=0
      kk(:)=0
!
      ii(1) = -1
      ii(2) = 1
      jj(3) = -1
      jj(4) = 1
      kk(5) = -1
      kk(6) = 1
!
      dLpw0(:) = huge(0.0d0)
      plmw0(:) = 0.0d0
!
!     wall boundary treatment for Lagrangian calculated particles
!
!     judgement occurrence of particle collision
!
      if ( LPTwbcal==1 ) then
!
#ifndef mpi
      do ip = 1,npt
#else
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
!
        if ( time>=pTimpt(ip) )  then ! active particle
!
          kcfp = kcfpt(ip)
          jcfp = jcfpt(ip)
          icfp = icfpt(ip)
!
#ifdef TPFIT
          if ( numw0(icfp,jcfp,kcfp)>=1 ) then
!
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp-1,jcfp,kcfp))==1.0d0
     &         ) then
              plmw0(1) =-(yfs(icfp,jcfp,kcfp)-yfs(icfp-1,jcfp,kcfp))
              dLpw0(1) = (pXpt(ip)-xfgrid(icfp-1))*plmw0(1)
            else
              plmw0(1) = 0.0d0
              dLpw0(1) = -huge(0.0d0)
            endif
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp+1,jcfp,kcfp))==1.0d0
     &         ) then
              plmw0(2) =-(yfs(icfp+1,jcfp,kcfp)-yfs(icfp,jcfp,kcfp))
              dLpw0(2) = (pXpt(ip)-xfgrid(icfp))*plmw0(2)
            else
              plmw0(2) = 0.0d0
              dLpw0(2) = -huge(0.0d0)
            endif
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp-1,kcfp))==1.0d0
     &         ) then
              plmw0(3) =-(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp-1,kcfp))
              dLpw0(3) = (pYpt(ip)-yfgrid(jcfp-1))*plmw0(3)
            else
              plmw0(3) = 0.0d0
              dLpw0(3) = -huge(0.0d0)
            endif
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp+1,kcfp))==1.0d0
     &         ) then
              plmw0(4) =-(yfs(icfp,jcfp+1,kcfp)-yfs(icfp,jcfp,kcfp))
              dLpw0(4) = (pYpt(ip)-yfgrid(jcfp))*plmw0(4)
            else
              plmw0(4) = 0.0d0
              dLpw0(4) = -huge(0.0d0)
            endif
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp,kcfp-1))==1.0d0
     &         ) then
              plmw0(5) =-(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp,kcfp-1))
              dLpw0(5) = (pZpt(ip)-zfgrid(kcfp-1))*plmw0(5)
            else
              plmw0(5) = 0.0d0
              dLpw0(5) = -huge(0.0d0)
            endif
            if ( dabs(yfs(icfp,jcfp,kcfp)-yfs(icfp,jcfp,kcfp+1))==1.0d0
     &         ) then
              plmw0(6) =-(yfs(icfp,jcfp,kcfp+1)-yfs(icfp,jcfp,kcfp))
              dLpw0(6) = (pZpt(ip)-zfgrid(kcfp))*plmw0(6)
            else
              plmw0(6) = 0.0d0
              dLpw0(6) = -huge(0.0d0)
            endif
!
            dLpwmn = 1.0d6
            nLpwmn = 0
            do nm=1,6
              if ( dLpw0(nm)>-pDiapt(ip)*0.5d0 .and.
     &             dabs(dLpw0(nm))<dabs(dLpwmn) ) then
                dLpwmn = dLpw0(nm)
                nLpwmn = nm
              endif
            enddo
!
!            if ( yfs(icfp,jcfp,kcfp)/=1.0d0
!     &        .and. pEwall(icfp,jcfp,kcfp)>=0.0d0 ) then
!              if (nLpwmn==0) then
!                write(6,*) "error particle in wall is not detected"
!                write(6,*) icfp,jcfp,kcfp,pXpt(ip),pYpt(ip),pZpt(ip)
!                stop
!              endif
!            endif
!!
            if ( nLpwmn>=1 ) then
!
              if ( yfs(icfp,jcfp,kcfp)==1.0d0 ) then
!
                pEwall0
     &        = pEwall(icfp+ii(nLpwmn),jcfp+jj(nLpwmn),kcfp+kk(nLpwmn))
!
              else
!
                pEwall0 = pEwall(icfp,jcfp,kcfp)
!
              endif
!
              if ( dLpwmn>=-pDiapt(ip)*0.5d0 .and.
     &             pEwall0>=0.0d0 ) then
!
!               evaluate particle position when contacting occurred.
!
                if ( nLpwmn<=2 ) then
                  pXpt(ip) = pXpt(ip)
     &                     -plmw0(nLpwmn)*dabs(dlpwmn-pDiapt(ip)*0.5d0)
!                  pUxpt(ip) = -dabs(pUxpt(ip))*plmw0(nLpwmn)*pEwall0
                  pUxpt(ip) = -pUxpt(ip)*pEwall0
                elseif ( nLpwmn<=4 ) then
                  pYpt(ip) = pYpt(ip)
     &                     -plmw0(nLpwmn)*dabs(dlpwmn-pDiapt(ip)*0.5d0)
!                  pUypt(ip) = -dabs(pUypt(ip))*plmw0(nLpwmn)*pEwall0
                  pUypt(ip) = -pUypt(ip)*pEwall0
                else
                  pZpt(ip) = pZpt(ip)
     &                     -plmw0(nLpwmn)*dabs(dlpwmn-pDiapt(ip)*0.5d0)
!                  pUzpt(ip) = -dabs(pUzpt(ip))*plmw0(nLpwmn)*pEwall0
                  pUzpt(ip) = -pUzpt(ip)*pEwall0
                endif
!
              endif
!
              if ( dLpwmn>=-pDiapt(ip)*0.5d0 ) then
                iWPsrd(ip) = 1  ! particle in/on wall
                dLpwll(ip) = dLpwmn
              else
                iWPsrd(ip) = 0
              endif
!
            endif
!
          elseif ( ndimpls(icfp,jcfp,kcfp)/=0 ) then
!
!           evaluate distance between particle and interface
!
            dLpwll(ip) =plms(icfp,jcfp,kcfp,1)*(pXpt(ip)-xfgrid(icfp-1))
     &                 +plms(icfp,jcfp,kcfp,2)*(pYpt(ip)-yfgrid(jcfp-1))
     &                 +plms(icfp,jcfp,kcfp,3)*(pZpt(ip)-zfgrid(kcfp-1))
     &                 -plals(icfp,jcfp,kcfp)
!
            if ( dLpwll(ip)>=-pDiapt(ip)*0.5d0 .and.
     &           pEwall(icfp,jcfp,kcfp)>=0.0d0 ) then
!
!             evaluate particle position when contacting occurred.
!
              xpc = pXpt(ip)
     &         -plms(icfp,jcfp,kcfp,1)*(dlpwll(ip)-pDiapt(ip)*0.5d0)
!
              ypc = pYpt(ip)
     &         -plms(icfp,jcfp,kcfp,2)*(dlpwll(ip)-pDiapt(ip)*0.5d0)
!
              zpc = pZpt(ip)
     &         -plms(icfp,jcfp,kcfp,3)*(dlpwll(ip)-pDiapt(ip)*0.5d0)
!
!             particle position back to contacting position
!
              pXpt(ip) = xpc
              pYpt(ip) = ypc
              pZpt(ip) = zpc
!
!             evaluate new particle velocity vector
!
!             velocity vector normal to wall
!            +velocity vector tangential to wall
!
               pUxpt(ip) =
     &        -pUxpt(ip)*dabs(plms(icfp,jcfp,kcfp,1))
     &                  *pEwall(icfp,jcfp,kcfp)
     &        +pUxpt(ip)*dsqrt(1.0d0-plms(icfp,jcfp,kcfp,1)**2)
               pUypt(ip) =
     &        -pUypt(ip)*dabs(plms(icfp,jcfp,kcfp,2))
     &                  *pEwall(icfp,jcfp,kcfp)
     &        +pUypt(ip)*dsqrt(1.0d0-plms(icfp,jcfp,kcfp,2)**2)
               pUzpt(ip) =
     &        -pUzpt(ip)*dabs(plms(icfp,jcfp,kcfp,3))
     &                  *pEwall(icfp,jcfp,kcfp)
     &        +pUzpt(ip)*dsqrt(1.0d0-plms(icfp,jcfp,kcfp,3)**2)
!
!            elseif ( dLpwll(ip)> 0.0d0 ) then
!!
!
!            else
!!
            endif
!
            if ( dLpwll(ip)>=pDiapt(ip)*0.5d0 ) then
              iWPsrd(ip) = 1 ! particle in/on wall
            else
              iWPsrd(ip) = 0 ! particle in fluid
            endif
!
         endif
#else
!
!        JUPITER defines boundary condition in C-language.
!
         call LPTudwbc(ip,pUxpt(ip),pUypt(ip),pUzpt(ip),
     &        pXpt(ip),pYpt(ip),pZpt(ip),pDiapt(ip),dLpwll(ip),
     &        iWPsrd(ip),icfpt(ip),jcfpt(ip),kcfpt(ip))
#endif
!
        endif
!
      enddo
!
      endif
!
!     set as non active particle when particle not in domain
!
#ifndef mpi
      do ip = 1,npt
#else
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
          kcfp = kcfpt(ip)
          jcfp = jcfpt(ip)
          icfp = icfpt(ip)
!
!       Set true if MPI-neighbor does not exist for each direction
!
#ifdef mpi
        lbnd(:) = nrk(:) .lt. 0
#else
        lbnd(:) = .true.
#endif
!
!       boundary in x- or x+ direction
!
        if (lbnd(5)) lbnd(5) = pXpt(ip)<(pXwbcm-pDiapt(ip)*0.5d0)
        if (lbnd(6)) lbnd(6) = pXpt(ip)>(pXwbcp+pDiapt(ip)*0.5d0)
        if (lbnd(5) .or. lbnd(6)) then
#ifdef TPFIT
          pUypt(ip) = 0.0d0
          pUzpt(ip) = 0.0d0
          pTimpt(ip) = huge(0.0d0) ! set to non-active particle
#else
!
!     Compute restitution on computational boundary of X- or X+ direction.
!
          if (lbnd(5)) then
            pXw = pXwbcm
          else
            pXw = pXwbcp
          endif
          pExw = pEwall(icfp, jcfp, kcfp)
          if (pExw .ge. 0.0d0) then
            pXpt(ip) = pXw-pExw*(pXpt(ip)-pXw)
            pUxpt(ip) = -pExw*pUxpt(ip)
!
            if (pExw .eq. 0.0d0) then
              pUypt(ip) = 0.0d0
              pUzpt(ip) = 0.0d0
              pTimpt(ip) = huge(0.0d0) ! set to non-active particle
            endif
          else
            pTimpt(ip) = huge(0.0d0) ! set to non-active particle
          endif
#endif
        endif
!
!       boundary in y- or y+ direction
!
        if (lbnd(3)) lbnd(3) = pYpt(ip)<(pYwbcm-pDiapt(ip)*0.5d0)
        if (lbnd(4)) lbnd(4) = pYpt(ip)>(pYwbcp+pDiapt(ip)*0.5d0)
        if (lbnd(3) .or. lbnd(4)) then
#ifdef TPFIT
          pUxpt(ip) = 0.0d0
          pUzpt(ip) = 0.0d0
          pTimpt(ip) = huge(0.0d0) ! set to non-active particle
#else
!
!     Compute restitution on computational boundary of Y- or Y+ direction.
!
          if (lbnd(3)) then
            pYw = pYwbcm
          else
            pYw = pYwbcp
          endif
          pEyw = pEwall(icfp, jcfp, kcfp)
          if (pEyw .ge. 0.0d0) then
            pYpt(ip) = pYw-pEyw*(pYpt(ip)-pYw)
            pUypt(ip) = -pEyw*pUypt(ip)
!
            if (pEyw .eq. 0.0d0) then
              pUxpt(ip) = 0.0d0
              pUzpt(ip) = 0.0d0
              pTimpt(ip) = huge(0.0d0) ! set to non-active particle
            endif
          else
            pTimpt(ip) = huge(0.0d0) ! set to non-active particle
          endif
#endif
        endif
!
!       wall boundary in z- or z+ direction
!
        if (lbnd(1)) lbnd(1) = pZpt(ip)<(pZwbcm-pDiapt(ip)*0.5d0)
        if (lbnd(2)) lbnd(2) = pZpt(ip)>(pZwbcp+pDiapt(ip)*0.5d0)
        if (lbnd(1) .or. lbnd(2)) then
#ifdef TPFIT
          pUxpt(ip) = 0.0d0
          pUypt(ip) = 0.0d0
          pTimpt(ip) = huge(0.0d0) ! set to non-active particle
#else
!
!     Compute restitution on computational boundary of Z- or Z+ direction.
!
          if (lbnd(1)) then
            pZw = pZwbcm
          else
            pZw = pZwbcp
          endif
          pEzw = pEwall(icfp, jcfp, kcfp)
          if (pEzw .ge. 0.0d0) then
            pZpt(ip) = pZw-pEzw*(pZpt(ip)-pZw)
            pUzpt(ip) = -pEzw * pUzpt(ip)
!
            if (pEzw .eq. 0.0d0) then
              pUxpt(ip) = 0.0d0
              pUypt(ip) = 0.0d0
              pTimpt(ip) = huge(0.0d0) ! set to non-active particle
            endif
          else
            pTimpt(ip) = huge(0.0d0) ! set to non-active particle
          endif
#endif
        endif
!
      enddo
!
      end subroutine LPTwbc
!
! *************
      subroutine LPTint(psetXs,psetXe,psetYs,psetYe,psetZs,psetZe,
     &                  psetTms,psetTme,psetDi,psetRi,
     &                  psetUx,psetUy,psetUz,
     &                  pXpt,pYpt,pZpt,pTimpt,pDiapt,pRhopt,
     &                  pUxpt,pUypt,pUzpt,
     &                  nistpt,idxpt,npt,npt0,itrdm)
!     Set of initial particle conditions
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.01  Modified by H. Yoshida (JAEA) 14.Mar.2018
!        1. 1st release
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        psetXs : starting point of setting area of initial particles
!                                                    in x-direction [m]
!        psetXe : ending point of setting area of initial particles
!                                                    in x-direction [m]
!        psetYs : starting point of setting area of initial particle
!                                                    in y-direction [m]
!        psetYe : ending point of setting area of initial particle
!                                                    in y-direction [m]
!        psetZs : starting point of setting area initial particle
!                                                    in z-direction [m]
!        psetZe : ending point of setting area initial particle
!                                                    in z-direction [m]
!        psetTms: starting time for generating particles [s]
!        psetTme: end time for generating particles [s]
!        psetDi : setting particle diameter [m]
!        psetRi : setting particle density [kg/m3]
!        psetUx : setting initial particle velocity in x-direction [m/s]
!        psetUy : setting initial particle velocity in y-direction [m/s]
!        psetUz : setting initial particle velocity in z-direction [m/s]
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pTimpt : active time for particle [s]
!        pDiapt : particle diameter [m]
!        pRhopt : particle density [kg/m3]
!        pUxpt  : particle velocity in x-direction [m/s]
!        pUypt  : particle velocity in y-direction [m/s]
!        pUzpt  : particle velocity in z-direction [m/s]
!        nistpt : number of setting initial particles
!        idxpt  : start index of setting particles
!        npt    : number of particles
!        npt0   : npt of previous job from restart data
!        itrdm  : flag for time random generation of particles
!
! *************
!
      implicit none
!
      integer(kind=4) :: nistpt,idxpt,npt,npt0,ip,itrdm
      real(kind=8) :: psetXs,psetXe,psetYs,psetYe,psetZs,psetZe,
     &                psetTms,psetTme,psetDi,psetRi,
     &                psetUx,psetUy,psetUz,
     &                pXpt(npt),pYpt(npt),pZpt(npt),
     &                pTimpt(npt),pDiapt(npt),pRhopt(npt),
     &                pUxpt(npt),pUypt(npt),pUzpt(npt),
     &                plenX,plenY,plenZ
!
      real(kind=8) :: rndx,rndy,rndz,rndt,delt
!
! --------------------------------------------------------------------
!
!  set initial particle position, diameter, density and velocities
!
      plenX = psetXe-psetXs
      plenY = psetYe-psetYs
      plenZ = psetZe-psetZs
!
      if ( itrdm==0 ) then
        delt = (psetTme-psetTms)/dble(max0(nistpt-1,1))
      endif
!
      do ip=idxpt,idxpt+nistpt-1
!
        if ( ip>npt0 ) then ! newly or add input data
           call random_number(rndx)
           call random_number(rndy)
           call random_number(rndz)
           pXpt(ip) = psetXs+rndx*plenX
           pYpt(ip) = psetYs+rndy*plenY
           pZpt(ip) = psetZs+rndz*plenZ
!
           pUxpt(ip) = psetUx
           pUypt(ip) = psetUy
           pUzpt(ip) = psetUz
        endif
!
        if ( pTimpt(ip)<huge(0.0d0) ) then ! except non-active particle
           if ( itrdm==0 ) then
              pTimpt(ip) = psetTms + delt*dble(ip-idxpt)
           else
              call random_number(rndt)
              pTimpt(ip) = psetTms+rndt*(psetTme-psetTms)
           endif
        endif
!
        pDiapt(ip) = psetDi
        pRhopt(ip) = psetRi
!
      enddo
!
      end subroutine LPTint
!
! *************
      subroutine LPTint0
!     Initialize and finalize random number
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.0  Created by H. Yoshida (JAEA) 08.Mar.2018
!        1. 1st release
!
! *************
!
      implicit none
!
      integer(kind=4) :: seedsize, i
      integer(kind=4), allocatable :: seed(:)
!
! --------------------------------------------------------------------
!
!     initialize seed of random number generation
!
      call random_seed(size=seedsize)
      allocate(seed(seedsize))
      do i = 1,seedsize
        call system_clock(count=seed(i))
      enddo
      call random_seed(put=seed(:))
!
      deallocate(seed)
!
      end subroutine LPTint0
!
! *************
      subroutine LPTprt(iunLPT,time,pXpt,pYpt,pZpt,pUxpt,pUypt,pUzpt,
#ifndef LPT_mpi_dbg
     &                  pDiapt,pTimpt,npt)
#else
     &                  pDiapt,pTimpt,pMrank,npt)
#endif
!     Print of particle positions and velocities
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.001 Modified by H. Yoshida (JAEA) 25.Feb.2018
!        1. Change format control for printing time
!     Ver.1.0  Created by Y. Ose (YSE) 23.Feb.2018
!        1. 1st release
!
!      arguments
!        iunLPT : unit number for output file
!        time   : current time [s]
!        pXpt   : particle position in x-direction [m]
!        pYpt   : particle position in y-direction [m]
!        pZpt   : particle position in z-direction [m]
!        pDiapt : particle diameter [m]
!        pRhopt : particle density [kg/m3]
!        pUxpt  : particle velocity in x-direction [m/s]
!        pUypt  : particle velocity in y-direction [m/s]
!        pUzpt  : particle velocity in z-direction [m/s]
!        pDiapt : particle diameter [m]
!        pTimpt : active time for particle [s]
!        npt    : number of particles
!
! *************
!
      use MT_Parallel_Attribute0, only: myrank
      implicit none
!
      integer(kind=4) :: iunLPT,npt,ip
      real(kind=8) :: time
      real(kind=8) :: pXpt(npt),pYpt(npt),pZpt(npt),
     &                pUxpt(npt),pUypt(npt),pUzpt(npt),
     &                pDiapt(npt),pTimpt(npt)
#ifdef LPT_mpi_dbg
      real(kind=8) :: pMrank(npt)
#endif
!
! --------------------------------------------------------------------
!
      write(iunLPT,6001) time
 6001 format('Time=',1pe12.5,' [s]')
      write(iunLPT,6002) npt
 6002 format('npt=',i5,10x,'Xpt',10x,'Ypt',10x,'Zpt',
     &                 9x,'Uxpt',9x,'Uypt',9x,'Uzpt',
#ifndef LPT_mpi_dbg
     &                 8x,'Diapt',7x,'pTimpt')
#else
     &                 8x,'Diapt',7x,'pTimpt',7x,'pMrank')
#endif
      do ip = 1,npt
         write(iunLPT,6003) ip,pXpt(ip),pYpt(ip),pZpt(ip),
     &                      pUxpt(ip),pUypt(ip),pUzpt(ip),
#ifndef LPT_mpi_dbg
     &                      pDiapt(ip),pTimpt(ip)
#else
     &                      pDiapt(ip),pTimpt(ip),pMrank(ip)
#endif
      enddo
!!! 6003 format(4x,i5,8(1x,1pe12.5))
 6003 format(4x,i5,9(1x,1pe12.5))
!
      end subroutine LPTprt
!! NOTE: Removed from compilation because it is unused.
!! TODO: Re-implement for JUPITER if needed.
#ifdef TPFIT
! *************
      subroutine LPTtwc
!     All Rights Reserved (c) Japan Atomic Energy Agency
!     Ver.1.0  Created by H. Yoshida (JAEA) 01.Mar.2018
!        1. 1st release
! *************
!
      use cgeom1,       only: dt,dxc,dyc,dzc,dxr,dyr,dzr,
     &                        xcntr,ycntr,zcntr
      use cmcs0,        only: iwe,iww,iwn,iws,iwt,iwb
      use cmvel,        only: yulv,yvlu,yvlw,ywlv,ywlu,yulw,yugv,yvgu,
     &                        yvgw,ywgv,ywgu,yugw
      use cpvel,        only: yuln,yugn,yvln,yvgn,ywln,ywgn,
     &                        yul,yug,yvl,yvg,ywl,ywg
      use eqsolv,       only: kslvu,kslvv,kslvw
      use jval,         only: ciw
      use ngrid,        only: nx,ny,nz
      use times,        only: time
      use ycbnd2,       only: yrbx,yrby,yrbz
!
      use LPTval,       only: pXwbcm,pXwbcp,pYwbcm,pYwbcp,pZwbcm,pZwbcp,
     &                        pXpt, pYpt, pZpt,
     &                        pTimpt
      use LPTval,       only: npt, icfpt, jcfpt, kcfpt
      use LPTval,       only: fFxpt,fFypt,fFzpt
#ifdef mpi
#if defined(LPT_USE_MPI_F08_INTERFACE)
      use MPI_F08, only: MPI_COMM_WORLD,MPI_REAL8,MPI_MAX
      use MPI_F08, only: MPI_Allreduce
#else
      use MPI, only: MPI_COMM_WORLD,MPI_REAL8,MPI_MAX
#if defined(LPT_USE_MPI_INTERFACE)
      use MPI, only: MPI_Allreduce
#endif
#endif
      use MT_Parallel_Attribute0, only: myrank
      use LPTval, only: nptlvc, iptlvc
#endif
!
      implicit none
!
      real(kind=8) :: fMass
      integer(kind=4) :: ip,ic,jc,kc,icfp,jcfp,kcfp,ic_twc
#ifdef mpi
      integer(kind=4) :: ipp, ierr
      integer :: ic_twct
#ifdef LPT_DEFINE_MPI_INTERFACE
!
!     Explicitly define interface of MPI_Allreduce
!
      interface
         subroutine MPI_Allreduce(sendbuf,recvbuf,count,datatype,op,
     &        comm,ierror)
         type(*) :: sendbuf, recvbuf
         integer :: count, datatype, op, comm, ierror
         end subroutine MPI_Allreduce
      end interface
#endif
#endif
! -------------------------------------------------------------------
!
      ic_twc = 0
!
!     --- two-way coupling ---
!
#ifndef mpi
      do ip = 1,npt
#else
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
#endif
!
        if ( time>=pTimpt(ip) )  then ! active particle
!
          ic_twc = 1
!
          kcfp = kcfpt(ip)
          jcfp = jcfpt(ip)
          icfp = icfpt(ip)
!
!         x-direction
!
          if ( kslvu/=0 ) then
!
            if ( pXpt(ip)>pXwbcm .and. pXpt(ip)<pXwbcp )  then
!
              do ic = max0(icfp,0),min0(icfp+1,nx)
!
                if ( xcntr(ic-1)<=pXpt(ip) .and.
     &               xcntr(ic)>pXpt(ip) )  then
!
                  fMass = yrbx(ic,jcfp,kcfp)*dxr(ic)*dyc(jcfp)*dzc(kcfp)
                  yuln(ic,jcfp,kcfp) = yuln(ic,jcfp,kcfp)
     &                               + dt*fFxpt(ip)/fMass
     &                                 *ciw(iwe(ic,jcfp,kcfp,1),
     &                                      iww(ic+1,jcfp,kcfp,1))
                  yugn(ic,jcfp,kcfp) = yugn(ic,jcfp,kcfp)
     &                               + dt*fFxpt(ip)/fMass
     &                                 *ciw(iwe(ic,jcfp,kcfp,1),
     &                                      iww(ic+1,jcfp,kcfp,1))
!
                endif
!
              enddo
!
            endif
!
          endif
!
!         y-direction
!
          if ( kslvv/=0 ) then
!
            if ( pYpt(ip)>pYwbcm .and. pYpt(ip)<pYwbcp )  then
!
              do jc = max0(jcfp,0),min0(jcfp+1,ny)
!
                if ( ycntr(jc-1)<=pYpt(ip) .and.
     &               ycntr(jc)>pYpt(ip) )  then
!
                  fMass = yrby(icfp,jc,kcfp)*dxc(icfp)*dyr(jc)*dzc(kcfp)
                  yvln(icfp,jc,kcfp) = yvln(icfp,jc,kcfp)
     &                               + dt*fFypt(ip)/fMass
     &                                 *ciw(iwn(icfp,jc,kcfp,2),
     &                                      iws(icfp,jc+1,kcfp,2))
                  yvgn(icfp,jc,kcfp) = yvgn(icfp,jc,kcfp)
     &                               + dt*fFypt(ip)/fMass
     &                                 *ciw(iwn(icfp,jc,kcfp,2),
     &                                      iws(icfp,jc+1,kcfp,2))
!
                endif
!
              enddo
!
            endif
!
          endif
!
!         z-direction
!
          if ( kslvw/=0 ) then
!
            if ( pZpt(ip)>pZwbcm .and. pZpt(ip)<pZwbcp )  then
!
              do kc = max0(kcfp,0),min0(kcfp+1,nz)
!
                if ( zcntr(kc-1)<=pZpt(ip) .and.
     &               zcntr(kc)>pZpt(ip) )  then
!
                  fMass = yrbz(icfp,jcfp,kc)*dxc(icfp)*dyc(jcfp)*dzr(kc)
                  ywln(icfp,jcfp,kc) = ywln(icfp,jcfp,kc)
     &                               + dt*fFZpt(ip)/fMass
     &                                 *ciw(iwt(icfp,jcfp,kc,3),
     &                                      iwb(icfp,jcfp,kc+1,3))
                  ywgn(icfp,jcfp,kc) = ywgn(icfp,jcfp,kc)
     &                               + dt*fFZpt(ip)/fMass
     &                                 *ciw(iwt(icfp,jcfp,kc,3),
     &                                      iwb(icfp,jcfp,kc+1,3))
!
                endif
!
              enddo
!
            endif
!
          endif
!
        endif
!
      enddo
!
#ifdef mpi
      ic_twct = ic_twc
      call MPI_Allreduce(ic_twct,ic_twc,1,MPI_INTEGER,MPI_MAX,
     &                   MPI_COMM_WORLD,ierr)
#endif
      if ( ic_twc==1 ) then
!
        if ( kslvu/=0 ) then
!
          call boundu(yuln,yul,yvlu,ywlu,1)
          call boundu(yugn,yug,yvgu,ywgu,2)
!
        endif
!
        if ( kslvv/=0 ) then
!
          call boundv(yvln,yvl,yulv,ywlv,1)
          call boundv(yvgn,yvg,yugv,ywgv,2)
!
        endif
!
        if ( kslvw/=0 ) then
!
          call boundw(ywln,ywl,yulw,yvlw,1)
          call boundw(ywgn,ywg,yugw,yvgw,2)
!
        endif
!
      endif
!
      end subroutine
#endif
!! NOTE: Remove from compilation because unused.
#ifdef TPFIT
! *************
      subroutine LPTres(ipRes)
!     Read/Write restart data of paricles for LPT
!     All Rights Reserved (c) Japan Atomic Energy Agency
!
!      arguments
!        ipRes : flag for restart data =1 : read data
!                                      =2 : write data
!
! *************
!
      use times,        only: istep,time
      use LPTval,       only: Timprn, TipLPT,
     &                        pXpt, pYpt, pZpt, pUxpt, pUypt, pUzpt,
     &                        pTimpt, pRhopt, pDiapt,
     &                        pFUxpt, pFUypt, pFUzpt,
     &                        pFdUxt, pFdUyt, pFdUzt
      use LPTval,       only: npt0, npt
#ifdef mpi
      use MPI, only: MPI_REAL8, MPI_INTEGER, MPI_COMM_WORLD
      use MT_PARALLEL_ATTRIBUTE0, only: myrank
#endif
!
      implicit none
!
      integer(kind=4) :: ipRes
!
      integer(kind=4) :: iunit, ip
#ifdef mpi
      integer(kind=4) :: myroot,ierr
#endif
!
! --------------------------------------------------------------------
!
!  Read restart data
!
      if ( ipRes==1 )  then
#ifdef mpi
         myroot = 0
         if ( myrank==myroot ) then
#endif
!
         iunit = 23
!
         read(iunit) npt0
         read(iunit) Timprn
         read(iunit) (pXpt(ip),pYpt(ip),pZpt(ip),ip=1,npt0)
         read(iunit) (pUxpt(ip),pUypt(ip),pUzpt(ip),ip=1,npt0)
         read(iunit) (pTimpt(ip),ip=1,npt0)
         read(iunit) (pFUxpt(ip),pFUypt(ip),pFUzpt(ip),ip=1,npt0)
         read(iunit) (pFdUxt(ip),pFdUyt(ip),pFdUzt(ip),ip=1,npt0)
#ifdef mpi
         endif
         call MPI_Bcast(npt0, 1,MPI_INTEGER,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFUxpt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFUypt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFUzpt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFdUxt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFdUyt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
         call MPI_Bcast(pFdUzt,npt,MPI_REAL8,myroot,MPI_COMM_WORLD,ierr)
#endif
#ifdef mpi
         if ( myrank==myroot ) then
#endif
         write(7,6010) istep,time
         write(6,6010) istep,time
 6010    format(' restart data for LPT at istep=',i10,
     &          ' time=',1pd12.5,' has been read.')
#ifdef mpi
         endif
#endif
!
! --------------------------------------------------------------------
!
!  Write restart data
!
      elseif ( ipRes==2 )  then
#ifdef mpi
         myroot = 0
         if ( myrank==myroot ) then
#endif
!
         iunit = 22
!
         write(iunit) npt
         write(iunit) Timprn-TipLPT
         write(iunit) (pXpt(ip),pYpt(ip),pZpt(ip),ip=1,npt)
         write(iunit) (pUxpt(ip),pUypt(ip),pUzpt(ip),ip=1,npt)
         write(iunit) (pTimpt(ip),ip=1,npt)
         write(iunit) (pFUxpt(ip),pFUypt(ip),pFUzpt(ip),ip=1,npt)
         write(iunit) (pFdUxt(ip),pFdUyt(ip),pFdUzt(ip),ip=1,npt)
!
         write(6,6020) istep,time
!
         call flush(iunit)
!
         rewind(iunit)
!
 6020    format(' restart data for LPT at istep=',i10,
     &          ' time=',1pd12.5,' has been writen.')
!
         write(6,*) 'restart file dump for LPT ended.'
!
!
#ifdef mpi
         endif
#endif
      endif
!
      end subroutine LPTres
#endif
!
#ifdef mpi
! *************
      subroutine LPTgat(pPhi,npt)
!     Gather variable data of paricles from all ranks
!     All Rights Reserved (c) Japan Atomic Energy Agency
!
!      arguments
!        pPhi   : variable data of particles
!        npt    : number of particles
!
! *************
!
#if defined(LPT_USE_MPI_F08_INTERFACE)
      use MPI_F08, only: MPI_COMM_WORLD,MPI_REAL8,MPI_SUM
      use MPI_F08, only: MPI_Allreduce, MPI_Barrier
#else
      use MPI, only: MPI_COMM_WORLD,MPI_REAL8,MPI_SUM
#if defined(LPT_USE_MPI_INTERFACE)
      use MPI, only: MPI_Allreduce
#endif
#endif
      use MT_Parallel_Attribute0, only: myrank, lpt_comm
      use LPTval, only: nptlvc, iptlvc, pPhi_t
!
      implicit none
!
      integer(kind=4) :: npt
      real(kind=8) :: pPhi(npt)
!
      integer(kind=4) :: ipp, ip, ierr
#ifdef LPT_DEFINE_MPI_INTERFACE
!
!     Explicitly define interface of MPI_Allreduce
!
      interface
         subroutine MPI_Allreduce(sendbuf,recvbuf,count,datatype,op,
     &        comm,ierror)
         type(*) :: sendbuf, recvbuf
         integer :: count, datatype, op, comm, ierror
         end subroutine MPI_Allreduce
      end interface
#endif
!
! --------------------------------------------------------------------
!
!     set variable data on myrank to initialized array data
!
      pPhi_t(:) = 0.0d0
      do ipp = 1,nptlvc(myrank)
         ip = iptlvc(ipp)
         pPhi_t(ip) = pPhi(ip)
      enddo
!
!     sums variable data from all ranks
!
      call mpi_barrier(mpi_comm_world,ierr)
      call MPI_Allreduce(pPhi_t,pPhi,npt,MPI_REAL8,MPI_SUM,
     &                   MPI_COMM_WORLD,ierr)
!
#ifdef mpi_dbg
!      do ip =1,npt
!         write(60+myrank,*) 
!     &        'ip=',ip,',pPhi_t=',pPhi_t(ip),',pPhi=',pPhi(ip)
!      enddo
#endif
!
      end subroutine LPTgat
#endif
!
#endif
