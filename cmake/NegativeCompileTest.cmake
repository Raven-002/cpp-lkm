macro(add_negative_compile_test TARGET_NAME SOURCE_FILE)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=${CMAKE_CXX_COMPILER}
        -DCXX_FLAGS="${CMAKE_CXX_FLAGS} -std=c++23 -ffreestanding -fno-exceptions -fno-rtti -Werror=unused-result -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/compat -I${CMAKE_SOURCE_DIR}/mock_kernel -I${CMAKE_SOURCE_DIR}/third_party"
        -DSOURCE=${SOURCE_FILE}
        -P ${CMAKE_SOURCE_DIR}/cmake/RunNegativeCompileTest.cmake
        DEPENDS ${SOURCE_FILE}
    )
    add_dependencies(kernel_module ${TARGET_NAME})
endmacro()

macro(add_negative_link_test TARGET_NAME SOURCE_FILE)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=${CMAKE_CXX_COMPILER}
        -DCXX_FLAGS="${CMAKE_CXX_FLAGS} -std=c++23 -ffreestanding -fno-exceptions -fno-rtti"
        -DLINK_FLAGS="-Wl,--no-undefined"
        -DSOURCE=${SOURCE_FILE}
        -DOBJ_TO_LINK="${CMAKE_BINARY_DIR}/CMakeFiles/kernel_module.dir/src/operator_new.cpp.o"
        -P ${CMAKE_SOURCE_DIR}/cmake/RunNegativeLinkTest.cmake
        DEPENDS ${SOURCE_FILE} kernel_module
    )
endmacro()
