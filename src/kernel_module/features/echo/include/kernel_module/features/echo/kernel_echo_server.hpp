#pragma once

#include "kernel_module/core/chardev/ihandler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

class KernelEchoServer final : public IUserspaceDeviceHandler
{
  public:
    [[nodiscard]] cpp_ssize_t read_kernel(void* kbuf, size_t len, const std::int64_t* pos) override;
    [[nodiscard]] cpp_ssize_t write_kernel(const void* kbuf, size_t len,
                                           const std::int64_t* pos) override;

  private:
    static constexpr size_t k_buf_size = 256;
    std::array<char, k_buf_size> _write_buf{};
    size_t _write_len = 0;
};
