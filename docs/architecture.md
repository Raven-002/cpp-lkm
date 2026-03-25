# Architecture Document

## Two-Phase Initialization

The module uses a strictly defined two-phase initialization pattern:

1. **Phase 1 (Construction)**: The `CppKernelModule` constructor is called via
   placement-new. This phase is trivial, performs zero allocations, and cannot
   fail.
2. **Phase 2 (Initialization)**: The `init()` method is called. This phase is
   fallible and may allocate internal resources,
   and returns `std::expected<void, ErrorCode>`. Any failures must be handled gracefully.

## Extending the module implementation (`src/module/`)

The **C++ logic of the loadable module**—what the module actually does beyond
runtime and bridge code—lives under `src/module/`. The canonical class is
`CppKernelModule` (`include/cpp_lkm/module/module.hpp`,
`src/module/src/module.cpp`).

**How to grow it:**

1. **Types and files**: Add members, private helpers, and nested types on
   `CppKernelModule`, or split cohesive subsystems into additional `.hpp`/`.cpp`
   files under `src/module/include/cpp_lkm/module/` and `src/module/src/`.
2. **Build system**: For each new `.cpp`, add it to the `OBJECT` library in
   `src/module/CMakeLists.txt`. Add new public headers to the `module_core`
   `FILE_SET HEADERS` in the same file so tooling and consumers see them
   consistently.
3. **Initialization contract**: Keep Phase 1 construction trivial (no
   allocation); put fallible setup in `init()`; free everything in the
   destructor. Use `kalloc<T>()` / `kfree_obj()` for heap data (see below).
4. **Kernel surface**: Do not call Linux internals directly from C++ module code.
   Declare needed operations in `src/runtime/include/cpp_lkm/runtime/kernel_api.h`
   and implement them in `linux_bridge.c` and `tests/support/mock_kernel_bridge.cpp`
   as described in [The C/C++ Kernel Bridge](#the-cc-kernel-bridge).
5. **Lifecycle glue**: `src/runtime/src/bridge.cpp` allocates storage for
   `CppKernelModule`, placement-news it, calls `init()`, and destroys it on
   unload. Change that file only if the global instance shape or the exported
   `extern "C"` entry points (`module_entry.h`) need to change—not for ordinary
   feature work inside the class.

## Context-Aware Allocator (`kalloc`)

Standard `new` is disabled (see Linker Trap section). All dynamic memory
allocations happen via `kalloc<T>()`, which returns a
`std::expected<T*, ErrorCode>`.
`kalloc` automagically queries the current CPU execution context (e.g.,
interrupts disabled, NMI, or atomic context) and selects the right `kmalloc`
bitmask (`GFP_KERNEL` or `GFP_ATOMIC`).

## The `operator new` Linker Trap

Global heap-form `operator new` and `new[]` are intentionally **declared but
not defined**.
If any developer accidentally uses `new Foo{}` instead of `kalloc<Foo>()`, the
code will compile, but the linker will fail with an undefined reference to
`operator new`, catching the mistake at build time. This is strictly better
than runtime panics or null pointer dereferences.

## Compatibility Layer (`compat/`)

The goal is to write zero `#ifdef`s inside `src/` and `include/` regarding
compiler versions or stdlib capabilities.
The `compat/` layer is intentionally small and currently provides freestanding
placement-new support via `compat/new_shim.hpp`. The project now requires
native `std::expected` from the toolchain.

## The C/C++ Kernel Bridge

Kernel code is written in C. This module exposes its functionality through
exported C functions, mapped to `extern "C"` wrappers.

**Dual-Implementation Symmetry:**

- `src/runtime/src/linux_bridge.c` — The real bridge compiled by Kbuild. It forwards
  `cpp_kmalloc` to the real Linux `kmalloc`, `cpp_printk` to `vprintk`, etc.
- `tests/support/mock_kernel_bridge.cpp` — The host-mode counterpart. It forwards
  `cpp_kmalloc` to `malloc()` (with fail-injection capabilities),
  `cpp_printk` to `printf()`, etc.

When a new kernel API feature is needed, declare it in
`src/runtime/include/cpp_lkm/runtime/kernel_api.h`, implement it once for real
in `src/runtime/src/linux_bridge.c`, and once for host tests in
`tests/support/mock_kernel_bridge.cpp`.
