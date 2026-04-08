
if(NOT BUILD_JUPITER_LPT)
  return()
endif()

include(CheckFortranSourceCompiles)
include(CheckFortranCompilerFlag)
include(CheckFortranModuleDefines)

check_fortran_source_compiles("program test
implicit none
call implicitly_called_subroutine(1.0d0)
call implicitly_called_subroutine(1)
end program test
subroutine implicitly_called_subroutine(a)
end subroutine implicitly_called_subroutine
" Fortran_ALLOW_TYPE_MISMATCH SRC_EXT F90)

check_fortran_compiler_flag("-fallow-argument-mismatch"
  Fortran_FLAG_ALLOW_ARGUMENT_MISMATCH)
if(Fortran_FLAG_ALLOW_ARGUMENT_MISMATCH)
  set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS -fallow-argument-mismatch)
  check_fortran_source_compiles("program test
implicit none
call implicitly_called_subroutine(1.0d0)
call implicitly_called_subroutine(1)
end program test
subroutine implicitly_called_subroutine(a)
end subroutine implicitly_called_subroutine
" Fortran_ALLOW_TYPE_MISMATCH_WITH_FLAGS SRC_EXT F90)
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})
else()
  set(Fortran_ALLOW_TYPE_MISMATCH_WITH_FLAGS FALSE)
endif()

# This is Fortran 2018 feature
check_fortran_source_compiles("program test
implicit none
interface
  subroutine implicitly_called_subroutine(a)
    type(*) :: a
  end subroutine
end interface
call implicitly_called_subroutine(1.0d0)
call implicitly_called_subroutine(1)
end program test
subroutine implicitly_called_subroutine(a)
end subroutine implicitly_called_subroutine
" Fortran_SUPPORTS_ASSUMED_TYPE SRC_EXT F90)

if(JUPITER_USE_MPI)
  if(NOT TARGET MPI::MPI_Fortran)
    message(FATAL_ERROR "MPI Fortran support is required")
  endif()
  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_Fortran)
  check_fortran_module_defines(Fortran_MPI_MOD MPI)
  check_fortran_module_defines(Fortran_MPI_F08_MOD MPI_F08)
  check_fortran_module_defines(Fortran_MPI_MOD_INTERFACE MPI MPI_Allreduce)
  check_fortran_module_defines(Fortran_MPI_F08_MOD_INTERFACE MPI_F08 MPI_Allreduce)
endif()

set(LPT_Fortran_SOURCES LPTjupiter.f90 LPTval.f90 LPTcal.f LPTbnd.f90)
set(LPT_C_SOURCES LPTbnd.c)

set(JUPITER_LPT_IDEFS)
set(JUPITER_LPT_PDEFS)
set(JUPITER_LPT_DEFS)
set(JUPITER_LPT_INCS)
set(JUPITER_LPT_FFLAGS)
set(JUPITER_LPT_LFLAGS)
set(JUPITER_LPT_LIBS)

list(APPEND JUPITER_LPT_DEFS LPT)

if(JUPITER_LPT_MODULE STREQUAL "SHARED" OR
    (JUPITER_LPT_MODULE STREQUAL "INHERIT" AND BUILD_SHARED_LIBS))
  list(APPEND JUPITER_LPT_PDEFS JUPITER_LPT_EXPORT)
  list(APPEND JUPITER_LPT_IDEFS JUPITER_LPT_IMPORT)
endif()

if(JUPITER_FP_PRECISION STREQUAL SINGLE)
  # This makes C interface to use `float`. If not, the interface would
  # be `double`. This does not affect to LPT calculation, which is
  # always done with kind 8.
  list(APPEND JUPITER_LPT_DEFS  JUPITER_LPT_API_SINGLE)
endif()

# Ninja mandated the compiler must support preprocessing.
if (NOT "${CMAKE_GENERATOR}" MATCHES "Ninja" AND "${CMAKE_VERSION}" VERSION_LESS "3.18")
  find_package(FortranPreprocessFlags REQUIRED)
  list(APPEND JUPITER_LPT_FFLAGS "${Fortran_PREPROCESS_FLAGS}")
endif()

if(JUPITER_USE_MPI)
  if(Fortran_MPI_MOD_INTERFACE)
    list(APPEND JUPITER_LPT_PDEFS LPT_USE_MPI_INTERFACE)
  elseif(Fortran_MPI_F08_MOD_INTERFACE)
    list(APPEND JUPITER_LPT_PDEFS LPT_USE_MPI_F08_INTERFACE)
  else()
    if(NOT Fortran_ALLOW_TYPE_MISMATCH)
      if(Fortran_SUPPORTS_ASSUMED_TYPE)
        list(APPEND JUPITER_LPT_PDEFS LPT_DEFINE_MPI_INTERFACE)
      elseif(Fortran_ALLOW_TYPE_MISMATCH_WITH_FLAGS)
        list(APPEND JUPITER_LPT_FFLAGS -fallow-argument-mismatch)
      endif()
    endif()
  endif()

  list(APPEND JUPITER_LPT_PDEFS mpi)
  list(APPEND JUPITER_LPT_DEFS JUPITER_LPT_USE_MPI)
  list(APPEND JUPITER_LPT_LIBS MPI::MPI_Fortran)
endif()

if(ENABLE_OPENMP)
  list(APPEND JUPITER_LPT_LIBS OpenMP::OpenMP_Fortran)
endif()

set_source_files_properties(${LPT_Fortran_SOURCES} PROPERTIES
  COMPILE_OPTIONS "${JUPITER_LPT_FFLAGS}")

unset(_M)
if(NOT JUPITER_LPT_MODULE STREQUAL "INHERIT")
  set(_M "${JUPITER_LPT_MODULE}")
endif()
add_library(jupiter-lpt ${_M} ${LPT_Fortran_SOURCES} ${LPT_C_SOURCES})
add_library(JUPITER::jupiter-lpt ALIAS jupiter-lpt)

target_compile_definitions(jupiter-lpt
  PRIVATE ${JUPITER_LPT_PDEFS}
  PUBLIC ${JUPITER_LPT_DEFS}
  INTERFACE ${JUPITER_LPT_IDEFS})
target_link_options(jupiter-lpt PRIVATE ${JUPTER_LPT_LFLAGS})
target_include_directories(jupiter-lpt
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:Fortran>:${CMAKE_CURRENT_BINARY_DIR}>>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLDUEDIR}>
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(jupiter-lpt ${JUPITER_LPT_LIBS})

set(JUPITER_LPT_PUB_HEADERS
  LPTbnd.h LPTdefs.h)

set_target_properties(jupiter-lpt PROPERTIES
  PUBLIC_HEADER "${JUPITER_LPT_PUB_HEADERS}")

if ("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.18")
  set_target_properties(jupiter-lpt PROPERTIES
    Fortran_PREPROCESS ON)
endif()

install(TARGETS jupiter-lpt EXPORT LPT
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/lpt)

install(EXPORT LPT
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter
  EXPORT_LINK_INTERFACE_LIBRARIES)
