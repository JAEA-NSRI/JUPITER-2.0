##
#  @addtogroup CMakeScripts
#  @brief documentation of user-defined function and variables in CMake scripts.
#  @{
#  @file print_conf.cmake
#  @brief Configuration printer
#
#  Print message of configuration of JUPITER and include time

# Output configuration info
string(TOUPPER "${CMAKE_BUILD_TYPE}" BT)
set(CONF_MESSAGE "")

## @brief add configuration message
#  @param  ... message
#  @return append message to `CONF_MESSAGE`
#
macro(add_conf_message)
  if(${ARGC} GREATER 0)
    set(T "${ARGV}")
  else()
    set(T "")
  endif()
  set(CONF_MESSAGE "${CONF_MESSAGE}
** ${T}")
endmacro()

macro(add_conf_header TEXT)
  set(S "                                                         ")
  string(LENGTH "${TEXT}" LT)
  string(LENGTH "${S}" LS)
  math(EXPR LB "${LS} / 2 - ${LT} / 2 - 2")
  if("${LB}" LESS 0)
    set(LB 0)
  endif()
  string(SUBSTRING "${S}" 0 "${LB}" S)
  add_conf_message("${S}** ${TEXT} **")
endmacro()

## @brief add name value pair to message
#  @param  NAME variable title
#  @param  VALUE value of the variable
#  @return append message to `CONF_MESSAGE`
#
macro(add_conf_value NAME VALUE)
  set(S "                         ")
  string(LENGTH "${NAME}" LN)
  string(LENGTH "${S}" LS)
  math(EXPR LB "${LS} - ${LN}")
  if("${LB}" LESS 0)
    set(LB 0)
  endif()
  string(SUBSTRING "${S}" 0 "${LB}" S)
  add_conf_message("${S}${NAME} : ${VALUE}")
endmacro()

unset(__BIG)
if(CMAKE_C_COMPILER_LOADED)
  include(TestBigEndian)
  TEST_BIG_ENDIAN(__BIG)
endif()

add_conf_value("Build Type" "${CMAKE_BUILD_TYPE}")
if(CMAKE_C_COMPILER_LOADED)
  add_conf_value("C Compiler" "${CMAKE_C_COMPILER}  (${CMAKE_C_COMPILER_ID})")
  add_conf_value("C Flags" "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${BT}}")
endif()
if(CMAKE_CXX_COMPILER_LOADED)
  add_conf_value("C++ Compiler" "${CMAKE_CXX_COMPILER}  (${CMAKE_CXX_COMPILER_ID})")
  add_conf_value("C++ Flags" "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BT}}")
endif()
if(CMAKE_CUDA_COMPILER_LOADED)
  add_conf_value("CUDA Compiler" "${CMAKE_CUDA_COMPILER}  (${CMAKE_CUDA_COMPILER_ID})")
  add_conf_value("CUDA Host Compiler" "${CMAKE_CUDA_HOST_COMPILER}")
  add_conf_value("CUDA Flags" "${CMAKE_CUDA_FLAGS} ${CMAKE_CUDA_FLAGS_${BT}}")
endif()
if(CMAKE_Fortran_COMPILER_LOADED)
  add_conf_value("Fortran Compiler" "${CMAKE_Fortran_COMPILER}  (${CMAKE_Fortran_COMPILER_ID})")
  add_conf_value("Fortran Flags" "${CMAKE_Fortran_FLAGS} ${CMAKE_Fortran_FLAGS_${BT}}")
endif()
add_conf_value("Enable Test" "${BUILD_TESTING}")

add_conf_message()
add_conf_header("System Info")
add_conf_value("OS (CMake System Name)" "${CMAKE_SYSTEM_NAME}")
if (DEFINED __BIG)
  if (__BIG EQUAL 1)
    add_conf_value("Byte Order" "Big Endian")
  else()
    add_conf_value("Byte Order" "Little Endian")
  endif()
else()
  add_conf_value("Byte Order" "Not tested")
endif()

# MPI
foreach(_LANG C CXX Fortran)
  # The language CXX is about whether MPI C API is callable from C++.
  if(_LANG STREQUAL "CXX")
    set(_LANG_D "C++")
  else()
    set(_LANG_D "${_LANG}")
  endif()
  if(CMAKE_${_LANG}_COMPILER_LOADED)
    if(DEFINED MPI_${_LANG}_FOUND)
      if(MPI_${_LANG}_FOUND)
        if(MPI_${_LANG}_LIB_NAMES)
          string(REPLACE ";" ", " _V "${MPI_${_LANG}_LIB_NAMES}")
          set(_V " (${_V})")
        else()
          set(_V "")
        endif()
        add_conf_value("MPI ${_LANG_D} found" "YES${_V}")
      else()
        add_conf_value("MPI ${_LANG_D} found" "(not found)")
      endif()
    else()
      add_conf_value("MPI ${_LANG_D} found" "(not requested for search)")
    endif()
  endif()
endforeach()

# OpenMP
foreach(_MOD OpenMP)
  foreach(_LANG C CXX Fortran)
    if(_LANG STREQUAL "CXX")
      set(_LANG_D "C++")
    else()
      set(_LANG_D "${_LANG}")
    endif()
    if(CMAKE_${_LANG}_COMPILER_LOADED)
      if(DEFINED ${_MOD}_${_LANG}_FOUND)
        if(${_MOD}_${_LANG}_FOUND)
          add_conf_value("${_MOD} for ${_LANG_D}" "${${_MOD}_${_LANG}_FLAGS}")
        else()
          add_conf_value("${_MOD} for ${_LANG_D}" "(not available)")
        endif()
      else()
        add_conf_value("${_MOD} for ${_LANG_D}" "(not requested for check)")
      endif()
    endif()
  endforeach()
endforeach()

# CUDA
if(CMAKE_CUDA_COMPILER_LOADED)
  if(CUDAToolkit_FOUND)
    add_conf_value("CUDA found" "YES (${CUDAToolkit_VERSION})")
  else()
    add_conf_value("CUDA found" "NO")
  endif()
endif()

# VTK
if(VTK_FOUND)
  add_conf_value("VTK found" "YES (${VTK_VERSION})")
else()
  add_conf_value("VTK found" "NO")
endif()

# re2c
if(RE2C_FOUND)
  add_conf_value("re2c found" "YES (${RE2C_VERSION})")
else()
  add_conf_value("re2c found" "NO")
endif()

add_conf_message()
add_conf_header("Global options")
add_conf_value("Use OpenMP" "${ENABLE_OPENMP}")
add_conf_value("Build shared libraries" "${BUILD_SHARED_LIBS}")

add_conf_message()
add_conf_header("Binaries to build")
add_conf_value("Build JUPITER" "${BUILD_JUPITER}")
add_conf_value("Build RADIATION" "${BUILD_RADIATION}")
if(TARGET make-vtk)
  add_conf_value("Build VTK XML generator" "ON")
else()
  add_conf_value("Build VTK XML generator" "(dependency error)")
endif()
if(TARGET make-xdmf)
  add_conf_value("Build XDMF generator" "ON")
else()
  add_conf_value("Build XDMF generator" "(dependency error)")
endif()

add_conf_message()
add_conf_header("Optional libraries to build")
add_conf_value("Build LPT module" "${BUILD_JUPITER_LPT}")
add_conf_value("Build LPTX module" "${BUILD_JUPITER_LPTX}")

if(BUILD_JUPITER)
  add_conf_message()
  add_conf_header("JUPITER options")
  add_conf_value("Floating-point precision" "${JUPITER_FP_PRECISION}")
  add_conf_value("Use MPI" "${JUPITER_USE_MPI}")
  add_conf_value("Use CUDA" "${JUPITER_ENABLE_CUDA}")
  if(JUPITER_ENABLE_CUDA AND JUPITER_USE_MPI)
    add_conf_value("Use CUDA AWARE MPI" "${JUPITER_ENABLE_CUDA_AWARE_MPI}")
  endif()
  if(JUPITER_ENABLE_CUDA)
    get_target_property(__CUDA_ARCH libjupiter CUDA_ARCHITECTURES)
    if(__CUDA_ARCH)
      add_conf_value("CUDA Archs to build" "${__CUDA_ARCH}")
    endif()
    foreach(DIR X Y Z)
      set(_VAR JUPITER_SOLVER_CUDA_N${DIR}BLOCK)
      set(_VAL "${${_VAR}}")
      if("${_VAL}" STREQUAL "")
        set(_VAL "(use hard-coded default)")
      endif()
      add_conf_value("CUDA solver ${DIR} block size" "${_VAL}")
    endforeach()
  endif()
  add_conf_value("Poisson Solver" "${JUPITER_SOLVER}")
  add_conf_value("Poisson Residual Output" "${JUPITER_ENABLE_RES_HISTORY}")
  add_conf_value("Profile Poisson" "${JUPITER_ENABLE_PROFILE}")
  add_conf_value("METI Feature" "${JUPITER_ENABLE_METI}")
  add_conf_value("Particle Tracking" "${JUPITER_ENABLE_LPT}")
endif()
if(BUILD_JUPITER_LPT)
  add_conf_message()
  add_conf_header("LPT module options")
  get_target_property(__T jupiter-lpt TYPE)
  string(REPLACE "_LIBRARY" "" __T "${__T}")
  add_conf_value("Library Form" "${__T}")
endif()

message("
******************** JUPITER/RADIATION configuration ***********************${CONF_MESSAGE}
****************************************************************************
")

## @}
