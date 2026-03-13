// compat/new_shim.h
#pragma once
#include <stddef.h>

#if __has_include(<new>)
#include <new>
#else
inline void* operator new  (size_t, void* p) noexcept { return p; }
inline void  operator delete(void*, void*)   noexcept {}
#endif
