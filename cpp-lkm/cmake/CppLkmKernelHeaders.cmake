# CppLkmKernelHeaders.cmake
# Attaches kernel include paths and compile definitions to an INTERFACE target.
#
# Usage:
#   cpp_lkm_attach_kernel_headers(<target> KDIR <kdir> MODULE_NAME <name>)

function(cpp_lkm_attach_kernel_headers target)
    set(oneValueArgs KDIR MODULE_NAME)
    cmake_parse_arguments(_AKH "" "${oneValueArgs}" "" ${ARGN})

    set(_kdir "${_AKH_KDIR}")
    set(_mod "${_AKH_MODULE_NAME}")

    target_compile_definitions(${target} INTERFACE KBUILD_MODNAME=\"${_mod}\")

    if(NOT EXISTS "${_kdir}/Makefile")
        message(
            STATUS
            "cpp-lkm: kernel build dir '${_kdir}' not found; kernel include paths not added."
        )
        return()
    endif()

    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE _arch
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    target_include_directories(
        ${target}
        INTERFACE "${_kdir}/include"
                  "${_kdir}/include/uapi"
                  "${_kdir}/include/generated"
                  "${_kdir}/include/generated/uapi"
                  "${_kdir}/arch/${_arch}/include"
                  "${_kdir}/arch/${_arch}/include/uapi"
                  "${_kdir}/arch/${_arch}/include/generated"
                  "${_kdir}/arch/${_arch}/include/generated/uapi"
    )
endfunction()
