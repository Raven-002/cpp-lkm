# C++23 Kernel Module Framework (Mocked)

A C++23 kernel module framework targeting a freestanding environment. It uses functional error handling via `std::expected`, avoids the C++ runtime (no exceptions/RTTI), and implements a context-aware memory allocator.

## Design Philosophy
- **Panic-Free**: No code path inside the module may trigger a kernel panic (`BUG()`, `BUG_ON()`, `panic()`). All runtime failures must be returned and handled gracefully.
- **No Exceptions/RTTI**: The module relies strictly on `std::expected` for error propagation.
- **Linker Trap for `operator new`**: Heap-form `operator new` is an intentional linker trap to prevent accidental raw panicking allocations. Use `kalloc<T>()` instead.
- **Idiomatic C++23**: The codebase is written in idiomatic C++23, using a thin `compat/` layer to polyfill missing `<expected>` support on GCC 12.

For deeper architectural details, see [docs/architecture.md](docs/architecture.md).
For the technical requirements specification, see [docs/requirements.md](docs/requirements.md).

## Build Modes

This project supports a three-mode build matrix:

### 1. Host / Dev Mode (Default)
Compiles using your host compiler (Clang 16+ or GCC 13+) against mock kernel headers. Ideal for rapid iteration and unit testing.
```bash
cmake -B build -DBUILD_MODE=host
cmake --build build
ctest --test-dir build -V
```

### 2. Platform Mode
Compiles using a GCC 12 cross-compiler targeting the actual platform architecture, but still links against mock headers for CI test validation.
```bash
cmake -B build-platform -DBUILD_MODE=platform -DGCC12_CXX=/path/to/g++12
cmake --build build-platform
ctest --test-dir build-platform -V
```

### 3. CI Mode
Identical to platform mode but acts as a mandatory test gate before release.

## Directory Structure
- `compat/`: Polyfills and compiler workarounds (e.g. `tl::expected` for GCC 12).
- `mock_kernel/`: Mock Linux kernel headers for host-side compilation and testing.
- `include/`: Core module headers (`kalloc.h`, `module.h`, `error.h`).
- `src/`: Core implementation (`module.cpp`, `operator_new.cpp`, `bridge.cpp`).
- `tests/`: 21 test cases encompassing initialization, allocation, and negative compile/link tests.
- `third_party/`: Vendored dependencies (`tl::expected`).
