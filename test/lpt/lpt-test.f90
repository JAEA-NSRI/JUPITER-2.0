module xLPTtest
  use ISO_C_BINDING
  use xLPTtest_val, only: log_unit

  implicit none
contains
  integer(kind=c_int) function xLPTtest_istat() bind(c,name='xLPTtest_istat')
    use xLPTtest_val, only: istat

    xLPTtest_istat = istat
    return
  end function xLPTtest_istat

  subroutine xLPTtest_run(name) bind(c,name='xLPTtest_run')
    use xLPTtest_val, only: istat

    type(c_ptr), value :: name
    character(len=:), pointer :: fname
    integer :: i

    call c_f_pointer(name, fname)
    do i = 1, 100
       if (fname(i:i) .eq. c_null_char) exit
    end do

    if (i .le. 1) then
       write(log_unit, '("[ERROR] Empty string given for name")')
       istat = 998
       return
    end if

    i = i - 1
    call xLPTtest_run_f(fname(1:i), i)
  end subroutine xLPTtest_run

  subroutine xLPTtest_run_f(name, i)
    use xLPTtest_val, only: istat
    use xLPTtest_mxpset, only: test_mxpset, name_mxpset => name
    use xLPTtest_set_get_npset, only: test_set_get_npset, name_set_get_npset => name
    use xLPTtest_set_get_pset, only: test_set_get_pset, name_set_get_pset => name
    use xLPTtest_set_get_ipttim, only: test_set_get_ipttim, name_set_get_ipttim => name
    use xLPTtest_open_log_file, only: test_open_log_file, name_open_log_file => name
    use xLPTtest_allocate, only: test_allocate, name_allocate => name
    use xLPTtest_set_get_pts, only: test_set_get_pts, name_set_get_pts => name

    integer, intent(in) :: i
    character(len=i), intent(in) :: name
    logical :: lstat

    lstat = .false.
    write(*, '("Testing ", a, "...")') name
    select case (name)
    case (name_mxpset)
       lstat = test_mxpset()
    case ('set_get_npset')
       lstat = test_set_get_npset()
    case ('set_get_pset')
       lstat = test_set_get_pset()
    case ('set_get_ipttim')
       lstat = test_set_get_ipttim()
    case ('open_log_file')
       lstat = test_open_log_file()
    case ('allocate')
       lstat = test_allocate()
    case ('set_get_pts')
       lstat = test_set_get_pts()
    case default
       write(log_unit, '("[ERROR] No such fortran test: ", a)') name
       istat = 999
    end select
    if (lstat) then
       write(*, '(" ---> OK")')
    else
       if (istat .eq. 0) then
          istat = 1
       end if
    end if
    return
  end subroutine xLPTtest_run_f
end module xLPTtest
