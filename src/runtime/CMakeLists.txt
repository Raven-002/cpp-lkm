add_library(kernel_runtime OBJECT src/bridge.cpp src/operator_new.cpp)
target_link_libraries(kernel_runtime PUBLIC libkernel cpp-lkm-kernel-headers module_common)
target_include_directories(kernel_runtime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../module/include)
target_compile_options(kernel_runtime PRIVATE ${KERNEL_ABI_COMPILE_OPTIONS})
target_sources(kernel_runtime
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/cpp_lkm/runtime/kalloc.hpp
          include/cpp_lkm/runtime/kernel_api.h
          include/cpp_lkm/runtime/module_entry.h
)
