module xLPTtest_allocate
  implicit none

  character(len=8), parameter :: name = 'allocate'
contains
  logical function test_allocate()
    use ISO_C_BINDING, only: c_int, c_loc

    use xLPTtest_val, only: log_unit
    use LPTval, only: pXpt, pYpt
    use cLPTbnd, only: cLPTalloc, cLPTdealloc

    integer(kind=c_int), target :: icstat
    integer :: iistat

    test_allocate = .true.
    icstat = 0
    call cLPTalloc(1, 10, 2, 2, 2, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, '("[ERROR] Minimal set of memory could not be allocated")')
       test_allocate = .false.
       return
    end if

    if (.not. allocated(pXpt)) then
       write(log_unit, 1000) name, 'not allocated'
       test_allocate = .false.
       return
    end if

    !! deallocate pXpt to tweak
    deallocate(pXpt, stat=iistat)
    if (iistat .ne. 0) then
       write(log_unit, 1000) name, 'failed to deallocate pXpt (ignored)'
    end if

    !! This should not fail
    call cLPTdealloc(1)

    if (allocated(pYpt) .or. allocated(pXpt)) then
       write(log_unit, 1000) name, 'still allocated'
       test_allocate = .false.
       return
    end if

    !! allocate pXpt to tweak
    allocate(pXpt(1), stat=iistat)
    if (iistat .ne. 0) then
       write(log_unit, 1000) name, 'failed to allocate pXpt (ignored)'
    end if
    if (allocated(pYpt)) then
       write(log_unit, 1000) name, 'Assertion failed to pYpt is not allocated'
       test_allocate = .false.
    end if

    icstat = 0
    call cLPTalloc(1, 10, 2, 2, 2, c_loc(icstat))
    if (icstat .eq. 0) then
       write(log_unit, 1000) name, 'allocation succeeds if some allocated'
       test_allocate = .false.
       return
    else if (allocated(pYpt)) then
       write(log_unit, 1000) name, 'pYpt is allcoated even if it returned error'
       test_allocate = .false.
    end if
    deallocate(pXpt)

    icstat = 0
    call cLPTalloc(2, 10, 2, 2, 2, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, '("[ERROR] Minimal set of memory could not be allocated")')
       test_allocate = .false.
       return
    end if

    !! These should not fail
    call cLPTdealloc(1)

    icstat = 0
    call cLPTalloc(1, 10, 2, 2, 2, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name, 'Re-allocation failed'
       test_allocate = .false.
       return
    end if

    !! These should not fail
    call cLPTdealloc(1)
    call cLPTdealloc(2)

1000 format('[ERROR] ', a, ' test failed: ', a)
  end function test_allocate
end module xLPTtest_allocate
