#include <linux/slab.h>
#include <stddef.h>

// Heap-form operator new is intentionally NOT defined.
// If you are seeing a linker error referencing operator new(size_t),
// you have used 'new T{}' in business logic. Use kalloc<T>() instead.
//
// Placement new (operator new(size_t, void*)) is defined in compat/new_shim.hpp
// and is the only permitted form of new in this codebase.

void operator delete(void* p) noexcept
{
    kfree(p);
}

void operator delete(void* p, size_t) noexcept
{
    kfree(p);
}
