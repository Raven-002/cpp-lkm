#pragma once

#include "cpp_lkm/runtime/kernel_module.hpp"
#include "userspace_device.hpp"

// Example kernel module implementation.
// Implements IKernelModule (two-phase init: trivial ctor + fallible init()).
class KernelModule : public IKernelModule
{
  public:
    KernelModule();
    KernelModule(const KernelModule&) = delete;
    KernelModule& operator=(const KernelModule&) = delete;
    KernelModule(KernelModule&&) = delete;
    KernelModule& operator=(KernelModule&&) = delete;

    [[nodiscard]] Result<void> init() override;
    ~KernelModule() override;

  private:
    class Resource;
    Resource* _resource1 = nullptr;
    Resource* _resource2 = nullptr;
    UserspaceDevice _userspace;
};
