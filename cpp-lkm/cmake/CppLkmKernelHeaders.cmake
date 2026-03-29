# CppLkmKernelHeaders.cmake
# Attaches kernel include paths and compile definitions to an INTERFACE target.
#
# Usage:
#   cpp_lkm_attach_kernel_headers(<target> KDIR <kdir> MODULE_NAME <name>)

function(cpp_lkm_attach_kernel_headers iface_target)
    cpp_lkm_assert_nonempty("cpp_lkm_attach_kernel_headers()" "<iface_target>" "${iface_target}")

    set(oneValueArgs KDIR MODULE_NAME)
    cmake_parse_arguments(_AKH "" "${oneValueArgs}" "" ${ARGN})

    cpp_lkm_assert_nonempty_vars("cpp_lkm_attach_kernel_headers()" "_AKH" KDIR MODULE_NAME)

    set(_kdir "${_AKH_KDIR}")
    set(_mod "${_AKH_MODULE_NAME}")

    target_compile_definitions(${iface_target} INTERFACE KBUILD_MODNAME=\"${_mod}\")

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

    # Kernel source uses arch/x86/ for both i386 and x86_64; uname -m is i686/x86_64.
    if(_arch STREQUAL "x86_64" OR _arch STREQUAL "i386" OR _arch STREQUAL "i686")
        set(_karch "x86")
    else()
        set(_karch "${_arch}")
    endif()

    target_include_directories(
        ${iface_target}
        INTERFACE "${_kdir}/include"
                  "${_kdir}/include/uapi"
                  "${_kdir}/include/generated"
                  "${_kdir}/include/generated/uapi"
                  "${_kdir}/arch/${_karch}/include"
                  "${_kdir}/arch/${_karch}/include/uapi"
                  "${_kdir}/arch/${_karch}/include/generated"
                  "${_kdir}/arch/${_karch}/include/generated/uapi"
    )
endfunction()
