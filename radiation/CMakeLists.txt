
if(NOT BUILD_RADIATION)
  return()
endif()

if(BUILD_JUPITER)
  # JUPITER and RADIATION will not be linked each other,
  # but used in test (because MPI_DOUBLE_PRECISION may not be
  # equivalent to double in C.)
  include(FortranCInterface)
endif()

set(RADIATION_FLAGS)
set(RADIATION_INCS)

if(NOT "${CMAKE_GENERATOR}" MATCHES "Ninja" AND "${CMAKE_VERSION}" VERSION_LESS "3.18")
  find_package(FortranPreprocessFlags REQUIRED)
  list(APPEND RADIATION_FLAGS "${Fortran_PREPROCESS_FLAGS}")
endif()

find_package(VAXStructure REQUIRED)

set(RADIATION_SOURCES calc_normal_vector.f90 cell_identify.f90
  mod_def.f90 mpi_communication_array_utility.f90
  mpi_communication_with_other_procs.f90
  mpi_initialize_decomposition.f90 mpi_isend_irecv_dble.f90
  mpi_isend_irecv_int.f90 mpi_pack_dble.f90 mpi_pack_dble_normal.f90
  mpi_pack_int.f90 mpi_unpack_dble.f90 mpi_unpack_dble_normal.f90
  mpi_unpack_int.f90 newton_loop.f90 output_exec_log_rad.f90
  output_result_data.f90 radiation.f90 ray_trace.f90 readline.f90
  set_boundary_cell_state.f90 set_boundary_normal_vector.f90
  set_boundary_radiation.f90 set_boundary_tmp_cell.f90
  set_radiation_configuration.f90 set_radiation_initial_condition.f90
  upwind_schemes.f90)


list(APPEND RADIATION_INCS "${MPI_Fortran_INCLUDE_PATH}")
list(APPEND RADIATION_INCS "${CMAKE_CURRENT_SOURCE_DIR}")
if(OpenMP_Fortran_FLAGS)
  list(APPEND RADIATION_FLAGS "${OpenMP_Fortran_FLAGS}")
endif()

string(REPLACE ";" " " RADIATION_FLAGS "${RADIATION_FLAGS}")

add_executable(radiation ${RADIATION_SOURCES})
target_link_libraries(radiation ${MPI_Fortran_LIBRARIES})
set_target_properties(radiation PROPERTIES
  INCLUDE_DIRECTORIES "${RADIATION_INCS}"
  COMPILE_FLAGS "${RADIATION_FLAGS}"
  LINK_FLAGS "${RADIATION_FLAGS}")

if("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.18")
  set_target_properties(radiation PROPERTIES
    Fortran_PREPROCESS ON)
endif()

install(TARGETS radiation RUNTIME DESTINATION bin)
