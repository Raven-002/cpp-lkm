# CppLkmTargets.cmake
# Public CMake API for the cpp-lkm framework. Included by cpp-lkm/CMakeLists.txt.
#
# Exports:
#   cpp_lkm_create_kernel_interface(<target> [MODULE_NAME <name>] [KDIR <path>] [ABI_MODE ko])
#   cpp_lkm_add_ko_target(TARGET <lib> MODULE_NAME <name> MODULE_OBJECT <Class>
#                         MODULE_HEADER <header> KERNEL_API_SRC_DIR <dir-with-*.c>
#                         [KERNEL_API_INCLUDE_DIR <dir>]  # default: <KERNEL_API_SRC_DIR>/../include
#                         [KERNEL_INTERFACE <iface>] [KDIR <path>]
#                         [OBJTOOL_MODE <disable|keep>] [ALL])

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
#   [ABI_MODE ko]        # pass "ko" to add ABI-strict flags for real .ko build
# )
#
# Creates an INTERFACE target with:
#   - freestanding C++23 compiler flags
#   - framework include directories (error.hpp Result alias, kalloc.hpp, cpp_lkm/runtime/kernel_api.h shim, ...)
#   - kernel header directories (if the kernel tree exists)
#   - kernel ABI compile options (when ABI_MODE is "ko")
# ---------------------------------------------------------------------------
function(cpp_lkm_create_kernel_interface target)
    set(oneValueArgs MODULE_NAME KDIR ABI_MODE)
    cmake_parse_arguments(_CLKI "" "${oneValueArgs}" "" ${ARGN})

    if(NOT _CLKI_MODULE_NAME)
        set(_CLKI_MODULE_NAME "${target}")
    endif()

    add_library(${target} INTERFACE)

    # C++ compiler flags
    cpp_lkm_get_cxx_flags(_cxx_flags)
    target_compile_options(${target} INTERFACE ${_cxx_flags})

    # Framework headers (repo root for compat/, framework include/)
    target_include_directories(
        ${target}
        INTERFACE "${CPP_LKM_ROOT}"        # lets #include "compat/new_shim.hpp" resolve
                  "${CPP_LKM_DIR}/include" # cpp_lkm/common, cpp_lkm/runtime
    )

    # Kernel definitions common to all kernel module targets
    target_compile_definitions(${target} INTERFACE __KERNEL__ MODULE)

    # Kernel header paths + KBUILD_MODNAME
    cpp_lkm_resolve_kdir(_kdir "${_CLKI_KDIR}")
    cpp_lkm_attach_kernel_headers(
        ${target}
        KDIR "${_kdir}"
        MODULE_NAME "${_CLKI_MODULE_NAME}"
    )

    # ABI flags only for real .ko builds
    if(_CLKI_ABI_MODE STREQUAL "ko")
        cpp_lkm_attach_kernel_abi(${target})
    endif()
endfunction()

# ---------------------------------------------------------------------------
# cpp_lkm_add_ko_target(
#   TARGET          <static-lib-target>   # consumer library
#   MODULE_NAME     <name>                # .ko name, also Kbuild module name
#   MODULE_OBJECT   <ClassName>           # concrete class implementing IKernelModule
#   MODULE_HEADER   <include/path.hpp>    # header declaring MODULE_OBJECT (relative to any
#                                         # include dir reachable from MODULE_LIB's iface)
#   [KERNEL_INTERFACE <iface-target>]     # the target from create_kernel_interface;
#                                         # defaults to <MODULE_NAME>.iface
#   [KDIR <path>]                         # override kernel build directory
#   [OBJTOOL_MODE <disable|keep>]         # override CPP_LKM_OBJTOOL_MODE for this module
#   [ALL]                                 # add to default build target
# )
#
# What it does:
#   1. Links KERNEL_INTERFACE into TARGET (if not already).
#   2. Generates module_bridge.cpp that placement-news MODULE_OBJECT.
#   3. Creates <MODULE_NAME>.bridge OBJECT target for the generated bridge.
#   4. Stages archives + generated linux_entry.c + Kbuild Makefile.
#   5. Creates <MODULE_NAME>_ko custom target that invokes Kbuild.
# ---------------------------------------------------------------------------
function(cpp_lkm_add_ko_target)
    set(options ALL)
    set(oneValueArgs TARGET MODULE_NAME MODULE_OBJECT MODULE_HEADER KERNEL_API_SRC_DIR KERNEL_API_INCLUDE_DIR
                     KERNEL_INTERFACE KDIR OBJTOOL_MODE
    )
    set(multiValueArgs MODULE_INCLUDE_DIRS)
    cmake_parse_arguments(_CLAT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT _CLAT_KERNEL_API_SRC_DIR)
        message(
            FATAL_ERROR
                "cpp_lkm_add_ko_target(): KERNEL_API_SRC_DIR is required (directory of *.c sources "
                "for cpp_* symbols; compiled only by Kbuild, not the host C compiler)."
        )
    endif()

    if(NOT _CLAT_KERNEL_INTERFACE)
        set(_CLAT_KERNEL_INTERFACE "${_CLAT_MODULE_NAME}.iface")
    endif()

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

    # Resolve objtool mode (global default can be overridden per target).
    if(_CLAT_OBJTOOL_MODE)
        set(_objtool_mode "${_CLAT_OBJTOOL_MODE}")
    else()
        set(_objtool_mode "${CPP_LKM_OBJTOOL_MODE}")
    endif()
    string(TOLOWER "${_objtool_mode}" _objtool_mode)
    if(NOT _objtool_mode STREQUAL "disable" AND NOT _objtool_mode STREQUAL "keep")
        message(
            FATAL_ERROR
                "Invalid OBJTOOL_MODE '${_objtool_mode}'. Expected one of: disable, keep."
        )
    endif()

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
        KERNEL_API_SRC_DIR
        "${_CLAT_KERNEL_API_SRC_DIR}"
        BRIDGE_TARGET
        "${_bridge_target}"
        RUNTIME_LIB
        cpp_lkm_runtime
        KDIR
        "${_kdir}"
        OBJTOOL_MODE
        "${_objtool_mode}"
        ${_all_opt}
    )
    if(_CLAT_KERNEL_API_INCLUDE_DIR)
        list(
            APPEND
            _kbuild_stage_args
            KERNEL_API_INCLUDE_DIR
            "${_CLAT_KERNEL_API_INCLUDE_DIR}"
        )
    endif()
    cpp_lkm_add_kbuild_stage(${_kbuild_stage_args})
endfunction()
