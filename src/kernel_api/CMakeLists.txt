# Consumer kernel_api: C ABI headers + sources compiled by Kbuild for the real .ko. INTERFACE target
# carries include path and public headers via FILE_SET; .c files are grouped on a source-holder
# custom target that cpp_lkm_add_ko_target() consumes via KBUILD_SOURCE_TARGETS.

add_library(kernel_api_iface INTERFACE)
target_sources(
    kernel_api_iface
    INTERFACE FILE_SET
              HEADERS
              BASE_DIRS
              "${CMAKE_CURRENT_SOURCE_DIR}/include"
              FILES
              include/kernel_api/assert.h
              include/kernel_api/chardev.h
              include/kernel_api/context.h
              include/kernel_api/kernel_api.h
              include/kernel_api/logging.h
              include/kernel_api/memory.h
              include/kernel_api/types.h
)

add_custom_target(
    kernel_api_sources SOURCES src/assert_kernel.c src/chardev_kernel.c src/logging_kernel.c
)
