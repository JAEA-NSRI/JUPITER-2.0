
if (VTK_FOUND)
  message(STATUS "Using VTK ${VTK_VERSION}")

  unset(MAKE_VTK_EXE_LIBS)
  unset(MAKE_XDMF_EXE_LIBS)
  set(MAKE_VTK_DEFS)

  if (JUPITER_USE_MPI)
    list(APPEND MAKE_VTK_DEFS MAKE_VTK_MPI)
  endif()

  if (VTK_VERSION VERSION_LESS "9.0.0")
    if (JUPITER_USE_MPI)
      if (NOT TARGET vtkParallelMPI)
        message(SEND_ERROR "VTK did not bulid with MPI.")
        set(MAKE_VTK_MPI_LIB)
      else()
        set(MAKE_VTK_MPI_LIB vtkParallelMPI MPI::MPI_CXX)
      endif()
    else()
      set(MAKE_VTK_MPI_LIB vtkParallelCore)
    endif()

    set(MAKE_VTK_BASE_LIBS vtkIOCore vtkCommonCore vtkCommonExecutionModel)
    if (MAKE_VTK_MPI_LIB)
      set(MAKE_VTK_EXE_LIBS vtkIOParallelXML)
      set(MAKE_XDMF_EXE_LIBS vtkIOParallelXdmf3)
    else()
      set(MAKE_VTK_EXE_LIBS vtkIOXML)
      set(MAKE_XDMF_EXE_LIBS vtkIOXdmf3)
    endif()
  else()
    find_package(VTK COMPONENTS IOCore CommonCore CommonExecutionModel REQUIRED)
    set(MAKE_VTK_BASE_LIBS VTK::IOCore VTK::CommonCore VTK::CommonExecutionModel)
    if (JUPITER_USE_MPI)
      find_package(VTK COMPONENTS ParallelMPI REQUIRED)
      find_package(VTK COMPONENTS IOParallelXML IOParallelXdmf3)
      set(MAKE_VTK_MPI_LIB VTK::ParallelMPI MPI::MPI_CXX)
      set(MAKE_VTK_EXE_LIBS VTK::IOParallelXML)
      set(MAKE_XDMF_EXE_LIBS VTK::IOParallelXdmf3)
    else()
      find_package(VTK COMPONENTS ParallelCore REQUIRED)
      find_package(VTK COMPONENTS IOXML IOXdmf3)
      set(MAKE_VTK_MPI_LIB VTK::ParallelCore)
      set(MAKE_VTK_EXE_LIBS VTK::IOXML)
      seT(MAKE_XDMF_EXE_LIBS VTK::IOXdmf3)
    endif()
  endif()

  add_library(MakeVTKApp STATIC make-vtk-app.cxx make-vtk-optparse.cxx)
  target_link_libraries(MakeVTKApp jupiter-cxxoptparse jupiter-vtkiobind ${MAKE_VTK_MPI_LIB})
  target_include_directories(MakeVTKApp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(MakeVTKApp PUBLIC ${MAKE_VTK_DEFS})

  if (TARGET "${MAKE_VTK_EXE_LIBS}")
    add_executable(make-vtk make-vtk.cxx)

    set_target_properties(make-vtk PROPERTIES
      INCLUDE_DIRECTORIES "${MAKE_VTK_INCLUDES}"
      COMPILE_DEFINITIONS "${MAKE_VTK_DEFS}"
      LINKER_LANGUAGE C)

    install(TARGETS make-vtk RUNTIME DESTINATION bin)

    target_link_libraries(make-vtk
      MakeVTKApp vtkIOJUPITER ${MAKE_VTK_EXE_LIBS})
  else()
    message(WARNING "Skip building make-vtk because VTK module ${MAKE_VTK_EXE_LIBS} is not provided.")
  endif()

  if (TARGET "${MAKE_XDMF_EXE_LIBS}")
    add_executable(make-xdmf make-xdmf.cxx)

    set_target_properties(make-xdmf PROPERTIES
      INCLUDE_DIRECTORIES "${MAKE_VTK_INCLUDES}"
      COMPILE_DEFINITIONS "${MAKE_VTK_DEFS}"
      LINKER_LANGUAGE C)

    target_link_libraries(make-xdmf
      MakeVTKApp vtkIOJUPITER ${MAKE_XDMF_EXE_LIBS})

    install(TARGETS make-xdmf RUNTIME DESTINATION bin)
  else()
    message(WARNING "Skip building make-vtk because VTK module ${MAKE_XDMF_EXE_LIBS} is not provided.")
  endif()
endif()

# Simple Non-VTK version
jupiter_re2c_target(make-vtk-unconventional.c.re)
add_executable(make-vtk-unconventional make-vtk-unconventional.c)
target_link_libraries(make-vtk-unconventional jupiter-vtkiobind)
target_include_directories(make-vtk-unconventional PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
