
add_library(jupiter-vtkiobind bind.c)
add_library(JUPITER::jupiter-vtkiobind ALIAS jupiter-vtkiobind)

target_link_libraries(jupiter-vtkiobind libjupiter jupiter-os)
target_include_directories(jupiter-vtkiobind
  INTERFACE $<BUILD_INTERFACE:${JUPITER_UTILS_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(jupiter-vtkiobind
  PRIVATE JUPITER_VTKIOBIND_EXPORT
  INTERFACE JUPITER_VTKIOBIND_IMPORT)

set_target_properties(jupiter-vtkiobind PROPERTIES
  PUBLIC_HEADER bind.h)

install(TARGETS jupiter-vtkiobind EXPORT vtkiobind
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/vtkiobind)

install(EXPORT vtkiobind
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
