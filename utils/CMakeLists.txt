
# Installable libraries (technically header files) goes to under `jupiter`
set(JUPITER_UTILS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(JUPITER_UTILS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(jupiter/cxxoptparse)
add_subdirectory(jupiter/vtkiobind)
add_subdirectory(jupiter/vtkJUPITER)
add_subdirectory(jupiter/vtkIOJUPITER)
add_subdirectory(pack2txt)
add_subdirectory(dumpmesh)
add_subdirectory(make-vtk)
add_subdirectory(stl2bin)
add_subdirectory(win32-scripts)
add_subdirectory(extfile-extractor)
