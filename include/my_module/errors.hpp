#pragma once

#include "cpp_lkm/common/error.hpp"

#include <cerrno>

enum class MyError : int
{
    AllocFail = -ENOMEM,
    CharDevRegFail = -EIO,
};

[[nodiscard]] constexpr int to_errno(MyError e) noexcept
{
    return static_cast<int>(e);
}
