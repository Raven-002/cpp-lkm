# CppLkmBridgeGen.cmake
# Generates module_bridge.cpp (the C++ entry point) for a given module target.
#
# Internal helper used by cpp_lkm_add_ko_target().
#
# Usage:
#   cpp_lkm_generate_module_bridge(
#     MODULE_NAME      <name>
#     MODULE_OBJECT    <ClassName>
#     MODULE_HEADER    <path/to/header.hpp>
#     KERNEL_INTERFACE <iface-target>
#     MODULE_INCLUDE_DIRS <dir1> [<dir2> ...]
#     OUT_OBJECT_TARGET <out-var>   # receives the name of the created OBJECT target
#   )

function(cpp_lkm_generate_module_bridge)
    set(oneValueArgs MODULE_NAME MODULE_OBJECT MODULE_HEADER KERNEL_INTERFACE OUT_OBJECT_TARGET)
    set(multiValueArgs MODULE_INCLUDE_DIRS)
    cmake_parse_arguments(_GEN "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/cpp_lkm_bridge_${_GEN_MODULE_NAME}")
    file(MAKE_DIRECTORY "${_gen_dir}")

    # Variables consumed by configure_file inside the template.
    set(CPP_LKM_MODULE_NAME "${_GEN_MODULE_NAME}")
    set(CPP_LKM_MODULE_OBJECT "${_GEN_MODULE_OBJECT}")
    set(CPP_LKM_MODULE_HEADER "${_GEN_MODULE_HEADER}")

    set(_bridge_src "${_gen_dir}/module_bridge.cpp")
    configure_file(
        "${CPP_LKM_DIR}/cmake/templates/module_bridge.cpp.in" "${_bridge_src}" @ONLY
    )

    set(_bridge_target "${_GEN_MODULE_NAME}.bridge")
    add_library(${_bridge_target} OBJECT "${_bridge_src}")
    target_link_libraries(${_bridge_target} PUBLIC ${_GEN_KERNEL_INTERFACE})
    if(_GEN_MODULE_INCLUDE_DIRS)
        target_include_directories(${_bridge_target} PRIVATE ${_GEN_MODULE_INCLUDE_DIRS})
    endif()

    set(${_GEN_OUT_OBJECT_TARGET}
        "${_bridge_target}"
        PARENT_SCOPE
    )
endfunction()
