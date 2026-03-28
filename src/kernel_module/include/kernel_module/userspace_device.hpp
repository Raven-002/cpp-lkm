#pragma once

#include "cpp_lkm/runtime/kernel_api.h"
#include "errors.hpp" // IWYU pragma: keep

#include <array>
#include <cstddef>
#include <cstdint>

// Userspace-facing character device: read/write via the runtime bridge (miscdevice).
class UserspaceDevice
{
  public:
    UserspaceDevice() = default;

    UserspaceDevice(const UserspaceDevice&) = delete;
    UserspaceDevice& operator=(const UserspaceDevice&) = delete;
    UserspaceDevice(UserspaceDevice&&) = delete;
    UserspaceDevice& operator=(UserspaceDevice&&) = delete;

    [[nodiscard]] Result<void> init();
    ~UserspaceDevice();

    // Bridge callbacks (C linkage); not for general use.
    [[nodiscard]] cpp_ssize_t read_kernel(void* kbuf, size_t len, const std::int64_t* pos);
    [[nodiscard]] cpp_ssize_t write_kernel(const void* kbuf, size_t len, const std::int64_t* pos);

  private:
    static constexpr size_t k_buf_size = 256;

    bool _registered = false;
    std::array<char, k_buf_size> _write_buf{};
    size_t _write_len = 0;
};
