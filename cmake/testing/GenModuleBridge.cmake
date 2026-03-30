# GenModuleBridge.cmake Generates and compiles the module bridge
# (cpp_module_init / cpp_module_exit) for use in host-mode tests and as part of
# the consumer STATIC library.
#
# In BUILD_KO=ON mode the bridge is also compiled by cpp_lkm_add_ko_target as an
# OBJECT target; in host mode we compile it here and link it into the STATIC lib
# so tests can call cpp_module_init/exit directly.
#
# Usage: cpp_lkm_gen_host_bridge( MODULE_NAME      <name> MODULE_OBJECT
# <ClassName> MODULE_HEADER <include/path.hpp> KERNEL_INTERFACE <iface-target>
# MODULE_INCLUDE_DIRS <dir1> [<dir2> ...]   # include dirs for MODULE_HEADER
# lookup )

function(cpp_lkm_gen_host_bridge)
    set(oneValueArgs MODULE_NAME MODULE_OBJECT MODULE_HEADER KERNEL_INTERFACE TARGET)
    set(multiValueArgs MODULE_INCLUDE_DIRS)
    cmake_parse_arguments(_HB "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    cpp_lkm_assert_nonempty_vars("cpp_lkm_gen_host_bridge()" "_HB" MODULE_NAME MODULE_OBJECT
                                 MODULE_HEADER KERNEL_INTERFACE)

    if(NOT _HB_TARGET)
        set(_HB_TARGET "${_HB_MODULE_NAME}")
    endif()

    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/cpp_lkm_bridge_${_HB_MODULE_NAME}")
    file(MAKE_DIRECTORY "${_gen_dir}")

    set(CPP_LKM_MODULE_NAME "${_HB_MODULE_NAME}")
    set(CPP_LKM_MODULE_OBJECT "${_HB_MODULE_OBJECT}")
    set(CPP_LKM_MODULE_HEADER "${_HB_MODULE_HEADER}")

    set(_bridge_src "${_gen_dir}/module_bridge.cpp")
    configure_file("${CPP_LKM_DIR}/cmake/templates/module_bridge.cpp.in" "${_bridge_src}" @ONLY)

    set(_bridge_obj_target "${_HB_MODULE_NAME}.bridge")
    add_library(${_bridge_obj_target} OBJECT "${_bridge_src}")

    # The bridge needs the kernel interface (flags, kernel includes, etc.)
    target_link_libraries(${_bridge_obj_target} PUBLIC ${_HB_KERNEL_INTERFACE})
    # Provide include dirs for the MODULE_HEADER without depending on the lib target itself.
    if(_HB_MODULE_INCLUDE_DIRS)
        target_include_directories(${_bridge_obj_target} PRIVATE ${_HB_MODULE_INCLUDE_DIRS})
    endif()
    # Runtime for operator delete etc.
    target_link_libraries(${_bridge_obj_target} PUBLIC cpp_lkm_runtime)

    # Fold the bridge objects into the consumer STATIC library so that anything linking it gets
    # cpp_module_init/exit.
    target_sources(${_HB_TARGET} PRIVATE $<TARGET_OBJECTS:${_bridge_obj_target}>)
endfunction()
