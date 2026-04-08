jupiter_re2c_target(csvtest.c.re)

if (BUILD_JUPITER)
  add_executable(csvtest csvtest.c)
  target_link_libraries(csvtest libjupiter re2c_lparser)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/csvtestTmp")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/csvtest.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/csvtest.cmake.in" @ONLY)
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/csvtest.cmake"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/csvtest.cmake.in")
  add_test(NAME csvtest
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/csvtest.cmake")
endif()
