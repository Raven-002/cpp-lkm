# CppLkmKbuild.cmake
# Stages a .ko build via out-of-tree Kbuild.
#
# Internal helper used by cpp_lkm_add_ko_target().
#
# Usage:
#   cpp_lkm_add_kbuild_stage(
#     MODULE_NAME       <name>
#     MODULE_LIB        <static-lib-target>   # consumer's STATIC library
#     SOURCE_TARGETS    <target>...           # targets that own staged .c sources
#     [INCLUDE_DIRS     <path>...]            # extra include dirs for staged C compile
#     BRIDGE_TARGET     <bridge-object-target>
#     RUNTIME_LIB       <runtime-static-target>
#     KDIR              <kernel-build-dir>
#     [ALL]
#   )
#
# Produces: <MODULE_NAME>_ko custom target + <name>.ko in CMAKE_BINARY_DIR.

function(cpp_lkm_add_kbuild_stage)
    set(options ALL)
    set(oneValueArgs MODULE_NAME MODULE_LIB BRIDGE_TARGET RUNTIME_LIB KDIR)
    set(multiValueArgs SOURCE_TARGETS INCLUDE_DIRS)
    cmake_parse_arguments(_KB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    cpp_lkm_assert_nonempty_vars(
        "cpp_lkm_add_kbuild_stage()"
        "_KB"
        MODULE_NAME
        MODULE_LIB
        BRIDGE_TARGET
        RUNTIME_LIB
        KDIR
    )
    if(NOT _KB_SOURCE_TARGETS)
        message(FATAL_ERROR "cpp_lkm_add_kbuild_stage(): SOURCE_TARGETS is required.")
    endif()

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

    set(_kbuild_c_sources "")
    set(_include_roots "${_KB_INCLUDE_DIRS}")

    foreach(_src_target IN LISTS _KB_SOURCE_TARGETS)
        if(NOT TARGET "${_src_target}")
            message(
                FATAL_ERROR
                    "cpp_lkm_add_kbuild_stage(): SOURCE_TARGETS item '${_src_target}' is not a target."
            )
        endif()

        get_target_property(_target_sources "${_src_target}" SOURCES)
        if(NOT _target_sources OR _target_sources STREQUAL "_target_sources-NOTFOUND")
            message(
                FATAL_ERROR
                    "cpp_lkm_add_kbuild_stage(): source target '${_src_target}' has no SOURCES property."
            )
        endif()

        get_target_property(_target_source_dir "${_src_target}" SOURCE_DIR)
        foreach(_src IN LISTS _target_sources)
            if(IS_ABSOLUTE "${_src}")
                set(_abs_src "${_src}")
            else()
                get_filename_component(_abs_src "${_src}" ABSOLUTE BASE_DIR "${_target_source_dir}")
            endif()
            get_filename_component(_src_ext "${_abs_src}" EXT)
            if(_src_ext STREQUAL ".c")
                list(APPEND _kbuild_c_sources "${_abs_src}")
            endif()
        endforeach()

        foreach(_prop_name IN ITEMS INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
            get_target_property(_prop_dirs "${_src_target}" ${_prop_name})
            if(_prop_dirs AND NOT _prop_dirs STREQUAL "_prop_dirs-NOTFOUND")
                foreach(_inc_dir IN LISTS _prop_dirs)
                    if(_inc_dir MATCHES "^\\$<")
                        continue()
                    endif()
                    if(IS_ABSOLUTE "${_inc_dir}")
                        list(APPEND _include_roots "${_inc_dir}")
                    else()
                        get_filename_component(_inc_abs "${_inc_dir}" ABSOLUTE BASE_DIR "${_target_source_dir}")
                        list(APPEND _include_roots "${_inc_abs}")
                    endif()
                endforeach()
            endif()
        endforeach()
    endforeach()

    if(NOT _kbuild_c_sources)
        message(FATAL_ERROR "cpp_lkm_add_kbuild_stage(): SOURCE_TARGETS contain no .c files.")
    endif()
    list(REMOVE_DUPLICATES _kbuild_c_sources)
    list(SORT _kbuild_c_sources)

    set(_kbuild_include_dirs "${CPP_LKM_DIR}/include")
    foreach(_inc_dir IN LISTS _include_roots)
        if(_inc_dir MATCHES "^\\$<")
            continue()
        endif()
        if(IS_ABSOLUTE "${_inc_dir}")
            set(_inc_abs "${_inc_dir}")
        else()
            get_filename_component(_inc_abs "${_inc_dir}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif()
        if(EXISTS "${_inc_abs}")
            list(APPEND _kbuild_include_dirs "${_inc_abs}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _kbuild_include_dirs)

    set(_kbuild_obj_sp "")
    set(_kbuild_src_names "")
    foreach(_src IN LISTS _kbuild_c_sources)
        get_filename_component(_name "${_src}" NAME)
        if(_name IN_LIST _kbuild_src_names)
            message(
                FATAL_ERROR
                    "cpp_lkm_add_kbuild_stage(): duplicate source basename '${_name}' in SOURCE_TARGETS. "
                    "Kbuild staging copies sources into one directory, so basenames must be unique."
            )
        endif()
        list(APPEND _kbuild_src_names "${_name}")
        get_filename_component(_stem ${_src} NAME_WE)
        if(_kbuild_obj_sp STREQUAL "")
            set(_kbuild_obj_sp "${_stem}.o")
        else()
            set(_kbuild_obj_sp "${_kbuild_obj_sp} ${_stem}.o")
        endif()
    endforeach()

    set(_kbuild_header_deps "")
    foreach(_inc_dir IN LISTS _kbuild_include_dirs)
        file(GLOB_RECURSE _inc_headers CONFIGURE_DEPENDS "${_inc_dir}/*.h")
        if(_inc_headers)
            list(APPEND _kbuild_header_deps ${_inc_headers})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _kbuild_header_deps)

    # The Kbuild Makefile links:
    #   <mod>_linux_entry.o — C file compiled by Kbuild (module metadata + init/exit)
    #   *.o                 — staged C sources from SOURCE_TARGETS, compiled only by Kbuild
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

    set(_makefile_content "")
    foreach(_inc_dir IN LISTS _kbuild_include_dirs)
        string(APPEND _makefile_content "ccflags-y += -I${_inc_dir}\n")
    endforeach()
    string(
        APPEND
        _makefile_content
        "obj-m += ${_mod}.o\n"
        "${_mod}-y := ${_mod}_linux_entry.o ${_kbuild_obj_sp} ${_bridge_archive} ${_module_archive} ${_runtime_archive}\n"
    )
    file(GENERATE OUTPUT "${_stage_dir}/Makefile" CONTENT "${_makefile_content}")

    if(_KB_ALL)
        set(_all_arg ALL)
    else()
        set(_all_arg "")
    endif()

    set(_copy_kbuild_sources "")
    foreach(_src IN LISTS _kbuild_c_sources)
        get_filename_component(_name ${_src} NAME)
        list(
            APPEND
            _copy_kbuild_sources
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
        ${_copy_kbuild_sources}
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
        DEPENDS
            ${_KB_MODULE_LIB}
            ${_KB_BRIDGE_TARGET}
            ${_KB_RUNTIME_LIB}
            ${_kbuild_c_sources}
            ${_kbuild_header_deps}
        WORKING_DIRECTORY "${_stage_dir}"
        COMMENT "Building ${_mod}.ko via Kbuild"
        VERBATIM
    )
endfunction()
