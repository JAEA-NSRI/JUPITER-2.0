module xLPTtest_val
  use ISO_C_BINDING, only: c_int
  use ISO_FORTRAN_ENV, only: ERROR_UNIT

  integer(kind=c_int) :: istat = 0
  integer, parameter :: log_unit = ERROR_UNIT
end module xLPTtest_val
