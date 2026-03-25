# Architecture Document

## Two-Phase Initialization

The module uses a strictly defined two-phase initialization pattern:

1. **Phase 1 (Construction)**: The `CppKernelModule` constructor is called via
   placement-new. This phase is trivial, performs zero allocations, and cannot
   fail.
2. **Phase 2 (Initialization)**: The `init()` method is called. This phase is
   fallible, may allocate resources (like `TestResource`s defined in `test_resource.hpp`),
   and returns `std::expected<void, ErrorCode>`. Any failures must be handled gracefully.

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

- `src/linux_bridge.c` — The real bridge compiled by Kbuild. It forwards
  `cpp_kmalloc` to the real Linux `kmalloc`, `cpp_printk` to `vprintk`, etc.
- `tests/mock_kernel_bridge.cpp` — The host-mode counterpart. It forwards
  `cpp_kmalloc` to `malloc()` (with fail-injection capabilities),
  `cpp_printk` to `printf()`, etc.

When a new kernel API feature is needed, it is declared in `mock_kernel/linux/`
headers, implemented once for real in `linux_bridge.c`, and once for the test
harness in `mock_kernel_bridge.cpp`.
