# Consumer kernel_api: C ABI headers + sources compiled by Kbuild for the real .ko. INTERFACE target
# carries include path and public headers via FILE_SET; .c files stay on a custom target for IDE
# listing only (do not use INTERFACE_SOURCES — that would compile them into consumers).

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

file(GLOB _kernel_api_c CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_custom_target(kernel_api_sources SOURCES ${_kernel_api_c})
