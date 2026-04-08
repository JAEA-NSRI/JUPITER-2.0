
if(UNIX OR APPLE)
  set(JUPITER_OS posix)
endif()
if(WIN32)
  set(JUPITER_OS win32)
endif()

jupiter_re2c_target(posix/os.c.re)
jupiter_re2c_target(win32/os.c.re)

add_library(jupiter-os ${JUPITER_OS}/os.c asprintf.c)
add_library(JUPITER::jupiter-os ALIAS jupiter-os)

set(JUPITER_OS_HEADERS defs.h os.h asprintf.h)

set(JUPITER_OS_LIBS)
set(JUPITER_OS_INTERNAL_DEFS)
set(JUPITER_OS_INTERFACE_DEFS)

get_target_property(JUPITER_OS_TYPE jupiter-os TYPE)

if(JUPITER_OS_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_OS_INTERNAL_DEFS "JUPITER_OS_EXPORT")
  list(APPEND JUPITER_OS_INTERFACE_DEFS "JUPITER_OS_IMPORT")
endif()

list(APPEND JUPITER_OS_LIBS jupiter-geometry-headers)

target_link_libraries(jupiter-os ${JUPITER_OS_LIBS})

target_compile_definitions(jupiter-os
  PRIVATE ${JUPITER_OS_INTERNAL_DEFS}
  INTERFACE ${JUPITER_OS_INTERFACE_DEFS})

target_include_directories(jupiter-os
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(jupiter-os PROPERTIES
  PUBLIC_HEADER "${JUPITER_OS_HEADERS}")

install(TARGETS jupiter-os EXPORT OS
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/os)

install(EXPORT OS
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
