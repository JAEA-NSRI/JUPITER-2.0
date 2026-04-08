# This is CPU only.

if("$ENV{MODULESHOME}" STREQUAL "")
  message(FATAL_ERROR "This toolchain file is for SGI8600 system at JAEA

(environment variable MODULESHOME is not set)")
endif()
include($ENV{MODULESHOME}/init/cmake)

module(purge)
module(load "intel/2025.1.0")
module(load "mpt/2.29")

# intel module does not set CC, CXX, FC etc.
set(CMAKE_C_COMPILER icx)
set(CMAKE_CXX_COMPILER icpx)
set(CMAKE_Fortran_COMPILER ifx)

set(CMAKE_C_FLAGS "-O3 -xHost -fma -fargument-noalias")
set(CMAKE_CXX_FLAGS "-O3 -xHost -fma -fargument-noalias")
set(CMAKE_Fortran_FLAGS "-O3 -xHost -fma")
