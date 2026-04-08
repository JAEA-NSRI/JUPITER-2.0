
if (NOT TARGET vtkJUPITER OR NOT TARGET vtkIOJUPITER)
  return()
endif()
set(MISSING)
if (NOT TARGET VTK::IOGeometry)
  set(MISSING "${MISSING}
 * IOGeometry")
endif()
if (NOT TARGET VTK::IOXML)
  set(MISSING "${MISSING}
 * IOXML")
endif()
if (JUPITER_USE_MPI)
  if(NOT TARGET VTK::ParallelMPI)
    set(MISSING "${MISSING}
 * ParallelMPI")
  endif()
  if(NOT TARGET VTK::IOParallelXML)
    set(MISSING "${MISSING}
 * IOParallelXML")
  endif()
endif()
if(MISSING)
  message(WARNING "Skipping building stl2bin because following VTK modules are missing:${MISSING}")
  return()
endif()

set(STL2BIN_DEFS)
set(STL2BIN_LIBS vtkJUPITER vtkIOJUPITER jupiter-cxxoptparse VTK::IOGeometry
  VTK::ParallelCore VTK::IOXML)
if(JUPITER_USE_MPI AND TARGET VTK::ParallelMPI)
  list(APPEND STL2BIN_LIBS VTK::ParallelMPI VTK::IOParallelXML)
  list(APPEND STL2BIN_DEFS JUPITER_MPI)
endif()

add_executable(jupiter-stl2bin stl2bin.cxx)
target_link_libraries(jupiter-stl2bin ${STL2BIN_LIBS})
target_compile_definitions(jupiter-stl2bin PUBLIC ${STL2BIN_DEFS})

install(TARGETS jupiter-stl2bin RUNTIME DESTINATION bin)
