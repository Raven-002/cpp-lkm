# Shared C++ flags and include dirs for kernel C++ code. Consumed by libkernel and negative tests.
set(LIBKERNEL_CXX_FLAGS
    -std=c++23
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -fno-stack-protector
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -Wall
    -Wextra
    -Werror
    -Werror=unused-result
    -Werror=return-type
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13"
)
    list(APPEND LIBKERNEL_CXX_FLAGS -Wno-interference-size)
endif()

set(LIBKERNEL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}
                           ${CMAKE_SOURCE_DIR}/mock_kernel ${CMAKE_SOURCE_DIR}/third_party
)
