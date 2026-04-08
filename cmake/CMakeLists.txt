##
#  @addtogroup CMakeScripts
#  @{
#  @file JUPITER-src/cmake/CMakeLists.txt
#  @brief Build script for cmake directory
#

if (NOT BUILD_DOCS)
  return()
endif()

if(NOT CMAKE_CROSSCOMPILING)
  jupiter_re2c_target(cmake-doxygen-filter.c.re)
  jupiter_re2c_target(doxygen-filter.c.re)

  set(FILTER_INCS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

  add_executable(cmake-doxygen-filter cmake-doxygen-filter.c doxygen-filter.c)
  set_target_properties(cmake-doxygen-filter PROPERTIES
    INCLUDE_DIRECTORIES "${FILTER_INCS}")
  target_link_libraries(cmake-doxygen-filter re2c_lparser)

else()
  add_executable(cmake-doxygen-filter IMPORTED)
endif()

## @}
