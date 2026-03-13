# cmake/KbuildModule.cmake

function(add_kbuild_module TARGET_NAME CPP_LIB_TARGET)
    set(options "")
    set(oneValueArgs KBUILD_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(KB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 1. Resolve KBUILD_DIR
    if(NOT KB_KBUILD_DIR)
        if(DEFINED CACHE{KBUILD_DIR})
            set(KB_KBUILD_DIR $CACHE{KBUILD_DIR})
        else()
            # Default to running system's build dir
            execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(KB_KBUILD_DIR "/lib/modules/${KERNEL_RELEASE}/build")
        endif()
    endif()

    # 2. Validation
    if(NOT EXISTS "${KB_KBUILD_DIR}/Makefile")
        message(FATAL_ERROR
            "Kernel build directory not found at '${KB_KBUILD_DIR}'.\n"
            "Kbuild integration requires the kernel development headers.\n"
            "On Fedora, run: sudo dnf install kernel-devel"
        )
    endif()

    message(STATUS "Found Kbuild directory: ${KB_KBUILD_DIR}")

    # 3. Directories
    set(MOD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/kbuild_${TARGET_NAME}")
    file(MAKE_DIRECTORY "${MOD_BUILD_DIR}")

    # 4. Get the static library path
    # We need the actual file on disk
    set(LIB_PATH "$<TARGET_FILE:${CPP_LIB_TARGET}>")

    # 5. Generate Kbuild Makefile
    # We use a template approach or just write it directly
    set(KBUILD_MAKEFILE_CONTENT
"obj-m += ${TARGET_NAME}.o\n"
"${TARGET_NAME}-y := ${LIB_PATH}\n"
    )
    
    # We need to use file(GENERATE) because targets files are only known at generate time
    file(GENERATE 
        OUTPUT "${MOD_BUILD_DIR}/Makefile"
        CONTENT "${KBUILD_MAKEFILE_CONTENT}"
    )

    # 6. Custom target to run Kbuild
    # We must ensure the CPP_LIB_TARGET is built first
    add_custom_target(${TARGET_NAME}_ko
        COMMAND make -C "${KB_KBUILD_DIR}" M="${MOD_BUILD_DIR}" modules
        DEPENDS ${CPP_LIB_TARGET}
        WORKING_DIRECTORY "${MOD_BUILD_DIR}"
        COMMENT "Invoking Kbuild to produce ${TARGET_NAME}.ko"
        VERBATIM
    )

    # 7. Post-build: copy the .ko to the main build dir for convenience
    add_custom_command(TARGET ${TARGET_NAME}_ko POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${MOD_BUILD_DIR}/${TARGET_NAME}.ko" "${CMAKE_BINARY_DIR}/${TARGET_NAME}.ko"
        COMMENT "Copying ${TARGET_NAME}.ko to build directory"
    )

endfunction()
