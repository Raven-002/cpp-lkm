#pragma once
#include "error.hpp"

#include <expected>

// Forward declaration — full definition lives in test_resource.hpp.
// module.cpp includes test_resource.hpp for the complete type.
struct TestResource;

class CppKernelModule
{
  public:
    CppKernelModule();
    [[nodiscard]] std::expected<void, ErrorCode> init();
    ~CppKernelModule();

  private:
    TestResource* _resource1 = nullptr;
    TestResource* _resource2 = nullptr;
};
