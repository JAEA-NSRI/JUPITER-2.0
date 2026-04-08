
include(CheckFortranSourceCompiles)

##
# @fn check_fortran_module_defines(RESULT MODULE SYMBOL_NAMES_FOR_CHECKING...)
# @param RESULT Variable name to store result
# @param MODULE Fortran module nmae
# @param SYMBOL_NAMES_FOR_CHECKING... Symbol names in ISO_FORTRAN_ENV to be checked
#
# `RESULT` would be true only if all given symbol names do exist.
#
# `RESULT` would be automatically cached (via check_fortran_source_compiles)
function(check_fortran_module_defines RESULT MODULE)
  if(DEFINED "${RESULT}")
    return()
  endif()
  set(__USE_DECLARATIONS)
  set(__N 1)
  set(__L "${ARGN}")
  list(LENGTH __L __C)
  foreach(__V IN LISTS __L)
    set(__USE_DECLARATIONS "${__USE_DECLARATIONS}use ${MODULE},only:${__V}
")
    if(${__N} LESS 3 OR ${__N} EQUAL ${__C})
      if(NOT DEFINED __MSG)
        set(__MSG "defines ${__V}")
      else()
        set(__MSG "${__MSG}, ${__V}")
      endif()
    elseif(${__N} EQUAL 3)
      set(__MSG "${__MSG}, ...")
    endif()
    math(EXPR __N "${N} + 1")
  endforeach()
  if(NOT DEFINED __MSG)
    set(__USE_DECLARATIONS "use ${MODULE}
")
    set(__MSG "exists")
  endif()
  set(__SRC "
program test
${__USE_DECLARATIONS}implicit none
end program test
")
  if(NOT DEFINED CMAKE_REQUIRED_QUIET OR NOT CMAKE_REQUIRED_QUIET)
    set(__MSG "Checking whether Fortran module ${MODULE} ${__MSG}")
    message(STATUS "${__MSG}")
  else()
    unset(__MSG)
  endif()
  set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_fortran_source_compiles("${__SRC}" ${RESULT} SRC_EXT F90)
  set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})
  if(DEFINED __MSG)
    if(${${RESULT}})
      message(STATUS "${__MSG} -- ok")
      set(${RESULT} TRUE)
    else()
      message(STATUS "${__MSG} -- notfound")
      set(${RESULT} ${RESULT}-NOTFOUND)
    endif()
  endif()
endfunction()
