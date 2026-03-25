#pragma once

#include <expected>

enum class ErrorCode : int
{
    None = 0,
    AllocFail = -12,     // -ENOMEM
    CharDevRegFail = -5, // -EIO (generic registration failure; see bridge return values)
};

// Named conversion to avoid scattered static_cast<int> at every call site.
[[nodiscard]] constexpr int to_errno(ErrorCode e) noexcept
{
    return static_cast<int>(e);
}

template <typename T> using Result = std::expected<T, ErrorCode>;
