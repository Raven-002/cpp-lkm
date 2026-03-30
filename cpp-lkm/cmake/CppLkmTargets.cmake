# CppLkmTargets.cmake
# Public CMake API for the cpp-lkm framework. Included by cpp-lkm/CMakeLists.txt.
#
# Exports:
#   cpp_lkm_create_kernel_interface(<target> [MODULE_NAME <name>] [KDIR <path>]
#                                   MODULE_API_HEADER <header> [ABI_MODE ko])
#   cpp_lkm_add_ko_target(TARGET <lib> MODULE_NAME <name> MODULE_OBJECT <Class>
#                         MODULE_HEADER <header> KBUILD_SOURCE_TARGETS <tgt>...
#                         [KBUILD_INCLUDE_DIRS <dir>...]
#                         [KERNEL_INTERFACE <iface>] [KDIR <path>] [ALL])

include("${CPP_LKM_DIR}/../cmake/Utils.cmake")
include(${CPP_LKM_DIR}/cmake/CppLkmFlags.cmake)
include(${CPP_LKM_DIR}/cmake/CppLkmKernelDir.cmake)
include(${CPP_LKM_DIR}/cmake/CppLkmKernelHeaders.cmake)
include(${CPP_LKM_DIR}/cmake/CppLkmKernelAbi.cmake)
include(${CPP_LKM_DIR}/cmake/CppLkmBridgeGen.cmake)
include(${CPP_LKM_DIR}/cmake/CppLkmKbuild.cmake)

# ---------------------------------------------------------------------------
# cpp_lkm_create_kernel_interface(<target>
#   [MODULE_NAME <name>]
#   [KDIR <kernel-build-dir>]
#   MODULE_API_HEADER <header-path>  # e.g. "project/module_api.h"
#   [ABI_MODE ko]        # pass "ko" to add ABI-strict flags for real .ko build
# )
#
# Creates an INTERFACE target with:
#   - freestanding C++23 compiler flags
#   - framework include directory (cpp_lkm/...) and cpp_lkm_compat (compat/ shims via INTERFACE link)
#   - kernel header directories (if the kernel tree exists)
#   - kernel ABI compile options (when ABI_MODE is "ko")
# ---------------------------------------------------------------------------
function(cpp_lkm_create_kernel_interface iface_target)
    cpp_lkm_assert_nonempty("cpp_lkm_create_kernel_interface()" "<iface_target>" "${iface_target}")

    set(oneValueArgs MODULE_NAME KDIR MODULE_API_HEADER ABI_MODE)
    cmake_parse_arguments(_CLKI "" "${oneValueArgs}" "" ${ARGN})

    if(NOT _CLKI_MODULE_NAME)
        set(_CLKI_MODULE_NAME "${iface_target}")
    endif()
    if(NOT _CLKI_MODULE_API_HEADER)
        message(FATAL_ERROR "cpp_lkm_create_kernel_interface(): MODULE_API_HEADER is required.")
    endif()

    add_library(${iface_target} INTERFACE)

    # C++ compiler flags
    cpp_lkm_get_cxx_flags(_cxx_flags)
    target_compile_options(${iface_target} INTERFACE ${_cxx_flags})

    # Framework headers + compat shims (cpp_lkm_compat exposes compat/ includes)
    target_link_libraries(${iface_target} INTERFACE cpp_lkm_compat)
    target_include_directories(${iface_target} INTERFACE "${CPP_LKM_DIR}/include")

    # Kernel definitions common to all kernel module targets
    target_compile_definitions(${iface_target} INTERFACE __KERNEL__ MODULE)
    target_compile_definitions(${iface_target}
                               INTERFACE CPP_LKM_MODULE_API_HEADER="<${_CLKI_MODULE_API_HEADER}>"
    )

    # Kernel header paths + KBUILD_MODNAME
    cpp_lkm_resolve_kdir(_kdir "${_CLKI_KDIR}")
    cpp_lkm_attach_kernel_headers(
        ${iface_target}
        KDIR "${_kdir}"
        MODULE_NAME "${_CLKI_MODULE_NAME}"
    )

    # ABI flags only for real .ko builds
    if(_CLKI_ABI_MODE STREQUAL "ko")
        cpp_lkm_attach_kernel_abi(${iface_target})
    endif()
endfunction()

# ---------------------------------------------------------------------------
# cpp_lkm_add_ko_target(
#   TARGET          <static-lib-target>   # consumer library
#   MODULE_NAME     <name>                # .ko name, also Kbuild module name
#   MODULE_OBJECT   <ClassName>           # concrete class implementing IKernelModule
#   MODULE_HEADER   <include/path.hpp>    # header declaring MODULE_OBJECT (relative to any
#                                         # include dir reachable from MODULE_LIB's iface)
#   KBUILD_SOURCE_TARGETS <target>...     # optional project-owned staged .c source targets
#   [KBUILD_INCLUDE_DIRS <dir>...]        # extra include dirs for staged C compile
#   [KERNEL_INTERFACE <iface-target>]     # the target from create_kernel_interface;
#                                         # defaults to <MODULE_NAME>.iface
#   [KDIR <path>]                         # override kernel build directory
#   [ALL]                                 # add to default build target
# )
#
# What it does:
#   1. Links KERNEL_INTERFACE into TARGET (if not already).
#   2. Generates module_bridge.cpp that placement-news MODULE_OBJECT.
#   3. Creates <MODULE_NAME>.bridge OBJECT target for the generated bridge.
#   4. Stages archives + generated linux_entry.c + Kbuild Makefile.
#      Also stages cpp-lkm runtime C bridge sources (memory/context) automatically.
#   5. Creates <MODULE_NAME>_ko custom target that invokes Kbuild.
# ---------------------------------------------------------------------------
function(cpp_lkm_add_ko_target)
    set(options ALL)
    set(oneValueArgs TARGET MODULE_NAME MODULE_OBJECT MODULE_HEADER KERNEL_INTERFACE KDIR)
    set(multiValueArgs MODULE_INCLUDE_DIRS KBUILD_SOURCE_TARGETS KBUILD_INCLUDE_DIRS)
    cmake_parse_arguments(_CLAT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    cpp_lkm_assert_nonempty_vars(
        "cpp_lkm_add_ko_target()"
        "_CLAT"
        TARGET
        MODULE_NAME
        MODULE_OBJECT
        MODULE_HEADER
    )

    if(NOT _CLAT_KERNEL_INTERFACE)
        set(_CLAT_KERNEL_INTERFACE "${_CLAT_MODULE_NAME}.iface")
    endif()

    set(_clat_kbuild_sources ${_CLAT_KBUILD_SOURCE_TARGETS} cpp_lkm_runtime_kernel_api_sources)

    # Ensure runtime objects are built with kernel/freestanding flags in ko mode.
    target_link_libraries(cpp_lkm_runtime PUBLIC ${_CLAT_KERNEL_INTERFACE})

    # Generate and build the bridge
    cpp_lkm_generate_module_bridge(
        MODULE_NAME "${_CLAT_MODULE_NAME}"
        MODULE_OBJECT "${_CLAT_MODULE_OBJECT}"
        MODULE_HEADER "${_CLAT_MODULE_HEADER}"
        KERNEL_INTERFACE "${_CLAT_KERNEL_INTERFACE}"
        MODULE_INCLUDE_DIRS ${_CLAT_MODULE_INCLUDE_DIRS}
        OUT_OBJECT_TARGET _bridge_target
    )

    # Resolve Kbuild dir
    cpp_lkm_resolve_kdir(_kdir "${_CLAT_KDIR}")

    # Stage + build .ko
    if(_CLAT_ALL)
        set(_all_opt ALL)
    else()
        set(_all_opt "")
    endif()

    set(_kbuild_stage_args
        MODULE_NAME
        "${_CLAT_MODULE_NAME}"
        MODULE_LIB
        "${_CLAT_TARGET}"
        SOURCE_TARGETS
        ${_clat_kbuild_sources}
        BRIDGE_TARGET
        "${_bridge_target}"
        RUNTIME_LIB
        cpp_lkm_runtime
        KDIR
        "${_kdir}"
        ${_all_opt}
    )
    if(_CLAT_KBUILD_INCLUDE_DIRS)
        list(APPEND _kbuild_stage_args INCLUDE_DIRS ${_CLAT_KBUILD_INCLUDE_DIRS})
    endif()
    cpp_lkm_add_kbuild_stage(${_kbuild_stage_args})
endfunction()
