# CppLkmKbuild.cmake
# Stages a .ko build via out-of-tree Kbuild.
#
# Internal helper used by cpp_lkm_add_ko_target().
#
# Usage:
#   cpp_lkm_add_kbuild_stage(
#     MODULE_NAME       <name>
#     MODULE_LIB        <static-lib-target>   # consumer's STATIC library
#     KERNEL_API_SRC_DIR <path>               # dir of consumer *.c (Kbuild-compiled only)
#     [KERNEL_API_INCLUDE_DIR <path>]         # dir for #include "kernel_api/..." (default: <parent>/include)
#     BRIDGE_TARGET     <bridge-object-target>
#     RUNTIME_LIB       <runtime-static-target>
#     KDIR              <kernel-build-dir>
#     [OBJTOOL_MODE <disable|keep>]
#     [ALL]
#   )
#
# Produces: <MODULE_NAME>_ko custom target + <name>.ko in CMAKE_BINARY_DIR.

function(cpp_lkm_add_kbuild_stage)
    set(options ALL)
    set(oneValueArgs MODULE_NAME MODULE_LIB KERNEL_API_SRC_DIR KERNEL_API_INCLUDE_DIR BRIDGE_TARGET RUNTIME_LIB KDIR
                     OBJTOOL_MODE
    )
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

    if(NOT _KB_KERNEL_API_SRC_DIR)
        message(
            FATAL_ERROR
                "cpp_lkm_add_kbuild_stage(): KERNEL_API_SRC_DIR is required (directory of *.c sources "
                "compiled only by Kbuild, e.g. cpp_* bridge)."
        )
    endif()

    file(GLOB _ka_c CONFIGURE_DEPENDS "${_KB_KERNEL_API_SRC_DIR}/*.c")
    list(SORT _ka_c)
    if(NOT _ka_c)
        message(
            FATAL_ERROR
                "KERNEL_API_SRC_DIR '${_KB_KERNEL_API_SRC_DIR}' contains no .c files."
        )
    endif()

    if(_KB_KERNEL_API_INCLUDE_DIR)
        set(_ka_include_dir "${_KB_KERNEL_API_INCLUDE_DIR}")
    else()
        get_filename_component(_ka_parent "${_KB_KERNEL_API_SRC_DIR}" DIRECTORY)
        set(_ka_include_dir "${_ka_parent}/include")
        if(NOT EXISTS "${_ka_include_dir}")
            message(
                FATAL_ERROR
                    "cpp_lkm_add_kbuild_stage(): KERNEL_API_INCLUDE_DIR not set and '${_ka_include_dir}' "
                    "does not exist (expected sibling of KERNEL_API_SRC_DIR named 'include')."
            )
        endif()
    endif()
    if(NOT IS_ABSOLUTE "${_ka_include_dir}")
        get_filename_component(_ka_include_abs "${_ka_include_dir}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    else()
        set(_ka_include_abs "${_ka_include_dir}")
    endif()

    file(GLOB _ka_h CONFIGURE_DEPENDS "${_ka_include_abs}/kernel_api/*.h")

    set(_ka_obj_sp "")
    foreach(_src ${_ka_c})
        get_filename_component(_stem ${_src} NAME_WE)
        if(_ka_obj_sp STREQUAL "")
            set(_ka_obj_sp "${_stem}.o")
        else()
            set(_ka_obj_sp "${_ka_obj_sp} ${_stem}.o")
        endif()
    endforeach()

    # The Kbuild Makefile links:
    #   <mod>_linux_entry.o — C file compiled by Kbuild (module metadata + init/exit)
    #   *.o                 — consumer kernel_api/*.c (cpp_* bridge), compiled only by Kbuild
    #   lib<mod>_bridge.a   — module_bridge OBJECT library archived for Kbuild
    #   lib<mod>.a          — consumer STATIC library
    #   lib<mod>_runtime.a  — framework runtime (operator_delete, etc.)
    set(_entry_src_in "${CPP_LKM_DIR}/cmake/templates/linux_entry.c.in")
    set(_entry_src "${_stage_dir}/${_mod}_linux_entry.c")
    set(CPP_LKM_MODULE_NAME "${_mod}")
    configure_file("${_entry_src_in}" "${_entry_src}" @ONLY)

    set(_bridge_archive "lib${_mod}_bridge.a")
    set(_module_archive "lib${_mod}.a")
    set(_runtime_archive "lib${_mod}_runtime.a")

    set(_objtool_mode "${_KB_OBJTOOL_MODE}")
    if(NOT _objtool_mode)
        set(_objtool_mode "disable")
    endif()
    string(TOLOWER "${_objtool_mode}" _objtool_mode)
    if(_objtool_mode STREQUAL "disable")
        string(
            CONCAT
            _objtool_makefile_prefix
            "OBJECT_FILES_NON_STANDARD := y\n"
            "OBJECT_FILES_NON_STANDARD_${_mod}.o := y\n"
            "${_mod}.o: objtool-enabled :=\n"
        )
    elseif(_objtool_mode STREQUAL "keep")
        set(_objtool_makefile_prefix "")
    else()
        message(
            FATAL_ERROR
                "Invalid OBJTOOL_MODE '${_objtool_mode}'. Expected one of: disable, keep."
        )
    endif()

    string(
        CONCAT
        _makefile_content
        "${_objtool_makefile_prefix}"
        # cpp_lkm shim (kernel_api.h) + consumer kernel_api/*.h prototypes for staged *.c
        "ccflags-y += -I${CPP_LKM_DIR}/include\n"
        "ccflags-y += -I${_ka_include_abs}\n"
        "obj-m += ${_mod}.o\n"
        "${_mod}-y := ${_mod}_linux_entry.o ${_ka_obj_sp} ${_bridge_archive} ${_module_archive} ${_runtime_archive}\n"
    )
    file(GENERATE OUTPUT "${_stage_dir}/Makefile" CONTENT "${_makefile_content}")

    if(_KB_ALL)
        set(_all_arg ALL)
    else()
        set(_all_arg "")
    endif()

    set(_copy_kernel_api "")
    foreach(_src ${_ka_c})
        get_filename_component(_name ${_src} NAME)
        list(
            APPEND
            _copy_kernel_api
            COMMAND
            ${CMAKE_COMMAND}
            -E
            copy
            "${_src}"
            "${_stage_dir}/${_name}"
        )
    endforeach()

    add_custom_target(
        ${_mod}_ko
        ${_all_arg}
        ${_copy_kernel_api}
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
        # Avoid M="path" with VERBATIM: sh leaves quotes in M and Kbuild fails.
        COMMAND make -C "${_kdir}" M=${_stage_dir} modules
        # Copy final .ko up to the CMake binary dir
        COMMAND
            ${CMAKE_COMMAND} -E copy "${_stage_dir}/${_mod}.ko"
            "${CMAKE_BINARY_DIR}/${_mod}.ko"
        DEPENDS ${_KB_MODULE_LIB} ${_KB_BRIDGE_TARGET} ${_KB_RUNTIME_LIB} ${_ka_c} ${_ka_h}
        WORKING_DIRECTORY "${_stage_dir}"
        COMMENT "Building ${_mod}.ko via Kbuild"
        VERBATIM
    )
endfunction()
