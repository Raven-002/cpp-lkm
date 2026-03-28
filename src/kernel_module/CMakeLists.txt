add_library(
    kernel_module STATIC
    src/kernel_module.cpp
    src/userspace_device.cpp
)

target_include_directories(kernel_module PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(kernel_module PUBLIC my_kernel_module.iface cpp_lkm_runtime)
