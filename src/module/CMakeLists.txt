add_library(module_core OBJECT src/module.cpp)
target_link_libraries(module_core PUBLIC libkernel cpp-lkm-kernel-headers module_common)
target_include_directories(module_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/include)
target_compile_options(module_core PRIVATE ${KERNEL_ABI_COMPILE_OPTIONS})
target_sources(module_core
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/cpp_lkm/module/module.hpp
)
