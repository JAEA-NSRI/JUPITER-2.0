
include(CheckFortranSourceCompiles)

set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${FIND_VAXStructureFlags_QUIETLY})

# Intel compiler does not support `structure /foo/` notation
# So here, using `type name`.
set(VAX_STRUCTURE_TEST_SOURCE
"
       module mod_test
       type item1
         union
           map
             character(2) w0, w1, w2
           end map
           map
             character(6) long
           end map
         end union
       end type
       end module

       program test
       use mod_test
       implicit none
       type(item1) :: rec
       rec%long = 'hello!'
       end
")

function(_VAX_STRUCTURE_FLAG_CANDIDATES)
  set(VAXS_FLAG_CANDIDATES
    #GNU gfortran
    "-fdec-structure" # Recent version required.
    #Intel, SunPro
    " " # None required
    #PGI
    # (N/A)
    #G95
    # (N/A)
    #HP, XL, MIPSPro
    # (unknown)
    )

  set(VAXS_FLAG_GNU "-fdec-structure")
  set(VAXS_FLAG_Intel " ")
  set(VAXS_FLAG_SunPro " ")
  if(VAXS_FLAG_${CMAKE_Fortran_COMPILER_ID})
    list(REMOVE_ITEM VAXS_FLAG_CANDIDATES "${VAXS_FLAG_${CMAKE_Fortran_COMPILER_ID}}")
    list(INSERT VAXS_FLAG_CANDIDATES 0 "${VAXS_FLAG_${CMAKE_Fortran_COMPILER_ID}}")
  endif()
  set(VAX_STRUCTURE_FLAG_CANDIDATES "${VAXS_FLAG_CANDIDATES}" PARENT_SCOPE)
endfunction()

if(VAX_STRUCTURE_FLAGS)
else()
  _VAX_STRUCTURE_FLAG_CANDIDATES()
  foreach(FLAG IN LISTS VAX_STRUCTURE_FLAG_CANDIDATES)
    set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    message(STATUS "Try VAX Structure flag -- [ ${FLAG} ]")
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    unset(VAX_STRUCTURE_FLAG_DETECTED CACHE)
    check_fortran_source_compiles("${VAX_STRUCTURE_TEST_SOURCE}" VAX_STRUCTURE_FLAG_DETECTED)
    set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
    if(VAX_STRUCTURE_FLAG_DETECTED)
      set(VAX_STRUCTURE_FLAGS "${FLAG}")
      break()
    endif()
  endforeach()
  set(VAX_STRUCTURE_FLAGS "${VAX_STRUCTURE_FLAGS}"
    CACHE STRING "Fortran compiler flags to use VAX structure")
endif()

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})

find_package_handle_standard_args(VAXStructure
  REQUIRED_VARS VAX_STRUCTURE_FLAGS)
mark_as_advanced(VAX_STRUCTURE_FLAGS)
