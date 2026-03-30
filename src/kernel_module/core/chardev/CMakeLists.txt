add_library(kernel_module_chardev OBJECT src/userspace_device.cpp)

target_include_directories(
    kernel_module_chardev PRIVATE "${CMAKE_SOURCE_DIR}/src/kernel_module/include"
)

target_sources(
    kernel_module_chardev
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           "${CMAKE_CURRENT_SOURCE_DIR}/include"
           FILES
           include/kernel_module/core/chardev/ihandler.hpp
           include/kernel_module/core/chardev/userspace_device.hpp
)

target_link_libraries(kernel_module_chardev PUBLIC "my_kernel_module.iface" cpp_lkm_runtime)
