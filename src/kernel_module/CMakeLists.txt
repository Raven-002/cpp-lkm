add_subdirectory(core)
add_subdirectory(features)

add_library(kernel_module STATIC src/kernel_module.cpp)
target_sources(
    kernel_module
    PRIVATE $<TARGET_OBJECTS:kernel_module_chardev> $<TARGET_OBJECTS:kernel_module_echo>
            $<TARGET_OBJECTS:kernel_module_files_hider>
)

target_sources(
    kernel_module
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           "${CMAKE_CURRENT_SOURCE_DIR}/include"
           FILES
           include/kernel_module/kernel_module.hpp
           include/kernel_module/errors.hpp
)

target_link_libraries(
    kernel_module PUBLIC "my_kernel_module.iface" cpp_lkm_runtime kernel_module_chardev
                         kernel_module_echo kernel_module_files_hider
)
