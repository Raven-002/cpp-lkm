# cpp-lkm Developer Guidelines

## Architecture & Mocking
This repository is a C++23 kernel module framework targeting a freestanding environment, with a host-side mocked mode for rapid iteration. We use `compat/` to transparently bridge C++23 features (like `<expected>`) for older toolchains (e.g., GCC 12).

## Core Rules

1. **No Kernel Panics**: `BUG()`, `BUG_ON()`, `panic()` must **never** be called in module source. The `check_no_bug` target enforces this.
2. **No Exceptions or RTTI**: `-fno-exceptions` and `-fno-rtti` are always active.
3. **No Heap `operator new`**: It is configured as a linker trap. Attempting to use `new T{}` in `src/` or `include/` will fail to link.
4. **Use `kalloc<T>()`**: All heap allocation goes through `kalloc<T>()` which returns `std::expected<T*>`. Free using `kfree_obj(p)`.
5. **Fallible Initialization**: Phase 1 initialization (construction) must be trivial and fallible operations belong in the `init()` Phase 2 method.
6. **No `#ifdef` Noise**: Any compiler/stdlib version detection MUST be localized to the `compat/` directory.

## Testing & Workflows
- **Host mode**: `cmake -B build -DBUILD_MODE=host`, `cmake --build build`, `ctest --test-dir build`
- **Platform/CI mode**: Needs `-DBUILD_MODE=platform` and `-DGCC12_CXX=...`.
