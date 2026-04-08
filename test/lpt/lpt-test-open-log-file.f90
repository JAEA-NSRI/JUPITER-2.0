module xLPTtest_open_log_file
  implicit none

  character(len=13), parameter :: name = 'open_log_file'
contains
  logical function test_open_log_file()
    use ISO_C_BINDING, only: c_int, c_loc, c_null_ptr, c_null_char
    use ISO_FORTRAN_ENV, only: ERROR_UNIT, OUTPUT_UNIT

    use xLPTtest_val, only: log_unit
    use LPTval, only: iunLPT
    use cLPTbnd, only: cLPTopenlogfile

    integer(kind=c_int), parameter :: lpath = 4096
    integer(kind=c_int), target :: icstat
    character(len=lpath), target :: fname
    integer :: istat
    integer :: iunit
    logical :: lopened

    test_open_log_file = .true.
    icstat = 0
    call cLPTopenlogfile(ERROR_UNIT, 0, c_null_ptr, 0, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1001) name // ' status', 0, icstat
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. ERROR_UNIT) then
       write(log_unit, 1001) name // ' iunLPT', 0, iunLPT
       test_open_log_file = .false.
       return
    end if

    icstat = 0
    call cLPTopenlogfile(OUTPUT_UNIT, 0, c_null_ptr, 1, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1001) name // ' status', 0, icstat
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. OUTPUT_UNIT) then
       write(log_unit, 1001) name // ' iunLPT', 0, iunLPT
       test_open_log_file = .false.
       return
    end if

    icstat = 0
    fname = 'dummy.dat' // c_null_char
    call cLPTopenlogfile(11, lpath, c_loc(fname), 0, c_loc(icstat))
    inquire(file='dummy.dat', opened=lopened, number=iunit)
    close(11, status='delete')
    if (icstat .eq. 0 .and. .not. lopened) then
       write(log_unit, 1000) name // ' simple open', 'reported opened successfully but inquire() returned its not opened'
       test_open_log_file = .false.
       return
    end if
    if (icstat .ne. 0 .and. lopened) then
       write(log_unit, 1000) name // ' simple open', 'reported opened failed but inquire() returned its opened'
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. 11) then
       write(log_unit, 1001) name // ' iunLPT', 11, iunLPT
       test_open_log_file = .false.
       return
    end if
    if (iunit .ne. 11) then
       write(log_unit, 1001) name // ' iunit', 11, iunit
       test_open_log_file = .false.
       return
    end if

    fname = 'another.dat' // c_null_char
    call cLPTopenlogfile(-1, lpath, c_loc(fname), 1, c_loc(icstat))
    inquire(file='dummy.dat', opened=lopened)
    if (lopened) then
       write(log_unit, 1000) name // ' simple open', 'last file still opened'
       close(11, status='delete')
       test_open_log_file = .false.
       return
    end if
    inquire(file='another.dat', opened=lopened, number=iunit)
    close(11, status='delete')
    if (icstat .eq. 0 .and. .not. lopened) then
       write(log_unit, 1000) name // ' simple open', &
            'reported opened successfully but inquire() returned ' // &
            'its not opened'
       test_open_log_file = .false.
       return
    end if
    if (icstat .ne. 0 .and. lopened) then
       write(log_unit, 1000) name // ' simple open', &
            'reported opened failed but inquire() returned its opened'
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. 11) then
       write(log_unit, 1001) name // ' simple open iunLPT', 11, iunLPT
       test_open_log_file = .false.
       return
    end if
    if (iunit .ne. 11) then
       write(log_unit, 1001) name // ' simple open iunit', 11, iunit
       test_open_log_file = .false.
       return
    end if

    fname = c_null_char
    call cLPTopenlogfile(12, lpath, c_loc(fname), 1, c_loc(icstat))
    inquire(unit=12, opened=lopened)
    close(12, status='delete')
    if (icstat .eq. 0 .and. .not. lopened) then
       write(log_unit, 1000) name // ' defname open', &
            'reported opened successfully but inquire() returned ' // &
            'its not opened'
       test_open_log_file = .false.
       return
    end if
    if (icstat .ne. 0 .and. lopened) then
       write(log_unit, 1000) name // ' defname open', &
            'reported opened failed but inquire() returned its opened'
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. 12) then
       write(log_unit, 1001) name // ' iunLPT', 12, iunLPT
       test_open_log_file = .false.
       return
    end if

    call cLPTopenlogfile(13, 0, c_null_ptr, 0, c_loc(icstat))
    inquire(unit=13, opened=lopened)
    close(13, status='delete')
    if (icstat .eq. 0 .and. .not. lopened) then
       write(log_unit, 1000) name // ' defname (null) open', &
            'reported opened successfully but inquire() returned ' // &
            'its not opened'
       test_open_log_file = .false.
       return
    end if
    if (icstat .ne. 0 .and. lopened) then
       write(log_unit, 1000) name // ' defname (null) open', &
            'reported opened failed but inquire() returned its opened'
       test_open_log_file = .false.
       return
    end if
    if (iunLPT .ne. 13) then
       write(log_unit, 1001) name // ' iunLPT', 13, iunLPT
       test_open_log_file = .false.
       return
    end if

    fname = 'dummy.dat' // c_null_char
    call cLPTopenlogfile(13, lpath, c_loc(fname), 0, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ' append test', 'open failed'
       test_open_log_file = .false.
       return
    end if
    write(13, '("This is line 1.")')
    close(13)

    call cLPTopenlogfile(14, lpath, c_loc(fname), 1, c_loc(icstat))
    if (icstat .ne. 0) then
       write(log_unit, 1000) name // ' append test', 'open failed'
       test_open_log_file = .false.
       return
    end if
    write(14, '("This is line 2.")')
    close(14)

    open(14, file='dummy.dat', status='old', form='formatted', action='read', iostat=istat)
    if (istat .ne. 0) then
       write(log_unit, 1000) name // ' append test', 'open failed for reading'
       test_open_log_file = .false.
       return
    end if
    read(14, '(a)') fname
    if (trim(fname) .ne. 'This is line 1.') then
       write(log_unit, 1000) name // ' append test', &
            'Got line 1: ' // trim(fname)
       test_open_log_file = .false.
    end if

    read(14, '(a)') fname
    if (trim(fname) .ne. 'This is line 2.') then
       write(log_unit, 1000) name // ' append test', &
            'Got line 2: ' // trim(fname)
       test_open_log_file = .false.
    end if
    close(14, status='delete')

1000 format('[ERROR] ', a, ' test failed: ', a)
1001 format('[ERROR] ', a, ' test failed: expected ', i10, ', but got ', i10)
  end function test_open_log_file
end module xLPTtest_open_log_file
