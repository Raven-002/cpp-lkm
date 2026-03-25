cmake_minimum_required(VERSION 3.20)
project(cpp_kernel_module LANGUAGES CXX)
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()

set(BUILD_MODE
    "host"
    CACHE STRING "Build mode: host, platform, ci"
)
# Default OFF so host test builds do not require kernel-devel. Explicit -DBUILD_KO=ON
# (e.g. just ko, build-ko/) enables Kbuild regardless of BUILD_MODE.
option(BUILD_KO "Build the real .ko via Kbuild" OFF)

include(cmake/kernel/ToolchainCheck.cmake)
include(cmake/cpp/CxxFlags.cmake)
include(cmake/cpp/CheckExpected.cmake)

add_library(libkernel INTERFACE)
target_compile_options(libkernel INTERFACE ${LIBKERNEL_CXX_FLAGS})
target_include_directories(libkernel INTERFACE ${LIBKERNEL_INCLUDE_DIRS})

include(cmake/kernel/KernelHeaders.cmake)

include(cmake/kernel/KernelAbiFlags.cmake)
include(cmake/kernel/KbuildModule.cmake)

add_subdirectory(src/common)
add_subdirectory(src/runtime)
add_subdirectory(src/module)

add_library(
    kernel_module STATIC
    $<TARGET_OBJECTS:kernel_runtime>
    $<TARGET_OBJECTS:module_core>
)
target_link_libraries(kernel_module PUBLIC libkernel kernel_runtime module_core)

add_custom_target(
    check_no_bug
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}/src -P
            ${CMAKE_SOURCE_DIR}/cmake/testing/CheckNoBug.cmake
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)

enable_testing()
include(cmake/testing/NegativeCompileTest.cmake)

set(TEST_SRCS tests/test_init.cpp tests/test_allocator.cpp tests/test_compat.cpp
              tests/test_userspace_device.cpp)
# mock_kernel_bridge.cpp provides host-mode kernel API stubs (state + bridge).
set(TEST_MOCK_SRC tests/support/mock_kernel_bridge.cpp)

if(NOT BUILD_KO)
    foreach(test_src ${TEST_SRCS})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src} ${TEST_MOCK_SRC})
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

include(cmake/cpp/FormatTargets.cmake)
