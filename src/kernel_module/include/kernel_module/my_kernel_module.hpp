#pragma once

#include "cpp_lkm/runtime/kernel_module.hpp"
#include "kernel_module/userspace_device.hpp"

// Example kernel module implementation.
// Implements IKernelModule (two-phase init: trivial ctor + fallible init()).
class MyKernelModule : public IKernelModule
{
  public:
    MyKernelModule();
    MyKernelModule(const MyKernelModule&) = delete;
    MyKernelModule& operator=(const MyKernelModule&) = delete;
    MyKernelModule(MyKernelModule&&) = delete;
    MyKernelModule& operator=(MyKernelModule&&) = delete;

    [[nodiscard]] Result<void> init() override;
    ~MyKernelModule() override;

  private:
    class Resource;
    Resource* _resource1 = nullptr;
    Resource* _resource2 = nullptr;
    UserspaceDevice _userspace;
};
