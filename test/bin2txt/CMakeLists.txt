
if (NOT BUILD_TESTING OR NOT BUILD_JUPITER)
  return()
endif()

add_library(bin2txt-convert-s STATIC convert.c)
add_library(bin2txt-convert-d STATIC convert.c)
add_library(bin2txt-convert-i STATIC convert_i.c)
target_compile_definitions(bin2txt-convert-d PRIVATE JUPITER_DOUBLE)

add_library(bin2txt-convert STATIC common.c)
target_include_directories(bin2txt-convert PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(bin2txt-convert
  bin2txt-convert-s bin2txt-convert-d bin2txt-convert-i)

jupiter_re2c_target(bin2txt.c.re)
add_executable(bin2txt bin2txt.c)
target_link_libraries(bin2txt bin2txt-convert libjupiter)

set(TOLOWER "bin2txt-${JUPITER_FP_PRECISION}" N)
set_target_properties(bin2txt PROPERTIES OUTPUT_NAME "${N}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/bin")

add_executable(bin2txt-test bin2txt-test.c)
target_link_libraries(bin2txt-test bin2txt-convert libjupiter)
add_test(NAME bin2txt-test COMMAND $<TARGET_FILE:bin2txt-test>)
set(TOLOWER "bin2txt-test-${JUPITER_FP_PRECISION}" N)
set_target_properties(bin2txt PROPERTIES OUTPUT_NAME "${N}")

jupiter_re2c_target(bincomp.c.re)
add_executable(bincomp bincomp.c)
target_link_libraries(bincomp re2c_lparser)
set(TOLOWER "bincomp-${JUPITER_FP_PRECISION}" N)
set_target_properties(bincomp PROPERTIES OUTPUT_NAME "${N}")
