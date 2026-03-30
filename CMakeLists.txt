cmake_minimum_required(VERSION 4.3.0)
project(my_kernel_module LANGUAGES CXX C)

# So clangd/IDEs get correct includes and flags (see .clangd CompilationDatabase).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_MODE
    "host"
    CACHE STRING "Build mode: host, platform, ci")
# Default OFF so host test builds do not require kernel-devel. Pass -DBUILD_KO=ON (e.g. just ko) to
# build the real .ko via Kbuild.
option(BUILD_KO "Build the real .ko via Kbuild" OFF)

# ---------------------------------------------------------------------------
# Load the cpp-lkm framework (defines cpp_lkm_create_kernel_interface and cpp_lkm_add_ko_target, and
# creates the cpp_lkm_runtime STATIC target).
# ---------------------------------------------------------------------------
add_subdirectory(cpp-lkm)

include(cmake/cpp/CheckExpected.cmake)
include(cmake/kernel/ToolchainCheck.cmake)

# ---------------------------------------------------------------------------
# Kernel interface target — carries all compiler flags, include paths, and kernel definitions
# required to compile kernel module C++ code. In host mode we omit ABI flags so the same code
# compiles for testing.
# ---------------------------------------------------------------------------
if(BUILD_KO)
    set(_abi_mode ko)
else()
    set(_abi_mode "")
endif()

cpp_lkm_create_kernel_interface(
    "my_kernel_module.iface"
    MODULE_NAME
    my_kernel_module
    MODULE_API_HEADER
    "kernel_api/kernel_api.h"
    ABI_MODE
    "${_abi_mode}")

# C-only interface for linting kernel_api bridge sources with kernel-like flags/includes.
add_library(my_kernel_module.c_iface INTERFACE)
cpp_lkm_get_c_flags(_my_kernel_module_c_flags)
target_compile_options("my_kernel_module.c_iface"
                       INTERFACE $<$<COMPILE_LANGUAGE:C>:${_my_kernel_module_c_flags}>)
target_compile_definitions("my_kernel_module.c_iface" INTERFACE __KERNEL__ MODULE)
cpp_lkm_resolve_kdir(_my_kernel_module_kdir "")
cpp_lkm_attach_kernel_headers("my_kernel_module.c_iface" KDIR "${_my_kernel_module_kdir}"
                              MODULE_NAME "my_kernel_module")
target_compile_options(
    "my_kernel_module.c_iface"
    INTERFACE
        $<$<COMPILE_LANGUAGE:C>:-include${_my_kernel_module_kdir}/include/generated/autoconf.h>
        $<$<COMPILE_LANGUAGE:C>:-include${_my_kernel_module_kdir}/include/linux/kconfig.h>)
if(BUILD_KO)
    cpp_lkm_attach_kernel_abi("my_kernel_module.c_iface")
endif()

# ---------------------------------------------------------------------------
# Consumer STATIC library — module implementation plus optional project kernel_api.
# ---------------------------------------------------------------------------
add_subdirectory(src)
target_link_libraries(cpp_lkm_runtime PUBLIC "my_kernel_module.iface")
target_link_libraries("my_kernel_module.iface" INTERFACE kernel_api_iface)

# ---------------------------------------------------------------------------
# BUG() gate — verify no BUG() calls in source.
# ---------------------------------------------------------------------------
add_custom_target(
    check_no_bug
    COMMENT "Verify module sources contain no BUG() calls"
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}/src -P
            ${CMAKE_SOURCE_DIR}/cmake/testing/CheckNoBug.cmake
    VERBATIM)
add_dependencies(kernel_module check_no_bug)

# ---------------------------------------------------------------------------
# Bridge: generates and compiles cpp_module_init/cpp_module_exit.
#
# HOST mode: cpp_lkm_gen_host_bridge() creates my_kernel_module.bridge and folds it into the static
# lib so tests can call cpp_module_init.
#
# KO mode:   cpp_lkm_add_ko_target() internally calls cpp_lkm_generate_module_bridge() which also
# creates my_kernel_module.bridge; only one must run.
# ---------------------------------------------------------------------------
include(cmake/testing/GenModuleBridge.cmake)

if(BUILD_KO)
    cpp_lkm_add_ko_target(
        TARGET
        kernel_module
        MODULE_NAME
        my_kernel_module
        MODULE_OBJECT
        KernelModule
        MODULE_HEADER
        kernel_module/kernel_module.hpp
        KBUILD_SOURCE_TARGETS
        kernel_api_sources
        KBUILD_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/src/kernel_api/include"
        KERNEL_INTERFACE
        "my_kernel_module.iface"
        MODULE_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/src/kernel_module/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/core/chardev/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/features/echo/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/features/files_hider/include"
        ALL)
else()
    cpp_lkm_gen_host_bridge(
        MODULE_NAME
        my_kernel_module
        TARGET
        kernel_module
        MODULE_OBJECT
        KernelModule
        MODULE_HEADER
        kernel_module/kernel_module.hpp
        KERNEL_INTERFACE
        "my_kernel_module.iface"
        MODULE_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/src/kernel_module/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/core/chardev/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/features/echo/include"
        "${CMAKE_SOURCE_DIR}/src/kernel_module/features/files_hider/include")
endif()

# ---------------------------------------------------------------------------
# Host tests (only when NOT BUILD_KO — separate build tree).
# ---------------------------------------------------------------------------
enable_testing()
include(cmake/testing/NegativeCompileTest.cmake)

if(NOT BUILD_KO)
    set(TEST_SRCS tests/test_init.cpp tests/test_allocator.cpp tests/test_compat.cpp
                  tests/test_userspace_device.cpp tests/test_files_hider_api.cpp)
    # mock_kernel_bridge.cpp provides host-mode kernel API stubs.
    set(TEST_MOCK_SRC tests/support/mock_kernel_bridge.cpp)

    foreach(test_src ${TEST_SRCS})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src} ${TEST_MOCK_SRC})
        target_include_directories(${test_name} PRIVATE "${CMAKE_SOURCE_DIR}")
        target_link_libraries(${test_name} PRIVATE kernel_module)
        target_link_options(${test_name} PRIVATE "-Wl,--no-undefined")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    add_negative_link_test(test_no_raw_new ${CMAKE_SOURCE_DIR}/tests/test_negative_new.cpp)
    add_test(NAME test_no_raw_new_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                                               --target test_no_raw_new)
endif()

add_negative_compile_test(test_nodiscard ${CMAKE_SOURCE_DIR}/tests/test_nodiscard.cpp)
add_test(NAME test_nodiscard_eval COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target
                                          test_nodiscard)

# ---------------------------------------------------------------------------
# Format / lint convenience targets.
# ---------------------------------------------------------------------------
include(cmake/cpp/FormatTargets.cmake)
