# CppLkmKbuild.cmake
# Stages a .ko build via out-of-tree Kbuild.
#
# Internal helper used by cpp_lkm_add_ko_target().
#
# Usage:
#   cpp_lkm_add_kbuild_stage(
#     MODULE_NAME   <name>
#     MODULE_LIB    <static-lib-target>   # consumer's STATIC library
#     BRIDGE_TARGET <bridge-object-target>
#     RUNTIME_LIB   <runtime-static-target>
#     KDIR          <kernel-build-dir>
#     [ALL]
#   )
#
# Produces: <MODULE_NAME>_ko custom target + <name>.ko in CMAKE_BINARY_DIR.

function(cpp_lkm_add_kbuild_stage)
    set(options ALL)
    set(oneValueArgs MODULE_NAME MODULE_LIB BRIDGE_TARGET RUNTIME_LIB KDIR)
    cmake_parse_arguments(_KB "${options}" "${oneValueArgs}" "" ${ARGN})

    set(_kdir "${_KB_KDIR}")
    if(NOT EXISTS "${_kdir}/Makefile")
        message(
            FATAL_ERROR
                "Kernel build directory not found at '${_kdir}'.\n"
                "Set KDIR in cpp_lkm_add_ko_target() or install kernel headers.\n"
                "  Fedora/RHEL: sudo dnf install kernel-devel\n"
                "  Debian/Ubuntu: sudo apt install linux-headers-$(uname -r)"
        )
    endif()

    message(STATUS "cpp-lkm: Kbuild directory: ${_kdir}")

    set(_mod "${_KB_MODULE_NAME}")
    set(_stage_dir "${CMAKE_CURRENT_BINARY_DIR}/kbuild_${_mod}")
    file(MAKE_DIRECTORY "${_stage_dir}")

    # The Kbuild Makefile links:
    #   <mod>_linux_entry.o — C file compiled by Kbuild (no C++ ABI issues)
    #   lib<mod>_bridge.a   — bridge OBJECT library archived for Kbuild
    #   lib<mod>.a          — consumer STATIC library
    #   lib<mod>_runtime.a  — framework runtime (operator_delete, etc.)
    set(_entry_src_in "${CPP_LKM_DIR}/cmake/templates/linux_entry.c.in")
    set(_entry_src "${_stage_dir}/${_mod}_linux_entry.c")
    set(CPP_LKM_MODULE_NAME "${_mod}")
    configure_file("${_entry_src_in}" "${_entry_src}" @ONLY)

    set(_bridge_archive "lib${_mod}_bridge.a")
    set(_module_archive "lib${_mod}.a")
    set(_runtime_archive "lib${_mod}_runtime.a")

    string(
        CONCAT
        _makefile_content
        "obj-m += ${_mod}.o\n"
        "${_mod}-y := ${_mod}_linux_entry.o ${_bridge_archive} ${_module_archive} ${_runtime_archive}\n"
    )
    file(GENERATE OUTPUT "${_stage_dir}/Makefile" CONTENT "${_makefile_content}")

    if(_KB_ALL)
        set(_all_arg ALL)
    else()
        set(_all_arg "")
    endif()

    add_custom_target(
        ${_mod}_ko
        ${_all_arg}
        # Stage the consumer static lib
        COMMAND
            ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_KB_MODULE_LIB}>"
            "${_stage_dir}/${_module_archive}"
        # Archive the bridge objects into a static lib for Kbuild
        COMMAND
            ${CMAKE_AR} rcs "${_stage_dir}/${_bridge_archive}"
            "$<TARGET_OBJECTS:${_KB_BRIDGE_TARGET}>"
        # Stage the runtime static lib
        COMMAND
            ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_KB_RUNTIME_LIB}>"
            "${_stage_dir}/${_runtime_archive}"
        # Invoke Kbuild
        COMMAND make -C "${_kdir}" M="${_stage_dir}" modules
        # Copy final .ko up to the CMake binary dir
        COMMAND
            ${CMAKE_COMMAND} -E copy "${_stage_dir}/${_mod}.ko"
            "${CMAKE_BINARY_DIR}/${_mod}.ko"
        DEPENDS ${_KB_MODULE_LIB} ${_KB_BRIDGE_TARGET} ${_KB_RUNTIME_LIB}
        WORKING_DIRECTORY "${_stage_dir}"
        COMMENT "Building ${_mod}.ko via Kbuild"
        VERBATIM
    )
endfunction()
