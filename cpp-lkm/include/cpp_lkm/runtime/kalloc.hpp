#pragma once

#include "compat/new_shim.hpp" // IWYU pragma: keep
#include "cpp_lkm/common/error.hpp"
#include "cpp_lkm/runtime/kernel_api/context.h"
#include "cpp_lkm/runtime/kernel_api/memory.h"
#include "cpp_lkm/runtime/kernel_api/types.h"

#include <cerrno>
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
        return std::unexpected(-ENOMEM);
    }
    return mem;
}

// Allocate memory for one T, construct it with args, and return a pointer.
// Returns -ENOMEM if kmalloc returns null.
// The caller owns the returned pointer and must free it with kfree_obj<T>().
template <typename T, typename... Args> [[nodiscard]] Result<T*> kalloc(Args&&... args) noexcept
{
    auto mem = kmalloc_or_error(sizeof(T));
    if (!mem)
    {
        return std::unexpected(mem.error());
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Result<T*> API; caller frees via kfree_obj.
    return new (*mem) T{std::forward<Args>(args)...};
}

// Allocate a fixed-size array of trivially constructible types.
template <typename T> [[nodiscard]] Result<T*> kalloc_array(size_t count) noexcept
{
    static_assert(std::is_trivially_default_constructible_v<T>,
                  "kalloc_array requires trivially constructible types; use kalloc() for others");
    if (count == 0)
    {
        return std::unexpected(-ENOMEM);
    }

    constexpr size_t max_size = std::numeric_limits<size_t>::max();
    if (count > (max_size / sizeof(T)))
    {
        return std::unexpected(-ENOMEM);
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

// Move-only owner for objects created by kalloc<T>().
// Prefer this for normal single-object ownership in module code.
// Keep raw kalloc()/kfree_obj() for low-level/manual control paths.
// Do not use this with kalloc_array<T>() allocations; arrays are freed via cpp_kfree().
template <typename T> class KOwned
{
  public:
    KOwned() noexcept = default;
    explicit KOwned(T* ptr) noexcept : _ptr(ptr) {}

    ~KOwned()
    {
        reset();
    }

    KOwned(const KOwned&) = delete;
    KOwned& operator=(const KOwned&) = delete;

    KOwned(KOwned&& other) noexcept : _ptr(other.release()) {}

    KOwned& operator=(KOwned&& other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] T* get() const noexcept
    {
        return _ptr;
    }

    [[nodiscard]] T& operator*() const noexcept
    {
        return *_ptr;
    }

    [[nodiscard]] T* operator->() const noexcept
    {
        return _ptr;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return _ptr != nullptr;
    }

    T* release() noexcept
    {
        return std::exchange(_ptr, nullptr);
    }

    void reset(T* replacement = nullptr) noexcept
    {
        T* old = std::exchange(_ptr, replacement);
        kfree_obj(old);
    }

  private:
    T* _ptr = nullptr;
};

template <typename T, typename... Args>
[[nodiscard]] Result<KOwned<T>> kalloc_owned(Args&&... args) noexcept
{
    auto raw = kalloc<T>(std::forward<Args>(args)...);
    if (!raw)
    {
        return std::unexpected(raw.error());
    }
    return KOwned<T>{*raw};
}
