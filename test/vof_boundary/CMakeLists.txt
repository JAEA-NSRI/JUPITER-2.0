add_executable(vof_boundary_test vof_boundary_test.c)
target_link_libraries(vof_boundary_test libjupiter)
add_test(NAME vof-boundary COMMAND $<TARGET_FILE:vof_boundary_test>)

if(JUPITER_USE_MPI AND MPIEXEC_EXECUTABLE)
  add_test(NAME vof-boundary-mpi COMMAND
    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:vof_boundary_test> ${MPIEXEC_POSTFLAGS})
  set_tests_properties(vof-boundary-mpi PROPERTIES PROCESSORS 2)
endif()
