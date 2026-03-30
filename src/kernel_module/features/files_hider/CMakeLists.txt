add_library(kernel_module_files_hider OBJECT src/kernel_files_hider_server.cpp)

target_sources(
    kernel_module_files_hider
    PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" FILES
           include/kernel_module/features/files_hider/kernel_files_hider_server.hpp
)

target_link_libraries(
    kernel_module_files_hider PUBLIC "my_kernel_module.iface" kernel_module_chardev
)
