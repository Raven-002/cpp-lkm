add_library(module_common INTERFACE)
target_sources(module_common
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/cpp_lkm/common/error.hpp
)
target_link_libraries(module_common INTERFACE libkernel)
