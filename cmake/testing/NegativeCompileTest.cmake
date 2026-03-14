list(JOIN LIBKERNEL_CXX_FLAGS " " _neg_flags)
set(_neg_inc "")
foreach(_neg_d ${LIBKERNEL_INCLUDE_DIRS})
    set(_neg_inc "${_neg_inc} -I${_neg_d}")
endforeach()
set(_neg_cxx_flags "${CMAKE_CXX_FLAGS} ${_neg_flags} ${_neg_inc}")

macro(add_negative_compile_test TARGET_NAME SOURCE_FILE)
    add_custom_target(
        ${TARGET_NAME}
        COMMAND
            ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER} -DCXX_FLAGS="${_neg_cxx_flags}"
            -DSOURCE=${SOURCE_FILE} -P
            ${CMAKE_SOURCE_DIR}/cmake/testing/RunNegativeCompileTest.cmake
        DEPENDS ${SOURCE_FILE}
    )
    add_dependencies(kernel_module ${TARGET_NAME})
endmacro()

macro(add_negative_link_test TARGET_NAME SOURCE_FILE)
    add_custom_target(
        ${TARGET_NAME}
        COMMAND
            ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER} -DCXX_FLAGS="${_neg_cxx_flags}"
            -DLINK_FLAGS="-Wl,--no-undefined" -DSOURCE=${SOURCE_FILE}
            -DOBJ_TO_LINK="${CMAKE_BINARY_DIR}/CMakeFiles/kernel_module.dir/src/operator_new.cpp.o"
            -P ${CMAKE_SOURCE_DIR}/cmake/testing/RunNegativeLinkTest.cmake
        DEPENDS ${SOURCE_FILE} kernel_module
    )
endmacro()
