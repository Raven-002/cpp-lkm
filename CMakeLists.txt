cmake_minimum_required(VERSION 3.20)
project(cpp_kernel_module LANGUAGES CXX)

set(BUILD_MODE
    "host"
    CACHE STRING "Build mode: host, platform, ci"
)
option(BUILD_KO "Build the real .ko via Kbuild" OFF)
if(NOT BUILD_MODE STREQUAL "host")
    set(BUILD_KO_DEFAULT ON)
else()
    set(BUILD_KO_DEFAULT OFF)
endif()
if(NOT DEFINED CACHE{BUILD_KO})
    set(BUILD_KO
        ${BUILD_KO_DEFAULT}
        CACHE BOOL "Build the real .ko via Kbuild" FORCE
    )
endif()

include(cmake/kernel/ToolchainCheck.cmake)
include(cmake/cpp/CxxFlags.cmake)
include(cmake/cpp/CheckExpected.cmake)

add_library(libkernel INTERFACE)
target_compile_options(libkernel INTERFACE ${LIBKERNEL_CXX_FLAGS})
target_include_directories(libkernel INTERFACE ${LIBKERNEL_INCLUDE_DIRS})

include(cmake/kernel/KernelAbiFlags.cmake)
include(cmake/kernel/KbuildModule.cmake)

add_library(kernel_module STATIC src/module.cpp src/operator_new.cpp src/bridge.cpp)
target_link_libraries(kernel_module PUBLIC libkernel)
target_compile_options(kernel_module PRIVATE ${KERNEL_ABI_COMPILE_OPTIONS})

add_custom_target(
    check_no_bug
    COMMAND
        bash -c
        "grep -rn 'BUG(' ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include --include='*.cpp' --include='*.h' --include='*.hpp' \
         && echo 'FAIL: BUG() found in module source' && exit 1 \
         || echo 'OK: no BUG() in module source'"
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)

enable_testing()
include(cmake/testing/NegativeCompileTest.cmake)

set(TEST_SRCS tests/test_init.cpp tests/test_allocator.cpp tests/test_compat.cpp)

if(NOT BUILD_KO)
    foreach(test_src ${TEST_SRCS})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src} tests/mock_globals.cpp)
        target_link_libraries(${test_name} PRIVATE kernel_module)
        target_link_options(${test_name} PRIVATE "-Wl,--no-undefined")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    add_negative_link_test(test_no_raw_new ${CMAKE_SOURCE_DIR}/tests/test_negative_new.cpp)
    add_test(NAME test_no_raw_new_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                                               --target test_no_raw_new
    )
endif()

add_negative_compile_test(test_nodiscard ${CMAKE_SOURCE_DIR}/tests/test_nodiscard.cpp)
add_test(NAME test_nodiscard_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target
                                          test_nodiscard
)

if(BUILD_KO)
    add_kbuild_module(cpp_lkm kernel_module ALL)
endif()

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(
        GLOB_RECURSE
        FORMAT_SOURCES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/tests/*.hpp"
        "${CMAKE_SOURCE_DIR}/compat/*.hpp"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*.h"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*/*.h"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*/*/*.h"
    )

    add_custom_target(
        format
        COMMAND "${CLANG_FORMAT_EXE}" -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-format"
        VERBATIM
    )

    add_custom_target(
        format-check
        COMMAND "${CLANG_FORMAT_EXE}" --dry-run -Werror ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking clang-format"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found; format targets disabled")
endif()

# CMake format and lint (cmakelang: cmake-format, cmake-lint)
find_program(CMAKE_FORMAT_EXE NAMES cmake-format)
find_program(CMAKE_LINT_EXE NAMES cmake-lint)
if(CMAKE_FORMAT_EXE)
    file(GLOB_RECURSE CMAKE_SOURCES "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
         "${CMAKE_SOURCE_DIR}/cmake/*.cmake" "${CMAKE_SOURCE_DIR}/cmake/*/*.cmake"
    )
    add_custom_target(
        cmake-format
        COMMAND "${CMAKE_FORMAT_EXE}" -i ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running cmake-format"
        VERBATIM
    )
    add_custom_target(
        cmake-format-check
        COMMAND "${CMAKE_FORMAT_EXE}" --check ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking cmake-format"
        VERBATIM
    )
endif()
if(CMAKE_LINT_EXE)
    if(NOT CMAKE_FORMAT_EXE)
        file(GLOB_RECURSE CMAKE_SOURCES "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
             "${CMAKE_SOURCE_DIR}/cmake/*.cmake" "${CMAKE_SOURCE_DIR}/cmake/*/*.cmake"
        )
    endif()
    add_custom_target(
        cmake-lint
        COMMAND "${CMAKE_LINT_EXE}" ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running cmake-lint"
        VERBATIM
    )
endif()
if(NOT CMAKE_FORMAT_EXE)
    message(STATUS "cmake-format not found; cmake-format targets disabled")
endif()
if(NOT CMAKE_LINT_EXE)
    message(STATUS "cmake-lint not found; cmake-lint target disabled")
endif()
