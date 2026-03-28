# CppLkmFlags.cmake
# Computes CPP_LKM_CXX_FLAGS — the freestanding C++23 kernel-compatible compiler flags.
# Call cpp_lkm_get_cxx_flags(<out-var>) to obtain the list.

function(cpp_lkm_get_cxx_flags out_var)
    set(_flags
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
        -U_GLIBCXX_ASSERTIONS
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
       AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12"
       AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13"
    )
        list(APPEND _flags -Wno-interference-size)
    endif()
    set(${out_var}
        "${_flags}"
        PARENT_SCOPE
    )
endfunction()
