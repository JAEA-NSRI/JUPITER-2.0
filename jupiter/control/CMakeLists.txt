
set(JUPITER_CONTROL_DEFS)
set(JUPITER_CONTROL_INTERNAL_DEFS)
set(JUPITER_CONTROL_INTERFACE_DEFS)
set(JUPITER_CONTROL_FLAGS)
set(JUPITER_CONTROL_LIBS)

include(CheckCSourceCompiles)

# see https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
check_c_source_compiles("
#include <stddef.h>

int main(int argc, char **argv)
{
  int r;
  __builtin_add_overflow(1, 2, &r);
  __builtin_sub_overflow(1, 2, &r);
  __builtin_mul_overflow(1, 2, &r);
  return 0;
}" HAVE_BUILTIN_OVERFLOW)
if (HAVE_BUILTIN_OVERFLOW)
  list(APPEND JUPITER_CONTROL_DEFS JCNTRL_HAVE_OVERFLOW)
endif()

if(ENABLE_OPENMP)
  list(APPEND JUPITER_CONTROL_DEFS JCNTRL_USE_OPENMP)
  list(APPEND JUPITER_CONTROL_LIBS OpenMP::OpenMP_C)
endif()

if(MPI_C_FOUND AND JUPITER_USE_MPI)
  list(APPEND JUPITER_CONTROL_DEFS JCNTRL_USE_MPI)
  list(APPEND JUPITER_CONTROL_LIBS MPI::MPI_C)
endif()

list(APPEND JUPITER_CONTROL_LIBS jupiter-serializer jupiter-geometry jupiter-table re2c_lparser)

jupiter_re2c_target(csvparser.c.re)

add_library(jupiter-control error.c shared_object.c abstract_array.c
  executive.c information.c input.c output.c connection.c data_array.c global.c
  manager.c struct_grid.c field_variable.c field_function.c data_object.c
  mask_object.c field_object.c grid_data.c cell.c cell_data.c mask_data.c
  mask_function.c geometry.c logical_variable.c subarray.c static_array.c
  mpi_controller.c string_array.c csvparser.c abstract_stats.c

  postp_mask.c postp_volume_integral.c postp_sum.c postp_pass_arrays.c

  mask_extent.c mask_geometry.c mask_point.c mask_lop.c

  fv_table.c fv_get.c

  write_fv_csv.c)
add_library(JUPITER::jupiter-control ALIAS jupiter-control)

get_target_property(JUPITER_CONTROL_TYPE jupiter-control TYPE)
if(JUPITER_CONTROL_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_CONTROL_INTERNAL_DEFS "JUPITER_CONTROL_EXPORT")
  list(APPEND JUPITER_CONTROL_INTERFACE_DEFS "JUPITER_CONTROL_IMPORT")
endif()

target_link_libraries(jupiter-control ${JUPITER_CONTROL_LIBS})
target_compile_options(jupiter-control PUBLIC ${JUPITER_CONTROL_FLAGS})
target_compile_definitions(jupiter-control
  PUBLIC ${JUPITER_CONTROL_DEFS}
  PRIVATE ${JUPITER_CONTROL_INTERNAL_DEFS}
  INTERFACE ${JUPITER_CONTROL_INTERFACE_DEFS})
target_include_directories(jupiter-control
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(JUPITER_CONTROL_PUB_HEADERS connection.h abstract_array.h data_array.h
  defs.h error.h executive.h executive_data.h field_variable.h field_function.h
  grid_data.h cell_data.h mask_data.h mask_function.h geometry.h global.h
  information.h input.h manager.h output.h shared_object.h field_object.h
  mask_object.h struct_data.h struct_grid.h data_object.h logical_variable.h
  overflow.h extent.h string_array.h csvparser.h logical_operator.h
  abstract_stats.h para_util.h comparator.h

  postp_mask.h postp_volume_integral.h postp_sum.h postp_pass_arrays.h

  mask_extent.h mask_geometry.h mask_point.h mask_lop.h

  fv_table.h fv_get.h

  write_fv_csv.h

  # for inherit control class
  cell.h shared_object_priv.h abstract_array_data.h data_array_data.h
  reduce_op.h abstract_stats_data.h geometry_data.h)

set_target_properties(jupiter-control PROPERTIES
  PUBLIC_HEADER "${JUPITER_CONTROL_PUB_HEADERS}")

install(TARGETS jupiter-control EXPORT Control
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/control)

install(EXPORT Control
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
