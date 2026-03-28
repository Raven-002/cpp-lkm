#pragma once

#include <expected>

template <typename T> using Result = std::expected<T, int>;
