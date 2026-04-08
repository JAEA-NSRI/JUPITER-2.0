
if(TARGET jupiter-lpt)
  # TODO: This may not work on Windows when libraries are shared linked,
  #       because major Fortran symbols are not exported.
  get_target_property(__T jupiter-lpt TYPE)
  if("${__T}" STREQUAL "SHARED_LIBRARY")
    set(JUPITER_TEST_LPT_MODULE SHARED)
    set(JUPITER_TEST_LPT_DEFS TEST_LPT_SHARED TEST_LPT_EXPORT)
    set(JUPITER_TEST_LPT_IDEFS TEST_LPT_SHARED)
  else()
    set(JUPITER_TEST_LPT_MODULE STATIC)
  endif()
  add_library(lpt-test-lib ${JUPITER_TEST_LPT_MODULE}
    lpt-test-val.f90 lpt-test.f90
    lpt-test-mxpset.f90
    lpt-test-set-get-npset.f90
    lpt-test-set-get-pset.f90
    lpt-test-set-get-ipttim.f90
    lpt-test-open-log-file.f90
    lpt-test-allocate.f90
    lpt-test-set-get-pts.f90
    lpt-test-lib.c)
  target_link_libraries(lpt-test-lib jupiter-lpt)
  set_target_properties(lpt-test-lib PROPERTIES
    COMPILE_DEFINITIONS "${JUPITER_TEST_LPT_DEFS}"
    INTERFACE_COMPILE_DEFINITIONS "${JUPITER_TEST_LPT_IDEFS}")

  add_executable(lpt-test lpt-test.c)
  target_link_libraries(lpt-test lpt-test-lib)
  if(JUPITER_LPT_USE_MPI AND NOT MPIEXEC STREQUAL "")
    set(__MPI "${MPIEXEC}" ${MPIEXEC_NUMPROC_FLAG} 1)
  else()
    set(__MPI)
  endif()
  add_test(NAME lpt-test COMMAND ${__MPI} $<TARGET_FILE:lpt-test>)
endif()
