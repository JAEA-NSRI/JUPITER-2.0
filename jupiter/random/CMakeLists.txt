
set(JUPITER_RANDOM_SOURCES random.c)
set(JUPITER_RANDOM_F90_SOURCES random_f90.c)
set(JUPITER_RANDOM_F90M_SOURCES random.f90)
set(JUPITER_RANDOM_HEADERS random.h random_base.h random_vectors.h defs.h)

include(TestBigEndian)
test_big_endian(__BIG)

include(CMakeDependentOption)
cmake_dependent_option(JUPITER_RANDOM_ENABLE_F90
  "Support Fortran support in random module" ON
  "CMAKE_Fortran_COMPILER_LOADED" OFF)

add_library(jupiter-random ${JUPITER_RANDOM_SOURCES})
add_library(JUPITER::jupiter-random ALIAS jupiter-random)
get_target_property(JUPITER_RANDOM_TYPE jupiter-random TYPE)

if (JUPITER_RANDOM_ENABLE_F90)
  # Ninja perform preprocess in separate step
  if(NOT "${CMAKE_GENERATOR}" MATCHES "Ninja" AND "${CMAKE_VERSION}" VERSION_LESS "3.18")
    find_package(FortranPreprocessFlags REQUIRED)

    set_source_files_properties(${JUPITER_RANDOM_F90M_SOURCES} PROPERTIES
      COMPILE_OPTIONS
      "$<$<COMPILE_LANGUAGE:Fortran>:${Fortran_PREPROCESS_FLAGS}>")
  endif()

  add_library(jupiter-random-f90-lib ${JUPITER_RANDOM_F90_SOURCES})
  get_target_property(JUPITER_RANDOM_F90_TYPE jupiter-random-f90-lib TYPE)
  add_library(jupiter-random-f90 OBJECT ${JUPITER_RANDOM_F90M_SOURCES})
endif()

set(JUPITER_RANDOM_DEFS)
set(JUPITER_RANDOM_INTERNAL_DEFS)
set(JUPITER_RANDOM_INTERFACE_DEFS)
set(JUPITER_RANDOM_LIBS)
set(JUPITER_RANDOM_F90_DEFS)
set(JUPITER_RANDOM_F90_INTERNAL_DEFS)
set(JUPITER_RANDOM_F90_INTERFACE_DEFS)
set(JUPITER_RANDOM_F90_LIBS)

list(APPEND JUPITER_RANDOM_LIBS m)
if(__BIG)
  list(APPEND JUPITER_RANDOM_INTERNAL_DEFS "JUPITER_RANDOM_BIG")
endif()
if(JUPITER_RANDOM_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_RANDOM_INTERNAL_DEFS "JUPITER_RANDOM_EXPORT")
  list(APPEND JUPITER_RANDOM_INTERFACE_DEFS "JUPITER_RANDOM_IMPORT")
endif()
if(JUPITER_RANDOM_F90_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_RANDOM_F90_INTERNAL_DEFS "JUPITER_RANDOM_F90_EXPORT")
  list(APPEND JUPITER_RANDOM_F90_INTERFACE_DEFS "JUPITER_RANDOM_F90_IMPORT")
endif()
if(JUPITER_USE_MPI)
  list(APPEND JUPITER_RANDOM_DEFS "JUPITER_RANDOM_MPI")
  list(APPEND JUPITER_RANDOM_LIBS MPI::MPI_C)
  if(CMAKE_Fortran_COMPILER_LOADED AND TARGET MPI::MPI_Fortran)
    list(APPEND JUPITER_RANDOM_F90_LIBS MPI::MPI_Fortran)
  endif()
endif()
if(ENABLE_OPENMP)
  list(APPEND JUPITER_RANDOM_LIBS OpenMP::OpenMP_C)
endif()

target_compile_definitions(jupiter-random
  PUBLIC ${JUPITER_RANDOM_DEFS}
  PRIVATE ${JUPITER_RANDOM_INTERNAL_DEFS}
  INTERFACE ${JUPITER_RANDOM_INTERFACE_DEFS})

target_include_directories(jupiter-random
  PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(jupiter-random PROPERTIES
  PUBLIC_HEADER "${JUPITER_RANDOM_HEADERS}")

target_link_libraries(jupiter-random ${JUPITER_RANDOM_LIBS})

install(TARGETS jupiter-random EXPORT Random
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/random)

if(JUPITER_RANDOM_ENABLE_F90)
  target_include_directories(jupiter-random-f90-lib
    PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

  target_compile_definitions(jupiter-random-f90-lib
    PUBLIC ${JUPITER_RANDOM_F90_DEFS}
    PRIVATE ${JUPITER_RANDOM_F90_INTERNAL_DEFS}
    INTERFACE ${JUPITER_RANDOM_F90_INTERFACE_DEFS})

  set_target_properties(jupiter-random-f90-lib PROPERTIES
    OUTPUT_NAME jupiter-random-f90)

  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    set_target_properties(jupiter-random-f90 PROPERTIES
      Fortran_PREPROCESS ON)
  endif()

  target_link_libraries(jupiter-random-f90-lib
    jupiter-random ${JUPITER_RANDOM_F90_LIBS})
  target_link_libraries(jupiter-random-f90 jupiter-random-f90-lib)

  get_target_property(FMODDIR jupiter-random-f90 Fortran_MODULE_DIRECTORY)
  if(NOT FMODDIR)
    set(FMODDIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  target_include_directories(jupiter-random-f90
    PUBLIC $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
           $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:Fortran>:${FMODDIR}>>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jupiter>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

  # modulefiles may be capital letters.
  install(FILES ${FMODDIR}/jupiter_random.mod
                ${FMODDIR}/jupiter_random_types.mod
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/jupiter/random)

  install(FILES $<TARGET_OBJECTS:jupiter-random-f90>
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/jupiter/random)

  install(TARGETS jupiter-random-f90-lib EXPORT Random
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/random)

  install(TARGETS jupiter-random-f90 EXPORT Random
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/random)

  # Interface fortran source code is always usable
  install(FILES random.f90
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/jupiter/random)
endif()

install(EXPORT Random
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
