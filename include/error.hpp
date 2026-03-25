#pragma once

enum class ErrorCode : int
{
    None = 0,
    AllocFail = -12,  // -ENOMEM
    HwHandshake = -5, // -EIO
    BadState = -22,   // -EINVAL
};

// Named conversion to avoid scattered static_cast<int> at every call site.
[[nodiscard]] constexpr int to_errno(ErrorCode e) noexcept
{
    return static_cast<int>(e);
}
