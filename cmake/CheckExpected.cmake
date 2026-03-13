include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_FLAGS "-std=c++23")
check_cxx_source_compiles("
    #include <expected>
    #if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
    #  error missing
    #endif
    int main() { std::expected<int,int> e{1}; return e.value(); }
" HAS_NATIVE_EXPECTED)

if(HAS_NATIVE_EXPECTED)
    message(STATUS "std::expected: native")
else()
    message(STATUS "std::expected: using tl::expected backport")
endif()
