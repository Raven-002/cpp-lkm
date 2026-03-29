#include "kernel_module/kernel_echo_server.hpp"

#include "cpp_lkm/runtime/kernel_api.h"

#include <cstring>
#include <limits>
#include <string_view>

namespace
{
constexpr std::string_view k_default_status{"cpp_lkm ok\n"};
} // namespace

cpp_ssize_t KernelEchoServer::read_kernel(void* kbuf, size_t len, const std::int64_t* pos)
{
    (void)pos;
    if (_write_len > 0)
    {
        const size_t copy_len = _write_len < len ? _write_len : len;
        memcpy(kbuf, _write_buf.data(), copy_len);
        return static_cast<cpp_ssize_t>(copy_len);
    }

    const size_t status_len = k_default_status.size();
    const size_t copy_len = status_len < len ? status_len : len;
    memcpy(kbuf, k_default_status.data(), copy_len);
    return static_cast<cpp_ssize_t>(copy_len);
}

cpp_ssize_t KernelEchoServer::write_kernel(const void* kbuf, size_t len, const std::int64_t* pos)
{
    (void)pos;
    if (len == 0)
    {
        return 0;
    }

    const size_t cap = k_buf_size - 1U;
    const size_t copy_len = len < cap ? len : cap;
    memcpy(_write_buf.data(), kbuf, copy_len);
    // copy_len < k_buf_size is guaranteed by cap and clamp above.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    _write_buf[copy_len] = '\0';
    _write_len = copy_len;

    const auto max_log_len = static_cast<size_t>(std::numeric_limits<int>::max());
    const auto log_len = static_cast<int>(copy_len < max_log_len ? copy_len : max_log_len);
    cpp_printk(CPP_KERN_INFO "[CPP] EchoServer write: %.*s\n", log_len, _write_buf.data());
    return static_cast<cpp_ssize_t>(copy_len);
}
