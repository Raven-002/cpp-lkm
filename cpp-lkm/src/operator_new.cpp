// Heap-form operator new is intentionally NOT defined.
// If you are seeing a linker error referencing operator new(size_t),
// you have used 'new T{}' in business logic. Use kalloc<T>() instead.
//
// Placement new (operator new(size_t, void*)) is defined in compat/new_shim.hpp
// and is the only permitted form of new in this codebase.

#include "cpp_lkm/runtime/kernel_api/memory.h"

// operator new(size_t) — declared by the standard headers, intentionally NOT defined.
// The linker will error if any TU calls heap-form new.
//
// NOLINTBEGIN(cert-dcl54-cpp, hicpp-new-delete-operators, misc-new-delete-overloads):
// clang-tidy does not treat <new> as user code for pairing checks; heap operator new
// is intentionally omitted in this TU (see file comment).

void operator delete(void* ptr) noexcept
{
    cpp_kfree(ptr);
}

void operator delete(void* ptr, size_t /*size*/) noexcept
{
    cpp_kfree(ptr);
}

// NOLINTEND(cert-dcl54-cpp, hicpp-new-delete-operators, misc-new-delete-overloads)
