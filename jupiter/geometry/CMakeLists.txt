

find_package(Vasprintf)

add_library(jupiter-geometry-headers INTERFACE)
add_library(JUPITER::jupiter-geometry-headers ALIAS jupiter-geometry-headers)

add_library(jupiter-geometry global.c error.c variant.c alloc_list.c
  abuilder.c init.c shape.c data.c file.c common.c infomap.c udata.c
  vector.c svector.c matrix.c funcs_common.c shape_transform.c shape_mesh.c
  2d_path.c 2d_path_element.c 2d_pathcalc.c surface_shape.c enumutil.c

  if_none.c if_const.c if_linear.c if_poly.c if_poly_n.c if_exp_poly.c

  shp_specials.c shp_box.c shp_pla.c shp_app.c shp_wed.c shp_arb.c shp_rpr.c
  shp_trp.c shp_sph.c shp_rcc.c shp_ell.c shp_trc.c shp_rec.c shp_tec.c
  shp_tor.c shp_eto.c shp_pl1.c shp_pl2.c shp_pl3.c shp_pl4.c shp_pln.c
  shp_tra.c shp_rot.c shp_sca.c shp_mat.c

  surfshp_specials.c surfshp_parallelogram.c
)
add_library(JUPITER::jupiter-geometry ALIAS jupiter-geometry)

get_target_property(JUPITER_GEOMETRY_TYPE jupiter-geometry TYPE)

set(JUPITER_GEOMETRY_CFLAGS)
set(JUPITER_GEOMETRY_LFLAGS)
set(JUPITER_GEOMETRY_DEFS)
set(JUPITER_GEOMETRY_INTERNAL_DEFS)
set(JUPITER_GEOMETRY_INTERFACE_DEFS)
set(JUPITER_GEOMETRY_LIBS)
set(JUPITER_GEOMETRY_INCLUDE_PATH)

list(APPEND JUPITER_GEOMETRY_LIBS jupiter-geometry-headers)

if(ENABLE_OPENMP)
  set(JUPITER_GEOMETRY_USE_OPENMP ON)
else()
  set(JUPITER_GEOMETRY_USE_OPENMP OFF)
endif()

if(JUPITER_GEOMETRY_TYPE STREQUAL "SHARED_LIBRARY")
  list(APPEND JUPITER_GEOMETRY_INTERNAL_DEFS "JUPITER_GEOMETRY_EXPORT")
  list(APPEND JUPITER_GEOMETRY_INTERFACE_DEFS "JUPITER_GEOMETRY_IMPORT")
endif()

cmake_dependent_option(JUPITER_GEOMETRY_USE_MPI
  "Enable MPI support for geometry library" ON
  "MPI_C_FOUND" OFF)

if(JUPITER_GEOMETRY_USE_OPENMP)
  list(APPEND JUPITER_GEOMETRY_DEFS  "JUPITER_GEOMETRY_USE_OPENMP")
  list(APPEND JUPITER_GEOMETRY_LIBS  OpenMP::OpenMP_C)
endif()
if(JUPITER_GEOMETRY_USE_MPI)
  list(APPEND JUPITER_GEOMETRY_DEFS  "JUPITER_GEOMETRY_USE_MPI")
  list(APPEND JUPITER_GEOMETRY_LIBS   MPI::MPI_C)
endif()

if (VASPRINTF_FOUND)
  list(APPEND GEOM_PRIV_DEFS HAVE_VASPRINTF)
  foreach(V IN LISTS VASPRINTF_DEFINITIONS)
    string(REGEX REPLACE "^-D" "" VV "${V}")
    list(APPEND JUPITER_GEOMETRY_INTERNAL_DEFS "${VV}")
  endforeach()
endif()

target_link_libraries(jupiter-geometry ${JUPITER_GEOMETRY_LIBS})
if(ENABLE_OPENMP)
  target_link_libraries(jupiter-geometry OpenMP::OpenMP_C)
endif()

target_compile_definitions(jupiter-geometry
  PUBLIC ${JUPITER_GEOMETRY_DEFS}
  PRIVATE ${JUPITER_GEOMETRY_INTERNAL_DEFS}
  INTERFACE ${JUPITER_GEOMETRY_INTERFACE_DEFS})

target_compile_options(jupiter-geometry PUBLIC ${JUPITER_GEOMETRY_CFLAGS})

target_include_directories(jupiter-geometry
  PRIVATE   ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
  INTERFACE $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_include_directories(jupiter-geometry-headers
  INTERFACE $<BUILD_INTERFACE:${JUPITER_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Header-concluded container creation support
set(JUPITER_GEOMETRY_COM_HEADERS
  geom_assert.h list.h rbtree.h bitarray.h bitarrayv.h)

set(JUPITER_GEOMETRY_PUB_HEADERS
  2d_circle.h 2d_circular_arc.h 2d_inout_tests.h 2d_intersects.h
  2d_level_set.h 2d_line.h 2d_line_segment.h 2d_path.c 2d_path.h
  2d_path_element.c 2d_path_element.h 2d_pathcalc.c 2d_pathcalc.h
  2d_quad.h 2d_rectangle.h abuilder.h alloc_list.h common.h data.h
  defs.h error.h file.h func_defs.h funcs_common.h geom_math.h
  global.h infomap.h init.h list.h mat22.h mat33.h mat43.h mat44.h
  matrix.h polynomial.h quat.h shape_extrusion.h shape.h
  shape_mesh.h shape_revolution.h shape_tetrahedra.h svector.h svector_defs.h
  udata.h util.h variant.h vector.h surface_shape.h enumutil.h)

set(JUPITER_GEOMETRY_PRI_HEADERS
  abuilder-priv.h func_data.h shp_app.h shp_arb.h shp_box.h shp_ell.h
  shp_eto.h shp_mat.h shp_pl1.h shp_pl2.h shp_pl3.h shp_pl4.h
  shp_pla.h shp_pln.h shp_rcc.h shp_rec.h shp_rot.h shp_rpr.h
  shp_sca.h shp_specials.h shp_sph.h shp_tec.h shp_tor.h shp_tra.h
  shp_trc.h shp_trp.h shp_wed.h struct_data.h surfshp_parallelogram.h
  surfshp_specials.h udata-priv.h geom_static_assert.h)

set_target_properties(jupiter-geometry PROPERTIES
  PUBLIC_HEADER "${JUPITER_GEOMETRY_PUB_HEADERS}")

install(TARGETS jupiter-geometry EXPORT Geometry
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/geometry)

set_target_properties(jupiter-geometry-headers PROPERTIES
  PUBLIC_HEADER "${JUPITER_GEOMETRY_COM_HEADERS}")

install(TARGETS jupiter-geometry-headers EXPORT Geometry
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jupiter/geometry)

install(EXPORT Geometry
  NAMESPACE JUPITER::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jupiter)
