if(TARGET libjupiter)
  add_executable(csvutil_test csvutil_test.c)
  target_link_libraries(csvutil_test libjupiter)
  add_test(NAME csvutil_test COMMAND $<TARGET_FILE:csvutil_test>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  add_executable(print_level_test print_level_test.c)
  target_link_libraries(print_level_test libjupiter)
  add_test(NAME print-level-test COMMAND $<TARGET_FILE:print_level_test>)

  jupiter_re2c_target(print_param_test.c.re print_param_test.c)
  add_executable(print_param_test print_param_test.c)
  target_link_libraries(print_param_test libjupiter test-util re2c_lparser)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/print-param-test-1)
  set(__ARGS $<TARGET_FILE:print_param_test>)
  if (JUPITER_USE_MPI)
    list(PREPEND __ARGS
      ${MPIEXEC_EXECUTABLE} ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 1
      ${MPIEXEC_POSTFLAGS})
  endif()
  add_test(NAME print-param-test-1 COMMAND ${__ARGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/print-param-test-1)

  if (JUPITER_USE_MPI)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/print-param-test-2)
    set(__ARGS $<TARGET_FILE:print_param_test>)
    list(PREPEND __ARGS
      ${MPIEXEC_EXECUTABLE} ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 2
      ${MPIEXEC_POSTFLAGS} )
    add_test(NAME print-param-test-2 COMMAND ${__ARGS}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/print-param-test-2)
  endif()
endif()
