#pragma once

#include "cpp_lkm/common/error.hpp"

#include <expected>

class CppKernelModule
{
  public:
    CppKernelModule();
    [[nodiscard]] std::expected<void, ErrorCode> init();
    ~CppKernelModule();

  private:
    class Resource;
    Resource* _resource1 = nullptr;
    Resource* _resource2 = nullptr;
};
