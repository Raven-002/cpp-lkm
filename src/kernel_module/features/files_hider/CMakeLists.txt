add_library(
    kernel_module_files_hider OBJECT
    src/kernel_files_hider_server.cpp src/kernel_files_hider_manager.cpp
    src/dummy_kernel_files_hider_backend.cpp)

target_sources(
    kernel_module_files_hider
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           "${CMAKE_CURRENT_SOURCE_DIR}/include"
           FILES
           include/kernel_module/features/files_hider/i_kernel_files_hider_backend.hpp
           include/kernel_module/features/files_hider/dummy_kernel_files_hider_backend.hpp
           include/kernel_module/features/files_hider/kernel_files_hider_manager.hpp
           include/kernel_module/features/files_hider/kernel_files_hider_server.hpp)

target_link_libraries(kernel_module_files_hider PUBLIC "my_kernel_module.iface"
                                                       kernel_module_chardev)
