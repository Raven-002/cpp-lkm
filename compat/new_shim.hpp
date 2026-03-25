// compat/new_shim.hpp
//
// IMPORTANT CONTRACT:
//   - This shim provides placement-new only.
//   - It must never declare or define heap-form operator new/delete.
//   - Heap forms (including new[], and nothrow heap forms) are intentionally
//     left unresolved and trapped at link time in operator_new.cpp policy.
//
// Why: in freestanding/kernel code with no exceptions, heap-form new is not a
// safe failure-propagation mechanism for this project.
#pragma once
#include <stddef.h>

#if __has_include(<new>)
#include <new>
#else
inline void* operator new(size_t, void* p) noexcept
{
    return p;
}
inline void operator delete(void*, void*) noexcept {}
#endif
