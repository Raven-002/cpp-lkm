# CppLkmKernelAbi.cmake
# Attaches x86_64 kernel ABI flags (-mcmodel=kernel, retpoline/thunk) to a target.
#
# Usage:
#   cpp_lkm_attach_kernel_abi(<target>)
#
# Only adds flags when the host processor is x86_64/AMD64.

function(cpp_lkm_attach_kernel_abi target)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        return()
    endif()

    set(_abi_flags -mcmodel=kernel)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(
            APPEND
            _abi_flags
            -mindirect-branch=thunk-extern
            -mindirect-branch-register
            -mfunction-return=thunk-extern
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND _abi_flags -mretpoline-external-thunk -mfunction-return=thunk-extern)
    endif()

    target_compile_options(${target} INTERFACE ${_abi_flags})
endfunction()
