// Heap-form operator new is intentionally NOT defined.
// If you are seeing a linker error referencing operator new(size_t),
// you have used 'new T{}' in business logic. Use kalloc<T>() instead.
//
// Placement new (operator new(size_t, void*)) is defined in compat/new_shim.hpp
// and is the only permitted form of new in this codebase.

#include "cpp_lkm/runtime/kernel_api.h"

// operator new(size_t) — declared by the standard headers, intentionally NOT defined.
// The linker will error if any TU calls heap-form new.

void operator delete(void* ptr) noexcept
{
    cpp_kfree(ptr);
}

void operator delete(void* ptr, size_t /*size*/) noexcept
{
    cpp_kfree(ptr);
}
