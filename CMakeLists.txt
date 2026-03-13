cmake_minimum_required(VERSION 3.20)
project(cpp_kernel_module LANGUAGES CXX)

set(BUILD_MODE "host" CACHE STRING "Build mode: host, platform, ci")

# Toolchain version enforcement
if(BUILD_MODE STREQUAL "host")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.0")
        message(FATAL_ERROR "Host Clang must be 16+")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        message(FATAL_ERROR "Host GCC must be 13+")
    endif()
elseif(BUILD_MODE STREQUAL "platform" OR BUILD_MODE STREQUAL "ci")
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Platform mode requires GCC")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13.0")
        message(FATAL_ERROR "Platform mode requires GCC 12.x")
    endif()
endif()

# Common Compile Flags
add_compile_options(
    -std=c++23
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -fno-stack-protector
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -Wall -Wextra -Werror
    -Werror=unused-result
    -Werror=return-type
)

# G++12 Workarounds
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13")
    add_compile_options(-Wno-interference-size)
endif()

# Probe std::expected availability
include(cmake/CheckExpected.cmake)

# Includes
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/mock_kernel ${CMAKE_SOURCE_DIR}/third_party)

# Library target
add_library(kernel_module STATIC
    src/module.cpp
    src/operator_new.cpp
    src/bridge.cpp
    src/mock_globals.cpp
)

# BUG() Usage Gate
add_custom_target(check_no_bug
    COMMAND bash -c
        "grep -rn 'BUG(' ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include \
         && echo 'FAIL: BUG() found in module source' && exit 1 \
         || echo 'OK: no BUG() in module source'"
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)

# Testing
enable_testing()
include(cmake/NegativeCompileTest.cmake)

# Test Targets
set(TEST_SRCS tests/test_init.cpp tests/test_allocator.cpp tests/test_compat.cpp)

foreach(test_src ${TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE kernel_module)
    target_link_options(${test_name} PRIVATE "-Wl,--no-undefined")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Negative Tests
add_negative_compile_test(test_nodiscard ${CMAKE_SOURCE_DIR}/tests/test_nodiscard.cpp)
add_negative_link_test(test_no_raw_new ${CMAKE_SOURCE_DIR}/tests/test_negative_new.cpp)

add_test(NAME test_nodiscard_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_nodiscard)
add_test(NAME test_no_raw_new_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_no_raw_new)
