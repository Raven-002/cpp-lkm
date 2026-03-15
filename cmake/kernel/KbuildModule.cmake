function(add_kbuild_module TARGET_NAME CPP_LIB_TARGET)
    set(options ALL)
    set(oneValueArgs KBUILD_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(KB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT KB_KBUILD_DIR)
        if(DEFINED KBUILD_DIR AND NOT "${KBUILD_DIR}" STREQUAL "")
            set(KB_KBUILD_DIR "${KBUILD_DIR}")
        else()
            execute_process(
                COMMAND uname -r
                OUTPUT_VARIABLE KERNEL_RELEASE
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(KB_KBUILD_DIR "/lib/modules/${KERNEL_RELEASE}/build")
        endif()
    endif()

    if(NOT EXISTS "${KB_KBUILD_DIR}/Makefile")
        message(
            FATAL_ERROR
                "Kernel build directory not found at '${KB_KBUILD_DIR}'.\n"
                "Kbuild integration requires the kernel development headers.\n"
                "On Fedora, run: sudo dnf install kernel-devel"
        )
    endif()

    message(STATUS "Found Kbuild directory: ${KB_KBUILD_DIR}")

    set(MOD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/kbuild_${TARGET_NAME}")
    file(MAKE_DIRECTORY "${MOD_BUILD_DIR}")

    set(BRIDGE_SRC "${CMAKE_SOURCE_DIR}/src/linux_bridge.c")
    set(LIB_NAME "libkernel_module.a")

    string(CONCAT KBUILD_MAKEFILE_CONTENT "obj-m += ${TARGET_NAME}.o\n"
                  "${TARGET_NAME}-y := linux_bridge.o ${LIB_NAME}\n"
    )

    file(
        GENERATE
        OUTPUT "${MOD_BUILD_DIR}/linux_bridge.c"
        INPUT "${BRIDGE_SRC}"
    )

    file(
        GENERATE
        OUTPUT "${MOD_BUILD_DIR}/Makefile"
        CONTENT "${KBUILD_MAKEFILE_CONTENT}"
    )

    if(KB_ALL)
        set(ALL_ARG "ALL")
    else()
        set(ALL_ARG "")
    endif()

    add_custom_target(
        ${TARGET_NAME}_ko
        ${ALL_ARG}
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${CPP_LIB_TARGET}>"
                "${MOD_BUILD_DIR}/${LIB_NAME}"
        COMMAND make -C "${KB_KBUILD_DIR}" M=${MOD_BUILD_DIR} modules
        DEPENDS ${CPP_LIB_TARGET}
        WORKING_DIRECTORY "${MOD_BUILD_DIR}"
        COMMENT "Invoking Kbuild to produce ${TARGET_NAME}.ko"
        VERBATIM
    )

    add_custom_command(
        TARGET ${TARGET_NAME}_ko
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${MOD_BUILD_DIR}/${TARGET_NAME}.ko"
                "${CMAKE_BINARY_DIR}/${TARGET_NAME}.ko"
        COMMENT "Copying ${TARGET_NAME}.ko to build directory"
    )

endfunction()
