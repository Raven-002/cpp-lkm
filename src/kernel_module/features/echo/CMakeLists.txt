add_library(kernel_module_echo OBJECT src/kernel_echo_server.cpp)

target_sources(
    kernel_module_echo PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
                              FILES include/kernel_module/features/echo/kernel_echo_server.hpp
)

target_link_libraries(kernel_module_echo PUBLIC "my_kernel_module.iface" kernel_module_chardev)
