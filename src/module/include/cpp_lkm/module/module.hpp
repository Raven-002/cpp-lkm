#pragma once

#include "cpp_lkm/common/error.hpp"

class CppKernelModule
{
  public:
    CppKernelModule();
    [[nodiscard]] Result<void> init();
    ~CppKernelModule();

  private:
    class Resource;
    Resource* _resource1 = nullptr;
    Resource* _resource2 = nullptr;
};
