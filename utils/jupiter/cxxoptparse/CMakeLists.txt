
set(JUPITER_CXXOPTPARSE_HEADERS optparse.h common.h traits.h exceptions.h
  help_printer.h value_printer.h help.h stop.h sequence.h match.h types.h enum.h
  doc.h parsers.h)

add_library(jupiter-cxxoptparse INTERFACE)
add_library(JUPITER::jupiter-cxxoptparse ALIAS jupiter-cxxoptparse)

target_include_directories(jupiter-cxxoptparse
  INTERFACE $<BUILD_INTERFACE:${JUPITER_UTILS_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(jupiter-cxxoptparse INTERFACE jupiter-os)

set_target_properties(jupiter-cxxoptparse PROPERTIES
  PUBLIC_HEADER "${JUPITER_CXXOPTPARSE_HEADERS}")

install(TARGETS jupiter-cxxoptparse EXPORT cxxoptparse
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/cxxoptparse)

install(EXPORT cxxoptparse
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)

# For generating compile_commands.json
add_library(jupiter-cxxoptparse-dummy STATIC optparse.cxx)
target_link_libraries(jupiter-cxxoptparse-dummy jupiter-cxxoptparse)
set_target_properties(jupiter-cxxoptparse-dummy PROPERTIES
  EXCLUDE_FROM_ALL TRUE
  EXCLUDE_FROM_DEFAULT_BUILD TRUE)
