# This script only sets information for CMake 3.18 or later
cmake_minimum_required(VERSION 3.18)

if("$ENV{MODULESHOME}" STREQUAL "")
  message(FATAL_ERROR "This toolchain file is for SGI8600 system at JAEA

(environment variable MODULESHOME is not set)")
endif()
include($ENV{MODULESHOME}/init/cmake)

module(purge)
module(load "intel/2022.2.1")
module(load "mpt/2.23-ga")
module(load "cuda/11.8")

set(CMAKE_C_COMPILER icc)
set(CMAKE_CXX_COMPILER icpc)
set(CMAKE_Fortran_COMPILER ifort)
set(CMAKE_CUDA_COMPILER nvcc)
find_program(CMAKE_CUDA_HOST_COMPILER icpc)
set(CMAKE_CUDA_ARCHITECTURES 70)
set(CUDAToolkit_ROOT "$ENV{CUDA_PATH}")

set(CMAKE_C_FLAGS "-O3 -xHost -fma -fargument-noalias")
set(CMAKE_CXX_FLAGS "-O3 -xHost -fma -fargument-noalias")
set(CMAKE_CUDA_FLAGS "-O3 -Xcompiler -xHost -Xcompiler -fma -Xcompiler -fargument-noalias")
set(CMAKE_Fortran_FLAGS "-O3 -xHost -fma")

