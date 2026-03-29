add_library(
    kernel_module STATIC
    src/kernel_module.cpp
    src/userspace_device.cpp
    src/kernel_echo_server.cpp
    include/kernel_module/kernel_module.hpp
    include/kernel_module/userspace_device.hpp
    include/kernel_module/kernel_echo_server.hpp
)

target_include_directories(kernel_module PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(kernel_module PUBLIC my_kernel_module.iface cpp_lkm_runtime)
