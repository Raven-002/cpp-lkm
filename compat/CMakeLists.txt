# compat — freestanding shims (placement new, gsl_owner, …) for kernel / freestanding C++.
cmake_minimum_required(VERSION 4.3.0)

add_library(cpp_lkm_compat INTERFACE)
target_sources(
    cpp_lkm_compat
    INTERFACE FILE_SET
              HEADERS
              BASE_DIRS
              "${CMAKE_CURRENT_SOURCE_DIR}/include"
              FILES
              compat/new_shim.hpp
              compat/gsl_owner.hpp
)
