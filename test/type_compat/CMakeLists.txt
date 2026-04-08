
if(BUILD_RADIATION AND BUILD_JUPITER)
  include(FortranCInterface)
  FortranCInterface_HEADER("type_compat.h"
    MACRO_NAMESPACE TYPE_COMPAT_
    SYMBOL_NAMESPACE TYPE_COMPAT_FRT_
    SYMBOLS test)
  set_source_files_properties(type_compat.f90 PROPERTIES
    COMPILE_FLAGS "${MPI_Fortran_COMPILE_FLAGS}")
  set_source_files_properties(type_compat.c PROPERTIES
    COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}")
  add_executable(type_compat_test type_compat.f90 type_compat.c)
  set_target_properties(type_compat_test PROPERTIES
    INCLUDE_DIRECTORIES "${MPI_C_INCLUDE_PATH};${MPI_Fortran_INCLUDE_PATH};${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}"
    LINK_FLAGS "${MPI_Fortran_LINK_FLAGS}")
  target_link_libraries(type_compat_test
    ${MPI_C_LIBRARIES} ${MPI_Fortran_LIBRARIES})
  if (MPIEXEC AND MPIEXEC_NUMPROC_FLAG AND MPIEXEC_MAX_NUMPROCS GREATER 1)
    add_test(NAME "type_compat_test" COMMAND
      "${MPIEXEC}" "${MPIEXEC_NUMPROC_FLAG}" 2 $<TARGET_FILE:type_compat_test>)
  else()
    message(WARNING "Type compatibility test will be compiled for test, but cannot be run because MPIEXEC is not found or MPIEXEC_MAX_NUMPROCS is less than 2.
  MPIEXEC: ${MPIEXEC}
  MPIEXEC_MAX_NUMPROCS: ${MPIEXEC_MAX_NUMPROCS}
If MPIEXEC is available, you can force to run this test by setting MPIEXEC_MAX_NUMPROCS to 2 or greater.")
  endif()
endif()
