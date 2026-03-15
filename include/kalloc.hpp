#pragma once
#include "compat/expected.hpp"
#include "compat/new_shim.hpp"
#include "error.hpp"

#include <linux/preempt.h>
#include <linux/slab.h>

// Select the correct GFP flag for the current CPU context.
// Must never be called with a size of zero.
[[nodiscard]] inline gfp_t current_gfp_flags() noexcept
{
    return (in_atomic() || irqs_disabled() || in_nmi()) ? GFP_ATOMIC : GFP_KERNEL;
}

// Allocate memory for one T, construct it with args, and return a pointer.
// Returns ErrorCode::AllocFail if kmalloc returns null.
// The caller owns the returned pointer and must free it with kfree_obj<T>().
template <typename T, typename... Args>
[[nodiscard]] std::expected<T*, ErrorCode> kalloc(Args&&... args) noexcept
{
    void* mem = kmalloc(sizeof(T), current_gfp_flags());
    if (!mem) [[unlikely]]
        return std::unexpected(ErrorCode::AllocFail);
    return new (mem) T{static_cast<Args&&>(args)...};
}

// Allocate a fixed-size array of trivially constructible types.
template <typename T> [[nodiscard]] std::expected<T*, ErrorCode> kalloc_array(size_t count) noexcept
{
    static_assert(__is_trivially_constructible(T),
                  "kalloc_array requires trivially constructible types; use kalloc() for others");
    void* mem = kmalloc(sizeof(T) * count, current_gfp_flags());
    if (!mem) [[unlikely]]
        return std::unexpected(ErrorCode::AllocFail);
    return static_cast<T*>(mem);
}

// Destroy and free a pointer previously returned by kalloc<T>().
// Safe to call with nullptr.
template <typename T> void kfree_obj(T* p) noexcept
{
    if (p)
    {
        p->~T();
        kfree(p);
    }
}
