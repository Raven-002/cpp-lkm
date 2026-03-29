# Consumer kernel_api: C ABI headers + sources compiled by Kbuild for the real .ko. INTERFACE target
# carries include path and source list for IDE/dependency tracking.

add_library(kernel_api_iface INTERFACE)
target_include_directories(kernel_api_iface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# List sources for IDE visibility only (do not use INTERFACE_SOURCES — that would compile these .c
# files into consumers such as cpp_lkm_runtime with the host toolchain).
file(GLOB _kernel_api_c CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_custom_target(kernel_api_sources SOURCES ${_kernel_api_c})
