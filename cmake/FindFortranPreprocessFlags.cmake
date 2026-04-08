
if ("${CMAKE_GENERATOR}" MATCHES "Ninja")
  message(AUTHOR_WARNING "Compiling Fortran sources with Ninja generator will always be preprocessed.")
endif()

if ("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.18")
  message(AUTHOR_WARNING "In CMake version 3.18 or later, we recommend to use Fortran_PREPROCESS property instead.")
endif()

include(CheckFortranSourceCompiles)

set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${FIND_Fortran_PREPROCESS_FLAGS_QUIETLY})

set(Fortran_PREPROCESS_TEST_SOURCE
"
#define TEST 1
       program test
       implicit none
       integer :: i
       i = TEST
#ifndef TEST
       j = 1
#endif
       write(*, *) i
       end
")

function(_Fortran_PREPROCESS_FLAG_CANDIDATES)
  set(FPP_FLAG_CANDIDATES
    #GNU gfortran, Pathscale, Absoft
    "-cpp"
    #Intel, IntelLLVM, SunPro
    "-fpp"
    #Intel, IntelLLVM (Windows)
    "/fpp"
    #PGI
    "-Mcpp"
    #G95
    # (N/A)
    #HP
    "+cpp=yes"
    #XL, MIPSPro
    # (unknown)
    )

  set(FPP_FLAG_GNU "-cpp")
  set(FPP_FLAG_PathScale "-cpp")
  set(FPP_FLAG_Absoft "-cpp")
  if(WIN32)
    set(FPP_FLAG_Intel "/fpp")
    set(FPP_FLAG_IntelLLVM "/fpp")
  else()
    set(FPP_FLAG_Intel "-fpp")
    set(FPP_FLAG_IntelLLVM "-fpp")
  endif()
  set(FPP_FLAG_SunPro "-fpp")
  set(FPP_FLAG_HP "+cpp=yes")
  if(FPP_FLAG_${CMAKE_Fortran_COMPILER_ID})
    list(REMOVE_ITEM FPP_FLAG_CANDIDATES "${FPP_FLAG_${CMAKE_Fortran_COMPILER_ID}}")
    list(INSERT FPP_FLAG_CANDIDATES 0 "${FPP_FLAG_${CMAKE_Fortran_COMPILER_ID}}")
  endif()
  set(Fortran_PREPROCESS_FLAG_CANDIDATES "${FPP_FLAG_CANDIDATES}" PARENT_SCOPE)
endfunction()

if(Fortran_PREPROCESS_FLAGS)
else()
  _Fortran_PREPROCESS_FLAG_CANDIDATES()
  foreach(FLAG IN LISTS Fortran_PREPROCESS_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    message(STATUS "Try preprocess flag -- [ ${FLAG} ]")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(Fortran_PREPROCESS_FLAG_DETECTED CACHE)
    check_fortran_source_compiles("${Fortran_PREPROCESS_TEST_SOURCE}" Fortran_PREPROCESS_FLAG_DETECTED)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(Fortran_PREPROCESS_FLAG_DETECTED)
      set(Fortran_PREPROCESS_FLAGS "${FLAG}")
      break()
    endif()
  endforeach()
  set(Fortran_PREPROCESS_FLAGS "${Fortran_PREPROCESS_FLAGS}"
    CACHE STRING "Fortran compiler flags to enable C-style preprocess")
endif()

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

find_package_handle_standard_args(FortranPreprocessFlags
  REQUIRED_VARS Fortran_PREPROCESS_FLAGS)
mark_as_advanced(Fortran_PREPROCESS_FLAGS)
