include(CheckCSourceCompiles)

get_property(C_FEATURES GLOBAL PROPERTY CMAKE_C_KNOWN_FEATURES)
if(c_std_99 IN_LIST C_FEATURES)
  add_library(geometry_test_c99 STATIC
    geometry_test_error.c
    geometry_test_error2.c)
  target_link_libraries(geometry_test_c99 jupiter-geometry-headers)
  set_target_properties(geometry_test_c99 PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS OFF)

  set(GEOM_STATIC_FAIL_TESTS geometry_test_error4.c geometry_test_error5.c)

  set(__VARS_SAVE CMAKE_C_STANDARD CMAKE_C_STANDARD_REQUIRED CMAKE_C_EXTENSIONS
    CMAKE_REQUIRED_INCLUDES CMAKE_REQUIRED_QUIET)
  foreach(_V IN LISTS __VARS)
    set(__${_V}_SAVE ${${_V}})
  endforeach()
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED TRUE)
  set(CMAKE_C_EXTENSIONS FALSE)
  set(CMAKE_REQUIRED_INCLUDES "${__CMAKE_REQUIRED_INCLUDES_SAVE};${CMAKE_SOURCE_DIR}/jupiter")
  set(CMAKE_REQUIRED_QUIET TRUE)
  foreach(SRC IN LISTS GEOM_STATIC_FAIL_TESTS)
    unset(GEOM_ASSERT_EXERROR CACHE)
    unset(GEOM_ASSERT_EXERROR)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}" CODE)
    check_c_source_compiles("${CODE}" GEOM_ASSERT_EXERROR)
    if(GEOM_ASSERT_EXERROR)
      message(SEND_ERROR "GEOM_STATIC_ASSERT did not raise error as expected
(${SRC} has been unexpectedly compiled fine)")
    endif()
  endforeach()
  foreach(_V IN LISTS __VARS)
    set(${_V} ${__${_V}_SAVE})
  endforeach()
else()
  message(WARNING "Compiler does not support C99")
endif()

if(c_std_11 IN_LIST C_FEATURES)
  add_library(geometry_test_c11 STATIC geometry_test_error3.c)
  target_link_libraries(geometry_test_c11 jupiter-geometry-headers)
  set_target_properties(geometry_test_c11 PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS OFF)
  target_compile_features(geometry_test_c11 PRIVATE c_static_assert)
else()
  get_target_property(JUPITER_C_STANDARD jupiter C_STANDARD)
  if(NOT JUPITER_C_STANDARD)
    set(JUPITER_C_STANDARD ${CMAKE_C_STANDARD})
  endif()
  if("${JUPITER_C_STANDARD}" EQUAL 90 OR "${JUPITER_C_STANDARD}" EQUAL 99)
    set(JUPITER_NOTE "
(note: JUPITER main program does not require C11. Skipping some tests which are compiled in C11)")
  else()
    set(JUPITER_NOTE)
  endif()
  message(WARNING "Compiler does not support C11${JUPITER_NOTE}")
endif()

add_executable(geometry_test
  geometry_test.c
  geometry_test_bitarray.c
  geometry_test_variant.c
  geometry_test_rbtree.c
  geometry_test_math.c
  geometry_test_vector.c
  geometry_test_abuilder.c
  geometry_test_trip.c
  geometry_test_polynomial.c)
target_link_libraries(geometry_test m jupiter-geometry test-util)
if(TARGET geometry_test_c99)
  target_link_libraries(geometry_test geometry_test_c99)
endif()
if(TARGET geometry_test_c11)
  target_link_libraries(geometry_test geometry_test_c11)
endif()
foreach(CASE variant rbtree math vector abuilder trip bitarray)
  add_test(NAME geometry-test-${CASE}
    COMMAND $<TARGET_FILE:geometry_test> ${CASE})
endforeach()

#jupiter_re2c_target(geometry_trip_test.re)
#add_executable(geometry_trip_test
#  geometry_trip_test.c)
#target_link_libraries(geometry_trip_test m jupiter-geometry)
#if(JUPITER_GEOMETRY_USE_OPENMP)
#  set_target_properties(geometry_trip_test PROPERTIES
#    LINK_FLAGS "${OpenMP_C_FLAGS}")
#endif()
