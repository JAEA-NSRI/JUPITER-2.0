
include(CheckTypeSize)

if(ENABLE_OPENMP)
  set(JUPITER_TABLE_USE_OPENMP ON)
else()
  set(JUPITER_TABLE_USE_OPENMP OFF)
endif()

option(TABLE_PRINT_TREE "Print tree for tree-based search algorithm while initializing table" OFF)

check_type_size(uint8_t UINT8_T)
check_type_size(uint16_t UINT16_T)
check_type_size(uint32_t UINT32_T)
check_type_size(uint64_t UINT64_T)
check_type_size(int64_t INT64_T)

set(JUPITER_TABLE_DEFS)
set(JUPITER_TABLE_INTERNAL_DEFS)
set(JUPITER_TABLE_INTERFACE_DEFS)
set(JUPITER_TABLE_FLAGS)
set(JUPITER_TABLE_INCS)
set(JUPITER_TABLE_LIBS)
if(NOT ( UINT8_T EQUAL 1 AND UINT16_T EQUAL 2 AND
      UINT32_T EQUAL 4 AND UINT64_T EQUAL 8 ) )
  message(STATUS "one (or more) of uint[8,16,32,64]_t not available. IO functions are omitted")
  list(APPEND JUPITER_TABLE_DEFS TABLE_NO_IO_FUNCTIONS)
endif()

if(TABLE_PRINT_TREE)
  list(APPEND JUPITER_TABLE_DEFS TABLE_PRINT_TREE)
endif()

if(JUPITER_TABLE_USE_OPENMP)
  list(APPEND JUPITER_TABLE_DEFS TABLE_USE_OPENMP)
  list(APPEND JUPITER_TABLE_LIBS OpenMP::OpenMP_C)
endif()

add_library(jupiter-table table.c table-file.c
  linear.c linear-save.c bin-tree.c bin-tree-minmax.c)
add_library(JUPITER::jupiter-table ALIAS jupiter-table)

get_target_property(JUPITER_TABLE_TYPE jupiter-table TYPE)
if(JUPITER_TABLE_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_TABLE_INTERNAL_DEFS "JUPITER_TABLE_EXPORT")
  list(APPEND JUPITER_TABLE_INTERFACE_DEFS "JUPITER_TABLE_IMPORT")
endif()

target_link_libraries(jupiter-table ${JUPITER_TABLE_LIBS})
target_compile_options(jupiter-table PUBLIC ${JUPITER_TABLE_FLAGS})
target_compile_definitions(jupiter-table
  PUBLIC ${JUPITER_TABLE_DEFS}
  PRIVATE ${JUPITER_TABLE_INTERNAL_DEFS}
  INTERFACE ${JUPITER_TABLE_INTERFACE_DEFS})
target_include_directories(jupiter-table
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
         PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(JUPITER_TABLE_PUB_HEADERS table.h)

set_target_properties(jupiter-table PROPERTIES
  PUBLIC_HEADER "${JUPITER_TABLE_PUB_HEADERS}")

install(TARGETS jupiter-table EXPORT Table
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/table)

jupiter_re2c_target(csv2table.c.re)
add_library(libcsv2table csv2table.c)
set_target_properties(libcsv2table PROPERTIES
  OUTPUT_NAME jupiter-csv2table)
target_link_libraries(libcsv2table m jupiter-table)

set(JUPTER_CSV2TAB_DEFS)
set(JUPTER_CSV2TAB_INTERNAL_DEFS)
set(JUPTER_CSV2TAB_INTERFACE_DEFS)

get_target_property(JUPITER_CSV2TAB_TYPE libcsv2table TYPE)
if(JUPITER_CSV2TAB_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_CSV2TAB_INTERNAL_DEFS "JUPITER_CSV2TAB_EXPORT")
  list(APPEND JUPITER_CSV2TAB_INTERFACE_DEFS "JUPITER_CSV2TAB_IMPORT")
endif()

target_compile_definitions(libcsv2table
  PUBLIC ${JUPITER_CSV2TAB_DEFS}
  PRIVATE ${JUPITER_CSV2TAB_INTERNAL_DEFS}
  INTERFACE ${JUPITER_CSV2TAB_INTERFACE_DEFS})

target_include_directories(libcsv2table
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(JUPITER_CSV2TABLE_PUB_HEADERS csv2table.h)

set_target_properties(libcsv2table PROPERTIES
  PUBLIC_HEADER "${JUPITER_CSV2TABLE_PUB_HEADERS}")

install(TARGETS libcsv2table EXPORT Table
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/table)

jupiter_re2c_target(csv2table-main.c.re)

add_executable(csv2table csv2table-main.c)
target_link_libraries(csv2table jupiter-table libcsv2table)

target_include_directories(csv2table PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS csv2table RUNTIME DESTINATION bin)

install(EXPORT Table
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
