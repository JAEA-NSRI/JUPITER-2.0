 module LPTval
#ifdef LPT
!
!    Definition and allocation of valiables using LPT calculation
!
     implicit none
     save
     integer(kind=4), parameter :: mxpset=100
     real(kind=8) :: TipLPT,Timprn, &
                     pCoefa(1:3),pCoefb(1:3),pCoefg(1:3), &
                     psetXs(mxpset),psetXe(mxpset), &
                     psetYs(mxpset),psetYe(mxpset), &
                     psetZs(mxpset),psetZe(mxpset), &
                     psetTms(mxpset),psetTme(mxpset),psetDi(mxpset),psetRi(mxpset), &
                     psetUx(mxpset),psetUy(mxpset),psetUz(mxpset), &
                     pXwbcm, pXwbcp, pYwbcm, pYwbcp, pZwbcm, pZwbcp
     real(kind=8), allocatable :: pUxf(:),   pUyf(:),   pUzf(:), &
                                  pXpt(:),   pYpt(:),   pZpt(:), &
                                  pUxpt(:),  pUypt(:),  pUzpt(:), &
                                  pTimpt(:), pRhopt(:), pDiapt(:), &
                                  pFUxpt(:), pFUypt(:), pFUzpt(:), &
                                  pFdUxt(:), pFdUyt(:), pFdUzt(:), &
                                  vxfcor(:,:,:,:),vyfcor(:,:,:,:),vzfcor(:,:,:,:), &
                                  pEwall(:,:,:), &
                                  fFxpt(:),fFypt(:),fFzpt(:),dLpin(:)
     real(kind=8), allocatable :: plms(:,:,:,:),plals(:,:,:)
     real(kind=8), allocatable :: yvis(:,:,:)
     integer(kind=4), allocatable :: ndimpls(:,:,:),iplms(:,:,:,:),numw0(:,:,:)
     integer(kind=4) :: icLPT,ipttim,npt0,npt,npset,npic,npinw,nistpt(mxpset), &
                        itrdm(mxpset),iunLPT,LPTwbcal
     integer(kind=4), allocatable :: icfpt(:), jcfpt(:), kcfpt(:),iFPsrd(:),iWPsrd(:)
#ifdef mpi
     integer(kind=4), allocatable :: nptlvc(:), iptlvc(:)
     real(kind=8), allocatable :: pPhi_t(:)
#endif
     contains
         subroutine AllocLPT(iset,npt,nx,ny,nz,istat)
#ifdef mpi
           use MPI
           use MT_Parallel_Attribute0, only: numrank
#endif
             implicit none
             integer(kind=4), intent(in) :: iset,npt,nx,ny,nz
             integer(kind=4), intent(inout) :: istat
             integer(kind=4)             :: err
             !
             if ( iset==1 )  then
                allocate( pUxf(npt),   pUyf(npt),  pUzf(npt), &
                          pXpt(npt),   pYpt(npt),  pZpt(npt), &
                          pUxpt(npt),  pUypt(npt), pUzpt(npt), &
                          pTimpt(npt), pRhopt(npt), pDiapt(npt), &
                          pFUxpt(npt), pFUypt(npt), pFUzpt(npt), &
                          pFdUxt(npt), pFdUyt(npt), pFdUzt(npt), &
                          fFxpt(npt),fFypt(npt),fFzpt(npt),dLpin(npt), &
                          pEwall(0:nx+1,0:ny+1,0:nz+1), &
                          plms(0:nx+1,0:ny+1,0:nz+1,1:3), &
                          plals(0:nx+1,0:ny+1,0:nz+1) , &
                          yvis(0:nx+1,0:ny+1,0:nz+1), &
                          ndimpls(0:nx+1,0:ny+1,0:nz+1) ,&
                          iplms(0:nx+1,0:ny+1,0:nz+1,1:3), &
                          numw0(0:nx+1,0:ny+1,0:nz+1), &
                          stat=err)
                if (err/=0) then
                   print *, "Fail to allocate the member of LPTval_r."
                   istat = 1
                   return
                end if
                allocate( icfpt(npt),   jcfpt(npt),  kcfpt(npt), iFPsrd(npt),iWPsrd(npt), &
                          stat=err)
                if (err/=0) then
                   print *, "Fail to allocate the member of LPTval_i."
                   istat = 1
                   return
                end if
#ifdef mpi
                allocate( nptlvc(0:numrank-1), iptlvc(npt), stat=err )
                if (err/=0) then
                   print *, "Fail to allocate the member of LPTval_i_mpi."
                   istat = 1
                   return
                end if
                allocate( pPhi_t(npt), stat=err )
                if (err/=0) then
                   print *, "Fail to allocate the member of LPTval_r_mpi."
                   istat = 1
                   return
                end if
                nptlvc = 0 ; iptlvc = 0
                pPhi_t = 0.0d0
#endif
             !
                Timprn = 0.0d0
                pCoefa = 0.0d0; pCoefb = 0.0d0; pCoefg = 0.0d0
                pUxf   = 0.0d0; pUyf   = 0.0d0; pUzf   = 0.0d0
                pXpt   = 0.0d0; pYpt   = 0.0d0; pYpt   = 0.0d0
                pUxpt  = 0.0d0; pUypt  = 0.0d0; pUzpt  = 0.0d0
                pTimpt = 0.0d0; pRhopt = 0.0d0; pDiapt = 0.0d0
                pFUxpt = 0.0d0; pFUypt = 0.0d0; pFUzpt = 0.0d0
                pFdUxt = 0.0d0; pFdUyt = 0.0d0; pFdUzt = 0.0d0
                fFxpt = 0.0d0; fFypt = 0.0d0; fFzpt = 0.0d0
                pEwall = 0.0d0; dLpin = 0.0d0; yvis = 0.0d0
                plms = 0.0d0; plals=0.0d0; ndimpls=0; iplms=0
                icfpt = 0; jcfpt = 0; kcfpt = 0; iFPsrd = 0; iWPsrd=0; numw0=0
                iunLPT = 20 ! unit no. for output file
             elseif ( iset==2 )  then
                allocate( vxfcor(-1:nx+1,-1:ny+1,-1:nz+1,1:2), &
                          vyfcor(-1:nx+1,-1:ny+1,-1:nz+1,1:2), &
                          vzfcor(-1:nx+1,-1:ny+1,-1:nz+1,1:2), &
                          stat=err)
                if (err/=0) then
                   print *, "Fail to allocate the member of LPTval_v."
                   istat = 1
                   return
                end if
                vxfcor = 0.0d0; vyfcor = 0.0d0; vzfcor = 0.0d0
             endif
             !
         end subroutine AllocLPT
         !
         subroutine DeallocLPT(iset)
             implicit none
             integer(kind=4), intent(in) :: iset
             integer :: err  !! for ignoring errors.
             !
             if ( iset==1 )  then
                !! To deallocate all variables without error, they
                !! need to be deallocated one-by-one, because the
                !! deallocation will be performed only if specified
                !! all variables in `deallocate` statement are
                !! allocated.
                deallocate(pUxf, stat=err)
                deallocate(pUyf, stat=err)
                deallocate(pUzf, stat=err)
                deallocate(pXpt, stat=err)
                deallocate(pYpt, stat=err)
                deallocate(pZpt, stat=err)
                deallocate(pUxpt, stat=err)
                deallocate(pUypt, stat=err)
                deallocate(pUzpt, stat=err)
                deallocate(pTimpt, stat=err)
                deallocate(pRhopt, stat=err)
                deallocate(pDiapt, stat=err)
                deallocate(pFUxpt, stat=err)
                deallocate(pFUypt, stat=err)
                deallocate(pFUzpt, stat=err)
                deallocate(pFdUxt, stat=err)
                deallocate(pFdUyt, stat=err)
                deallocate(pFdUzt, stat=err)
                deallocate(fFxpt, stat=err)
                deallocate(fFypt, stat=err)
                deallocate(fFzpt, stat=err)
                deallocate(dLpin, stat=err)
                deallocate(pEwall, stat=err)
                deallocate(plms, stat=err)
                deallocate(plals, stat=err)
                deallocate(yvis, stat=err)
                deallocate(ndimpls, stat=err)
                deallocate(iplms, stat=err)
                deallocate(numw0, stat=err)
                deallocate(icfpt, stat=err)
                deallocate(jcfpt, stat=err)
                deallocate(kcfpt, stat=err)
                deallocate(iFPsrd, stat=err)
                deallocate(iWPsrd, stat=err)
#ifdef mpi
                deallocate(nptlvc, stat=err)
                deallocate(iptlvc, stat=err)
                deallocate(pPhi_t, stat=err)
#endif
             elseif ( iset==2 )  then
                deallocate(vxfcor, stat=err)
                deallocate(vyfcor, stat=err)
                deallocate(vzfcor, stat=err)
             endif
             !
         end subroutine DeallocLPT
#endif
 end module LPTval

