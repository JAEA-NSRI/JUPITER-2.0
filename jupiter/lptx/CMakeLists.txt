cmake_dependent_option(JUPITER_LPTX_EMULATE_LPT_BEHAVIOR
  "Emulate (old) LPT behavior" OFF "BUILD_JUPITER_LPTX" OFF)

if (NOT BUILD_JUPITER_LPTX)
  return()
endif()

if (NOT DEFINED VASPRINTF_FOUND)
  find_package(Vasprintf)
endif()

add_library(jupiter-lptx lptcalc.c param.c init_set.c particle.c util.c stat.c
  ptflags.c pvector.c priv_util.c comm.c error.c collision_list.c)
add_library(JUPITER::jupiter-lptx ALIAS jupiter-lptx)

get_target_property(JUPITER_LPTX_TYPE jupiter-lptx TYPE)

set(JUPITER_LPTX_DEFS)
set(JUPITER_LPTX_INTERNAL_DEFS)
set(JUPITER_LPTX_INTERFACE_DEFS)
set(JUPITER_LPTX_LIBS)

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
  list(APPEND JUPITER_LPTX_DEFS JUPITER_LPTX_HAVE_OVERFLOW)
endif()

if(JUPITER_LPTX_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_LPTX_INTERNAL_DEFS "JUPITER_LPTX_EXPORT")
  list(APPEND JUPITER_LPTX_INTERFACE_DEFS "JUPITER_LPTX_IMPORT")
endif()
if(VASPRINTF_FOUND)
  list(APPEND JUPITER_LPTX_INTERNAL_DEFS "HAVE_VASPRINTF")
  foreach(DEF IN LISTS VASPRINTF_DEFINITIONS)
    string(REGEX REPLACE "(^|[ \t])-D" "\\1" _DD "${DEF}")
    list(APPEND JUPITER_LPTX_INTERNAL_DEFS "${_DD}")
  endforeach()
endif()
list(APPEND JUPITER_LPTX_LIBS jupiter-geometry)
if(JUPITER_USE_MPI)
  list(APPEND JUPITER_LPTX_DEFS "JUPITER_LPTX_MPI")
  list(APPEND JUPITER_LPTX_LIBS MPI::MPI_C)
endif()
if(JUPITER_FP_PRECISION)
  list(APPEND JUPITER_LPTX_DEFS "JUPITER_LPTX_${JUPITER_FP_PRECISION}")
endif()
if(JUPITER_LPTX_EMULATE_LPT_BEHAVIOR)
  list(APPEND JUPITER_LPTX_DEFS JUPITER_LPTX_EMULATE_LPT_BEHAVIOR)
endif()
if(ENABLE_OPENMP)
  list(APPEND JUPITER_LPTX_LIBS OpenMP::OpenMP_C)
endif()
list(APPEND JUPITER_LPTX_LIBS jupiter-random)

target_link_libraries(jupiter-lptx ${JUPITER_LPTX_LIBS})

target_compile_definitions(jupiter-lptx
  PUBLIC ${JUPITER_LPTX_DEFS}
  PRIVATE ${JUPITER_LPTX_INTERNAL_DEFS}
  INTERFACE ${JUPITER_LPTX_INTERFACE_DEFS})

target_include_directories(jupiter-lptx
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

set(JUPITER_LPTX_PUB_HEADERS
  defs.h param.h init_set.h vector.h struct_defs.h type_math.h ptflags.h
  error.h pvector.h constants.h interpolator.h overflow.h collision_list.h)

set_target_properties(jupiter-lptx PROPERTIES
  PUBLIC_HEADER "${JUPITER_LPTX_PUB_HEADERS}")

install(TARGETS jupiter-lptx EXPORT LPTX
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/jupiter/lptx")

install(EXPORT LPTX
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
