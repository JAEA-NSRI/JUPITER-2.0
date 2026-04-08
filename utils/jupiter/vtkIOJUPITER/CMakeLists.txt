
if (NOT VTK_FOUND)
  return()
endif()

set(VTKIOJUPITER_DEFS)
set(VTKIOJUPITER_INTERNAL_DEFS)
set(VTKIOJUPITER_INTERFACE_DEFS)
set(VTKIOJUPITER_LIBS jupiter-vtkiobind)
if (VTK_VERSION VERSION_LESS "9.0.0")
  if (JUPITER_USE_MPI)
    if (TARGET vtkParallelMPI)
      list(APPEND VTKIOJUPITER_LIBS vtkParallelMPI)
    else()
      message(WARNING "Non-MPI enabled VTK support is not complete")
    endif()
  else()
    list(APPEND VTKIOJUPITER_LIBS vtkParallelCore)
  endif()
  list(APPEND VTKIOJUPITER_LIBS vtkCommonCore vtkCommonExecutionModel)
else()
  if (JUPITER_USE_MPI)
    list(APPEND VTKIOJUPITER_LIBS VTK::ParallelMPI)
  else()
    list(APPEND VTKIOJUPITER_LIBS VTK::ParallelCore)
  endif()
  list(APPEND VTKIOJUPITER_LIBS VTK::CommonCore VTK::CommonExecutionModel)
endif()


# If enabled, JUPITER is hardly linked with MPI. In this case, we
# cannot build non-MPI version togather, and the both MPI and non-MPI
# module name does not include "P".

add_library(vtkIOJUPITER vtkJUPITERReader.cxx vtkJUPITERWriter.cxx
  vtkJUPITERAttributeWriter.cxx vtkJUPITERDataSetWriter.cxx)
add_library(JUPITER::vtkIOJUPITER ALIAS vtkIOJUPITER)

get_target_property(VTKIOJUPITER_TYPE vtkIOJUPITER TYPE)

if(VTKIOJUPITER_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND VTKIOJUPITER_INTERNAL_DEFS "VTKIOJUPITER_EXPORT")
  list(APPEND VTKIOJUPITER_INTERFACE_DEFS "VTKIOJUPITER_IMPORT")
endif()

target_compile_definitions(vtkIOJUPITER
  PRIVATE ${VTKIOJUPITER_INTERNAL_DEFS}
  INTERFACE ${VTKIOJUPITER_INTERFACE_DEFS})

target_link_libraries(vtkIOJUPITER
  PUBLIC ${VTKIOJUPITER_LIBS})

target_include_directories(vtkIOJUPITER
  PRIVATE ${JUPITER_UTILS_SOURCE_DIR}
  INTERFACE $<BUILD_INTERFACE:${JUPITER_UTILS_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(VTKIOJUPITER_HEADERS vtkIOJUPITER.h vtkJUPITERReader.h
  vtkJUPITERWriter.h vtkJUPITERAttributeWriter.h vtkJUPITERDataSetWriter.h)

set_target_properties(vtkIOJUPITER PROPERTIES
  PUBLIC_HEADER "${VTKIOJUPITER_HEADERS}")

install(TARGETS vtkIOJUPITER EXPORT vtkIOJUPITER
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/vtkIOJUPITER)

install(EXPORT vtkIOJUPITER
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
