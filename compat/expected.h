// compat/expected.h
#pragma once

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#else
#include "tl/expected.hpp"
namespace std
{
template <class T, class E> using expected = tl::expected<T, E>;
template <class E> using unexpected = tl::unexpected<E>;
template <class E>
[[nodiscard]] constexpr auto make_unexpected(E&& e) -> tl::unexpected<std::decay_t<E>>
{
    return tl::make_unexpected(std::forward<E>(e));
}
} // namespace std
#endif
