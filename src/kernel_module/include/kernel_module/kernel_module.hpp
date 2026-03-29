#pragma once

#include "cpp_lkm/runtime/kalloc.hpp"
#include "cpp_lkm/runtime/kernel_module.hpp"
#include "kernel_echo_server.hpp"
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
    KOwned<Resource> _resource1;
    KOwned<Resource> _resource2;
    KernelEchoServer _echo_server;
    UserspaceDevice _userspace;
};
