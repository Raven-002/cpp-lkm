include(cmake/kernel/KernelBuildDir.cmake)

# Expose kernel header search paths/defines to CMake C++ targets so IDE diagnostics match kbuild's
# compile context (without requiring .ko builds).
add_library(cpp-lkm-kernel-headers INTERFACE)

set(CPP_LKM_KERNEL_INCLUDE_DIRS)
if(EXISTS "${CPP_LKM_KERNEL_BUILD_DIR}/Makefile")
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE CPP_LKM_KERNEL_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    list(
        APPEND
        CPP_LKM_KERNEL_INCLUDE_DIRS
        "${CPP_LKM_KERNEL_BUILD_DIR}/include"
        "${CPP_LKM_KERNEL_BUILD_DIR}/include/uapi"
        "${CPP_LKM_KERNEL_BUILD_DIR}/include/generated"
        "${CPP_LKM_KERNEL_BUILD_DIR}/include/generated/uapi"
        "${CPP_LKM_KERNEL_BUILD_DIR}/arch/${CPP_LKM_KERNEL_ARCH}/include"
        "${CPP_LKM_KERNEL_BUILD_DIR}/arch/${CPP_LKM_KERNEL_ARCH}/include/uapi"
        "${CPP_LKM_KERNEL_BUILD_DIR}/arch/${CPP_LKM_KERNEL_ARCH}/include/generated"
        "${CPP_LKM_KERNEL_BUILD_DIR}/arch/${CPP_LKM_KERNEL_ARCH}/include/generated/uapi"
    )
else()
    message(STATUS "Kernel build dir not found at '${CPP_LKM_KERNEL_BUILD_DIR}';"
                   " cpp-lkm-kernel-headers will not add kernel include paths."
    )
endif()

target_include_directories(cpp-lkm-kernel-headers INTERFACE ${CPP_LKM_KERNEL_INCLUDE_DIRS})
target_compile_definitions(
    cpp-lkm-kernel-headers INTERFACE __KERNEL__ MODULE KBUILD_MODNAME=\"cpp_lkm\"
)
