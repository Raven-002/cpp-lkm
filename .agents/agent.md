# cpp-lkm Developer Guidelines

Normative authority for project policy is `docs/requirements.md`. This file is
an operational summary for contributors/agents. If guidance conflicts, follow
`docs/requirements.md` and then synchronize this summary.

## Architecture & Mocking

This repository is a C++23 kernel module framework targeting a freestanding
environment, with a host-side mocked mode for rapid iteration. The toolchain
must provide native `<expected>`. The `compat/` directory remains only for
freestanding shims (for example, placement new support).

## Core Rules

1. **No Kernel Panics**: `BUG()`, `BUG_ON()`, `panic()` must **never** be
   called in module source. The `check_no_bug` target enforces this.
2. **No Exceptions or RTTI**: `-fno-exceptions` and `-fno-rtti` are always active.
3. **No Heap `operator new`**: It is configured as a linker trap. Attempting
   to use `new T{}` in `src/` or `include/` will fail to link.
4. **Use `kalloc<T>()`**: All heap allocation goes through `kalloc<T>()` which
   returns `std::expected<T*>`. Free using `kfree_obj(p)`.
5. **Fallible Initialization**: Phase 1 initialization (construction) must be
   trivial and fallible operations belong in the `init()` Phase 2 method.
6. **No `#ifdef` Noise**: Any unavoidable compatibility detection must be
   localized and kept minimal.

## Expanding module C++ logic

The **module’s own behavior** (everything beyond runtime/bridge glue) belongs
under `src/module/`: `include/cpp_lkm/module/` and `src/module/src/`. The
reference shape is `CppKernelModule` in `module.hpp` / `module.cpp`.

- **Add code**: extend `CppKernelModule` and/or add new `.cpp`/`.hpp` files next
  to the existing module sources for cohesive subsystems.
- **Wire the build**: register every new translation unit in
  `src/module/CMakeLists.txt` (`add_library(module_core OBJECT ...)`) and list
  new public headers in that target’s `FILE_SET HEADERS` `target_sources`.
- **Lifecycle**: trivial constructor (Phase 1); allocations and other fallible
  work in `init()` returning `Result<void>`; teardown in the destructor (use
  `kalloc` / `kfree_obj`, not `new`).
- **Kernel calls**: go through the C bridge (`kernel_api.h` and the real vs.
  mock implementations)—same pattern as in `docs/architecture.md`.
- **Entry glue**: `src/runtime/src/bridge.cpp` placement-news `CppKernelModule`
  and drives `init()`/destruction; touch it only if global lifetime or the
  exported C entry points must change.

## Testing & Workflows

- **Host mode**: `cmake -B build -DBUILD_MODE=host`, `cmake --build build`,
  `ctest --test-dir build`
- **Platform/CI mode**: Requires GCC 12.5.x (`-DBUILD_MODE=platform` or
  `-DBUILD_MODE=ci`).
- **Lint parity rule**: Keep `scripts/clang-tidy-files.sh` as the single source
  of truth for clang-tidy target file selection used by both `just` and GitHub
  Actions.
