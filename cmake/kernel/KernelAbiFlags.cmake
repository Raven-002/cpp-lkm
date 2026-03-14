set(KERNEL_ABI_COMPILE_OPTIONS)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND (BUILD_KO OR NOT BUILD_MODE STREQUAL "host"))
    list(APPEND KERNEL_ABI_COMPILE_OPTIONS -mcmodel=kernel)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND KERNEL_ABI_COMPILE_OPTIONS
            -mindirect-branch=thunk-extern
            -mindirect-branch-register
            -mfunction-return=thunk-extern
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND KERNEL_ABI_COMPILE_OPTIONS
            -mretpoline-external-thunk
            -mfunction-return=thunk-extern
        )
    endif()
endif()
