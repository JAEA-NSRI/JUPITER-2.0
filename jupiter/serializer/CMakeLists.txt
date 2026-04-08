
include(CheckTypeSize)
include(TestBigEndian)

if(ENABLE_OPENMP)
  set(JUPITER_SERIALIZER_USE_OPENMP ON)
else()
  set(JUPITER_SERIALIZER_USE_OPENMP OFF)
endif()

TEST_BIG_ENDIAN(JUPITER_SERIALIZER_SYS_BIG)
if(JUPITER_SERIALIZER_SYS_BIG EQUAL 0)
  set(JUPITER_SERIALIZER_SYS_BIG ON)
else()
  set(JUPITER_SERIALIZER_SYS_BIG OFF)
endif()

cmake_dependent_option(JUPITER_SERIALIZER_USE_BIG_ENDIAN "Serialize data in Big endian (this option only available platforms on Little endian)" OFF
  "JUPITER_SERIALIZER_SYS_BIG" 0)

check_type_size(uint8_t UINT8_T)
check_type_size(uint16_t UINT16_T)
check_type_size(uint32_t UINT32_T)
check_type_size(uint64_t UINT64_T)
check_type_size(int64_t INT64_T)

set(JUPITER_SERIALIZER_DEFS)
set(JUPITER_SERIALIZER_FLAGS)
set(JUPITER_SERIALIZER_INCS)

if(JUPITER_SERIALIZER_USE_OPENMP)
  list(APPEND JUPITER_SERIALIZER_DEFS SERIALIZER_USE_OPENMP)
  list(APPEND JUPITER_SERIALIZER_LIBS OpenMP::OpenMP_C)
endif()

if (JUPITER_SERIALIZER_USE_BIG_ENDIAN)
  list(APPEND JUPITER_SERIALIZER_DEFS JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN)
endif()

if(MPI_C_FOUND AND JUPITER_USE_MPI)
  list(APPEND JUPITER_SERIALIZER_DEFS SERIALIZER_USE_MPI)
  list(APPEND JUPITER_SERIALIZER_LIBS MPI::MPI_C)
endif()

set(JUPITER_SERIALIZER_PRIVATE_INCS
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(jupiter-serializer buffer.c msgpackx.c util.c error.c)
add_library(JUPITER::jupiter-serilaizer ALIAS jupiter-serializer)

target_link_libraries(jupiter-serializer ${JUPITER_SERIALIZER_LIBS})

target_compile_definitions(jupiter-serializer
  PUBLIC ${JUPITER_SERIALIZER_DEFS})

get_target_property(JUPITER_SERIALIZER_TYPE jupiter-serializer TYPE)
if("${JUPITER_SERIALIZER_TYPE}" STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(jupiter-serializer
    PRIVATE JUPITER_SERIALIZER_EXPORT
    INTERFACE JUPITER_SERIALIZER_IMPORT)
endif()


target_compile_options(jupiter-serializer
  PUBLIC ${JUPITER_SERIALIZER_FLAGS})

target_include_directories(jupiter-serializer
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(JUPITER_SERIALIZER_PUB_HEADERS
  buffer.h byteswap.h defs.h error.h msgpackx.h util.h)

set_target_properties(jupiter-serializer PROPERTIES
  PUBLIC_HEADER "${JUPITER_SERIALIZER_PUB_HEADERS}")

install(TARGETS jupiter-serializer EXPORT Serializer
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/serializer)

install(EXPORT Serializer
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
