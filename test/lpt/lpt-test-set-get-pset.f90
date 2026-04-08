module xLPTtest_set_get_pset
  use xLPTtest_val, only: log_unit

  implicit none

  character(len=12), parameter :: name = 'set_get_pset'

contains
  logical function test_set_get_pset()
    use ISO_C_BINDING, only: c_null_ptr, c_int, c_loc

    use LPTtype, only: LPT_c_type
    use LPTval, only: mxpset
    use cLPTbnd, only: cLPTsetpset, cLPTgetpset

    integer(kind=c_int), target :: icstat
    real(kind=LPT_c_type), target :: tmprvals(13)
    real(kind=8), target :: tmplvals(10)
    integer(kind=c_int), target :: tmpivals(10)

    test_set_get_pset = .true.
    if (mxpset .lt. 1) then
       write(log_unit, 1000) name, 'Maximum number of available particle set is 0 or negative (ignored)'
       return
    end if

    icstat = 0
    tmprvals(:) = (/ 0.0d0, 1.0d0, 0.0d0, 1.0d0, 0.0d0, 1.0d0, &
         0.0d0, 1.0d0, 1.0d-4, 3.0d3, 1.0d0, 0.0d0, 2.0d0 /)
    call cLPTsetpset(0, 1, 0, &
         tmprvals(1), tmprvals(2), tmprvals(3), tmprvals(4), &
         tmprvals(5), tmprvals(6), tmprvals(7), tmprvals(8), &
         tmprvals(9), tmprvals(10), &
         tmprvals(11), tmprvals(12), tmprvals(13), c_loc(icstat))
    if (icstat .eq. 0) then
       write(log_unit, 1000) name, 'Should fail to set on to zero index'
       test_set_get_pset = .false.
       return
    end if

    icstat = 0
    call cLPTgetpset(0, c_null_ptr, c_null_ptr, & ! np, it
         c_null_ptr, c_null_ptr, c_null_ptr, c_null_ptr, & ! x, y
         c_null_ptr, c_null_ptr, c_null_ptr, c_null_ptr, & ! z, tm
         c_null_ptr, c_null_ptr,                         & ! di, rho
         c_null_ptr, c_null_ptr, c_null_ptr, c_loc(icstat)) ! u
    if (icstat .eq. 0) then
       write(log_unit, 1000) name, 'Should fail to get from zero index'
       test_set_get_pset = .false.
       return
    end if

    if (mxpset .lt. huge(mxpset)) then
       icstat = 0
       tmprvals(:) = (/ 0.0d0, 1.0d0, 0.0d0, 1.0d0, 0.0d0, 1.0d0, &
            0.0d0, 1.0d0, 1.0d-4, 3.0d3, 1.0d0, 0.0d0, 2.0d0 /)
       call cLPTsetpset(0, 1, 0, &
            tmprvals(1), tmprvals(2), tmprvals(3), tmprvals(4), &
            tmprvals(5), tmprvals(6), tmprvals(7), tmprvals(8), &
            tmprvals(9), tmprvals(10), &
            tmprvals(11), tmprvals(12), tmprvals(13), c_loc(icstat))
       if (icstat .eq. 0) then
          write(log_unit, 1000) name, 'Should fail to set on to large index'
          test_set_get_pset = .false.
          return
       end if

       icstat = 0
       call cLPTgetpset(mxpset + 1, c_null_ptr, c_null_ptr, & ! np, it
            c_null_ptr, c_null_ptr, c_null_ptr, c_null_ptr, & ! x, y
            c_null_ptr, c_null_ptr, c_null_ptr, c_null_ptr, & ! z, tm
            c_null_ptr, c_null_ptr,                         & ! di, rho
            c_null_ptr, c_null_ptr, c_null_ptr, c_loc(icstat)) ! u
       if (icstat .eq. 0) then
          write(log_unit, 1000) name, 'Should fail to get from large index'
          test_set_get_pset = .false.
          return
       end if
    end if

    !! Choosing integral values to avoid truncation error for type
    !! conversion if applicable.
    icstat = 0
    tmprvals(:) = (/ 1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0, &
         7.0d0, 8.0d0, 1.0d1, 3.0d3, -1.0d0, -2.0d0, -3.0d0 /)
    call cLPTsetpset(1, 10, 0, &
         tmprvals(1), tmprvals(2), tmprvals(3), tmprvals(4), &
         tmprvals(5), tmprvals(6), tmprvals(7), tmprvals(8), &
         tmprvals(9), tmprvals(10), &
         tmprvals(11), tmprvals(12), tmprvals(13), c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name, 'Failed to set values to set 1'
       test_set_get_pset = .false.
       return
    end if

    test_set_get_pset = check_value(1, 10, 0, &
         1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0, &
         7.0d0, 8.0d0, 1.0d1, 3.0d3, -1.0d0, -2.0d0, -3.0d0)
    if (.not. test_set_get_pset) return

    call cLPTgetpset(1, c_null_ptr, c_loc(tmpivals(1)), & ! itrdm
         c_loc(tmprvals(1)), c_null_ptr, & ! psetXs
         c_loc(tmprvals(2)), c_null_ptr, & ! psetYs
         c_null_ptr, c_loc(tmprvals(3)), & ! psetZe
         c_null_ptr, c_null_ptr, &
         c_loc(tmprvals(4)), c_loc(tmprvals(5)), & ! di, ri
         c_null_ptr, c_loc(tmprvals(6)), c_null_ptr, & ! uy
         c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name, 'Failed get values from set 1'
       test_set_get_pset = .false.
       return
    end if
    tmplvals(1:6) = tmprvals(1:6)
    test_set_get_pset = check_value(1, itex=tmpivals(1), &
         exxs=tmplvals(1), exys=tmplvals(2), exze=tmplvals(3), &
         exdi=tmplvals(4), exri=tmplvals(5), exuy=tmplvals(6))
    if (.not. test_set_get_pset) return

1000 format('[ERROR] ', a, ' test failed: ', a)
    return
  end function test_set_get_pset

  logical function check_value(index, npex, itex, exxs, exxe, exys, exye, &
       exzs, exze, extms, extme, exdi, exri, exux, exuy, exuz)
    use LPTtype, only: LPT_c_type
    use LPTval, only: &
         psetXs, psetXe, psetYs, psetYe, psetZs, psetZe, &
         psetTms, psetTme, psetDi, psetRi, psetUx, psetUy, psetUz, &
         nistpt, itrdm

    integer(kind=4), intent(in) :: index
    integer(kind=4), intent(in), optional :: npex, itex
    real(kind=8), intent(in), optional :: exxs, exxe, exys, exye, &
         exzs, exze, extms, extme, exdi, exri, exux, exuy, exuz
    character(len=10) :: ib
    logical :: l(15)

    l = .true.
    write(ib, '(i10)') index
    if (present(npex))  l( 1) = check_ival(ib, 'nistpt ', npex, nistpt(index))
    if (present(itex))  l( 2) = check_ival(ib, 'itrdm  ', itex, itrdm(index))
    if (present(exxs))  l( 3) = check_rval(ib, 'psetXs ', exxs, psetXs(index))
    if (present(exxe))  l( 4) = check_rval(ib, 'psetXe ', exxe, psetXe(index))
    if (present(exys))  l( 5) = check_rval(ib, 'psetYs ', exys, psetYs(index))
    if (present(exye))  l( 6) = check_rval(ib, 'psetYe ', exye, psetYe(index))
    if (present(exzs))  l( 7) = check_rval(ib, 'psetZs ', exzs, psetZs(index))
    if (present(exze))  l( 8) = check_rval(ib, 'psetZe ', exze, psetZe(index))
    if (present(extms)) l( 9) = check_rval(ib, 'psetTms', extms, psetTms(index))
    if (present(extme)) l(10) = check_rval(ib, 'psetTme', extme, psetTme(index))
    if (present(exdi))  l(11) = check_rval(ib, 'psetDi ', exdi, psetDi(index))
    if (present(exri))  l(12) = check_rval(ib, 'psetRi ', exri, psetRi(index))
    if (present(exux))  l(13) = check_rval(ib, 'psetUx ', exux, psetUx(index))
    if (present(exuy))  l(14) = check_rval(ib, 'psetUy ', exuy, psetUy(index))
    if (present(exuz))  l(15) = check_rval(ib, 'psetUz ', exuz, psetUz(index))

    if (all(l)) then
       check_value = .true.
    else
       check_value = .false.
    end if
    return
  end function check_value

  logical function check_ival(ibuf, nval, expect, actual)
    integer(kind=4), intent(in) :: expect, actual
    character(len=7), intent(in) :: nval
    character(len=10), intent(in) :: ibuf

    check_ival = .true.
    if (expect .ne. actual) then
       write(log_unit, 1001) name, trim(nval), trim(adjustl(ibuf)), &
            expect, actual
       check_ival = .false.
    end if
1001 format('[ERROR] ', a, ': ', a, '(', a, '): test failed: expected ', i10, ', but got ', i10)
  end function check_ival

  logical function check_rval(ibuf, nval, expect, actual)
    real(kind=8), intent(in) :: expect, actual
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
end module xLPTtest_set_get_pset
