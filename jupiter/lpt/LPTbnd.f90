module cLPTbnd
  use ISO_C_BINDING
  use LPTtype

  implicit none

  type pnt2
     real(kind=8), pointer, dimension(:,:,:) :: p
     logical :: alloc
  end type pnt2

  integer, parameter :: fstm = 1, fstp = 1

  real(kind=8), allocatable, target, dimension(:,:,:) :: &
       fvxf1, fvyf1, fvzf1, frhof1, fvmuf1

  real(kind=8), allocatable, dimension(:) :: &
       fxfgrid, fyfgrid, fzfgrid

  type(pnt2) :: &
       fvxf2 = pnt2(null(),.false.), fvyf2 = pnt2(null(),.false.), &
       fvzf2 = pnt2(null(),.false.), frhof2 = pnt2(null(),.false.), &
       fvmuf2 = pnt2(null(),.false.)

  integer, parameter :: &
       IERR_ALLOCATION_FAILED = 1, &
       IERR_FILE_OPEN_FAILED = 2, &
       IERR_INQUIRE_UNIT_STATUS = 3, &
       IERR_ASSIGNMENT_WOULD_OVERFLOW = 4, &
       IERR_INVALID_ARGUMENT = 5, &
       IERR_NOT_ALLOCATED = 6

  type(c_funptr) :: error_callback = c_null_funptr
  type(c_ptr)    :: error_callback_argument = c_null_ptr

  abstract interface
     subroutine error_callback_interface(arg, ierr, iln, mesg) bind(c)
       use ISO_C_BINDING, only: c_ptr, c_int
       type(c_ptr), intent(in), value :: arg
       integer(kind=c_int), intent(in), value :: ierr, iln
       type(c_ptr), intent(in), value :: mesg
     end subroutine error_callback_interface
  end interface

  type(c_funptr) :: udwbc_callback = c_null_funptr
  type(c_ptr)    :: udwbc_callback_argument = c_null_ptr

  abstract interface
     subroutine udwbc_callback_interface(arg, ip, pt, put, pdia, icf, iwp, dl) bind(c)
       use ISO_C_BINDING, only: c_ptr, c_int
       use LPTtype, only: LPT_c_type
       type(c_ptr), intent(in), value :: arg
       integer(c_int), intent(in), value :: ip
       real(kind=LPT_c_type), intent(inout), dimension(3) :: pt, put
       real(kind=LPT_c_type), intent(in), value :: pdia
       integer(kind=c_int), intent(inout), dimension(3) :: icf
       integer(c_int), intent(out) :: iwp
       real(kind=LPT_c_type), intent(out) :: dl
     end subroutine udwbc_callback_interface
  end interface

contains
  subroutine cLPTset_error_callback(func, arg) &
       bind(c,name='cLPTset_error_callback')
    type(c_funptr), intent(in), value :: func
    type(c_ptr), intent(in), value :: arg
    error_callback = func
    error_callback_argument = arg
  end subroutine cLPTset_error_callback

  integer(kind=c_int) function cLPTmxpset() bind(c,name='cLPTmxpset')
    use LPTval, only: mxpset

    if (mxpset .gt. huge(cLPTmxpset)) then
       ! Assignment would overflow
       cLPTmxpset = huge(cLPTmxpset)
    else
       cLPTmxpset = mxpset
       if (cLPTmxpset .ne. mxpset) then
          cLPTmxpset = -1
       end if
    end if
    return
  end function cLPTmxpset

  integer(kind=c_int) function cLPTgetnpt() bind(c,name='cLPTgetnpt')
    use LPTval, only: npt
    cLPTgetnpt = npt
    return
  end function cLPTgetnpt

  integer(kind=c_int) function cLPTgetnpset() bind(c,name='cLPTgetnpset')
    use LPTval, only: npset
    cLPTgetnpset = npset
    return
  end function cLPTgetnpset

  subroutine cLPTsetnpset(nset, istat) bind(c,name='cLPTsetnpset')
    use LPTval, only: npset, mxpset
    integer(kind=c_int), value :: nset
    type(c_ptr), value :: istat

    if (nset .lt. 0) then
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='Number of particle set must be 0 or positive')
       return
    end if
    if (nset .gt. mxpset) then
       call lptbnd_error(istat, IERR_ASSIGNMENT_WOULD_OVERFLOW, &
            iovf=int(nset, kind=c_intmax_t))
       return
    end if

    npset = nset
    return
  end subroutine cLPTsetnpset

  integer(kind=c_int) function cLPTgetipttim() bind(c,name='cLPTgetipttim')
    use LPTval, only: ipttim
    cLPTgetipttim = ipttim
    return
  end function cLPTgetipttim

  subroutine cLPTsetipttim(ipt) bind(c,name='cLPTsetipttim')
    use LPTval, only: ipttim
    integer(kind=c_int), value :: ipt

    ipttim = ipt
    return
  end subroutine cLPTsetipttim

  real(kind=LPT_c_type) function cLPTgettip() bind(c,name='cLPTgettip')
    use LPTval, only: TipLPT
    cLPTgettip = TipLPT
    return
  end function cLPTgettip

  subroutine cLPTsettip(tip) bind(c,name='cLPTsettip')
    use LPTval, only: TipLPT
    real(kind=LPT_c_type), value :: tip
    TipLPT = tip
    return
  end subroutine cLPTsettip

  real(kind=LPT_c_type) function cLPTgettimprn() bind(c,name='cLPTgettimprn')
    use LPTval, only: Timprn
    cLPTgettimprn = Timprn
    return
  end function cLPTgettimprn

  integer(kind=c_int) function cLPTgetwbcal() bind(c,name='cLPTgetlptwbcal')
    use LPTval, only: LPTwbcal
    cLPTgetwbcal = LPTwbcal
    return
  end function cLPTgetwbcal

  subroutine cLPTsetwbcal(ival) bind(c,name='cLPTsetwbcal')
    use LPTval, only: LPTwbcal
    integer(kind=c_int), value :: ival
    LPTwbcal = ival
    return
  end subroutine cLPTsetwbcal

  subroutine cLPTsettimprn(prn) bind(c,name='cLPTsettimprn')
    use LPTval, only: Timprn
    real(kind=LPT_c_type), value :: prn
    Timprn = prn
    return
  end subroutine cLPTsettimprn

  subroutine cLPTopenlogfile(iun, lpath, cpathptr, iappend, istat) &
       bind(c,name='cLPTopenlogfile')
    use LPTval, only: iunLPT
    use ISO_FORTRAN_ENV, only: ERROR_UNIT, OUTPUT_UNIT

    integer(kind=c_int), value :: iun
    integer(kind=c_int), value :: lpath, iappend
    type(c_ptr), value :: cpathptr
    type(c_ptr), value :: istat
    character(len=1), dimension(:), pointer :: cpath => null()
    character(len=:), allocatable :: fpath
    integer :: iostat, i
    logical :: luse_default_name, lopend, lnamed

    if (iun .ge. 0) then
       iunLPT = iun
    end if
    if (iunLPT .eq. ERROR_UNIT .or. iunLPT .eq. OUTPUT_UNIT) then
       return
    end if

    if (iunLPT .lt. 0) then
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='The default unit number of logfile is invalid, ' // &
                 'and so must be given. This error is for development.')
       return
    end if

    luse_default_name = .true.
    if (c_associated(cpathptr)) then
       call c_f_pointer(cpathptr, cpath, (/ lpath /))
       do i = 1, lpath
          if (cpath(i) .eq. c_null_char) exit
       end do
       if (i .gt. 1) then
          i = i - 1
          allocate(character(len=i) :: fpath, stat=iostat)
          if (iostat .ne. 0) then
             call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=iostat)
             return
          end if
          fpath(1:i) = cpath(1)(1:i)
          luse_default_name = .false.
       end if
    end if

    inquire(unit=iunLPT, opened=lopend)
    if (lopend) close(iunLPT)
    if (luse_default_name) then
       if (iappend .ne. 0) then
          open(unit=iunLPT,form='formatted',status='old',action='write',position='append',iostat=iostat)
          if (iostat .ne. 0) then
             open(unit=iunLPT,form='formatted',status='unknown',action='write',position='append',iostat=iostat)
          end if
       else
          open(unit=iunLPT,form='formatted',status='unknown',action='write',position='rewind',iostat=iostat)
       end if
       if (iostat .ne. 0) then
          call lptbnd_error(istat, IERR_FILE_OPEN_FAILED, stat=iostat)
          return
       end if
       allocate(character(len=lpath) :: fpath, stat=iostat)
       if (iostat .ne. 0) then
          call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=iostat)
          return
       end if
       !! The result value of `lopend` is obvious. But it's not for
       !! `lnamed`, because the default name might not be obtained.
       inquire(unit=iunLPT, name=fpath, opened=lopend, &
            named=lnamed, iostat=iostat)
       if (iostat .ne. 0) then
          call lptbnd_error(istat, IERR_INQUIRE_UNIT_STATUS, stat=iostat)
          return
       end if
       if (lopend .and. lnamed) then
          if (c_associated(cpathptr)) then
             if (.not. associated(cpath)) then
                call c_f_pointer(cpathptr, cpath, (/ lpath /))
             end if
             i = len_trim(fpath)
             cpath(1)(1:i) = fpath(1:i)
             i = i + 1
             cpath(1)(i:i) = c_null_char
          end if
       end if
    else
       if (iappend .ne. 0) then
          open(unit=iunLPT,file=fpath,form='formatted',status='old',action='write',position='append',iostat=iostat)
          if (iostat .ne. 0) then
             open(unit=iunLPT,file=fpath,form='formatted',status='unknown',action='write',position='append',iostat=iostat)
          end if
       else
          open(unit=iunLPT,file=fpath,form='formatted',status='unknown',action='write',position='rewind',iostat=iostat)
       end if
       if (iostat .ne. 0) then
          call lptbnd_error(istat, IERR_FILE_OPEN_FAILED, &
               fname=fpath, stat=iostat)
          return
       end if
    end if
    return
  end subroutine cLPTopenlogfile

  integer(kind=c_int) function cLPTget_error_unit() bind(c,name='cLPTget_error_unit')
    use ISO_FORTRAN_ENV, only: ERROR_UNIT
    cLPTget_error_unit = ERROR_UNIT
    return
  end function cLPTget_error_unit

  integer(kind=c_int) function cLPTget_output_unit() bind(c,name='cLPTget_output_unit')
    use ISO_FORTRAN_ENV, only: OUTPUT_UNIT
    cLPTget_output_unit = OUTPUT_UNIT
    return
  end function cLPTget_output_unit

#ifdef JUPITER_LPT_USE_MPI
  subroutine cLPTsetmpivars(icomm) bind(c,name='cLPTsetmpivars')
#if defined(LPT_USE_MPI_F08_INTERFACE)
    use MPI_F08, only: MPI_comm_rank, MPI_comm_size, &
         MPI_COMM_NULL, MPI_COMM_WORLD, MPI_Comm
#else
    use MPI, only: MPI_COMM_NULL, MPI_COMM_WORLD
#endif
    use MT_Parallel_Attribute0

    !! On the C-side, MPI_Fint is used for icomm, which is guaranteed
    !! to be interoperable with Fortran default `integer` (note that
    !! it may not be `kind=4` nor `kind=c_int`) on the MPI standard.
    integer, value :: icomm
#if defined(LPT_USE_MPI_F08_INTERFACE)
    type(MPI_Comm) :: fcomm
#else
    integer :: fcomm
#endif
    integer :: ier

#if defined(LPT_USE_MPI_F08_INTERFACE)
    if (icomm .ne. MPI_COMM_NULL%MPI_VAL) then
       fcomm%MPI_VAL = icomm
    else
       fcomm = MPI_COMM_WORLD
    end if
#else
    if (icomm .ne. MPI_COMM_NULL) then
       fcomm = icomm
    else
       fcomm = MPI_COMM_WORLD
    end if
#endif

    call MPI_comm_size(fcomm, numrank, ier)
    call MPI_comm_rank(fcomm, myrank, ier)
    lpt_comm = fcomm
  end subroutine cLPTsetmpivars
#endif

  subroutine cLPTsetmpineighbors(zm,zp,ym,yp,xm,xp) &
       bind(c,name='cLPTsetmpineighbors')
    use MT_Parallel_Attribute0

    integer(kind=c_int), value :: zm, zp, ym, yp, xm, xp

    nrk(1) = zm
    nrk(2) = zp
    nrk(3) = ym
    nrk(4) = yp
    nrk(5) = xm
    nrk(6) = xp
    return
  end subroutine cLPTsetmpineighbors

  subroutine cLPTalloc(iset,npta,nx,ny,nz,istat) bind(c,name='cLPTalloc')
    use LPTval, only: AllocLPT, npt

    integer(kind=c_int), value :: nx, ny, nz, npta, iset
    type(c_ptr), value :: istat
    integer(kind=4) :: iastat

    iastat = 0
    call AllocLPT(iset, npta, nx, ny, nz, iastat)

    if (iastat .ne. 0) then
       call lptbnd_error(istat, IERR_ALLOCATION_FAILED)
    end if

    npt = npta
    return
  end subroutine cLPTalloc

  subroutine cLPTdealloc(iset) bind(c,name='cLPTdealloc')
    use LPTval, only: DeallocLPT

    integer(kind=c_int), value :: iset

    call DeallocLPT(iset)
    return
  end subroutine cLPTdealloc

  subroutine cLPTsetpset(iset,npt,itr,xs,xe,ys,ye,zs,ze,tms,tme, &
       di,ri,ux,uy,uz,istat) bind(c,name='cLPTsetpset')
    use LPTval, only: mxpset, nistpt, itrdm
    use LPTval, only: psetXs, psetXe, psetYs, psetYe, psetZs, psetZe
    use LPTval, only: psetTms, psetTme, psetDi, psetRi
    use LPTval, only: psetUx, psetUy, psetUz

    integer(kind=c_int), value :: iset, npt, itr
    real(kind=LPT_c_type), value :: xs, xe, ys, ye, zs, ze, tms, tme, di, ri, ux, uy, uz
    type(c_ptr), value :: istat

    if (iset .lt. 1 .or. iset .gt. mxpset) then
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='Invalid set index number given')
       return
    end if

    nistpt(iset) = npt
    itrdm(iset) = itr
    psetXs(iset) = xs
    psetXe(iset) = xe
    psetYs(iset) = ys
    psetYe(iset) = ye
    psetZs(iset) = zs
    psetZe(iset) = ze
    psetTms(iset) = tms
    psetTme(iset) = tme
    psetDi(iset) = di
    psetRi(iset) = ri
    psetUx(iset) = ux
    psetUy(iset) = uy
    psetUz(iset) = uz
    return
  end subroutine cLPTsetpset

  subroutine cLPTgetpset(iset,npt,itr,xs,xe,ys,ye,zs,ze,tms,tme, &
       di,ri,ux,uy,uz,istat) bind(c,name='cLPTgetpset')
    use LPTval, only: mxpset, nistpt, itrdm
    use LPTval, only: psetXs, psetXe, psetYs, psetYe, psetZs, psetZe
    use LPTval, only: psetTms, psetTme, psetDi, psetRi
    use LPTval, only: psetUx, psetUy, psetUz

    integer(kind=c_int), value :: iset
    type(c_ptr), value :: npt, itr
    type(c_ptr), value :: xs, xe, ys, ye, zs, ze, tms, tme, di, ri, ux, uy, uz
    type(c_ptr), value :: istat

    real(kind=LPT_c_type), pointer :: frp

    if (iset .lt. 1 .or. iset .gt. mxpset) then
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='Invalid set index number given')
       return
    end if

    call getivar(npt, nistpt, iset)
    call getivar(itr, itrdm, iset)
    call getrvar(xs, psetXs, iset)
    call getrvar(xe, psetXe, iset)
    call getrvar(ys, psetYs, iset)
    call getrvar(ye, psetYe, iset)
    call getrvar(zs, psetZs, iset)
    call getrvar(ze, psetZe, iset)
    call getrvar(tms, psetTms, iset)
    call getrvar(tme, psetTme, iset)
    call getrvar(di, psetDi, iset)
    call getrvar(ri, psetRi, iset)
    call getrvar(ux, psetUx, iset)
    call getrvar(uy, psetUy, iset)
    call getrvar(uz, psetUz, iset)
    return

  contains
    subroutine getivar(cip, farr, iset)
      type(c_ptr), intent(in) :: cip
      integer(kind=c_int), intent(in) :: iset
      integer(kind=4), dimension(:), intent(in) :: farr
      integer(kind=c_int), pointer :: fp

      if (c_associated(cip)) then
         call c_f_pointer(cip, fp)
         fp = farr(iset)
      end if
      return
    end subroutine getivar

    subroutine getrvar(cip, farr, iset)
      type(c_ptr), intent(in) :: cip
      integer(kind=c_int), intent(in) :: iset
      real(kind=8), dimension(:), intent(in) :: farr
      real(kind=LPT_c_type), pointer :: fp

      if (c_associated(cip)) then
         call c_f_pointer(cip, fp)
         fp = farr(iset)
      end if
      return
    end subroutine getrvar
  end subroutine cLPTgetpset

  !! Expands array of particle index ranges to array of particle indices
  subroutine build_idxtab(nidxs, idxs, idxtab, ntot, istat, fstat)
    use LPTval, only: npt
    integer(kind=c_size_t), intent(in) :: nidxs
    integer(kind=c_size_t), dimension(nidxs), intent(in) ::idxs
    integer(kind=c_size_t), intent(out) :: ntot
    integer(kind=c_size_t), dimension(:), allocatable, intent(out) :: idxtab
    type(c_ptr) :: istat
    integer, intent(out) :: fstat
    integer(kind=c_size_t) :: i, j, k, npts, ips, ipe, nidxsh
    integer(kind=c_size_t), dimension(:,:), allocatable :: iwk

    ntot = 0
    fstat = 0
    if (allocated(idxtab)) deallocate(idxtab)

    nidxsh = 0
    if (nidxs .lt. 1) return
    if (nidxs .eq. 1) then
       ntot = 1
       if (.not. is_valid_idx(idxs(1))) ntot = -1
    else
       nidxsh = nidxs / 2
       allocate(iwk(nidxsh, 2), stat=fstat)
       ntot = 0
       do i = 1, nidxsh
          if (allocated(iwk)) iwk(i, 1) = ntot + 1
          ips = idxs(2 * i - 1)
          ipe = idxs(2 * i)
          npts = ipe - ips + 1
          if (.not. is_valid_idx(ips)) npts = -1
          if (.not. is_valid_idx(ipe)) npts = -1
          if (npts .gt. 0) then
             if (ntot + npts .lt. ntot) then
                ntot = -1
                exit
             end if
             ntot = ntot + npts
          else
             ntot = -1
             exit
          end if
          if (allocated(iwk)) iwk(i, 2) = npts
       end do
       if (.not. allocated(iwk)) return
    end if
    if (ntot .lt. 0) then
       fstat = 1
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='No particles to be set or get their parameters')
       return
    end if
    if (ntot .eq. 0) return

    allocate(idxtab(ntot), stat=fstat)
    if (fstat .ne. 0) then
       call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=fstat)
       return
    end if

    if (nidxs .eq. 1) then
       idxtab(1) = idxs(1)
    else
       !$omp parallel private(i, j, k)
       i = -1
       !$omp do
       do k = 1, ntot
          if (i .eq. -1) then
             do j = nidxsh, 1, -1
                if (iwk(j, 1) .le. k) then
                   i = j
                   exit
                end if
             end do
          end if
          if (i .eq. -1) then
             idxtab(k) = 0
             !$omp atomic write
             fstat = 1
             cycle
          end if
          j = k - iwk(i, 1)
          if (j .ge. iwk(i, 2)) then
             i = i + 1
             if (i .le. nidxsh) then
                j = k - iwk(i, 1)
             else
                j = -1
             end if
          end if
          if (j .lt. 0) then
             idxtab(k) = 0
             !$omp atomic write
             fstat = 1
          else
             idxtab(k) = idxs(2 * i - 1) + j
          end if
       end do
       !$omp end do
       !$omp end parallel
       if (fstat .ne. 0) then
          call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
               desc='Consistency error occured')
       end if
    end if
    return

  contains
    logical function is_valid_idx(idx)
      integer(kind=c_size_t), intent(in) :: idx

      is_valid_idx = (idx .ge. 1 .and. idx .le. npt)
      return
    end function is_valid_idx
  end subroutine build_idxtab

  subroutine cLPTsetpts(nidxs,idxs,uxf,uyf,uzf,xpt,ypt,zpt,uxpt,uypt,uzpt, &
       timpt,rhopt,diapt,fuxpt,fuypt,fuzpt,fduxt,fduyt,fduzt, &
       icpt,jcpt,kcpt,ifps,iwps,istat) bind(c, name='cLPTsetpts')
    use LPTval, only: npt, &
         pUxf,   pUyf,   pUzf,   pXpt,   pYpt,   pZpt, &
         pUxpt,  pUypt,  pUzpt,  pTimpt, pRhopt, pDiapt, &
         pFUxpt, pFUypt, pFUzpt, pFdUxt, pFdUyt, pFdUzt, &
         icfpt,  jcfpt,  kcfpt,  iFPsrd, iWPsrd

    integer(kind=c_size_t), value :: nidxs
    integer(kind=c_size_t), dimension(nidxs), intent(in) :: idxs
    type(c_ptr), value :: uxf, uyf, uzf, xpt, ypt, zpt, uxpt, uypt, uzpt
    type(c_ptr), value :: timpt, rhopt, diapt, fuxpt, fuypt, fuzpt
    type(c_ptr), value :: fduxt, fduyt, fduzt, icpt, jcpt, kcpt, ifps, iwps
    type(c_ptr), value :: istat

    integer(kind=c_size_t) :: ntot
    integer(kind=c_size_t), dimension(:), allocatable :: idxtab
    integer :: iastat = 0

    call build_idxtab(nidxs, idxs, idxtab, ntot, istat, iastat)
    if (iastat .ne. 0) then
       return
    end if
    if (ntot .le. 0) then
       return
    end if

    call setrvar(pUxf, uxf)
    call setrvar(pUyf, uyf)
    call setrvar(pUzf, uzf)
    call setrvar(pXpt, xpt)
    call setrvar(pYpt, ypt)
    call setrvar(pZpt, zpt)
    call setrvar(pUxpt, uxpt)
    call setrvar(pUypt, uypt)
    call setrvar(pUzpt, uzpt)
    call setrvar(pTimpt, timpt)
    call setrvar(pRhopt, rhopt)
    call setrvar(pDiapt, diapt)
    call setrvar(pFUxpt, fuxpt)
    call setrvar(pFUypt, fuypt)
    call setrvar(pFUzpt, fuzpt)
    call setrvar(pFdUxt, fduxt)
    call setrvar(pFdUyt, fduyt)
    call setrvar(pFdUzt, fduzt)
    call setivar(icfpt, icpt)
    call setivar(jcfpt, jcpt)
    call setivar(kcfpt, kcpt)
    call setivar(iFPsrd, ifps)
    call setivar(iWPsrd, iwps)
    return
  contains
    subroutine setrvar(dst, csrc)
      real(kind=8), allocatable, dimension(:) :: dst
      type(c_ptr) :: csrc
      real(kind=LPT_c_type), dimension(:), pointer :: src
      integer(kind=c_size_t) :: i, j

      if (c_associated(csrc)) then
         if (.not. allocated(dst)) then
            call lptbnd_error(istat, IERR_NOT_ALLOCATED)
            return
         end if

         call c_f_pointer(csrc, src, (/ ntot /))

         !$omp parallel do private(i, j)
         do i = 1, ntot
            j = idxtab(i)
            dst(j) = src(i)
         end do
         !$omp end parallel do
      end if
    end subroutine setrvar

    subroutine setivar(dst, csrc)
      integer(kind=4), allocatable, dimension(:) :: dst
      type(c_ptr) :: csrc
      integer(kind=c_int), dimension(:), pointer :: src
      integer(kind=c_size_t) :: i, j

      if (c_associated(csrc)) then
         if (.not. allocated(dst)) then
            call lptbnd_error(istat, IERR_NOT_ALLOCATED)
            return
         end if

         call c_f_pointer(csrc, src, (/ ntot /))

         !$omp parallel do private(i, j)
         do i = 1, ntot
            j = idxtab(i)
            dst(j) = src(i)
         end do
         !$omp end parallel do
      end if
    end subroutine setivar
  end subroutine cLPTsetpts

  subroutine cLPTgetpts(nidxs,idxs,uxf,uyf,uzf,xpt,ypt,zpt,uxpt,uypt,uzpt, &
       timpt,rhopt,diapt,fuxpt,fuypt,fuzpt,fduxt,fduyt,fduzt, &
       icpt,jcpt,kcpt,ifps,iwps,istat) bind(c, name='cLPTgetpts')
    use LPTval, only: npt, &
         pUxf,   pUyf,   pUzf,   pXpt,   pYpt,   pZpt, &
         pUxpt,  pUypt,  pUzpt,  pTimpt, pRhopt, pDiapt, &
         pFUxpt, pFUypt, pFUzpt, pFdUxt, pFdUyt, pFdUzt, &
         icfpt,  jcfpt,  kcfpt,  iFPsrd, iWPsrd

    integer(kind=c_size_t), value :: nidxs
    integer(kind=c_size_t), dimension(nidxs), intent(in) :: idxs
    type(c_ptr), value :: uxf, uyf, uzf, xpt, ypt, zpt, uxpt, uypt, uzpt
    type(c_ptr), value :: timpt, rhopt, diapt, fuxpt, fuypt, fuzpt
    type(c_ptr), value :: fduxt, fduyt, fduzt, icpt, jcpt, kcpt, ifps, iwps
    type(c_ptr), value :: istat

    integer(kind=c_size_t) :: ntot
    integer(kind=c_size_t), dimension(:), allocatable :: idxtab
    integer :: iastat = 0

    call build_idxtab(nidxs, idxs, idxtab, ntot, istat, iastat)
    if (iastat .ne. 0) then
       return
    end if
    if (ntot .le. 0) then
       return
    end if

    call getrvar(pUxf, uxf)
    call getrvar(pUyf, uyf)
    call getrvar(pUzf, uzf)
    call getrvar(pXpt, xpt)
    call getrvar(pYpt, ypt)
    call getrvar(pZpt, zpt)
    call getrvar(pUxpt, uxpt)
    call getrvar(pUypt, uypt)
    call getrvar(pUzpt, uzpt)
    call getrvar(pTimpt, timpt)
    call getrvar(pRhopt, rhopt)
    call getrvar(pDiapt, diapt)
    call getrvar(pFUxpt, fuxpt)
    call getrvar(pFUypt, fuypt)
    call getrvar(pFUzpt, fuzpt)
    call getrvar(pFdUxt, fduxt)
    call getrvar(pFdUyt, fduyt)
    call getrvar(pFdUzt, fduzt)
    call getivar(icfpt, icpt)
    call getivar(jcfpt, jcpt)
    call getivar(kcfpt, kcpt)
    call getivar(iFPsrd, ifps)
    call getivar(iWPsrd, iwps)
    return
  contains
    subroutine getrvar(src, cdst)
      real(kind=8), allocatable, dimension(:) :: src
      type(c_ptr) :: cdst
      real(kind=LPT_c_type), dimension(:), pointer :: dst
      integer(kind=c_size_t) :: i, j

      if (c_associated(cdst)) then
         if (.not. allocated(src)) then
            call lptbnd_error(istat, IERR_NOT_ALLOCATED)
            return
         end if

         call c_f_pointer(cdst, dst, (/ ntot /))

         !$omp parallel do private(i, j)
         do i = 1, ntot
            j = idxtab(i)
            dst(i) = src(j)
         end do
         !$omp end parallel do
      end if
    end subroutine getrvar

    subroutine getivar(src, cdst)
      integer(kind=4), allocatable, dimension(:) :: src
      type(c_ptr) :: cdst
      integer(kind=c_int), dimension(:), pointer :: dst
      integer(kind=c_size_t) :: i, j

      if (c_associated(cdst)) then
         if (.not. allocated(src)) then
            call lptbnd_error(istat, IERR_NOT_ALLOCATED)
            return
         end if

         call c_f_pointer(cdst, dst, (/ ntot /))

         !$omp parallel do private(i, j)
         do i = 1, ntot
            j = idxtab(i)
            dst(i) = src(j)
         end do
         !$omp end parallel do
      end if
    end subroutine getivar
  end subroutine cLPTgetpts

  subroutine cLPTsetfield(ivar,input,nx,ny,nz,stm,stp,idxs,nidxs,istat) &
       bind(c,name='cLPTsetfield_unchecked')
    use LPTval, only: pEwall, plms, plals, yvis, ndimpls, iplms, numw0
    use LPTval, only: vxfcor, vyfcor, vzfcor

    integer(kind=c_int), value :: ivar, nx, ny, nz, stm, stp, nidxs
    type(c_ptr), value :: input, idxs, istat

    ! see LPT_realfield and LPT_intfield enums in LPTbnd.h
    select case(ivar)
    case(  1)
       call setrealfield(.false., .false., .false., pEwall)
    case(  2)
       call setrealfield(.false., .false., .false., plms(:,:,:,1))
    case(  3)
       call setrealfield(.false., .false., .false., plms(:,:,:,2))
    case(  4)
       call setrealfield(.false., .false., .false., plms(:,:,:,3))
    case(  5)
       call setrealfield(.false., .false., .false., plals)
    case(  6)
       call setrealfield(.false., .false., .false., yvis)
    case(  7)
       call setrealfield(.true., .true., .true., vxfcor(:,:,:,1))
    case(  8)
       call setrealfield(.true., .true., .true., vxfcor(:,:,:,2))
    case(  9)
       call setrealfield(.true., .true., .true., vyfcor(:,:,:,1))
    case( 10)
       call setrealfield(.true., .true., .true., vyfcor(:,:,:,2))
    case( 11)
       call setrealfield(.true., .true., .true., vzfcor(:,:,:,1))
    case( 12)
       call setrealfield(.true., .true., .true., vzfcor(:,:,:,2))
    case(101)
       call setintfield(.false., .false., .false., ndimpls)
    case(102)
       call setintfield(.false., .false., .false., iplms(:,:,:,1))
    case(103)
       call setintfield(.false., .false., .false., iplms(:,:,:,2))
    case(104)
       call setintfield(.false., .false., .false., iplms(:,:,:,3))
    case(105)
       call setintfield(.false., .false., .false., numw0)
    case default
       call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
            desc='Invalid field variable specified')
    end select

    return
  contains
    subroutine setup_idxs(is_inode,is_jnode,is_knode,fdim,cdim,cidxs,didxs,nregs)
      use ISO_FORTRAN_ENV, only: ERROR_UNIT
      logical, intent(in) :: is_inode, is_jnode, is_knode
      integer, dimension(3,2), intent(in) :: fdim
      integer(kind=c_int), dimension(3), intent(in) :: cdim
      integer(kind=c_int), dimension(:), pointer, intent(out) :: cidxs
      integer(kind=c_int), dimension(6), target, intent(out) :: didxs
      integer(kind=c_int), intent(out) :: nregs
      integer(kind=c_int), dimension(3) :: ifstm
      integer(kind=c_int), dimension(3) :: n

      cidxs => null()
      didxs = 0
      nregs = 0

      ifstm = fstm
      if (is_inode) ifstm(1) = ifstm(1) + 1
      if (is_jnode) ifstm(2) = ifstm(2) + 1
      if (is_knode) ifstm(3) = ifstm(3) + 1

      n(1) = nx
      n(2) = ny
      n(3) = nz
      if (any(fdim(:,2) - fdim(:,1) + 1 - ifstm - fstp .ne. n)) then
         call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
              desc='Assertion failed: Allocated fortran and ' // &
                   'C dimension does not match!')
         return
      end if

      if (c_associated(idxs) .and. nidxs .gt. 0) then
         call c_f_pointer(idxs, cidxs, (/ nidxs /))
         nregs = nidxs / 6
      else
         didxs(1) = stm - min(ifstm(1), stm)
         didxs(2) = stm + nx + min(fstp, stp) - 1
         didxs(3) = stm - min(ifstm(2), stm)
         didxs(4) = stm + ny + min(fstp, stp) - 1
         didxs(5) = stm - min(ifstm(3), stm)
         didxs(6) = stm + nz + min(fstp, stp) - 1
         cidxs => didxs
         nregs = 1
      end if

      return
    end subroutine setup_idxs

    subroutine setrealfield(is_inode,is_jnode,is_knode,output)
      logical, intent(in) :: is_inode, is_jnode, is_knode
      real(kind=8), dimension(:,:,:), intent(inout) :: output
      integer(kind=c_int), dimension(:), pointer :: cidxs
      real(kind=LPT_c_type), dimension(:), pointer :: cval
      integer(kind=c_int), dimension(3) :: cdim
      integer(kind=c_intptr_t) :: nsz, jj
      integer(kind=c_int) :: nregs, ireg, i, j, k, is, ie, js, je, ks, ke
      integer(kind=c_int) :: it, jt, kt
      integer, dimension(3,2) :: fdim
      integer(kind=c_int), dimension(6), target :: didxs

      if (.not. c_associated(input)) then
         call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
              desc='NULL pointer given for field varaible data')
         return
      end if

      fdim(:,1) = lbound(output)
      fdim(:,2) = ubound(output)

      call get_pointer_r_3d(cval, nsz, cdim, input, nx, ny, nz, stm, stp)
      call setup_idxs(is_inode,is_jnode,is_knode,fdim,cdim,cidxs,didxs,nregs)
      if (.not. associated(cidxs)) return

      !$omp parallel private(i, j, k, it, jt, kt, jj, ireg)
      do ireg = 1, nregs
         !$omp barrier
         !$omp single
         is = cidxs((ireg - 1) * 6 + 1)
         ie = cidxs((ireg - 1) * 6 + 2)
         js = cidxs((ireg - 1) * 6 + 3)
         je = cidxs((ireg - 1) * 6 + 4)
         ks = cidxs((ireg - 1) * 6 + 5)
         ke = cidxs((ireg - 1) * 6 + 6)
         !$omp end single
         !$omp do collapse(3)
         do i = is, ie
            do j = js, je
               do k = ks, ke
                  it = i - stm + fdim(1, 1) + fstm
                  jt = j - stm + fdim(2, 1) + fstm
                  kt = k - stm + fdim(3, 1) + fstm
                  jj = k
                  jj = jj * cdim(2)
                  jj = jj + j
                  jj = jj * cdim(1)
                  jj = jj + i
                  output(it, jt, kt) = cval(jj + 1)
               end do
            end do
         end do
         !$omp end do
      end do
      !$omp end parallel

      return
    end subroutine setrealfield

    subroutine setintfield(is_inode,is_jnode,is_knode,output)
      logical, intent(in) :: is_inode, is_jnode, is_knode
      integer(kind=4), dimension(:,:,:), intent(inout) :: output
      integer(kind=c_int), dimension(:), pointer :: cidxs
      integer(kind=c_int), dimension(:), pointer :: cval
      integer(kind=c_int), dimension(3) :: cdim
      integer(kind=c_intptr_t) :: nsz, jj
      integer(kind=c_int) :: nregs, ireg, i, j, k, is, ie, js, je, ks, ke
      integer(kind=c_int) :: it, jt, kt
      integer, dimension(3,2) :: fdim
      integer(kind=c_int), dimension(6), target :: didxs

      if (.not. c_associated(input)) then
         call lptbnd_error(istat, IERR_INVALID_ARGUMENT, &
              desc='NULL pointer given for field varaible data')
         return
      end if

      fdim(:,1) = lbound(output)
      fdim(:,2) = ubound(output)

      call get_pointer_i_3d(cval, nsz, cdim, input, nx, ny, nz, stm, stp)
      call setup_idxs(is_inode,is_jnode,is_knode,fdim,cdim,cidxs,didxs,nregs)
      if (.not. associated(cidxs)) return

      !$omp parallel private(i, j, k, it, jt, kt, jj, ireg)
      do ireg = 1, nregs
         !$omp barrier
         !$omp single
         is = cidxs((ireg - 1) * 6 + 1)
         ie = cidxs((ireg - 1) * 6 + 2)
         js = cidxs((ireg - 1) * 6 + 3)
         je = cidxs((ireg - 1) * 6 + 4)
         ks = cidxs((ireg - 1) * 6 + 5)
         ke = cidxs((ireg - 1) * 6 + 6)
         !$omp end single
         !$omp do collapse(3)
         do i = is, ie
            do j = js, je
               do k = ks, ke
                  it = i - stm + fdim(1, 1) + fstm
                  jt = j - stm + fdim(2, 1) + fstm
                  kt = k - stm + fdim(3, 1) + fstm
                  jj = k
                  jj = jj * cdim(2)
                  jj = jj + j
                  jj = jj * cdim(1)
                  jj = jj + i
                  output(it, jt, kt) = cval(jj + 1)
               end do
            end do
         end do
         !$omp end do
      end do
      !$omp end parallel

      return
    end subroutine setintfield
  end subroutine cLPTsetfield

  subroutine cLPTcal0(xfgrid,yfgrid,zfgrid,time,nx,ny,nz,stm,stp,istat) &
       bind(c,name='cLPTcal0')

    interface
       subroutine LPTcal0(xfgrid,yfgrid,zfgrid,time,nx,ny,nz)
         integer(kind=4) :: nx,ny,nz
         real(kind=8) :: time
         real(kind=8) :: xfgrid(-1:nx+1),yfgrid(-1:ny+1),zfgrid(-1:nz+1)
       end subroutine LPTcal0
    end interface

    integer(kind=c_int), value :: nx, ny, nz, stm, stp
    type(c_ptr), value :: xfgrid, yfgrid, zfgrid
    type(c_ptr), value :: istat
    real(kind=LPT_c_type), value :: time
    real(kind=8) :: time8
    integer :: iastat

    iastat = 0
    call setup_grid_points(xfgrid, fxfgrid, nx, fstm, fstp, stm, stp, istat, iastat)
    call setup_grid_points(yfgrid, fyfgrid, ny, fstm, fstp, stm, stp, istat, iastat)
    call setup_grid_points(zfgrid, fzfgrid, nz, fstm, fstp, stm, stp, istat, iastat)
    if (iastat .ne. 0) then
       return
    end if

    time8 = time
    call LPTcal0(fxfgrid,fyfgrid,fzfgrid,time8,nx,ny,nz)
    return
  end subroutine cLPTcal0

  subroutine cLPTcal(istep,vxf1,vyf1,vzf1,vxf2,vyf2,vzf2,rhof1,rhof2, &
       vmuf1,vmuf2,xfgrid,yfgrid,zfgrid,gx,gy,gz,dt,  &
       time,nx,ny,nz,stm,stp,istat) bind(c,name='cLPTcal')
    implicit none

    interface
       subroutine LPTcal(istep,vxf1,vyf1,vzf1,vxf2,vyf2,vzf2,rhof1,rhof2, &
            vmuf1,vmuf2,xfgrid,yfgrid,zfgrid,gx,gy,gz,dt,time,nx,ny,nz)
         integer(kind=4) :: istep,nx,ny,nz
         real(kind=8) :: gx, gy, gz, dt, time
         real(kind=8) :: vxf1(-1:nx+1, 0:ny+1, 0:nz+1), &
              vyf1( 0:nx+1,-1:ny+1, 0:nz+1), &
              vzf1( 0:nx+1, 0:ny+1,-1:nz+1), &
              vxf2(-1:nx+1, 0:ny+1, 0:nz+1), &
              vyf2( 0:nx+1,-1:ny+1, 0:nz+1), &
              vzf2( 0:nx+1, 0:ny+1,-1:nz+1), &
              rhof1(0:nx+1,0:ny+1,0:nz+1), &
              rhof2(0:nx+1,0:ny+1,0:nz+1), &
              vmuf1(0:nx+1,0:ny+1,0:nz+1), &
              vmuf2(0:nx+1,0:ny+1,0:nz+1), &
              xfgrid(-1:nx+1),yfgrid(-1:ny+1),zfgrid(-1:nz+1)
       end subroutine LPTcal
    end interface

    integer(kind=c_int), value :: istep, nx, ny, nz, stm, stp
    type(c_ptr), value :: vxf1, vyf1, vzf1, vxf2, vyf2, vzf2, &
         rhof1, rhof2, vmuf1, vmuf2, xfgrid, yfgrid, zfgrid
    real(kind=LPT_c_type), value :: gx, gy, gz, dt, time
    real(kind=8) :: gx8, gy8, gz8, dt8, time8
    type(c_ptr), value :: istat
    integer :: iastat = 0


    call setup_variable_3d(vxf1, vxf2, fvxf1, fvxf2, nx, ny, nz, &
         fstm, fstp, stm, stp, 1, istat, iastat)
    call setup_variable_3d(vyf1, vyf2, fvyf1, fvyf2, nx, ny, nz, &
         fstm, fstp, stm, stp, 2, istat, iastat)
    call setup_variable_3d(vzf1, vzf2, fvzf1, fvzf2, nx, ny, nz, &
         fstm, fstp, stm, stp, 3, istat, iastat)
    call setup_variable_3d(rhof1, rhof2, frhof1, frhof2, nx, ny, nz, &
         fstm, fstp, stm, stp, 0, istat, iastat)
    call setup_variable_3d(vmuf1, vmuf2, fvmuf1, fvmuf2, nx, ny, nz, &
         fstm, fstp, stm, stp, 0, istat, iastat)
    call setup_grid_points(xfgrid, fxfgrid, nx, fstm, fstp, stm, stp, istat, iastat)
    call setup_grid_points(yfgrid, fyfgrid, ny, fstm, fstp, stm, stp, istat, iastat)
    call setup_grid_points(zfgrid, fzfgrid, nz, fstm, fstp, stm, stp, istat, iastat)

    if (iastat .ne. 0) then
       return
    end if

    gx8 = gx
    gy8 = gy
    gz8 = gz
    time8 = time
    dt8 = dt
    call LPTcal(istep, fvxf1, fvyf1, fvzf1, fvxf2%p, fvyf2%p, fvzf2%p, &
         frhof1, frhof2%p, fvmuf1, fvmuf2%p, fxfgrid, fyfgrid, fzfgrid, &
         gx8, gy8, gz8, dt8, time8, nx, ny, nz)

    return
  end subroutine cLPTcal

  logical function size_changed(arry,nx,ny,nz)
    real(kind=8), dimension(:,:,:) :: arry
    integer(kind=c_int) :: nx, ny, nz
    integer, dimension(3) :: ubnd

    size_changed = .true.
    ubnd = ubound(arry)
    if (ubnd(1) .ne. nx + 1) then
       size_changed = .false.
       return
    end if
    if (ubnd(2) .ne. ny + 1) then
       size_changed = .false.
       return
    end if
    if (ubnd(3) .ne. nz + 1) then
       size_changed = .false.
       return
    end if
    return
  end function size_changed

  subroutine compute_bounds(nx,ny,nz,fstm,fstp,ityp,lbnd,ubnd)
    integer(kind=c_int), intent(in) :: nx, ny, nz
    integer, intent(in) :: ityp, fstm, fstp
    integer, intent(out) :: lbnd(3), ubnd(3)

    lbnd = 1 - fstm
    if (ityp .ge. 1 .and. ityp .le. 3) then
       lbnd(ityp) = lbnd(ityp) - 1
    end if
    ubnd(1) = nx + fstp
    ubnd(2) = ny + fstp
    ubnd(3) = nz + fstp
    return
  end subroutine compute_bounds

  subroutine reallocate_if_needed_p(arry,nx,ny,nz,fstm,fstp,ityp,istat,stat)
    type(pnt2) :: arry
    integer(kind=c_int) :: nx, ny, nz
    type(c_ptr) :: istat
    integer :: stat
    integer :: ityp, fstm, fstp, lbnd(3), ubnd(3), iastat

    if (associated(arry%p)) then
       if (size_changed(arry%p, nx, ny, nz)) then
          if (arry%alloc) then
             deallocate(arry%p)
          else
             arry%p => null()
          end if
       end if
    end if
    if (.not. associated(arry%p)) then
       call compute_bounds(nx,ny,nz,fstm,fstp,ityp,lbnd,ubnd)
       allocate(arry%p(lbnd(1):ubnd(1),lbnd(2):ubnd(2),lbnd(3):ubnd(3)), stat=iastat)
       if (iastat .eq. 0) then
          arry%alloc = .true.
       else
          arry%alloc = .false.
          stat = 1
          call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=iastat)
       end if
    end if

    return
  end subroutine reallocate_if_needed_p

  subroutine reallocate_if_needed(arry,nx,ny,nz,fstm,fstp,ityp,istat,stat)
    real(kind=8), dimension(:,:,:), allocatable :: arry
    integer(kind=c_int) :: nx, ny, nz
    type(c_ptr) :: istat
    integer :: stat
    integer :: ityp, fstm, fstp, lbnd(3), ubnd(3), iastat

    if (allocated(arry)) then
       if (size_changed(arry, nx, ny, nz)) then
          deallocate(arry)
       end if
    end if
    if (.not. allocated(arry)) then
       call compute_bounds(nx,ny,nz,fstm,fstp,ityp,lbnd,ubnd)
       allocate(arry(lbnd(1):ubnd(1),lbnd(2):ubnd(2),lbnd(3):ubnd(3)), stat=iastat)
       if (iastat .ne. 0) then
          stat = 1
          call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=iastat)
       end if
    end if
    return
  end subroutine reallocate_if_needed

  subroutine jupiter_lpt_copy(farry, carry, cstm, cstp, fstm, fstp, nx, ny, nz, ityp)
    real(kind=8), dimension(:,:,:) :: farry
    real(kind=LPT_c_type), dimension(:) :: carry
    integer(kind=c_int) :: nx, ny, nz, cstm, cstp, fstm, fstp
    integer :: lbnd(3), ubnd(3), nca(3), lfbnd(3), ufbnd(3), ityp
    integer :: i, j, k, it, jt, kt
    integer(kind=c_intptr_t) :: jj

    lfbnd = lbound(farry)

    lbnd = cstm
    ubnd(1) = lbnd(1) + nx
    ubnd(2) = lbnd(2) + ny
    ubnd(3) = lbnd(3) + nz
    nca = ubnd + cstp

    lbnd = lbnd + 1 - fstm
    if (ityp .ge. 1 .and. ityp .le. 3) then
       lbnd(ityp) = lbnd(ityp) - 1
    end if
    ubnd = ubnd + fstp

    !$omp parallel private(i, j, k, it, jt, kt, jj)
    !$omp do collapse(3)
    do k = lbnd(3), ubnd(3)
       do j = lbnd(2), ubnd(2)
          do i = lbnd(1), ubnd(1)
             it = i - lbnd(1) + lfbnd(1)
             jt = j - lbnd(2) + lfbnd(2)
             kt = k - lbnd(3) + lfbnd(3)
             jj = k - 1
             jj = jj * nca(2)
             jj = jj + j - 1
             jj = jj * nca(1)
             jj = jj + i
             farry(it, jt, kt) = carry(jj)
          end do
       end do
    end do
    !$omp end do
    !$omp end parallel

    return
  end subroutine jupiter_lpt_copy

  subroutine get_pointer_r_3d(outp, nsz, cdim, inp, nx, ny, nz, stm, stp)
    real(kind=LPT_c_type), dimension(:), pointer, intent(out) :: outp
    integer(kind=c_int), dimension(3), intent(out) :: cdim
    integer(kind=c_intptr_t), intent(out) :: nsz
    integer(kind=c_int), intent(in) :: nx, ny, nz, stm, stp
    type(c_ptr), intent(in) :: inp

    outp => null()
    cdim = -1
    nsz = 0

    if (.not. c_associated(inp)) return

    cdim(1) = nx + stm + stp
    cdim(2) = ny + stm + stp
    cdim(3) = nz + stm + stp
    nsz = cdim(1)
    nsz = nsz * cdim(2)
    nsz = nsz * cdim(3)

    call c_f_pointer(inp, outp, (/ nsz /))

    return
  end subroutine get_pointer_r_3d

  subroutine get_pointer_i_3d(outp, nsz, cdim, inp, nx, ny, nz, stm, stp)
    integer(kind=c_int), dimension(:), pointer, intent(out) :: outp
    integer(kind=c_int), dimension(3), intent(out) :: cdim
    integer(kind=c_intptr_t), intent(out) :: nsz
    integer(kind=c_int), intent(in) :: nx, ny, nz, stm, stp
    type(c_ptr), intent(in) :: inp

    outp => null()
    cdim = -1
    nsz = 0

    if (.not. c_associated(inp)) return

    cdim(1) = nx + stm + stp
    cdim(2) = ny + stm + stp
    cdim(3) = nz + stm + stp
    nsz = cdim(1)
    nsz = nsz * cdim(2)
    nsz = nsz * cdim(3)

    call c_f_pointer(inp, outp, (/ nsz /))

    return
  end subroutine get_pointer_i_3d

  subroutine setup_variable_3d(vf1, vf2, fvf1, fvf2, nx, ny, nz, &
       fstm, fstp, stm, stp, ityp, istat, fstat)
    type(c_ptr), intent(in) :: vf1, vf2
    real(kind=8), allocatable, target, dimension(:,:,:), intent(inout) :: fvf1
    type(pnt2), intent(inout) :: fvf2
    integer(kind=c_int), intent(in) :: nx, ny, nz, stm, stp
    integer, intent(in) :: fstm, fstp, ityp
    type(c_ptr) :: istat
    integer, intent(inout) :: fstat

    real(kind=LPT_c_type), pointer, dimension(:) :: fpv
    integer(kind=c_intptr_t) :: nsz
    integer(kind=c_int), dimension(3) :: cdim

    call get_pointer_r_3d(fpv, nsz, cdim, vf1, nx, ny, nz, stm, stp)
    call reallocate_if_needed(fvf1, nx, ny, nz, fstm, fstp, ityp, istat, fstat)
    if (fstat .ne. 0) return
    call jupiter_lpt_copy(fvf1, fpv, stm, stp, fstm, fstp, nx, ny, nz, ityp)

    !! same pointer given vf1 and vf2, or vf2 is NULL.
    if (c_associated(vf1, vf2) .or. .not. c_associated(vf2)) then
       if (fvf2%alloc) deallocate(fvf2%p)
       fvf2%p => fvf1
       fvf2%alloc = .false.
    else
       call c_f_pointer(vf2, fpv, (/ nsz /))
       call reallocate_if_needed_p(fvf2, nx, ny, nz, fstm, fstp, ityp, istat, fstat)
       if (fstat .ne. 0) return
       call jupiter_lpt_copy(fvf2%p, fpv, stm, stp, fstm, fstp, nx, ny, nz, ityp)
    end if

    return
  end subroutine setup_variable_3d

  subroutine setup_grid_points(vf1, fvf1, n, fstm, fstp, stm, stp, istat, fstat)
    type(c_ptr), intent(in) :: vf1
    real(kind=8), allocatable, target, dimension(:), intent(inout) :: fvf1
    integer(kind=c_int), intent(in) :: n, stm, stp
    integer, intent(in) :: fstm, fstp
    type(c_ptr) :: istat
    integer, intent(inout) :: fstat

    real(kind=LPT_c_type), pointer, dimension(:) :: fpv
    integer :: iastat, lbnd, ubnd

    !! Grid node has 1 more point
    call c_f_pointer(vf1, fpv, [n + stm + stp + 1])
    if (allocated(fvf1)) then
       if (ubound(fvf1,1) .ne. n + fstp) then
          deallocate(fvf1)
       end if
    end if
    if (.not. allocated(fvf1)) then
       allocate(fvf1(1-fstm-1:n+fstp), stat=iastat)
       if (iastat .ne. 0) then
          fstat = 1
          call lptbnd_error(istat, IERR_ALLOCATION_FAILED, stat=iastat)
          return
       end if
    end if

    lbnd = stm - fstm + 1      ! 1 because Fortran is 1-origin
    ubnd = stm + n + fstp + 1  ! 1 because Grid points are node based.

    !$omp parallel shared(fvf1,fpv,lbnd,ubnd)
    !$omp workshare
    fvf1(:) = fpv(lbnd:ubnd)
    !$omp end workshare
    !$omp end parallel

    return
  end subroutine setup_grid_points

  subroutine lptbnd_error(istat, ierr, iunit, iovf, stat, fname, desc)
    type(c_ptr) :: istat
    integer :: ierr
    integer, optional, value :: iunit
    character(*), optional :: fname, desc
    integer(kind=c_intmax_t), optional, value :: iovf
    integer, optional, value :: stat
    integer(kind=c_int), pointer :: fstat
    integer :: ifstat
    character(len=20) :: ibuf, ibuf2
    character(len=256), target :: msgbase
    character(len=:), pointer :: msgbuf
    integer :: lsz
    procedure(error_callback_interface), pointer :: fecbk
    integer :: iastat

    if (c_associated(istat)) then
       call c_f_pointer(istat, fstat)
       fstat = ierr
    end if
    if (c_associated(error_callback)) then
       call c_f_procpointer(error_callback, fecbk)
       lsz = len(msgbase)
       msgbuf => msgbase
       iastat = 0
       do while (.true.)
          select case (ierr)
          case (IERR_ALLOCATION_FAILED)
             if (present(stat)) then
                write(ibuf, 101) stat
             else
                write(ibuf, 102)
             end if
             write(msgbuf, 103, iostat=iastat) trim(adjustl(ibuf))
101          format(i20)
102          format('(unknown)')
103          format('Allocation failed with Fortran Error Code: ', a)
          case (IERR_FILE_OPEN_FAILED)
             if (present(fname)) then
                if (present(stat)) then
                   write(ibuf, 201) stat
                   write(msgbuf, 203, iostat=iastat) &
                        trim(adjustl(ibuf)), trim(fname)
                else
                   write(msgbuf, 204, iostat=iastat) trim(fname)
                end if
             else
                if (present(iunit)) then
                   write(ibuf2, 201) iunit
                else
                   write(ibuf2, 202)
                end if
                if (present(stat)) then
                   write(ibuf, 201) stat
                   write(msgbuf, 205, iostat=iastat) &
                        trim(adjustl(ibuf)), trim(adjustl(ibuf2))
                else
                   write(msgbuf, 206, iostat=iastat) &
                        trim(adjustl(ibuf2))
                end if
             end if
201          format(i20)
202          format('(unknown)')
203          format('File open failed with Fortran Error Code ', a, ': ', a)
204          format('File open failed: ', a)
205          format('File open failed with Fortran Error Code ', a, ' for unit ', a)
206          format('File open failed for fortran unit ', a)
          case (IERR_INQUIRE_UNIT_STATUS)
             if (present(fname)) then
                if (present(stat)) then
                   write(ibuf, 301) stat
                   write(msgbuf, 303, iostat=iastat) trim(fname), &
                        trim(adjustl(ibuf))
                else
                   write(msgbuf, 304, iostat=iastat) trim(fname)
                end if
             else if (present(iunit)) then
                write(ibuf2, 301) iunit
                if (present(stat)) then
                   write(ibuf, 301) stat
                   write(msgbuf, 305, iostat=iastat) &
                        trim(adjustl(ibuf2)), trim(adjustl(ibuf))
                else
                   write(msgbuf, 306, iostat=iastat) trim(adjustl(ibuf2))
                end if
             else
                write(msgbuf, 307, iostat=iastat)
             end if
301          format(i20)
302          format('(unknown)')
303          format('Inquery on file "', a, '" failed with Fortran Error Code: ', a)
304          format('Inquery on file "', a, '" failed')
305          format('Inquery on unit ', a, ' failed with Fortran Error Code: ', a)
306          format('Inquery on unit ', a, ' failed')
307          format('Inqeury failed on unknown unit or file failed')
          case (IERR_ASSIGNMENT_WOULD_OVERFLOW)
             if (present(iovf)) then
                write(ibuf, 401) iovf
                write(msgbuf, 402, iostat=iastat) trim(adjustl(ibuf))
             else
                write(msgbuf, 403, iostat=iastat)
             end if
401          format(i20)
402          format('Assignment of value ', a, ' will overflow for Fortran')
403          format('Overflow detected')
          case (IERR_INVALID_ARGUMENT)
             if (present(desc)) then
                write(msgbuf, 501, iostat=iastat) desc
             else
                write(msgbuf, 502, iostat=iastat)
             end if
501          format(a)
502          format('Invalid Argument')
          case (IERR_NOT_ALLOCATED)
             write(msgbuf, 601, iostat=iastat)
601          format('Required storage not allocated')
          case default
             write(msgbuf, 9901)
9901         format('Unknown error occured')
          end select
          if (iastat .eq. 0) then
             exit
          end if

          lsz = lsz * 2
          if (.not. associated(msgbuf, msgbase)) then
             deallocate(msgbuf)
          end if
          allocate(character(len=lsz) :: msgbuf, stat=iastat)
          if (iastat .ne. 0) then
             msgbuf => msgbase
             write(msgbuf, 9999)
9999         format('Allocation failed while generating error message')
             exit
          end if
       end do
       call fecbk(error_callback_argument, ierr, len_trim(msgbuf), c_loc(msgbuf))
    end if
    return
  end subroutine lptbnd_error

  subroutine cLPTset_udwbc_callback(func, arg) &
       bind(c,name='cLPTset_udwbc_callback')
    type(c_funptr), intent(in), value :: func
    type(c_ptr), intent(in), value :: arg
    udwbc_callback = func
    udwbc_callback_argument = arg
  end subroutine cLPTset_udwbc_callback

  subroutine LPTudwbc(ip,pUxpt,pUypt,pUzpt,pXpt,pYpt,pZpt, &
                      pDiapt,dLpwll,iWPsrd,icfpt,jcfpt,kcfpt)
    integer(kind=4), intent(in) :: ip
    integer(kind=4), intent(in) :: icfpt, jcfpt, kcfpt
    integer(kind=4), intent(inout) :: iWPsrd
    real(kind=8), intent(inout) :: pUxpt, pUypt, pUzpt, pXpt, pYpt, pZpt, dLpwll
    real(kind=8), intent(in) :: pDiapt

    real(kind=LPT_c_type), dimension(3) :: pt, put
    integer(kind=c_int), dimension(3) :: icf
    integer(kind=c_int) :: iwp
    real(kind=LPT_c_type) :: dl, pdia

    procedure(udwbc_callback_interface), pointer :: fwcbk

    if (c_associated(udwbc_callback)) then
       pt(1) = pXpt
       pt(2) = pYpt
       pt(3) = pZpt
       put(1) = pUxpt
       put(2) = pUypt
       put(3) = pUzpt
       pdia = pDiapt
       icf(1) = icfpt
       icf(2) = jcfpt
       icf(3) = kcfpt
       iwp = iWPsrd
       dl = 0.0d0

       call c_f_procpointer(udwbc_callback, fwcbk)
       call fwcbk(udwbc_callback_argument, ip, pt, put, pdia, icf, iwp, dl)

       iWPsrd = iwp
       dLpwll = dl
       pXpt = pt(1)
       pYpt = pt(2)
       pZpt = pt(3)
       pUxpt = put(1)
       pUypt = put(2)
       pUzpt = put(3)
    else
       iWPsrd = 0
       dLpwll = -huge(0.0d0)
    end if
  end subroutine LPTudwbc
end module cLPTbnd
