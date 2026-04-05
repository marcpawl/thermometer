# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.local")
set(PROJECT_VER "0.0.30")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(thermometer)


set(DESTINATION_PATH "${project_dir}/releases")

file(MAKE_DIRECTORY ${DESTINATION_PATH})

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
            "${DESTINATION_PATH}/${CMAKE_PROJECT_NAME}.bin"
    COMMENT "Copying firmware binary to releases folder..."
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Host Unit Tests
# -----------------------------------------------------------------------------
set(VECTOR_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/test/test_inplace_vector.cpp")
set(VECTOR_TEST_BIN "${CMAKE_BINARY_DIR}/test_inplace_vector_host")
set(MAIN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/main")
set(VECTOR_TEST_HEADERS
    "${MAIN_INCLUDE_DIR}/marcpawl_inplace_vector.hpp"
)

add_custom_target(run_vector_tests ALL
    COMMAND c++ -std=c++23 -I${MAIN_INCLUDE_DIR} ${VECTOR_TEST_SRC} -o ${VECTOR_TEST_BIN}
    COMMAND ${VECTOR_TEST_BIN}
    DEPENDS
        ${VECTOR_TEST_SRC}
        ${VECTOR_TEST_HEADERS}
    COMMENT "Building and running host tests for inplace_vector..."
    VERBATIM
)

set(STRING_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/test/test_inplace_string.cpp")
set(STRING_TEST_BIN "${CMAKE_BINARY_DIR}/test_inplace_string_host")
set(MAIN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/main")
set(STRING_TEST_HEADERS
    "${MAIN_INCLUDE_DIR}/marcpawl_inplace_string.hpp"
    "${MAIN_INCLUDE_DIR}/marcpawl_inplace_vector.hpp"
)

add_custom_target(run_string_tests ALL
    COMMAND c++ -std=c++23 -I${MAIN_INCLUDE_DIR} ${STRING_TEST_SRC} -o ${STRING_TEST_BIN}
    COMMAND ${STRING_TEST_BIN}
    DEPENDS
        ${STRING_TEST_SRC}
        ${STRING_TEST_HEADERS}
    COMMENT "Building and running host tests for inplace_string..."
    VERBATIM
)

# Ensure tests pass as part of the build
add_dependencies(app run_vector_tests)
