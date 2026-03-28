# CppLkmKernelDir.cmake
# Resolves the kernel build directory.
#
# Usage:
#   cpp_lkm_resolve_kdir(<out-var> [<explicit-kdir>])
#
# Resolution order:
#   1. Explicit argument (non-empty string passed by caller).
#   2. Cache variable CPP_LKM_KDIR set by the consumer project.
#   3. System default: /lib/modules/<uname -r>/build.

function(cpp_lkm_resolve_kdir out_var)
    set(_explicit "${ARGV1}")

    if(_explicit AND NOT _explicit STREQUAL "")
        set(${out_var}
            "${_explicit}"
            PARENT_SCOPE
        )
        return()
    endif()

    if(DEFINED CPP_LKM_KDIR AND NOT "${CPP_LKM_KDIR}" STREQUAL "")
        set(${out_var}
            "${CPP_LKM_KDIR}"
            PARENT_SCOPE
        )
        return()
    endif()

    # Legacy: honour KBUILD_DIR if the consumer set it the old way.
    if(DEFINED KBUILD_DIR AND NOT "${KBUILD_DIR}" STREQUAL "")
        set(${out_var}
            "${KBUILD_DIR}"
            PARENT_SCOPE
        )
        return()
    endif()

    execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE _release
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${out_var}
        "/lib/modules/${_release}/build"
        PARENT_SCOPE
    )
endfunction()
