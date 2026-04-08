module xLPTtest_mxpset
  implicit none

  character(len=6), parameter :: name = 'mxpset'

contains
  logical function test_mxpset()
    use ISO_C_BINDING,   only: c_int

    use xLPTtest_val, only: log_unit
    use cLPTbnd, only: cLPTmxpset
    use LPTval,  only: mxpset

    integer(kind=c_int) :: nmxpset

    test_mxpset = .true.

    nmxpset = cLPTmxpset()
    if (mxpset .ne. nmxpset) then
       write(log_unit, 1000) name, mxpset, nmxpset
       test_mxpset = .false.
    end if

1000 format('[ERROR] ', a, ' test failed: expected ', i10, ', but got ', i10)
  end function test_mxpset
end module xLPTtest_mxpset
