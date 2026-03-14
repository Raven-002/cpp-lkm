cmake_minimum_required(VERSION 3.20)
project(cpp_kernel_module LANGUAGES CXX)

set(BUILD_MODE "host" CACHE STRING "Build mode: host, platform, ci")
option(BUILD_KO "Build the real .ko via Kbuild" OFF)
if(NOT BUILD_MODE STREQUAL "host")
    set(BUILD_KO_DEFAULT ON)
else()
    set(BUILD_KO_DEFAULT OFF)
endif()
if(NOT DEFINED CACHE{BUILD_KO})
    set(BUILD_KO ${BUILD_KO_DEFAULT} CACHE BOOL "Build the real .ko via Kbuild" FORCE)
endif()

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

# Kbuild Integration
include(cmake/KbuildModule.cmake)

# Includes
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/mock_kernel ${CMAKE_SOURCE_DIR}/third_party)

# Library target
add_library(kernel_module STATIC
    src/module.cpp
    src/operator_new.cpp
    src/bridge.cpp
)

# Match kernel ABI so the .ko loads (x86_64).
# When BUILD_KO=ON we need these flags so the .ko is loadable.
# When BUILD_KO=OFF in host mode, we must NOT use them or test executables fail to link
# (they need __x86_return_thunk which is only in the kernel).
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    if(BUILD_KO OR NOT BUILD_MODE STREQUAL "host")
        target_compile_options(kernel_module PRIVATE -mcmodel=kernel)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(kernel_module PRIVATE
                -mindirect-branch=thunk-extern
                -mindirect-branch-register
                -mfunction-return=thunk-extern
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(kernel_module PRIVATE
                -mretpoline-external-thunk
                -mfunction-return=thunk-extern
            )
        endif()
    endif()
endif()

# BUG() Usage Gate (C++ sources only; linux_bridge.c may mention "C++ BUG()" in panic text)
add_custom_target(check_no_bug
    COMMAND bash -c
        "grep -rn 'BUG(' ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include --include='*.cpp' --include='*.h' --include='*.hpp' \
         && echo 'FAIL: BUG() found in module source' && exit 1 \
         || echo 'OK: no BUG() in module source'"
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)

# Testing
enable_testing()
include(cmake/NegativeCompileTest.cmake)

# Test Targets (only when not building .ko: kernel_module has retpoline when BUILD_KO=ON and tests would fail to link)
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
    add_test(NAME test_no_raw_new_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_no_raw_new)
endif()

# Negative compile test (does not link kernel_module, so always available)
add_negative_compile_test(test_nodiscard ${CMAKE_SOURCE_DIR}/tests/test_nodiscard.cpp)
add_test(NAME test_nodiscard_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_nodiscard)

# Real KO generation via Kbuild
if(BUILD_KO)
    add_kbuild_module(cpp_lkm kernel_module ALL)
endif()

# Formatting helpers (optional)
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE FORMAT_SOURCES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/include/*.h"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/tests/*.h"
        "${CMAKE_SOURCE_DIR}/compat/*.h"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*.h"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*/*.h"
        "${CMAKE_SOURCE_DIR}/mock_kernel/*/*/*.h"
    )

    add_custom_target(format
        COMMAND "${CLANG_FORMAT_EXE}" -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-format"
        VERBATIM
    )

    add_custom_target(format-check
        COMMAND "${CLANG_FORMAT_EXE}" --dry-run -Werror ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking clang-format"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found; format targets disabled")
endif()

