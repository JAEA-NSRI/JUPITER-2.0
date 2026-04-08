module xLPTtest_set_get_pts
  implicit none

  character(len=11), parameter :: name = 'set_get_pts'
contains
  logical function test_set_get_pts()
    use ISO_C_BINDING, only: c_int, c_size_t, c_loc, c_null_ptr

    use xLPTtest_val, only: log_unit
    use LPTval, only: npt, &
         pUxf,   pUyf,   pUzf,   pXpt,   pYpt,   pZpt, &
         pUxpt,  pUypt,  pUzpt,  pTimpt, pRhopt, pDiapt, &
         pFUxpt, pFUypt, pFUzpt, pFdUxt, pFdUyt, pFdUzt, &
         icfpt,  jcfpt,  kcfpt,  iFPsrd, iWPsrd
    use LPTtype, only: LPT_c_type
    use cLPTbnd, only: build_idxtab, cLPTsetpts, cLPTgetpts
    use cLPTbnd, only: cLPTalloc, cLPTdealloc

    integer(kind=c_int), target :: icstat
    integer :: ifstat
    integer(kind=c_size_t), dimension(10) :: idxs
    integer(kind=c_size_t) :: ntot
    integer(kind=c_size_t), dimension(:), allocatable :: idxtab, idxex
    logical :: lret
    real(kind=LPT_c_type), dimension(100), target :: rbuf
    integer(kind=c_int), dimension(100), target :: ibuf
    integer :: i
    character(len=10) :: ib

    test_set_get_pts = .true.

    call cLPTdealloc(1)

    icstat = 0
    call cLPTalloc(1, 20, 2, 2, 2, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name, 'Required memory cannot be allocated'
       test_set_get_pts = .false.
       return
    end if
    if (npt .ne. 20) then
       write(log_unit, 1001) name // ': npt', 20, npt
       test_set_get_pts = .false.
       return
    end if

    !! tests for build_idxtab
    ifstat = 0
    call build_idxtab(int(-1, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .ne. 0) then
       write(log_unit, 1000) name // ': build_idxtab', &
            'should not fail for negative number of indices'
       test_set_get_pts = .false.
    end if
    if (ntot .ne. 0) then
       write(log_unit, 1000) name // ': build_idxtab', &
            'total number should be 0 for negative number of indices'
       test_set_get_pts = .false.
    end if

    ifstat = 0
    idxs(1) = 1
    call build_idxtab(int(1, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .ne. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert succeeds'
       test_set_get_pts = .false.
    else
       if (ntot .eq. 1) then
          if (.not. allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should be allocated'
             test_set_get_pts = .false.
          else
             if (idxtab(1) .ne. 1) then
                write(log_unit, 1001) name // ': build_idxtab', 1, idxtab(1)
                test_set_get_pts = .false.
             end if
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', 1, ntot
          test_set_get_pts = .false.
       end if
    end if

    ifstat = 0
    idxs(1) = 0 ! bad index
    call build_idxtab(int(1, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .eq. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert fails'
       test_set_get_pts = .false.
    else
       if (ntot .eq. -1) then
          if (allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should not be allocated'
             test_set_get_pts = .false.
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', -1, ntot
          test_set_get_pts = .false.
       end if
    end if

    ifstat = 0
    idxs(1) = npt + 1 ! bad index
    call build_idxtab(int(1, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .eq. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert fails'
       test_set_get_pts = .false.
    else
       if (ntot .eq. -1) then
          if (allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should not be allocated'
             test_set_get_pts = .false.
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', -1, ntot
          test_set_get_pts = .false.
       end if
    end if

    ifstat = 0
    idxs(1) = 1
    idxs(2) = 4
    idxs(3) = 3
    idxs(4) = 9
    idxs(5) = 99 ! should be ignored
    call build_idxtab(int(5, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .ne. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert succeeds'
       test_set_get_pts = .false.
    else
       if (ntot .eq. 11) then
          if (.not. allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should be allocated'
             test_set_get_pts = .false.
          else
             allocate(idxex(ntot), stat=ifstat)
             if (ifstat .eq. 0) then
                idxex( 1) = 1
                idxex( 2) = 2
                idxex( 3) = 3
                idxex( 4) = 4
                idxex( 5) = 3
                idxex( 6) = 4
                idxex( 7) = 5
                idxex( 8) = 6
                idxex( 9) = 7
                idxex(10) = 8
                idxex(11) = 9
                do i = 1, ntot
                   write(ib, '(i10)') i
                   lret = check_ivals(ib, 'idxtab ', idxex(i), idxtab(i))
                   if (.not. lret) then
                      test_set_get_pts = .false.
                   end if
                end do
             else
                write(log_unit, 1000) name // ': build_idxtab', &
                     'failed to allocate memory for checking'
             end if
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', 10, ntot
          test_set_get_pts = .false.
       end if
    end if

    ifstat = 0
    idxs(1) = 1
    idxs(2) = 4
    idxs(3) = 3
    idxs(4) = npt + 1 ! bad idx
    call build_idxtab(int(4, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .eq. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert fails'
       test_set_get_pts = .false.
    else
       if (ntot .eq. -1) then
          if (allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should not be allocated'
             test_set_get_pts = .false.
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', -1, ntot
          test_set_get_pts = .false.
       end if
    end if

    ifstat = 0
    idxs(1) = 0 ! bad idx
    idxs(2) = 4
    idxs(3) = 3
    idxs(4) = 9
    call build_idxtab(int(4, c_size_t), idxs, idxtab, ntot, c_null_ptr, ifstat)
    if (ifstat .eq. 0) then
       write(log_unit, 1000) name // ': build_idxtab', 'assert fails'
       test_set_get_pts = .false.
    else
       if (ntot .eq. -1) then
          if (allocated(idxtab)) then
             write(log_unit, 1000) name // ': build_idxtab', &
                  'idxtab should not be allocated'
             test_set_get_pts = .false.
          end if
       else
          write(log_unit, 1001) name // ': build_idxtab', -1, ntot
          test_set_get_pts = .false.
       end if
    end if
    if (allocated(idxtab)) deallocate(idxtab)

    !! clear destination
    !$omp parallel
    !$omp workshare
    pUxf = 0.0d0
    pUyf = 0.0d0
    pUzf = 0.0d0
    pXpt = 0.0d0
    pYpt = 0.0d0
    pZpt = 0.0d0
    pUxpt = 0.0d0
    pUypt = 0.0d0
    pUzpt = 0.0d0
    pTimpt = 0.0d0
    pRhopt = 0.0d0
    pDiapt = 0.0d0
    pFUxpt = 0.0d0
    pFUypt = 0.0d0
    pFUzpt = 0.0d0
    pFdUxt = 0.0d0
    pFdUyt = 0.0d0
    pFdUzt = 0.0d0
    icfpt = 0
    jcfpt = 0
    kcfpt = 0
    iFPsrd = 0
    iWPsrd = 0
    !$omp end workshare
    !$omp end parallel

    !! tests for setpts
    icstat = 0
    idxs(1) = 5
    rbuf( 1) = 1.0d0 ! value to set
    rbuf( 2) = 2.0d0 ! value to set
    rbuf( 3) = 3.0d0 ! value to set
    rbuf( 4) = 4.0d0 ! value to set
    rbuf( 5) = 5.0d0 ! value to set
    rbuf( 6) = 6.0d0 ! value to set
    rbuf( 7) = 7.0d0 ! value to set
    rbuf( 8) = 8.0d0 ! value to set
    rbuf( 9) = 9.0d0 ! value to set
    rbuf(10) = 1.0d1 ! value to set
    rbuf(11) = 1.1d1 ! value to set
    rbuf(12) = 1.2d1 ! value to set
    rbuf(13) = 1.3d1 ! value to set
    rbuf(14) = 1.4d1 ! value to set
    rbuf(15) = 1.5d1 ! value to set
    rbuf(16) = 1.6d1 ! value to set
    rbuf(17) = 1.7d1 ! value to set
    rbuf(18) = 1.8d1 ! value to set
    ibuf(1) = 10
    ibuf(2) = 11
    ibuf(3) = 12
    ibuf(4) = 13
    ibuf(5) = 14
    call cLPTsetpts(int(1, c_size_t), idxs, &
         c_loc(rbuf(1)), c_loc(rbuf(2)), c_loc(rbuf(3)), &
         c_loc(rbuf(4)), c_loc(rbuf(5)), c_loc(rbuf(6)), &
         c_loc(rbuf(7)), c_loc(rbuf(8)), c_loc(rbuf(9)), &
         c_loc(rbuf(10)), c_loc(rbuf(11)), c_loc(rbuf(12)), &
         c_loc(rbuf(13)), c_loc(rbuf(14)), c_loc(rbuf(15)), &
         c_loc(rbuf(16)), c_loc(rbuf(17)), c_loc(rbuf(18)), &
         c_loc(ibuf(1)), c_loc(ibuf(2)), c_loc(ibuf(3)), &
         c_loc(ibuf(4)), c_loc(ibuf(5)), c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ': set', 'assert succeeds'
       test_set_get_pts = .false.
    else
       lret = check_pt(int(5, c_size_t), &
            uxf = 1.0d0, uyf = 2.0d0, uzf = 3.0d0, &
            xpt = 4.0d0, ypt = 5.d00, zpt = 6.0d0, &
            uxpt = 7.0d0, uypt = 8.0d0, uzpt = 9.0d0, &
            timpt = 1.0d1, rhopt = 1.1d1, diapt = 1.2d1, &
            fuxpt = 1.3d1, fuypt = 1.4d1, fuzpt = 1.5d1, &
            fduxt = 1.6d1, fduyt = 1.7d1, fduzt = 1.8d1, &
            icpt = 10, jcpt = 11, kcpt = 12, ifps = 13, iwps = 14)
       if (.not. lret) test_set_get_pts = .false.
    end if

    ! bulk set
    icstat = 0
    idxs(1) = 3
    idxs(2) = 5
    idxs(3) = 8
    idxs(4) = 9
    rbuf( 1) = 1.0d1 ! value to set
    rbuf( 2) = 2.0d1 ! value to set
    rbuf( 3) = 3.0d1 ! value to set
    rbuf( 4) = 4.0d1 ! value to set
    rbuf( 5) = 5.0d1 ! value to set
    rbuf( 6) = 6.0d1 ! value to set
    rbuf( 7) = 7.0d1 ! value to set
    rbuf( 8) = 8.0d1 ! value to set
    rbuf( 9) = 9.0d1 ! value to set
    rbuf(10) = 1.0d2 ! value to set
    rbuf(11) = 1.1d2 ! value to set
    rbuf(12) = 1.2d2 ! value to set
    rbuf(13) = 1.3d2 ! value to set
    rbuf(14) = 1.4d2 ! value to set
    rbuf(15) = 1.5d2 ! value to set
    rbuf(16) = 1.6d2 ! value to set
    rbuf(17) = 1.7d2 ! value to set
    rbuf(18) = 1.8d2 ! value to set
    ibuf(1) = 20
    ibuf(2) = 21
    ibuf(3) = 22
    ibuf(4) = 23
    ibuf(5) = 24
    call cLPTsetpts(int(4, c_size_t), idxs, &
         c_loc(rbuf(1)), c_null_ptr, c_null_ptr, & ! uxf
         c_null_ptr, c_null_ptr, c_loc(rbuf(6)), & ! zpt
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_loc(rbuf(11)), c_null_ptr, & ! rhopt
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_loc(ibuf(1)), c_null_ptr, & ! jcpt
         c_null_ptr, c_null_ptr, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ': set', 'assert succeeds'
       test_set_get_pts = .false.
    else
       lret = check_pt(int(3, c_size_t), &
            uxf = 1.0d1, uyf = 0.0d0, uzf = 0.0d0, &
            xpt = 0.0d0, ypt = 0.0d0, zpt = 6.0d1, &
            uxpt = 0.0d0, uypt = 0.0d0, uzpt = 0.0d0, &
            timpt = 0.0d0, rhopt = 1.1d2, diapt = 0.0d0, &
            fuxpt = 0.0d0, fuypt = 0.0d0, fuzpt = 0.0d0, &
            fduxt = 0.0d0, fduyt = 0.0d0, fduzt = 0.0d0, &
            icpt = 0, jcpt = 20, kcpt = 0, ifps = 0, iwps = 0)
       if (.not. lret) test_set_get_pts = .false.
       lret = check_pt(int(4, c_size_t), &
            uxf = 2.0d1, zpt = 7.0d1, rhopt = 1.2d2, &
            jcpt = 21, kcpt = 0)
       if (.not. lret) test_set_get_pts = .false.
       !! Unspecified variables left unchanged.
       lret = check_pt(int(5, c_size_t), &
            uxf = 3.0d1, uyf = 2.0d0, uzf = 3.0d0, &
            xpt = 4.0d0, ypt = 5.d00, zpt = 8.0d1, &
            uxpt = 7.0d0, uypt = 8.0d0, uzpt = 9.0d0, &
            timpt = 1.0d1, rhopt = 1.3d2, diapt = 1.2d1, &
            fuxpt = 1.3d1, fuypt = 1.4d1, fuzpt = 1.5d1, &
            fduxt = 1.6d1, fduyt = 1.7d1, fduzt = 1.8d1, &
            icpt = 10, jcpt = 22, kcpt = 12, ifps = 13, iwps = 14)
       if (.not. lret) test_set_get_pts = .false.
       !! Not specified range
       lret = check_pt(int(6, c_size_t), &
            uxf = 0.0d0, uyf = 0.0d0, uzf = 0.0d0, &
            xpt = 0.0d0, ypt = 0.0d0, zpt = 0.0d0, &
            uxpt = 0.0d0, uypt = 0.0d0, uzpt = 0.0d0, &
            timpt = 0.0d0, rhopt = 0.0d0, diapt = 0.0d0, &
            fuxpt = 0.0d0, fuypt = 0.0d0, fuzpt = 0.0d0, &
            fduxt = 0.0d0, fduyt = 0.0d0, fduzt = 0.0d0, &
            icpt = 0, jcpt = 0, kcpt = 0, ifps = 0, iwps = 0)
       if (.not. lret) test_set_get_pts = .false.
       lret = check_pt(int(8, c_size_t), &
            uxf = 4.0d1, zpt = 9.0d1, rhopt = 1.4d2, &
            jcpt = 23, kcpt = 0)
       if (.not. lret) test_set_get_pts = .false.
       lret = check_pt(int(9, c_size_t), &
            uxf = 5.0d1, zpt = 1.0d2, rhopt = 1.5d2, &
            jcpt = 24, kcpt = 0)
       if (.not. lret) test_set_get_pts = .false.
    end if

    !! Clear receive memory
    icstat = 0
    rbuf = 0.0d0
    ibuf = 0

    idxs(1) = 5
    call cLPTgetpts(int(1, c_size_t), idxs, &
         c_loc(rbuf(1)), c_loc(rbuf(2)), c_loc(rbuf(3)), &
         c_loc(rbuf(4)), c_loc(rbuf(5)), c_loc(rbuf(6)), &
         c_loc(rbuf(7)), c_loc(rbuf(8)), c_loc(rbuf(9)), &
         c_loc(rbuf(10)), c_loc(rbuf(11)), c_loc(rbuf(12)), &
         c_loc(rbuf(13)), c_loc(rbuf(14)), c_loc(rbuf(15)), &
         c_loc(rbuf(16)), c_loc(rbuf(17)), c_loc(rbuf(18)), &
         c_loc(ibuf(1)), c_loc(ibuf(2)), c_loc(ibuf(3)), &
         c_loc(ibuf(4)), c_loc(ibuf(5)), c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ': get', 'assert succeeds'
       test_set_get_pts = .false.
    else
       lret = check_rval('         1', 'rbuf   ', 3.0d1, rbuf(1))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         2', 'rbuf   ', 2.0d0, rbuf(2))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         3', 'rbuf   ', 3.0d0, rbuf(3))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         4', 'rbuf   ', 4.0d0, rbuf(4))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         5', 'rbuf   ', 5.d00, rbuf(5))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         6', 'rbuf   ', 8.0d1, rbuf(6))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         7', 'rbuf   ', 7.0d0, rbuf(7))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         8', 'rbuf   ', 8.0d0, rbuf(8))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         9', 'rbuf   ', 9.0d0, rbuf(9))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        10', 'rbuf   ', 1.0d1, rbuf(10))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        11', 'rbuf   ', 1.3d2, rbuf(11))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        12', 'rbuf   ', 1.2d1, rbuf(12))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        13', 'rbuf   ', 1.3d1, rbuf(13))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        14', 'rbuf   ', 1.4d1, rbuf(14))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        15', 'rbuf   ', 1.5d1, rbuf(15))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        16', 'rbuf   ', 1.6d1, rbuf(16))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        17', 'rbuf   ', 1.7d1, rbuf(17))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        18', 'rbuf   ', 1.8d1, rbuf(18))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         1', 'ibuf   ', 10, ibuf(1))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         2', 'ibuf   ', 22, ibuf(2))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         3', 'ibuf   ', 12, ibuf(3))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         4', 'ibuf   ', 13, ibuf(4))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         5', 'ibuf   ', 14, ibuf(5))
       if (.not. lret) test_set_get_pts = .false.
    end if

    !! Clear receive memory
    icstat = 0
    rbuf = 0.0d0
    ibuf = 0

    !! bulk get
    idxs(1) = 1
    idxs(2) = 3
    idxs(3) = 5
    idxs(4) = 8
    call cLPTgetpts(int(4, c_size_t), idxs, &
         c_loc(rbuf(1)), c_null_ptr, c_null_ptr, & ! uxf
         c_null_ptr, c_null_ptr, c_loc(rbuf(8)), & ! zpt
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_loc(rbuf(15)), c_null_ptr, & ! rhopt
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_null_ptr, c_null_ptr, &
         c_null_ptr, c_loc(ibuf(8)), c_null_ptr, & ! jcpt
         c_loc(ibuf(1)), c_null_ptr, c_loc(icstat))  ! ifps
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ': get', 'assert succeeds'
       test_set_get_pts = .false.
    else
       ! uxf
       lret = check_rval('         1', 'rbuf   ', 0.0d0, rbuf(1))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         2', 'rbuf   ', 0.0d0, rbuf(2))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         3', 'rbuf   ', 1.0d1, rbuf(3))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         4', 'rbuf   ', 3.0d1, rbuf(4))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         5', 'rbuf   ', 0.0d0, rbuf(5))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         6', 'rbuf   ', 0.0d0, rbuf(6))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         7', 'rbuf   ', 4.0d1, rbuf(7))
       if (.not. lret) test_set_get_pts = .false.
       ! zpt
       lret = check_rval('         8', 'rbuf   ', 0.0d0, rbuf(8))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('         9', 'rbuf   ', 0.0d0, rbuf(9))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        10', 'rbuf   ', 6.0d1, rbuf(10))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        11', 'rbuf   ', 8.0d1, rbuf(11))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        12', 'rbuf   ', 0.0d0, rbuf(12))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        13', 'rbuf   ', 0.0d0, rbuf(13))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        14', 'rbuf   ', 9.0d1, rbuf(14))
       if (.not. lret) test_set_get_pts = .false.
       ! rhopt
       lret = check_rval('        15', 'rbuf   ', 0.0d0, rbuf(15))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        16', 'rbuf   ', 0.0d0, rbuf(16))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        17', 'rbuf   ', 1.1d2, rbuf(17))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        18', 'rbuf   ', 1.3d2, rbuf(18))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        19', 'rbuf   ', 0.0d0, rbuf(19))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        20', 'rbuf   ', 0.0d0, rbuf(20))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        21', 'rbuf   ', 1.4d2, rbuf(21))
       ! further
       if (.not. lret) test_set_get_pts = .false.
       lret = check_rval('        22', 'rbuf   ', 0.0d0, rbuf(22))
       ! ifps
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         1', 'ibuf   ', 0, ibuf(1))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         2', 'ibuf   ', 0, ibuf(2))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         3', 'ibuf   ', 0, ibuf(3))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         4', 'ibuf   ', 13, ibuf(4))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         5', 'ibuf   ', 0, ibuf(5))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         6', 'ibuf   ', 0, ibuf(6))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         7', 'ibuf   ', 0, ibuf(7))
       if (.not. lret) test_set_get_pts = .false.
       ! jcpt
       lret = check_ivalc('         8', 'ibuf   ', 0, ibuf(8))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('         9', 'ibuf   ', 0, ibuf(9))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('        10', 'ibuf   ', 20, ibuf(10))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('        11', 'ibuf   ', 22, ibuf(11))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('        12', 'ibuf   ', 0, ibuf(12))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('        13', 'ibuf   ', 0, ibuf(13))
       if (.not. lret) test_set_get_pts = .false.
       lret = check_ivalc('        14', 'ibuf   ', 23, ibuf(14))
       if (.not. lret) test_set_get_pts = .false.
       ! further
       lret = check_ivalc('        15', 'ibuf   ', 0, ibuf(15))
       if (.not. lret) test_set_get_pts = .false.
    end if

    call cLPTdealloc(1)

1000 format('[ERROR] ', a, ' test failed: ', a)
1001 format('[ERROR] ', a, ' test failed: expected ', i10, ', but got ', i10)
1002 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', 1pe25.17, ', but got ', 1pe25.17)

  end function test_set_get_pts

  logical function check_ivalc(ibuf, nval, expect, actual)
    use ISO_C_BINDING, only: c_int
    use xLPTtest_val, only: log_unit

    integer(kind=kind(1)), intent(in) :: expect
    integer(kind=c_int), intent(in) :: actual
    character(len=7), intent(in) :: nval
    character(len=10), intent(in) :: ibuf

    check_ivalc = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_ivalc = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', i10, ', but got ', i10)
  end function check_ivalc

  logical function check_ivals(ibuf, nval, expect, actual)
    use ISO_C_BINDING, only: c_size_t
    use xLPTtest_val, only: log_unit

    integer(kind=c_size_t), intent(in) :: expect, actual
    character(len=7), intent(in) :: nval
    character(len=10), intent(in) :: ibuf

    check_ivals = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_ivals = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', i10, ', but got ', i10)
  end function check_ivals

  logical function check_rval(ibuf, nval, expect, actual)
    use LPTtype, only: LPT_c_type
    use xLPTtest_val, only: log_unit
    real(kind=kind(1.0d0)), intent(in) :: expect
    real(kind=LPT_c_type), intent(in) :: actual
    character(len=10), intent(in) :: ibuf
    character(len=7), intent(in) :: nval

    check_rval = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_rval = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', 1pe25.17, ', but got ', 1pe25.17)
  end function check_rval

  logical function check_ival4(ibuf, nval, expect, actual)
    use ISO_C_BINDING, only: c_size_t
    use xLPTtest_val, only: log_unit

    integer(kind=4), intent(in) :: expect, actual
    character(len=7), intent(in) :: nval
    character(len=10), intent(in) :: ibuf

    check_ival4 = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_ival4 = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', i10, ', but got ', i10)
  end function check_ival4

  logical function check_rval8(ibuf, nval, expect, actual)
    use LPTtype, only: LPT_c_type
    use xLPTtest_val, only: log_unit

    real(kind=8), intent(in) :: expect, actual
    character(len=10), intent(in) :: ibuf
    character(len=7), intent(in) :: nval

    check_rval8 = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_rval8 = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', 1pe25.17, ', but got ', 1pe25.17)
  end function check_rval8

  !! This function needs build_idxtab should work properly
  logical function check_pt(idx,uxf,uyf,uzf,xpt,ypt,zpt, &
       uxpt,uypt,uzpt,timpt,rhopt,diapt,fuxpt,fuypt,fuzpt,fduxt,fduyt,fduzt, &
       icpt,jcpt,kcpt,ifps,iwps)
    use ISO_C_BINDING, only: c_size_t
    use LPTval, only: npt, &
         pUxf,   pUyf,   pUzf,   pXpt,   pYpt,   pZpt, &
         pUxpt,  pUypt,  pUzpt,  pTimpt, pRhopt, pDiapt, &
         pFUxpt, pFUypt, pFUzpt, pFdUxt, pFdUyt, pFdUzt, &
         icfpt,  jcfpt,  kcfpt,  iFPsrd, iWPsrd

    integer(kind=c_size_t) :: idx
    character(len=10) :: ib
    real(kind=8), optional :: uxf, uyf, uzf, xpt, ypt, zpt
    real(kind=8), optional :: uxpt, uypt, uzpt
    real(kind=8), optional :: timpt, rhopt, diapt
    real(kind=8), optional :: fuxpt, fuypt, fuzpt
    real(kind=8), optional :: fduxt, fduyt, fduzt
    integer(kind=4), optional :: icpt, jcpt, kcpt, ifps, iwps
    logical, dimension(23) :: l

    write(ib, '(i10)') idx

    l = .true.
    if (present(  uxf)) l( 1) = check_rval8(ib, 'pUxf    ',   uxf, pUxf(idx))
    if (present(  uyf)) l( 2) = check_rval8(ib, 'pUyf    ',   uyf, pUyf(idx))
    if (present(  uzf)) l( 3) = check_rval8(ib, 'pUzf    ',   uzf, pUzf(idx))
    if (present(  xpt)) l( 4) = check_rval8(ib, 'pXpt    ',   xpt, pXpt(idx))
    if (present(  ypt)) l( 5) = check_rval8(ib, 'pYpt    ',   ypt, pYpt(idx))
    if (present(  zpt)) l( 6) = check_rval8(ib, 'pZpt    ',   zpt, pZpt(idx))
    if (present( uxpt)) l( 7) = check_rval8(ib, 'pUxpt   ',  uxpt, pUxpt(idx))
    if (present( uypt)) l( 8) = check_rval8(ib, 'pUypt   ',  uypt, pUypt(idx))
    if (present( uzpt)) l( 9) = check_rval8(ib, 'pUzpt   ',  uzpt, pUzpt(idx))
    if (present(timpt)) l(10) = check_rval8(ib, 'pTimpt  ', timpt, pTimpt(idx))
    if (present(rhopt)) l(11) = check_rval8(ib, 'pRhopt  ', rhopt, pRhopt(idx))
    if (present(diapt)) l(12) = check_rval8(ib, 'pDiapt  ', diapt, pDiapt(idx))
    if (present(fuxpt)) l(13) = check_rval8(ib, 'pFUxpt  ', fuxpt, pFUxpt(idx))
    if (present(fuypt)) l(14) = check_rval8(ib, 'pFUypt  ', fuypt, pFUypt(idx))
    if (present(fuzpt)) l(15) = check_rval8(ib, 'pFUzpt  ', fuzpt, pFUzpt(idx))
    if (present(fduxt)) l(16) = check_rval8(ib, 'pFdUxt  ', fduxt, pFdUxt(idx))
    if (present(fduyt)) l(17) = check_rval8(ib, 'pFdUyt  ', fduyt, pFdUyt(idx))
    if (present(fduzt)) l(18) = check_rval8(ib, 'pFdUzt  ', fduzt, pFdUzt(idx))
    if (present( icpt)) l(19) = check_ival4(ib, 'icfpt   ',  icpt, icfpt(idx))
    if (present( jcpt)) l(20) = check_ival4(ib, 'jcfpt   ',  jcpt, jcfpt(idx))
    if (present( kcpt)) l(21) = check_ival4(ib, 'kcfpt   ',  kcpt, kcfpt(idx))
    if (present( ifps)) l(22) = check_ival4(ib, 'iFPsrd  ',  ifps, iFPsrd(idx))
    if (present( iwps)) l(23) = check_ival4(ib, 'iWPsrd  ',  iwps, iWPsrd(idx))

    if (all(l)) then
       check_pt = .true.
    else
       check_pt = .false.
    end if
    return
  end function check_pt
end module xLPTtest_set_get_pts
