
add_executable(control_test
  control_test.c
  control_test_field_value_feeder.c
  control_test_grid_data_feeder.c
  control_test_expect_raise.c
  control_test_empty_exec.c
  control_test_util.c

  control_test_object.c
  control_test_error.c
  control_test_overflow.c
  control_test_extent.c
  control_test_logical_operator.c
  control_test_comparator.c
  control_test_cell.c
  control_test_information.c
  control_test_input.c
  control_test_output.c
  control_test_grid_data.c
  control_test_cell_data.c
  control_test_connection.c
  control_test_data_array.c
  control_test_executive.c
  control_test_manager.c
  control_test_subarray.c
  control_test_static_array.c
  control_test_string_array.c
  control_test_csvparser.c
  control_test_mpi_controller.c

  control_test_postp_mask.c
  control_test_postp_volume_integral.c
  control_test_postp_sum.c
  control_test_postp_pass_arrays.c
  control_test_mask_extent.c
  control_test_mask_point.c
  control_test_mask_lop.c
  control_test_fv_table.c
  control_test_fv_get.c

  control_test_write_fv_csv.c)

target_link_libraries(control_test m jupiter-control test-util)
if (MPI_C_FOUND AND JUPITER_USE_MPI)
  target_link_libraries(control_test MPI::MPI_C)
  target_compile_definitions(control_test PUBLIC JUPITER_MPI)
endif()
target_compile_definitions(control_test PRIVATE
  "CSVPARSER_TEST_FILE=\"${CMAKE_CURRENT_BINARY_DIR}/__test.txt\"")
target_compile_definitions(control_test PRIVATE
  "CSVWRITER_TEST_FILE=\"${CMAKE_CURRENT_BINARY_DIR}/__test.csv\"")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/control_test.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/control_test.cmake.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/control_test.cmake"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/control_test.cmake.in")

foreach(CASE util:1 grid_data_feeder:1

    object:1 error:1 overflow:1 extent:1 logical_operator:1 comparator:1 cell:1
    information:1 input:1 output:1 grid_data:1 cell_data:1 connection:1
    data_array:1 executive:1 subarray:1 static_array:1 string_array:1
    csvparser:1 mpi_controller:1 mpi_controller:2 mpi_controller:3
    mpi_controller:4

    postp_mask:1 postp_volume_integral:1 postp_volume_integral:2
    postp_volume_integral:4 postp_sum:1 postp_sum:2 postp_sum:4
    postp_pass_arrays:1

    mask_extent:1 mask_point:1 mask_lop:1

    fv_table:1 fv_get:1 fv_get:2 fv_get:4)
  string(REPLACE ":" ";" _CSEA "${CASE}")
  list(GET _CSEA 0 _CSE)
  list(GET _CSEA 1 _CSNP)
  add_test(NAME control-test-${CASE} COMMAND
    "${CMAKE_COMMAND}" "-DCASE=${_CSE}" "-DMPI_NP=${_CSNP}" -P
    "${CMAKE_CURRENT_BINARY_DIR}/control_test.cmake")

  if(NOT JUPITER_USE_MPI AND ${_CSNP} GREATER 1)
    set_tests_properties(control-test-${CASE} PROPERTIES DISABLED ON)
  endif()

  if (DEFINED JUPITER_TEST_MAX_NUMPROCS AND
      ${_CSNP} GREATER ${JUPITER_TEST_MAX_NUMPROCS})
    set_tests_properties(control-test-${CASE} PROPERTIES LABELS cores-shortage)
  endif()
endforeach()
