
add_subdirectory(os)
add_subdirectory(re2c_lparser)
add_subdirectory(random)
add_subdirectory(geometry)
add_subdirectory(table)
add_subdirectory(serializer)
add_subdirectory(lpt)
add_subdirectory(lptx)
add_subdirectory(control)

if(NOT BUILD_JUPITER)
  return()
endif()

if(JUPITER_USE_OPENMP AND NOT JUPITER_GEOMETRY_USE_OPENMP)
  message(SEND_ERROR "To build with JUPITER, OpenMP support is required for geometry library")
endif()

set(JUPITER_CPU_SOLVERS CG BiCG)
set(JUPITER_CUDA_SOLVERS CUDA_CG)
set(JUPITER_ALL_SOLVERS ${JUPITER_CPU_SOLVERS} ${JUPITER_CUDA_SOLVERS})

set(JUPITER_AVAILABLE_SOLVERS)
set(JUPITER_DEFAULT_SOLVER CG)

if(JUPITER_ENABLE_CUDA)
  list(APPEND JUPITER_AVAILABLE_SOLVERS ${JUPITER_CUDA_SOLVERS})
endif()

list(APPEND JUPITER_AVAILABLE_SOLVERS ${JUPITER_CPU_SOLVERS})

set(JUPITER_SOLVER ${JUPITER_DEFAULT_SOLVER} CACHE STRING
  "JUPITER Poisson Solver")
set_property(CACHE JUPITER_SOLVER PROPERTY
  STRINGS ${JUPITER_AVAILABLE_SOLVERS})

if(NOT JUPITER_SOLVER IN_LIST JUPITER_AVAILABLE_SOLVERS)
  string(REPLACE ";" "
    * " JUPITER_SOLVERS_LIST "${JUPITER_AVAILABLE_SOLVERS}")
  set(_MSG  "Invalid JUPITER_SOLVER used. Available solvers are:
    * ${JUPITER_SOLVERS_LIST}")
  if(JUPITER_SOLVER IN_LIST JUPITER_CUDA_SOLVERS)
    set(_MSG "${_MSG}
Note: To use ${JUPITER_SOLVER} solver, CUDA is required")
  endif()
  message(SEND_ERROR "${_MSG}")
endif()

set(JUPITER_RE2C_SOURCES
  csv.c.re csvutil_extra.c.re csvtmpl_format.c.re optparse.c.re)

set(JUPITER_SOURCES initial.c non_uniform_grid.c thinc.c plic.c phasefield.c
  level_set.c multi_layer.c
  tvd_runge_kutta.c advection.c viscosity.c surface.c forch.c
  divergence_free.c materials.c phase_change.c heat_source.c lpt.c
  IBM.c dt_control.c boundary.c set_param.c param.c set_geom.c
  io_process.c communication.c physical_properties.c analysis_cut.c
  alloc_variable.c solute_transport.c oxidation.c 
  component_data.c component_info.c component_info_frac.c component_vector.c
  csv.c csvutil.c csvutil_extra.c csvtmpl_format.c print_param.c
  ccse_poisson.c krylov_call_pcg.c sleev_comm.c support.c pcg.c
  pbicg.c krylov_call_block.c pcg_block3d.c sleev_comm_block3d.c
  boundary_util.c radiation.c optparse.c common_util.c
  update_level_set_flags.c tempdep_properties.c field_control.c
  init_component.c if_binary.c fv_time.c geometry_source.c real_array.c
  grid_data_feeder.c strlist.c
  general_vector_node.c general_dvector.c general_ivector.c
  print_param_core.c print_param_basic.c print_param_keywords.c
  print_param_vecmat.c print_param_comp.c print_param_variant.c
  print_param_infomap.c print_param_geom.c print_param_props.c
  print_param_comps.c 
  rect3d_relp.c rect3d_boundary.c
  validate_vof_init.c)

set(JUPITER_HEADERS boundary_cuda.h boundary_util.h cg.h common.h common_util.h
  component_info.h component_info_defs.h component_data.h component_data_defs.h
  component_info_frac.h component_info_frac_defs.h component_vector.h
  component_vector_defs.h constants.h csv.h csvtmpl_format.h csvutil.h
  csvutil_extra.h dccalc.h field_control.h func.h fv_time.h geometry_source.h
  grid_data_feeder.h grid_data_feeder_data.h if_binary.h init_component.h
  krylov_call.h level_set_cuda.h level_set_cuda_wrapper.h lpt.h materials.h
  optparse.h oxidation.h oxidation_defs.h heat_source.h trip_control.h
  pbicg.h pcg.h pcg_block3d.h pcg_block3d_res_lowtype.h pcg_cuda_BF16.h
  physical_properties.h reduciton_cuda.h sleev_comm.h
  sleev_comm_block3d.h sleev_comm_block3d_lowtype_cuda.h sleev_comm_cuda.h
  struct.h support.h tempdep_calc.h tempdep_properties.h tmcalc.h
  update_level_set_flags.h general_vector_node.h general_dvector.h
  general_ivector.h dccalc_defs.h inlet_component_defs.h tmcalc_defs.h)

if(JUPITER_ENABLE_CUDA)
  set(JUPITER_CUDA_SOURCES
    sleev_comm_cuda.cu
    pcg_cuda.cu
    krylov_call_block_cuda.cu
    pcg_block3d_cuda.cu
    sleev_comm_block3d_cuda.cu
    pcg_lowtype_cuda.cu
    pcg_block3d_lowtype_cuda.cu
    sleev_comm_block3d_lowtype_cuda.cu
    pcg_block3d_precon_lowtype_cuda.cu
    pcg_block3d_matvec_cuda.cu
    pcg_block3d_res_lowtype.cu
    level_set_cuda.cu
    boundary_cuda.cu
    communication_cuda.cu)

  set(JUPITER_CUDA_HEADERS LevelSetCuda.h communication_cuda.h
    pcg_block3d_cuda.h pcg_block3d_lowtype_cuda.h pcg_block3d_matvec_cuda.h
    pcg_block3d_precon_lowtype_cuda.h sleev_comm_block3d_cuda.h pcg_cuda.h)

  list(APPEND JUPITER_SOURCES ${JUPITER_CUDA_SOURCES})
  list(APPEND JUPITER_HEADERS ${JUPITER_CUDA_HEADERS})
endif()

foreach(RE2C_S IN LISTS JUPITER_RE2C_SOURCES)
  jupiter_re2c_target("${RE2C_S}")

  # Replace re2c-generated sources to the one in BINARY_DIR
  list(FIND JUPITER_SOURCES "${JUPITER_RE2C_GENERATED_C_NAME}" CS)
  set(RE2C_CBF "${JUPITER_RE2C_GENERATED_IN_BINARY_DIR}")
  if (${CS} GREATER -1)
    list(REMOVE_AT JUPITER_SOURCES ${CS})
    list(LENGTH JUPITER_SOURCES L)
    if (${CS} GREATER ${L} OR ${CS} EQUAL ${L})
      list(APPEND JUPITER_SOURCES "${RE2C_CBF}")
    else()
      list(INSERT JUPITER_SOURCES ${CS} "${RE2C_CBF}")
    endif()
  endif()
endforeach()

set(JUPITER_DEFS)
set(JUPTIER_INTERNAL_DEFS)
set(JUPTIER_INTERFACE_DEFS)
set(JUPITER_LIBS m jupiter-table jupiter-random jupiter-os)
set(JUPITER_INCS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set(JUPITER_FLAGS)

option(JUPITER_MASS_SOURCE_WITHIN_EQUATION
  "Include mass source to gas species in Yg equation" ON)
if (JUPITER_MASS_SOURCE_WITHIN_EQUATION)
  list(APPEND JUPITER_DEFS JUPITER_MASS_SOURCE_WITHIN_EQUATION)
endif()

option(JUPITER_MASS_SOURCE_USE_BREAK_CONSTRAINT
  "Ignore sum(Y)=1 while mass source calculation" OFF)
if (JUPITER_MASS_SOURCE_USE_BREAK_CONSTRAINT)
  list(APPEND JUPITER_DEFS JUPITER_MASS_SOURCE_USE_BREAK_CONSTRAINT)
endif()

option(JUPITER_USE_UNIFORM_GRID_EQUATION
  "Use the equation for uniform grid if the input is so" OFF)
if (JUPITER_USE_UNIFORM_GRID_EQUATION)
  list(APPEND JUPITER_DEFS JUPITER_USE_UNIFORM_GRID_EQ)
endif()

if(VASPRINTF_FOUND)
  list(APPEND JUPITER_DEFS HAVE_VASPRINTF)
  foreach(V IN LISTS VASPRINTF_DEFINITIONS)
    string(REGEX REPLACE "^-D" "" VV "${V}")
    list(APPEND JUPITER_DEFS "${VV}")
  endforeach()
endif()

set(__LPT_MODS OFF)
set(__HAVE_LPT OFF)
set(__LPT_DEF OFF)
if(TARGET jupiter-lpt)
  list(APPEND __LPT_MODS LPT)
  set(__HAVE_LPT ON)
  set(__LPT_DEF LPT)
endif()
if(TARGET jupiter-lptx)
  list(APPEND __LPT_MODS LPTX)
  set(__HAVE_LPT ON)
  set(__LPT_DEF LPTX)
endif()
if(__HAVE_LPT)
  if(DEFINED __JUPITER_ENABLE_LPT)
    set(JUPITER_ENABLE_LPT "${__JUPITER_ENABLE_LPT}")
  endif()
  if(NOT DEFINED JUPITER_ENABLE_LPT)
    set(JUPITER_ENABLE_LPT "${__LPT_DEF}")
  endif()
  set(JUPITER_ENABLE_LPT "${JUPITER_ENABLE_LPT}" CACHE STRING "LPT codebase which should be used in JUPITER")
  set_property(CACHE JUPITER_ENABLE_LPT PROPERTY STRINGS "${__LPT_MODS}")
  unset(__JUPITER_ENABLE_LPT CACHE)
else()
  set(__JUPITER_ENABLE_LPT "${JUPITER_ENABLE_LPT}" CACHE INTERNAL "" FORCE)
  unset(JUPITER_ENABLE_LPT CACHE)
  set(JUPITER_ENABLE_LPT "OFF")
endif()
if(NOT JUPITER_ENABLE_LPT IN_LIST __LPT_MODS)
  set(__MSG "")
  foreach(__M IN LISTS __LPT_MODS)
    set(__MSG "${__MSG}
 * ${__M}")
  endforeach()
  message(SEND_ERROR "JUPITER_ENABLE_LPT must be one of:${__MSG}")
endif()

cmake_dependent_option(JUPITER_LPTX_REPORT_BY_RANK
  "Output by-rank report of particle tracking"
  OFF "JUPITER_USE_MPI;JUPITER_ENABLE_LPT STREQUAL LPTX" OFF)
if(JUPITER_LPTX_REPORT_BY_RANK)
  list(APPEND JUPITER_DEFS JUPITER_LPTX_REPORT_BY_RANK)
endif()

# Poisson solver is always CCSE.
list(APPEND JUPITER_DEFS CCSE)
if (JUPITER_USE_MPI)
  list(APPEND JUPITER_DEFS JUPITER_MPI)
  list(APPEND JUPITER_LIBS MPI::MPI_C)
endif()
if (JUPITER_ENABLE_PROFILE)
  list(APPEND JUPITER_DEFS PROFILE)
endif()
if (JUPITER_ENABLE_RES_HISTORY)
  list(APPEND JUPITER_DEFS RES_history)
endif()
if (NOT JUPITER_ENABLE_METI)
  list(APPEND JUPITER_DEFS JUPITER_NOMETI)
endif()

set(__JUPITER_SOLVER_CUDA_NXBLOCK_DEFAULT 4)
set(__JUPITER_SOLVER_CUDA_NYBLOCK_DEFAULT 4)
set(__JUPITER_SOLVER_CUDA_NZBLOCK_DEFAULT 4)
if (JUPITER_ENABLE_CUDA)
  list(APPEND JUPITER_DEFS JUPITER_CUDA)
  if(JUPITER_USE_MPI)
    list(APPEND JUPITER_LIBS MPI::MPI_CXX)
  endif()
  if(JUPITER_ENABLE_CUDA_AWARE_MPI)
    list(APPEND JUPITER_DEFS JUPITER_CUDA_AWARE_MPI)
  endif()
  list(APPEND JUPITER_LIBS CUDA::cudart)
  foreach(DIR X Y Z)
    set(_VAR JUPITER_SOLVER_CUDA_N${DIR}BLOCK)
    set(_VAR_SAVE ${_VAR}_SAVE)
    if (DEFINED ${_VAR_SAVE})
      set(_DEF "${${_VAR_SAVE}}")
      unset(${_VAR_SAVE} CACHE)
    else()
      set(_DEF "${__JUPITER_SOLVER_CUDA_N${DIR}BLOCK_DEFAULT}")
    endif()
    set(${_VAR} "${_DEF}" CACHE STRING "Block size for CUDA solver in ${DIR} direction")
    set(_VAL "${${_VAR}}")
    if ("${_VAL}" MATCHES "^[1-9][0-9]*$")
      list(APPEND JUPITER_DEFS
        "JUPITER_SOLVER_CUDA_N${DIR}BLOCK=${_VAL}")
    elseif(NOT "${_VAL}" STREQUAL "")
      message(SEND_ERROR "${_VAR} must be a positve decimal value, got \"${_VAL}\"")
    endif()
  endforeach()
else()
  foreach(DIR X Y Z)
    set(_VAR JUPITER_SOLVER_CUDA_N${DIR}BLOCK)
    set(_VAR_SAVE ${_VAR}_SAVE)
    if (DEFINED ${_VAR})
      set(${_VAR_SAVE} "${${_VAR}}" CACHE INTERNAL "Block size for CUDA solver in ${DIR} direction")
      unset(${_VAR} CACHE)
    endif()
  endforeach()
endif()

set(SAN)
if (JUPITER_ENABLE_UBSAN)
  list(APPEND SAN undefined)
  #  list(APPEND SAN bounds
  #    float-divide-by-zero float-cast-overflow signed-integer-overflow
  #    vla-bound unreachable enum)
endif()
if(JUPITER_ENABLE_ASAN)
  list(APPEND SAN address)
endif()
if(JUPITER_ENABLE_TSAN)
  list(APPEND SAN thread)
endif()
list(APPEND JUPITER_LIBS jupiter-geometry)
list(APPEND JUPITER_LIBS jupiter-serializer)
list(APPEND JUPITER_LIBS jupiter-control)
list(APPEND JUPITER_LIBS re2c_lparser)
if(JUPITER_ENABLE_LPT STREQUAL "LPT")
  list(APPEND JUPITER_DEFS LPT HAVE_LPT)
  list(APPEND JUPITER_LIBS jupiter-lpt)
elseif(JUPITER_ENABLE_LPT STREQUAL "LPTX")
  list(APPEND JUPITER_DEFS LPTX HAVE_LPT)
  list(APPEND JUPITER_LIBS jupiter-lptx)
endif()
if(ENABLE_OPENMP)
  list(APPEND JUPITER_LIBS OpenMP::OpenMP_C)
endif()
if(ENABLE_FPE)
  list(APPEND JUPITER_DEFS JUPITER_FPE)
endif()

# Please modify after solver can be selectable by each variables.
list(APPEND JUPITER_DEFS JUPITER_SOLVER_${JUPITER_SOLVER})

if("${JUPITER_FP_PRECISION}" STREQUAL "DOUBLE")
  list(APPEND JUPITER_DEFS JUPITER_DOUBLE)
endif()

# TODO: Not all function is available for shared-library use
# (note: you can call any functions in Linux, but not in Windows)
add_library(libjupiter ${JUPITER_SOURCES})
add_library(JUPITER::libjupiter ALIAS libjupiter)

get_target_property(JUPITER_TYPE libjupiter TYPE)

if(JUPITER_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_INTERNAL_DEFS "JUPITER_EXPORT")
  list(APPEND JUPITER_INTERFACE_DEFS "JUPITER_IMPORT")
endif()

target_compile_options(libjupiter PUBLIC ${JUPITER_FLAGS})
target_compile_definitions(libjupiter
  PUBLIC ${JUPITER_DEFS}
  PRIVATE ${JUPITER_INTERNAL_DEFS}
  INTERFACE ${JUPITER_INTERFACE_DEFS})
target_link_libraries(libjupiter ${JUPITER_LIBS})
target_include_directories(libjupiter
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  INTERFACE $<BUILD_INTERFACE:${JUPITER_BINARY_DIR}>
  $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

string(TOLOWER "jupiter-${JUPITER_FP_PRECISION}" N)
set_target_properties(libjupiter PROPERTIES
  CUDA_SEPARABLE_COMPILATION ON
  CUDA_RESOLVE_DEVICE_SYMBOLS ON
  OUTPUT_NAME "${N}"
  PUBLIC_HEADER "${JUPITER_HEADERS}")

install(TARGETS libjupiter EXPORT JUPITER
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter)

add_executable(jupiter main.c)
target_link_libraries(jupiter libjupiter)
set_target_properties(jupiter PROPERTIES OUTPUT_NAME "${N}")
install(TARGETS jupiter EXPORT JUPITER RUNTIME DESTINATION bin)

install(EXPORT JUPITER
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
