module xLPTtest_set_get_ipttim
  implicit none

  character(len=12), parameter :: name = 'set_get_ipttim'

contains
  logical function test_set_get_ipttim()
    use ISO_C_BINDING, only: c_int
    use xLPTtest_val, only: log_unit
    use LPTval, only: ipttim
    use cLPTbnd, only: cLPTgetipttim, cLPTsetipttim

    integer(kind=c_int) :: ic

    test_set_get_ipttim = .true.
    ic = cLPTgetipttim()
    if (ic .ne. ipttim) then
       write(log_unit, 1000) name // ': get', ipttim, ic
       test_set_get_ipttim = .false.
    end if

    !! cLPTsetipttim does not check value
    call cLPTsetipttim(4)
    if (4 .ne. ipttim) then
       write(log_unit, 1000) name // ': set', 4, ipttim
       test_set_get_ipttim = .false.
    end if

1000 format('[ERROR] ', a, ' test failed: expected ', i10, ', but got ', i10)
  end function test_set_get_ipttim
end module xLPTtest_set_get_ipttim
