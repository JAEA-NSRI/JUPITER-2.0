if(NOT TARGET jupiter-lptx)
  return()
endif()

add_executable(lptx-test
  lptx-test.c
  lptx-test-param.c
  lptx-test-particle.c
  lptx-test-overflow.c
  lptx-test-util.c
  lptx-test-comm.c)

target_link_libraries(lptx-test jupiter-lptx test-util)
if (MPI_C_FOUND AND JUPITER_USE_MPI)
  target_link_libraries(lptx-test MPI::MPI_C)
  target_compile_definitions(lptx-test PUBLIC JUPITER_MPI)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/lptx-test.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lptx-test.cmake.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lptx-test.cmake"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/lptx-test.cmake.in")

foreach(CASE param:1 particle:1 overflow:1 util:1 comm:1 comm:2 comm:4 comm:8
    collision:1)
  string(REPLACE ":" ";" _CSEA "${CASE}")
  list(GET _CSEA 0 _CSE)
  list(GET _CSEA 1 _CSNP)
  add_test(NAME lptx-test-${CASE} COMMAND
    "${CMAKE_COMMAND}" "-DCASE=${_CSE}" "-DMPI_NP=${_CSNP}" -P
    "${CMAKE_CURRENT_BINARY_DIR}/lptx-test.cmake")

  if(NOT JUPITER_USE_MPI AND ${_CSNP} GREATER 1)
    set_tests_properties(lptx-test-${CASE} PROPERTIES DISABLED 1)
  endif()

  if (DEFINED JUPITER_TEST_MAX_NUMPROCS AND
      ${_CSNP} GREATER ${JUPITER_TEST_MAX_NUMPROCS})
    set_tests_properties(lptx-test-${CASE} PROPERTIES LABELS cores-shortage)
  endif()
endforeach()
