#pragma once

#include "cpp_lkm/common/error.hpp"

// Interface every kernel module implementation must satisfy.
//
// Lifecycle contract:
//   1. The concrete type is placement-new'd into a static buffer (trivial ctor, no alloc).
//   2. init() is called — may allocate resources; returns Result<void> on failure.
//   3. The destructor is called explicitly (placement new / no heap delete).
//      It must be safe even after a partial init().
class IKernelModule
{
  public:
    virtual ~IKernelModule() = default;

    IKernelModule(const IKernelModule&) = delete;
    IKernelModule& operator=(const IKernelModule&) = delete;
    IKernelModule(IKernelModule&&) = delete;
    IKernelModule& operator=(IKernelModule&&) = delete;

    [[nodiscard]] virtual Result<void> init() = 0;

  protected:
    IKernelModule() = default;
};
