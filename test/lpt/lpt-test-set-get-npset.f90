module xLPTtest_set_get_npset
  implicit none

  character(len=10), parameter :: name = 'set_get_npset'
contains
  logical function test_set_get_npset()
    use ISO_C_BINDING, only: c_int, c_null_ptr, c_loc

    use xLPTtest_val, only: log_unit
    use LPTval, only: npset, mxpset
    use cLPTbnd, only: cLPTgetnpset, cLPTsetnpset, cLPTmxpset

    integer(kind=c_int) :: ic
    integer(kind=c_int), target :: icstat

    test_set_get_npset = .true.

    call cLPTsetnpset(1, c_null_ptr)
    if (npset .ne. 1) then
       write(log_unit, 1000) name // ': simple set', 1, npset
       test_set_get_npset = .false.
       return
    end if

    ic = cLPTgetnpset()
    if (ic .ne. 1) then
       write(log_unit, 1000) name // ': get', 1, ic
       test_set_get_npset = .false.
       return
    end if

    icstat = 0
    call cLPTsetnpset(1, c_loc(icstat))
    call cLPTsetnpset(0, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ': stat ok', 0, icstat
       test_set_get_npset = .false.
       return
    end if

    ic = cLPTmxpset()
    if (ic .gt. 0) then
       call cLPTsetnpset(ic, c_loc(icstat))
       if (icstat .ne. 0) then
          write(log_unit, 1000) name // ': stat ok (max)', 0, icstat
          test_set_get_npset = .false.
          return
       end if
    else
       write(log_unit, 1000) name // ': stat ok (invalid max)', 0, ic
    end if

    icstat = 0
    call cLPTsetnpset(-1, c_loc(icstat))
    if (icstat .eq. 0) then
       write(log_unit, 1000) name // ': stat not ok (neg)', 1, icstat
       test_set_get_npset = .false.
       return
    end if

    icstat = 0
    if (ic .gt. 0) then
       if (ic .lt. huge(ic)) then
          call cLPTsetnpset(ic + 1, c_loc(icstat))
          if (icstat .eq. 0) then
             write(log_unit, 1000) name // ': stat not ok (huge)', 1, icstat
             test_set_get_npset = .false.
             return
          end if
       else
          write(*, *) 'mxpset is huge'
       end if
    end if

1000 format('[ERROR] ', a, ' test failed: expected ', i10, ', but got ', i10)
  end function test_set_get_npset
end module xLPTtest_set_get_npset
