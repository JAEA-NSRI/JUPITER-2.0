# Convert dummy-liqt.csv with csv2table and writes input/dummy-liqt.bin
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/test-util.cmake")

set(_INP "${CMAKE_CURRENT_LIST_DIR}/dummy-liqt.csv")
set(_OUT "${CMAKE_CURRENT_SOURCE_DIR}/input/dummy-liqt.bin")
message(STATUS "csv2table: ${_INP} -> ${_OUT}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/input")
csv2table("${_INP}" "${_OUT}")
