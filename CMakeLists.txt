##
#  @addtogroup CMakeScripts
#  @{
#  @file JUPITER-src/CMakeLists.txt
#  @brief Master build cmake script
#

cmake_minimum_required(VERSION 3.12)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source-build is not permitted.
Create separate directory for build files.")
endif()
set(JUPITER_OUT_OF_SOURCE_BUILD TRUE)

# Honor <PackageName>_ROOT variable (ex. MPI_ROOT, VTK_ROOT, etc.)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Use old behavior of cmake_parse_arguments() if you are using CMake 3.31 or
# later.
if(POLICY CMP0174)
  cmake_policy(SET CMP0174 OLD)
endif()

# C compiler is required even when you build radiation only
project(JUPITER C)

# Configurable install directories
include(GNUInstallDirs)

function(make_standard_strings VAR LANG PREFIX ACCEPT)
  set(__STANDARDS)
  set(__FEATURES "${CMAKE_${LANG}_COMPILE_FEATURES}")
  foreach(__A IN LISTS ACCEPT)
    if("${PREFIX}${__A}" IN_LIST __FEATURES)
      list(APPEND __STANDARDS "${__A}")
    endif()
  endforeach()
  set_property(CACHE ${VAR} PROPERTY STRINGS "${__STANDARDS}")
endfunction()

# Set default standard to C99
set(CMAKE_C_STANDARD 99 CACHE STRING "C standard to conform")
make_standard_strings(CMAKE_C_STANDARD C c_std_ "99;11;17;20;23")

# Enable Fortran if compiler is present
include(CheckLanguage)
check_language(Fortran)
if(CMAKE_Fortran_COMPILER)
  enable_language(Fortran)

  # Required standard is 2008 for LPT module, but CMake does not
  # support restriction or checking about this.
  #
  # set(CMAKE_Fortran_STANDARD 2008)
endif()

# Enable C++ if compiler is present
check_language(CXX)
if(CMAKE_CXX_COMPILER)
  enable_language(CXX)

  # Set default satndard to C++11
  set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to conform")
  make_standard_strings(CMAKE_CXX_STANDARD CXX cxx_std_ "11;14;17;20;23")
endif()

check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
  enable_language(CUDA)

  # Set default standard to C++11
  set(CMAKE_CUDA_STANDARD 11 CACHE STRING "C++ (CUDA) standard to conform")
  make_standard_strings(CMAKE_CUDA_STANDARD CUDA cuda_std_ "11;14;17;20;23")

  # Set default CUDA architectures to be compiled for.
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18.0")
    if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
      set(CMAKE_CUDA_ARCHITECTURES 60 70)
    endif()
    if (NOT DEFINED CMAKE_CUDA_COMPILE_SEPARABLE_COMPILATION)
      set(CMAKE_CUDA_COMPILE_SEPARABLE_COMPILATION ON)
    endif()
  endif()
endif()

if(NOT DEFINED CMAKE_MODULE_PATH)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
else()
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
endif()

include(CMakeDependentOption)

## @var BUILD_JUPITER
#  @brief Whether build jupiter executable.
#

## @var BUILD_RADIATION
#  @brief Whether build radiation executable.
#
#  This program requires f90 compiler.
#
#  @todo This program currently cannot be built by other than Intel
#        Compiler.

## @var BUILD_DOCS
#  @brief Whether build of documentation.
#
#  To get document, run
#
#      make doc
#  or
#
#      cmake --build <builddir> -- doc
#
#  Just running `make` won't build docs even if `BUILD_DOCS` is set to `ON`.
#  This flag adds `doc` target and build of extra applications.
#
#  To build documentation,
#  [http://www.stack.nl/~dimitri/doxygen/](doxygen), which can parse
#  source code and extract comments as document, is required.
#
#  @note If you cross-compile JUPITER with documentation, you must
#        build `cmake-doxygen-filter` with native compiler, and give
#        path of it.

## @var BUILD_TESTING
#  @brief Whether build testing applications.
#
#  @note If you cross-compile JUPITER, you cannot test built applications.
#
option(BUILD_JUPITER   "Build Jupiter" ON)
option(BUILD_RADIATION "Build Radiation Coupler" OFF)
option(BUILD_DOCS      "Build documentation" OFF)
option(BUILD_TESTING   "Enable testing apps" OFF)
option(ENABLE_OPENMP   "Use OpenMP" OFF)

## @var BUILD_SHARED_LIBS
#  @brief Build shared libraries if possible
#
#  If you build libraries as shared (ON), it reduces the compilation
#  time and perform error-prone building but it may impact on the
#  performance. Also, in Linux, you need to do 'install' the
#  application to make it portable.
#
#  If you build libraries as static (OFF), it needs more compilation
#  time especially when you enabled link time optimization (LTO) to
#  improve performance but it may fail to compile.
#
option(BUILD_SHARED_LIBS "Build shared libraries (if available)" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# BX and ICEX flags are not used anymore.
# CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_BX "Enables Fujitsu BX900 specific code" ON "BUILD_JUPITER" OFF)
# CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_ICEX "Enables SGI ICE-X specific code" ON "BUILD_JUPITER" OFF)

##
# @var JUPITER_ENABLE_PROFILE
# @brief Whether enable PROFILE on `ccse_poisson()`
#

##
# @var JUPITER_ENABLE_RES_HISTORY
# @brief Whether print out residual history of `ccse_poisson()`
#

##
# @var JUPITER_ENABLE_METI
# @brief Whether include METI features
#

CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_PROFILE "Enable Profile" OFF "BUILD_JUPITER" OFF)

CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_RES_HISTORY "Enable Print Residual history on Poisson equation" OFF "BUILD_JUPITER" OFF)

CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_METI "Enable METI features" ON "BUILD_JUPITER" OFF)

CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_CUDA "Enable CUDA" ON "BUILD_JUPITER;CMAKE_CUDA_COMPILER_LOADED" OFF)

##
# @var JUPITER_USE_MPI
# @brief Whether JUPITER should use with MPI parallelization
#
# The default value is ON. If you know what you are doing, you can disable MPI.
# (i.e., it is not tested for very long time)
#

CMAKE_DEPENDENT_OPTION(JUPITER_USE_MPI "Enable MPI" ON "BUILD_JUPITER" OFF)
if(BUILD_JUPITER AND BUILD_RADIATION)
  unset(JUPITER_USE_MPI CACHE)
  set(JUPITER_USE_MPI ON CACHE INTERNAL
    "MPI is required for JUPITER-RADIATION coupling")
endif()

##
# @var JUPITER_FP_PRECISION
# @brief Floating-point precision (SINGLE/DOUBLE) to be used in JUPITER
#
# Single precision option is remained by GPU calculation were
# present. Some part of JUPITER uses double precision floating point
# values even in single version. (i.e., JUPITER can not be run on
# single-precision-only architectures)
#
# The default is DOUBLE (double precision).
#
# dev warning: Test data require results for *both* precisions.
#

if(BUILD_JUPITER)
  set(JUPITER_FP_PRECISION DOUBLE CACHE STRING "Floating point precision to be used in JUPITER")
  set_property(CACHE JUPITER_FP_PRECISION PROPERTY STRINGS "SINGLE;DOUBLE")
  if("${JUPITER_FP_PRECISION}" MATCHES "^(DOUBLE|SINGLE)$")
    message(STATUS "Building ${JUPITER_FP_PRECISION} precision JUPITER")
  else()
    message(SEND_ERROR "Selected invalid precision given for JUPITER_FP_PRECISION
Must be DOUBLE or SINGLE, but got ${JUPITER_FP_PRECISION}")
  endif()
else()
  unset(JUPITER_BUILD_PRECISION)
endif()

##
# @var BUILD_JUPITER_LPTX
# @brief Whether include LPTX (LPT in C) module

option(BUILD_JUPITER_LPTX "Build LPTX library" ON)

##
# @var BUILD_JUPITER_LPT
# @brief Whether include LPT module

option(BUILD_JUPITER_LPT "Build LPT (Fortran) library" ON)

##
# @var JUPITER_LPT_MODULE
# @brief LPT library form of LPT module
#
# If this option is set to INHERIT (or undefined), we will respect the
# value of BUILD_SHARED_LIBS, which is the standard variable of CMake.
#
# If this option is set to SHARED, the LPT module will be built as a
# shared library. It can avoid problems when we link
# triple-language-mixed (C/C++/Fortran) applications, where both C++
# and Fortran require their own language runtime libraries which we
# don't know their locations or even names.
#
# If this option is set to STATIC, the LPT module will be built as a
# static library. It is for casual running, but it may require
# additional compiler options, or even it may be impossible to build
# with some combinations of compilers.
if(DEFINED JUPITER_LPT_BUILD_SHARED)
  message(DEPRECATION "JUPITER_LPT_BUILD_SHARED is deprecated. Set JUPITER_LPT_MODULE to SHARED or STATIC instead")
  if(JUPITER_LPT_BUILD_SHARED)
    set(JUPITER_LPT_MODULE SHARED)
  else()
    set(JUPITER_LPT_MODULE STATIC)
  endif()
  unset(JUPITER_LPT_BUILD_SHARED CACHE)
else()
  if(NOT DEFINED JUPITER_LPT_MODULE)
    set(JUPITER_LPT_MODULE INHERIT)
  endif()
endif()
if(BUILD_JUPITER_LPT)
  set(JUPITER_LPT_MODULE "${JUPITER_LPT_MODULE}" CACHE STRING "Whether build LPT module as a shared library" FORCE)
else()
  set(JUPITER_LPT_MODULE "${JUPITER_LPT_MODULE}" CACHE INTERNAL "" FORCE)
endif()
set_property(CACHE JUPITER_LPT_MODULE PROPERTY STRINGS
  "SHARED" "STATIC" "INHERIT")

# VTK is optional, 7.1.1 or later is required when use.
#
# - Until VTK 9.0.1, XDMF output needs to be patched.
# - Until VTK 8.2.0, VTK XML output does not include TimeValue data
find_package(VTK 7.1.1)

# re2c is optional, 2.2 or later is required when use.
#
# Upstream: http://re2c.org/
find_package(RE2C 2.2)

# For non-C-standard function vasprintf()
find_package(Vasprintf)

# CUDA is optional. Version limitation is currently TBD.
#
# To disable CUDA after enable, please unset CMAKE_CUDA_COMPILER. In
# this case, you have to rebuild from a clean directory.
if(CMAKE_CUDA_COMPILER_LOADED)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    find_package(CUDAToolkit REQUIRED)
  else()
    find_package(CUDA REQUIRED)
    set(CUDAToolkit_FOUND ${CUDA_FOUND})
    set(CUDAToolkit_VERSION ${CUDA_VERSION})

    # FindCUDA does not define imported target
    foreach(__LIB cudart cuda_driver)
      add_library(CUDA::${__LIB} INTERFACE IMPORTED)
      set_target_properties(CUDA::${__LIB} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CUDA_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${CUDA_LIBRARIES}")
    endforeach()

    add_library(CUDA::cufft INTERFACE IMPORTED)
    set_target_properties(CUDA::cufft PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CUDA_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${CUDA_CUFFT_LIBRARY}")

    add_library(CUDA::cublas INTERFACE IMPORTED)
    set_target_properties(CUDA::cublas PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CUDA_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${CUDA_CUBLAS_LIBRARY}")

    foreach(__LIB cudadevrt cupti curand cusolver cusparse npp nppc
        nppi nppial nppicc nppicom nppidei nppif nppig nppim nppist
        nppisu nppitc npps nvcuvenc nvcuvid nvToolsExt OpenCL)
      if (CUDA_${__LIB}_LIBRARY)
        add_library(CUDA::${__LIB} INTERFACE IMPORTED)
        set_target_properties(CUDA::${__LIB} PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${CUDA_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${CUDA_${__LIB}_LIBRARY}")
      endif()
    endforeach()
  endif()
endif()

# MPI is required when building radiation, otherwise optional.
if(JUPITER_USE_MPI)
  find_package(MPI REQUIRED)
  if(MPI_FOUND AND DEFINED MPIEXEC_PREFLAGS AND NOT DEFINED MPIEXEC_FLAGS)
    set(MPIEXEC_FLAGS "" CACHE STRING
      "These flags will be directly after mpiexec, before numproc flag")
    mark_as_advanced(MPIEXEC_FLAGS)
  endif()
endif()

CMAKE_DEPENDENT_OPTION(JUPITER_ENABLE_CUDA_AWARE_MPI "Enable CUDA Aware MPI on Poisson equation" ON "JUPITER_ENABLE_CUDA;JUPITER_USE_MPI;MPI_C_FOUND;MPI_CXX_FOUND" OFF)

if(ENABLE_OPENMP)
  find_package(OpenMP REQUIRED)
endif()

##
# @var JUPITER_RECOMPILE_RE2C
# @brief Enable recompilation of re2c sources.
#
# If re2c is present in your system, this flag is enabled by
# default. If re2c verion which is used in provided sources is newer
# than which is you have, or, you don't want to recompile these
# sources with re2c, please set to `OFF`.
#
cmake_dependent_option(JUPITER_RECOMPILE_RE2C
  "Enable recompilation of re2c sources" ON
  "RE2C_FOUND" OFF)

##
# @var JUPITER_RE2C_RECOMPILE_POLICY
#
# If set to ONLY_BUILD, it recompiles re2c sources only for your
# builds.
#
# If set to DEVELOPMENT, it recompiles re2c sources both of your
# builds and updating precompiled sources. You can not see this option
# in the list of cache editor when you are using inappropriate version
# of re2c.
#
# The default is ONLY_BUILD.
if(JUPITER_RECOMPILE_RE2C)
  set(JUPITER_REQUIRED_RE2C_VERSION "4.5")
  set(JUPITER_RE2C_RECOMPILE_POLICY ONLY_BUILD CACHE STRING
    "Recompilation policy of re2c sources")

  set(__JUPITER_RE2C_RECOMPILE_POLICIES ONLY_BUILD)
  if(RE2C_VERSION VERSION_EQUAL JUPITER_REQUIRED_RE2C_VERSION)
    list(APPEND __JUPITER_RE2C_RECOMPILE_POLICIES DEVELOPMENT)
  endif()
  set_property(CACHE JUPITER_RE2C_RECOMPILE_POLICY PROPERTY
    STRINGS "${__JUPITER_RE2C_RECOMPILE_POLICIES}")
  if(NOT JUPITER_RE2C_RECOMPILE_POLICY IN_LIST __JUPITER_RE2C_RECOMPILE_POLICIES)
    message(SEND_ERROR "Invalid value for JUPITER_RE2C_RECOMPILE_POLICY, it must be one of followings, but DEVELOPMENT mode is only available for re2c ${JUPITER_REQUIRED_RE2C_VERSION} (you are using ${RE2C_VERSION}).
 * ONLY_BUILD  : Recompiles re2c sources for your builds only
 * DEVELOPMENT : Recompiles re2c sources for your builds and precompiled sources.
")
  endif()
else()
  unset(JUPITER_RE2C_RECOMPILE_POLICY CACHE)
endif()

##
# @fn jupiter_re2c_target(IN)
# @brief Add re2c source to compiled to C source and copy to *source*
#        directory if neccesary.
# @param IN Path to re2c source (.re).
#
# If `IN` is absolute path, uses it.
#
# If `IN` is not absolute path and is *GENERATED* by another rule(s),
# `IN` is considered to relative to current *binary* directory. If
# not, `IN` will be relative to current *source* directory.
#
define_property(GLOBAL PROPERTY __JUPITER_RE2C_GENERATED
  BRIEF_DOCS "List of sources that should be generated with jupiter-re2c-update"
  FULL_DOCS "List of sources that should be generated with jupiter-re2c-update")
define_property(GLOBAL PROPERTY __JUPITER_RE2C_SOURCES
  BRIEF_DOCS "List of sources for jupiter-re2c-update"
  FULL_DOCS "List of sources for jupiter-re2c-update")
function(jupiter_re2c_target IN)
  if(NOT RE2C_FOUND)
    return()
  endif()
  if(NOT JUPITER_RECOMPILE_RE2C)
    return()
  endif()

  get_filename_component(RE2C_D "${IN}" DIRECTORY)
  get_filename_component(RE2C_B "${IN}" NAME)
  string(REGEX REPLACE "\\.re$" "" RE2C_C "${RE2C_B}")
  if (NOT "${RE2C_D}" STREQUAL "")
    set(RE2C_C "${RE2C_D}/${RE2C_C}")
  endif()
  set(RE2C_CBF "${CMAKE_CURRENT_BINARY_DIR}/${RE2C_C}")
  set(RE2C_CSF "${CMAKE_CURRENT_SOURCE_DIR}/${RE2C_C}")
  if (NOT IS_ABSOLUTE "${IN}")
    get_source_file_property(GEN "${IN}" GENERATED)
    if (GEN)
      set(IN "${CMAKE_CURRENT_BINARY_DIR}/${IN}")
    else()
      set(IN "${CMAKE_CURRENT_SOURCE_DIR}/${IN}")
    endif()
  endif()

  add_re2c_target("${RE2C_CBF}" "${IN}")

  if (JUPITER_RE2C_RECOMPILE_POLICY STREQUAL "DEVELOPMENT")
    get_property(__G GLOBAL PROPERTY __JUPITER_RE2C_GENERATED)
    get_property(__S GLOBAL PROPERTY __JUPITER_RE2C_SOURCES)
    list(APPEND __G "${RE2C_CSF}")
    list(APPEND __S "${IN}")
    set_property(GLOBAL PROPERTY __JUPITER_RE2C_GENERATED "${__G}")
    set_property(GLOBAL PROPERTY __JUPITER_RE2C_SOURCES "${__S}")
  endif()
endfunction()


option(ENABLE_FPE "Enable Floating Point Exception in JUPITER" OFF)

include(CheckFortranModuleDefines)
if(BUILD_JUPITER_LPT)
  check_fortran_module_defines(Fortran_ISO_C_BINDING ISO_C_BINDING c_int c_float c_double c_f_pointer)
  check_fortran_module_defines(Fortran_ISO_FORTRAN_ENV ISO_FORTRAN_ENV error_unit output_unit)
  if(NOT Fortran_ISO_C_BINDING OR NOT Fortran_ISO_FORTRAN_ENV)
    message(SEND_ERROR "Fortran module ISO_C_BINDING and ISO_FORTRAN_ENV are not found

Fortran compiler must support Fortran 2003.")
  endif()
endif()

add_subdirectory(jupiter)
add_subdirectory(radiation)

if (BUILD_TESTING)
  enable_testing()
endif()

add_subdirectory(utils)

add_subdirectory(cmake)
add_subdirectory(test)
add_subdirectory(doc)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/print_conf.cmake")

if(JUPITER_RE2C_RECOMPILE_POLICY STREQUAL "DEVELOPMENT")
  get_property(__G GLOBAL PROPERTY __JUPITER_RE2C_GENERATED)
  get_property(__S GLOBAL PROPERTY __JUPITER_RE2C_SOURCES)
  list(LENGTH __G __L)
  math(EXPR __L "${__L} - 1")

  set(RE2CFLAGS_SAVE "${RE2C_FLAGS}")
  set(RE2CFLAGS "-i --no-generation-date -s")
  foreach(I RANGE ${__L})
    list(GET __G ${I} RE2C_CSF)
    list(GET __S ${I} IN)

    add_re2c_target("${RE2C_CSF}" "${IN}")
  endforeach()
  set(RE2CFLAGS "${RE2CFLAGS_SAVE}")

  add_custom_target(update-jupiter-re2c ALL COMMENT
    "Updated re2c genereted sources in source directory"
    DEPENDS ${__G}
    SOURCES ${__S})
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "jupiterConfigVersion.cmake"
  VERSION 0.0.1
  COMPATIBILITY AnyNewerVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/jupiterConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)

if(MPI_C_FOUND OR MPI_CXX_FOUND OR MPI_Fortran_FOUND)
  set(ENABLE_MPI TRUE)
endif()

configure_file("cmake/jupiterConfig.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/jupiterConfig.cmake" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/jupiterConfig.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)

## @}
