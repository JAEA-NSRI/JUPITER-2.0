
add_executable(grid_data_feeder_test grid_data_feeder_test.c)

target_link_libraries(grid_data_feeder_test
  jupiter-random libjupiter test-util)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plist.txt.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plist.txt" @ONLY)

function(add_grid_data_feeder_test NAME MPI_NP)
  set(GRID_DATA_FEEDER_TEST_ARGS ${ARGV})
  list(REMOVE_AT GRID_DATA_FEEDER_TEST_ARGS 0 1)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/grid_data_feeder_test.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test-${NAME}.cmake" @ONLY)
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test-${NAME}.cmake"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/test-${NAME}.cmake")

  add_test(NAME grid-data-feeder-test-${NAME} COMMAND
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/test-${NAME}.cmake")

  if (NOT JUPITER_MPI_MPI AND MPI_NP GREATER 1)
    set_tests_properties(grid-data-feeder-test-${NAME} PROPERTIES DISABLED 1)
  endif()

  if (DEFINED JUPITER_TEST_MAX_NUMPROCS AND
      ${MPI_NP} GREATER ${JUPITER_TEST_MAX_NUMPROCS})
    set_tests_properties(grid-data-feeder-test-${NAME}
      PROPERTIES LABELS cores-shortage)
  endif()
endfunction()

add_grid_data_feeder_test(t1 1
  -flags ${CMAKE_CURRENT_SOURCE_DIR}/input/flags-1.txt
  -input ${CMAKE_CURRENT_SOURCE_DIR}/input/param-1.txt
  -plist ${CMAKE_CURRENT_BINARY_DIR}/plist.txt
  -geom  ${CMAKE_CURRENT_SOURCE_DIR}/input/geom.txt)

add_grid_data_feeder_test(t2 1
  -flags ${CMAKE_CURRENT_SOURCE_DIR}/input/flags-2.txt
  -input ${CMAKE_CURRENT_SOURCE_DIR}/input/param-1.txt
  -plist ${CMAKE_CURRENT_BINARY_DIR}/plist.txt
  -geom  ${CMAKE_CURRENT_SOURCE_DIR}/input/geom.txt)

add_grid_data_feeder_test(t3 2
  -flags ${CMAKE_CURRENT_SOURCE_DIR}/input/flags-1.txt
  -input ${CMAKE_CURRENT_SOURCE_DIR}/input/param-2.txt
  -plist ${CMAKE_CURRENT_BINARY_DIR}/plist.txt
  -geom  ${CMAKE_CURRENT_SOURCE_DIR}/input/geom.txt)

add_grid_data_feeder_test(t4 2
  -flags ${CMAKE_CURRENT_SOURCE_DIR}/input/flags-2.txt
  -input ${CMAKE_CURRENT_SOURCE_DIR}/input/param-2.txt
  -plist ${CMAKE_CURRENT_BINARY_DIR}/plist.txt
  -geom  ${CMAKE_CURRENT_SOURCE_DIR}/input/geom.txt)
