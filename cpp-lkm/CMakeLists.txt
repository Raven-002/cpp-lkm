cmake_minimum_required(VERSION 3.20)

# cpp-lkm: reusable C++23 Linux kernel module framework.
#
# Add this to your project with: add_subdirectory(cpp-lkm)
#
# Then use the two public functions: cpp_lkm_create_kernel_interface(<iface-target> MODULE_NAME
# <name> [KDIR <path>] [ABI_MODE ko]) cpp_lkm_add_ko_target(TARGET <lib> MODULE_NAME <n>
# MODULE_OBJECT <C> MODULE_HEADER <h> [ALL])

# Expose the framework root so helper cmake files can resolve relative paths.
set(CPP_LKM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
    CACHE INTERNAL "cpp-lkm framework root"
)

# Repo root (parent of cpp-lkm/) so compat/ headers are reachable via #include "compat/..."
set(CPP_LKM_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    CACHE INTERNAL "cpp-lkm repo root"
)

# Policy for objtool validation during out-of-tree .ko builds. - disable: emit
# OBJECT_FILES_NON_STANDARD := y in generated Kbuild Makefile - keep: leave objtool validation
# unchanged
set(CPP_LKM_OBJTOOL_MODE
    "disable"
    CACHE STRING "Objtool handling for generated Kbuild module builds (disable|keep)"
)
set_property(CACHE CPP_LKM_OBJTOOL_MODE PROPERTY STRINGS disable keep)

# ---------------------------------------------------------------------------
# cpp_lkm_runtime — reusable STATIC library compiled into every .ko: operator_new.cpp : traps heap
# new; implements operator delete -> kfree
# ---------------------------------------------------------------------------
add_library(cpp_lkm_runtime STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/operator_new.cpp")
target_include_directories(
    cpp_lkm_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" # cpp_lkm/common, cpp_lkm/runtime
                           "${CMAKE_CURRENT_SOURCE_DIR}/.." # compat/
)
# Headers under cpp-lkm/include/ infer flags from this TU in clangd; need C++23 for std::expected.
target_compile_features(cpp_lkm_runtime PUBLIC cxx_std_23)
# Runtime itself does not carry ABI/kernel flags — those come from the interface target that the
# consumer attaches.

# ---------------------------------------------------------------------------
# Load the public API functions.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppLkmTargets.cmake")
