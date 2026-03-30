# CppLkmFlags.cmake
# Computes CPP_LKM_CXX_FLAGS — the freestanding C++23 kernel-compatible compiler flags.
# Call cpp_lkm_get_cxx_flags(<out-var>) to obtain the list.

function(cpp_lkm_get_cxx_flags cxx_flags_out)
    cpp_lkm_assert_nonempty("cpp_lkm_get_cxx_flags()" "<cxx_flags_out>" "${cxx_flags_out}")

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
    set(${cxx_flags_out}
        "${_flags}"
        PARENT_SCOPE
    )
endfunction()

# Computes CPP_LKM_C_FLAGS — freestanding C kernel-compatible compiler flags.
# Call cpp_lkm_get_c_flags(<out-var>) to obtain the list.
function(cpp_lkm_get_c_flags c_flags_out)
    cpp_lkm_assert_nonempty("cpp_lkm_get_c_flags()" "<c_flags_out>" "${c_flags_out}")

    set(_flags
        -std=gnu11
        -ffreestanding
        -fno-stack-protector
        -Wall
        -Wextra
        -Wno-error
        -Wno-error=unused-parameter
    )
    set(${c_flags_out}
        "${_flags}"
        PARENT_SCOPE
    )
endfunction()
