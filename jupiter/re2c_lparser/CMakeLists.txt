

jupiter_re2c_target(re2c_lparser.h.re)

set(JUPITER_RE2C_LPARSER_HEADERS re2c_lparser.h)

add_library(re2c_lparser INTERFACE)
add_library(JUPITER::re2c_lparser ALIAS re2c_lparser)

target_include_directories(re2c_lparser INTERFACE
  $<BUILD_INTERFACE:${JUPITER_BINARY_DIR}>
  $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(re2c_lparser PROPERTIES
  PUBLIC_HEADER "${JUPITER_RE2C_LPARSER_HEADERS}")

install(TARGETS re2c_lparser EXPORT re2c_lparser
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/re2c_lparser)

install(EXPORT re2c_lparser
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
