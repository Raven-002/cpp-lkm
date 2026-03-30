cmake_minimum_required(VERSION 4.3.0)

# cpp-lkm: reusable C++23 Linux kernel module framework.
#
# Add this to your project with: add_subdirectory(cpp-lkm)
#
# Then use the two public functions: cpp_lkm_create_kernel_interface(<iface-target> MODULE_NAME
# <name> MODULE_API_HEADER <header> [KDIR <path>] [ABI_MODE ko]) cpp_lkm_add_ko_target(TARGET <lib>
# MODULE_NAME <n> MODULE_OBJECT <C> MODULE_HEADER <h> KBUILD_SOURCE_TARGETS <target>...
# [KBUILD_INCLUDE_DIRS <dir>...] [ALL])

# Expose the framework root so helper cmake files can resolve relative paths.
set(CPP_LKM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
    CACHE INTERNAL "cpp-lkm framework root")

if(NOT TARGET cpp_lkm_compat)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../compat" "${CMAKE_BINARY_DIR}/_cpp_lkm_compat")
endif()

# ---------------------------------------------------------------------------
# cpp_lkm_runtime — reusable STATIC library compiled into every .ko: operator_new.cpp : traps heap
# new; implements operator delete -> kfree
# ---------------------------------------------------------------------------
add_library(cpp_lkm_runtime STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/operator_new.cpp")
target_link_libraries(cpp_lkm_runtime PUBLIC cpp_lkm_compat)
target_sources(
    cpp_lkm_runtime
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           "${CMAKE_CURRENT_SOURCE_DIR}/include"
           FILES
           include/cpp_lkm/common/error.hpp
           include/cpp_lkm/runtime/kernel_api/context.h
           include/cpp_lkm/runtime/kernel_api/kernel_api.h
           include/cpp_lkm/runtime/kernel_api/memory.h
           include/cpp_lkm/runtime/kernel_api/types.h
           include/cpp_lkm/runtime/kalloc.hpp
           include/cpp_lkm/runtime/kernel_module.hpp
           include/cpp_lkm/runtime/module_entry.h)
# Headers under cpp-lkm/include/ infer flags from this TU in clangd; need C++23 for std::expected.
target_compile_features(cpp_lkm_runtime PUBLIC cxx_std_23)
# Runtime itself does not carry ABI/kernel flags — those come from the interface target that the
# consumer attaches.

# Runtime C bridge symbols compiled by Kbuild for .ko targets.
add_custom_target(
    cpp_lkm_runtime_kernel_api_sources
    COMMENT "List kernel_api C sources for Kbuild (not built by CMake)"
    SOURCES src/kernel_api/context_kernel.c src/kernel_api/memory_kernel.c)

# ---------------------------------------------------------------------------
# Load the public API functions.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppLkmTargets.cmake")

# Lint-only C target to provide compile_commands entries for clang-tidy on kernel_api C bridge files
# with kernel-style includes/defines.
add_library(cpp_lkm_runtime_kernel_api_lint_iface INTERFACE)
cpp_lkm_get_c_flags(_cpp_lkm_runtime_c_flags)
target_compile_options(cpp_lkm_runtime_kernel_api_lint_iface
                       INTERFACE $<$<COMPILE_LANGUAGE:C>:${_cpp_lkm_runtime_c_flags}>)
target_compile_definitions(cpp_lkm_runtime_kernel_api_lint_iface INTERFACE __KERNEL__ MODULE)
cpp_lkm_resolve_kdir(_cpp_lkm_runtime_kdir "")
cpp_lkm_attach_kernel_headers(cpp_lkm_runtime_kernel_api_lint_iface KDIR "${_cpp_lkm_runtime_kdir}"
                              MODULE_NAME "cpp_lkm_runtime")
target_compile_options(
    cpp_lkm_runtime_kernel_api_lint_iface
    INTERFACE $<$<COMPILE_LANGUAGE:C>:-include${_cpp_lkm_runtime_kdir}/include/generated/autoconf.h>
              $<$<COMPILE_LANGUAGE:C>:-include${_cpp_lkm_runtime_kdir}/include/linux/kconfig.h>)
if(BUILD_KO)
    cpp_lkm_attach_kernel_abi(cpp_lkm_runtime_kernel_api_lint_iface)
endif()
add_library(cpp_lkm_runtime_kernel_api_lint_objects OBJECT
            EXCLUDE_FROM_ALL src/kernel_api/context_kernel.c src/kernel_api/memory_kernel.c)
target_link_libraries(cpp_lkm_runtime_kernel_api_lint_objects
                      PRIVATE cpp_lkm_runtime_kernel_api_lint_iface)
target_include_directories(cpp_lkm_runtime_kernel_api_lint_objects
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
