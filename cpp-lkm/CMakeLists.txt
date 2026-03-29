cmake_minimum_required(VERSION 4.3.0)

# cpp-lkm: reusable C++23 Linux kernel module framework.
#
# Add this to your project with: add_subdirectory(cpp-lkm)
#
# Then use the two public functions: cpp_lkm_create_kernel_interface(<iface-target> MODULE_NAME
# <name> [KDIR <path>] [ABI_MODE ko]) cpp_lkm_add_ko_target(TARGET <lib> MODULE_NAME <n>
# MODULE_OBJECT <C> MODULE_HEADER <h> KERNEL_API_SRC_DIR <dir> [KERNEL_API_INCLUDE_DIR <dir>] [ALL])

# Expose the framework root so helper cmake files can resolve relative paths.
set(CPP_LKM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
    CACHE INTERNAL "cpp-lkm framework root"
)

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
           include/cpp_lkm/runtime/kernel_api.h
           include/cpp_lkm/runtime/kalloc.hpp
           include/cpp_lkm/runtime/kernel_module.hpp
           include/cpp_lkm/runtime/module_entry.h
)
# Headers under cpp-lkm/include/ infer flags from this TU in clangd; need C++23 for std::expected.
target_compile_features(cpp_lkm_runtime PUBLIC cxx_std_23)
# Runtime itself does not carry ABI/kernel flags — those come from the interface target that the
# consumer attaches.

# ---------------------------------------------------------------------------
# Load the public API functions.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppLkmTargets.cmake")
