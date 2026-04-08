# This script only sets information for CMake 3.18 or later
cmake_minimum_required(VERSION 3.18)

if("$ENV{MODULESHOME}" STREQUAL "")
  message(FATAL_ERROR "This toolchain file is for SGI8600 system at JAEA

(environment variable MODULESHOME is not set)")
endif()
include($ENV{MODULESHOME}/init/cmake)

module(purge)
module(load "gnu/12.4.0")
module(load "mpt/2.29")
module(load "cuda/12.6")

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_Fortran_COMPILER gfortran)
set(CMAKE_CUDA_COMPILER nvcc)
find_program(CMAKE_CUDA_HOST_COMPILER g++)
set(CMAKE_CUDA_ARCHITECTURES 70)
set(CUDAToolkit_ROOT "$ENV{CUDA_PATH}")

set(CMAKE_C_FLAGS "-O3 -mtune=native -mfma -fargument-noalias")
set(CMAKE_CXX_FLAGS "-O3 -mtune=native -mfma -fargument-noalias")
set(CMAKE_CUDA_FLAGS "-O3 -Xcompiler -mtune=native -Xcompiler -mfma -Xcompiler -fargument-noalias")
set(CMAKE_Fortran_FLAGS "-O3 -mtune=native -mfma")

