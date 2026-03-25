#include "cpp_lkm/runtime/kernel_api.h"

#include <cstddef>

// Heap-form operator new is intentionally NOT defined.
// If you are seeing a linker error referencing operator new(size_t),
// you have used 'new T{}' in business logic. Use kalloc<T>() instead.
//
// Placement new (operator new(size_t, void*)) is defined in compat/new_shim.hpp
// and is the only permitted form of new in this codebase.
//
// Intentionally unresolved heap forms:
//   - operator new(size_t)
//   - operator new[](size_t)
//   - nothrow heap forms
// These remain a linker trap by design so allocation must flow through kalloc.

void operator delete(void* ptr) noexcept
{
    cpp_kfree(ptr);
}

void operator delete(void* ptr, std::size_t size_bytes) noexcept
{
    (void)size_bytes;
    cpp_kfree(ptr);
}
