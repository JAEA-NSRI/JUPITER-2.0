module jupiter_random_types
  use ISO_C_BINDING

  implicit none

  type, bind(c) :: random_seed
     integer(kind = c_int64_t) :: seed(4)
  end type random_seed

  type, bind(c) :: random_seed_counter
     type(random_seed) :: seed
     integer(kind = c_int64_t) :: counter !! Actually it is unsigned
  end type random_seed_counter
end module jupiter_random_types

!! Intel Compiler removes `!` comments on preprocessor line before preprocessing
!! as the preprocessor for C/C++ does for the comments in their languages, but
!! does not for the regular line. It expands macros even if its in the comment
!! on the regular line. This means that we cannot define a macro includes token
!! of `!` in the source code.
!!
!! GNU gfortran and PGI treats `!` as a regular character (the preprocessor will
!! never remove them).
#if defined(JUPITER_RANDOM_F90_IMPORT)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(__INTEL_COMPILER) || defined(__PGI) || defined(__NVCOMPILER)
#define RANDOM_IMPORT(name) DEC$ ATTRIBUTES DLLIMPORT :: name
#elif defined(__GNUC__)
#define RANDOM_IMPORT(name) GCC$ ATTRIBUTES DLLIMPORT :: name
#else
#define RANDOM_IMPORT(name) ! unknown
#endif
#else
#define RANDOM_IMPORT(name) ! dynamic, but not win32
#endif
#else
#define RANDOM_IMPORT(name) ! static
#endif

module jupiter_random
  use ISO_C_BINDING
  use jupiter_random_types

  implicit none

  interface
     function random_nexti(seed) bind(c, name="jupiter_random_nexti_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nexti_f90)
       type(random_seed) :: seed
       integer(kind = c_int64_t) :: random_nexti
     end function random_nexti

     function random_nextis(seed) bind(c, name="jupiter_random_nextis_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextis_f90)
       type(random_seed) :: seed
       integer(kind = c_int64_t) :: random_nextis
     end function random_nextis

     function random_nexti63(seed) bind(c, name="jupiter_random_nexti63_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nexti63_f90)
       type(random_seed) :: seed
       integer(kind = c_int64_t) :: random_nexti63
     end function random_nexti63

     function random_nexti8(seed) bind(c, name="jupiter_random_nexti8_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nexti8_f90)
       type(random_seed) :: seed
       integer(kind = c_int8_t) :: random_nexti8
     end function random_nexti8

     function random_nextic(seed) bind(c, name="jupiter_random_nextic_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextic_f90)
       type(random_seed_counter) :: seed
       integer(kind = c_int64_t) :: random_nextic
     end function random_nextic

     function random_nextf(seed) bind(c, name="jupiter_random_nextf_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextf_f90)
       type(random_seed) :: seed
       real(kind = c_float) :: random_nextf
     end function random_nextf

     function random_nextfc(seed) bind(c, name="jupiter_random_nextfc_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextfc_f90)
       type(random_seed_counter) :: seed
       real(kind = c_float) :: random_nextfc
     end function random_nextfc

     function random_nextd(seed) bind(c, name="jupiter_random_nextd_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextd_f90)
       type(random_seed) :: seed
       real(kind = c_double) :: random_nextd
     end function random_nextd

     function random_nextde(seed) bind(c, name="jupiter_random_nextde_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextde_f90)
       type(random_seed) :: seed
       real(kind = c_double) :: random_nextde
     end function random_nextde

     function random_nextdn(seed) bind(c, name="jupiter_random_nextdn_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextdn_f90)
       type(random_seed) :: seed
       real(kind = c_double) :: random_nextdn
     end function random_nextdn

     function random_nextdc(seed) bind(c, name="jupiter_random_nextdc_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextdc_f90)
       type(random_seed_counter) :: seed
       real(kind = c_double) :: random_nextd
     end function random_nextdc

     function random_nextn(seed, n) bind(c, name="jupiter_random_nextn_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextn_f90)
       type(random_seed) :: seed
       integer(kind = c_int32_t) :: n
       integer(kind = c_int32_t) :: random_nextn
     end function random_nextn

     function random_nextnc(seed, n) bind(c, name="jupiter_random_nextnc_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_nextnc_f90)
       type(random_seed_counter) :: seed
       integer(kind = c_int32_t) :: n
       integer(kind = c_int32_t) :: random_nextnc
     end function random_nextnc

     subroutine random_jump32(seed) bind(c, name="jupiter_random_jump32_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump32_f90)
       type(random_seed) :: seed
     end subroutine random_jump32

     subroutine random_jump32c(seed) bind(c, name="jupiter_random_jump32c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump32c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump32c

     subroutine random_jump48(seed) bind(c, name="jupiter_random_jump48_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump48_f90)
       type(random_seed) :: seed
     end subroutine random_jump48

     subroutine random_jump48c(seed) bind(c, name="jupiter_random_jump48c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump48c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump48c

     subroutine random_jump64(seed) bind(c, name="jupiter_random_jump64_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump64_f90)
       type(random_seed) :: seed
     end subroutine random_jump64

     subroutine random_jump64c(seed) bind(c, name="jupiter_random_jump64c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump64c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump64c

     subroutine random_jump96(seed) bind(c, name="jupiter_random_jump96_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump96_f90)
       type(random_seed) :: seed
     end subroutine random_jump96

     subroutine random_jump96c(seed) bind(c, name="jupiter_random_jump96c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump96c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump96c

     subroutine random_jump128(seed) bind(c, name="jupiter_random_jump128_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump128_f90)
       type(random_seed) :: seed
     end subroutine random_jump128

     subroutine random_jump128c(seed) bind(c, name="jupiter_random_jump128c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump128c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump128c

     subroutine random_jump160(seed) bind(c, name="jupiter_random_jump160_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump160_f90)
       type(random_seed) :: seed
     end subroutine random_jump160

     subroutine random_jump160c(seed) bind(c, name="jupiter_random_jump160c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump160c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump160c

     subroutine random_jump192(seed) bind(c, name="jupiter_random_jump192_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump192_f90)
       type(random_seed) :: seed
     end subroutine random_jump192

     subroutine random_jump192c(seed) bind(c, name="jupiter_random_jump192c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump192c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump192c

     subroutine random_jump224(seed) bind(c, name="jupiter_random_jump224_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump224_f90)
       type(random_seed) :: seed
     end subroutine random_jump224

     subroutine random_jump224c(seed) bind(c, name="jupiter_random_jump224c_f90")
       use ISO_C_BINDING
       use jupiter_random_types
       !RANDOM_IMPORT(jupiter_random_jump224c_f90)
       type(random_seed_counter) :: seed
     end subroutine random_jump224c
  end interface

contains
!$  subroutine random_jump32_omp(shared, local)
!$    use jupiter_random_types
!$    use omp_lib
!$    implicit none
!$    type(random_seed) :: shared
!$    type(random_seed), intent(out) :: local
!$    integer tid
!$
!$    if (omp_get_num_threads() .gt. 1) then
!$       !omp$ barrier
!$       do tid = 0, omp_get_num_threads()
!$          if (tid .eq. omp_get_thread_num()) then
!$             call random_jump32(shared)
!$             local = shared
!$          end if
!$          !omp$ barrier
!$       end do
!$    else
!$       local = shared
!$    end if
!$  end subroutine random_jump32_omp
end module jupiter_random
