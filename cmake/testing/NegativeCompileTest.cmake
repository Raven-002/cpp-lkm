# NegativeCompileTest.cmake Provides add_negative_compile_test and add_negative_link_test macros.
#
# Reads flags from the my_kernel_module.iface INTERFACE target at configure time so no global
# LIBKERNEL_* variables are needed.

# Collect flags from the interface target into string variables usable in add_custom_target
# commands. This runs at configure time; genexes that need build-time evaluation are handled via
# file(GENERATE) where needed.
function(_cpp_lkm_collect_neg_flags out_flags_var)
    get_target_property(_opts my_kernel_module.iface INTERFACE_COMPILE_OPTIONS)
    get_target_property(_incs my_kernel_module.iface INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(_defs my_kernel_module.iface INTERFACE_COMPILE_DEFINITIONS)

    set(_flags "${CMAKE_CXX_FLAGS}")

    if(_opts)
        list(JOIN _opts " " _opts_str)
        string(APPEND _flags " ${_opts_str}")
    endif()

    if(_defs)
        foreach(_d IN LISTS _defs)
            string(APPEND _flags " -D${_d}")
        endforeach()
    endif()

    if(_incs)
        foreach(_inc IN LISTS _incs)
            string(APPEND _flags " -I${_inc}")
        endforeach()
    endif()

    set(${out_flags_var}
        "${_flags}"
        PARENT_SCOPE
    )
endfunction()

macro(add_negative_compile_test TARGET_NAME SOURCE_FILE)
    _cpp_lkm_collect_neg_flags(_neg_cxx_flags)
    add_custom_target(
        ${TARGET_NAME}
        COMMAND
            ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER} "-DCXX_FLAGS=${_neg_cxx_flags}"
            -DSOURCE=${SOURCE_FILE} -P
            ${CMAKE_SOURCE_DIR}/cmake/testing/RunNegativeCompileTest.cmake
        DEPENDS ${SOURCE_FILE}
    )
    add_dependencies(my_kernel_module ${TARGET_NAME})
endmacro()

macro(add_negative_link_test TARGET_NAME SOURCE_FILE)
    _cpp_lkm_collect_neg_flags(_neg_cxx_flags)
    add_custom_target(
        ${TARGET_NAME}
        COMMAND
            ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER} "-DCXX_FLAGS=${_neg_cxx_flags}"
            "-DLINK_FLAGS=-Wl,--no-undefined" -DSOURCE=${SOURCE_FILE}
            "-DLIB_TO_LINK=$<TARGET_FILE:my_kernel_module>" -P
            ${CMAKE_SOURCE_DIR}/cmake/testing/RunNegativeLinkTest.cmake
        DEPENDS ${SOURCE_FILE} my_kernel_module
    )
endmacro()
