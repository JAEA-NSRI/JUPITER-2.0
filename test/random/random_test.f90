function random_test() bind(c, name='random_test_f90')
  use ISO_C_BINDING
  use jupiter_random_types
  use jupiter_random

  implicit none

#if (defined(_WIN32) || defined(__CYGWIN__)) && defined(JUPITER_RANDOM_TEST_F90_EXPORT)
#if defined(__GNUC__)
  !GCC$ ATTRICUTES DLLEXPORT :: random_test
#else
  ! Compiler macro unknown for DEC Fortran, Visual Fortran or Intel Fortran.
  !DEC$ ATTRIBUTES DLLEXPORT :: random_test
#endif
#endif

  integer(kind=c_int) :: random_test
  integer :: i
  type(random_seed) :: seed, oseed
  integer(kind = c_int64_t) :: ri, fi(5)
  real(kind = c_float) :: rf, ff(5)
  real(kind = c_double) :: rd, fd(5)

  data oseed%seed / &
       Z'e220a8397b1dcdaf', Z'6e789e6aa1b965f4', &
       Z'06c45d188009454f', Z'f88bb8a8724c81ec' /
  data fi / &
       Z'daac60e1ed6a4f9b', Z'3156a1da0dc08435', &
       Z'f9ba3e3285d046ab', Z'4fd194611dba7b01', &
       Z'40b78599c31791bf' /
  data ff / &
       0.8541927337646484_c_float, 0.19272810220718384_c_float, &
       0.9754980802536011_c_float, 0.31179165840148926_c_float, &
       0.2528002858161926_c_float /
  data fd / &
       0.8541927863674711_c_double, 0.19272815297677148_c_double, &
       0.9754980920168359_c_double, 0.3117916810130995_c_double, &
       0.2528003216167163_c_double /

  random_test = 0
  seed = oseed
  do i = lbound(fi, 1), ubound(fi, 1)
     ri = random_nexti(seed)
     write(*, *) i, ri, fi(i)
     if (ri .ne. fi(i)) then
        random_test = 1
     end if
  end do

  seed = oseed
  do i = lbound(ff, 1), ubound(ff, 1)
     rf = random_nextf(seed)
     write(*, *) i, rf, ff(i)
     if (rf .ne. ff(i)) then
        random_test = 1
     end if
  end do

  seed = oseed
  do i = lbound(fd, 1), ubound(fd, 1)
     rd = random_nextd(seed)
     write(*, *) i, rd, fd(i)
     if (rd .ne. fd(i)) then
        random_test = 1
     end if
  end do

  return
end function random_test
