#pragma once

#include "cpp_lkm/common/error.hpp"

#include <cerrno>
#include <cstdint>

enum class MyError : std::int8_t
{
    AllocFail = -ENOMEM,
    CharDevRegFail = -EIO,
};

[[nodiscard]] constexpr int to_errno(MyError err) noexcept
{
    return static_cast<int>(err);
}
