#pragma once

#include "compat/new_shim.hpp" // IWYU pragma: keep
#include "cpp_lkm/common/error.hpp"
#include "cpp_lkm/runtime/kernel_api.h"

#include <expected>
#include <limits>
#include <type_traits>
#include <utility>

// Select the correct GFP flag for the current CPU context.
// Must never be called with a size of zero.
[[nodiscard]] inline cpp_gfp_t current_gfp_flags() noexcept
{
    return (cpp_in_atomic() != 0 || cpp_irqs_disabled() != 0 || cpp_in_nmi() != 0) ? CPP_GFP_ATOMIC
                                                                                   : CPP_GFP_KERNEL;
}

[[nodiscard]] inline Result<void*> kmalloc_or_error(size_t bytes) noexcept
{
    // Returned memory is written by placement new; not logically const void*.
    // NOLINTNEXTLINE(misc-const-correctness)
    void* mem = cpp_kmalloc(bytes, current_gfp_flags());
    if (mem == nullptr) [[unlikely]]
    {
        return std::unexpected(ErrorCode::AllocFail);
    }
    return mem;
}

// Allocate memory for one T, construct it with args, and return a pointer.
// Returns ErrorCode::AllocFail if kmalloc returns null.
// The caller owns the returned pointer and must free it with kfree_obj<T>().
template <typename T, typename... Args> [[nodiscard]] Result<T*> kalloc(Args&&... args) noexcept
{
    auto mem = kmalloc_or_error(sizeof(T));
    if (!mem)
    {
        return std::unexpected(mem.error());
    }
    return new (*mem) T{std::forward<Args>(args)...};
}

// Allocate a fixed-size array of trivially constructible types.
template <typename T> [[nodiscard]] Result<T*> kalloc_array(size_t count) noexcept
{
    static_assert(std::is_trivially_default_constructible_v<T>,
                  "kalloc_array requires trivially constructible types; use kalloc() for others");
    if (count == 0)
    {
        return std::unexpected(ErrorCode::AllocFail);
    }

    constexpr size_t max_size = std::numeric_limits<size_t>::max();
    if (count > (max_size / sizeof(T)))
    {
        return std::unexpected(ErrorCode::AllocFail);
    }

    auto mem = kmalloc_or_error(sizeof(T) * count);
    if (!mem)
    {
        return std::unexpected(mem.error());
    }
    return static_cast<T*>(*mem);
}

// Destroy and free a pointer previously returned by kalloc<T>().
// Safe to call with nullptr.
template <typename T> void kfree_obj(T* ptr) noexcept
{
    if (ptr != nullptr)
    {
        ptr->~T();
        cpp_kfree(ptr);
    }
}
