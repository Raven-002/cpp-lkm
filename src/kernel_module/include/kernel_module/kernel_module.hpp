#pragma once

#include "cpp_lkm/runtime/kernel_module.hpp"
#include "kernel_module/core/chardev/userspace_device.hpp"
#include "kernel_module/features/echo/kernel_echo_server.hpp"
#include "kernel_module/features/files_hider/kernel_files_hider_server.hpp"

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
    KernelEchoServer _echo_server;
    KernelFilesHiderServer _files_hider_server;
    UserspaceDevice _echo_userspace;
    UserspaceDevice _files_hider_userspace;
};
