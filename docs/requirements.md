# Requirements: Smart Modern C++ Kernel Module (Mocked)

## Revision 4 — Panic-Free Allocation & Typed Allocator Pattern

## 1. Project Overview

A C++23 kernel module framework targeting a freestanding environment. It uses
functional error handling via `std::expected`, avoids the C++ runtime (no
exceptions/RTTI), and implements a context-aware memory allocator that selects
the correct `kmalloc` GFP flags based on CPU execution context.

**Core design invariant:** No code path inside the module may trigger a kernel
panic (`BUG()`, `BUG_ON()`, `panic()`, or equivalent). All runtime failures —
including allocation failures — must be returned as `ErrorCode` values and
handled by the caller. The module must always be able to clean up after itself
and exit gracefully.

### 1.1 Compiler Support Matrix

The same C++ source must compile and pass tests under all three configurations
without modification:

| Mode | Compiler | Purpose |
| --- | --- | --- |
| **Host / Dev** | Clang 16+ or GCC 13+ (host) | Fast iteration, unit tests |
| **Platform / Native** | G++12 (12.5.x, target arch) | Platform build |
| **Platform / CI Test** | G++12 (12.5.x, target arch) | CI gate |

> **Architecture note:** The target platform architecture is opaque to this spec.
> The build system must not hardcode any arch-specific flags. Any arch-specific
> flags must be injected externally via `CMAKE_CXX_FLAGS` or a toolchain file
> provided by the platform owner.

### 1.2 Compatibility Philosophy

The project uses **idiomatic C++23 source throughout**, with minimal
freestanding compatibility shims in `compat/`.

**No source-level transpilation. No `#ifdef` noise in business logic.**

All compatibility decisions are made once, in `compat/`, and are invisible to
the rest of the codebase. When the toolchain improves, updating `compat/` is
the only change required.

---

## 2. Memory Allocation Architecture

This section is the most critical architectural change in Rev 4. Read it
carefully before implementing anything else.

### 2.1 The Problem with `operator new` in Kernel Context

Standard `operator new` has two behaviors under `-fno-exceptions`:

- It either returns a valid pointer, or
- It calls `std::terminate()` / invokes undefined behavior on null return.

Neither is acceptable here. Returning nullptr silently leads to
null-dereferences; calling terminate is a kernel panic. The throwing form is
unavailable. There is no safe way to use heap-form `operator new` in a
`-fno-exceptions` kernel module if you want graceful error recovery.

**Resolution: ban heap-form `operator new` entirely.**

### 2.2 `operator new` as a Linker Trap

The global heap-form `operator new` must be **declared but not defined**. Any TU
that accidentally calls `new Foo{}` will fail to link with an undefined
reference error, catching the mistake at build time rather than causing a panic
at runtime.

```cpp
// operator_new.cpp
// Heap-form operator new is intentionally NOT defined.
// If you are seeing a linker error referencing operator new(size_t),
// you have used 'new T{}' in business logic. Use kalloc<T>() instead.
//
// Placement new (operator new(size_t, void*)) is defined in compat/new_shim.hpp
// and is the only permitted form of new in this codebase.
void operator delete(void* p) noexcept;   // defined — wraps kfree
void operator delete(void* p, size_t) noexcept; // sized delete — same
```

> Do not provide a `operator new(size_t, std::nothrow_t)` nothrow overload
> either. It would bypass the linker trap and return a raw pointer that callers
> might forget to check.

The linker trap approach produces a clear, actionable error message pointing
exactly to the offending call site. It is strictly better than a runtime check.

### 2.3 The `kalloc<T>()` Helper

All dynamic allocation in business logic must go through `kalloc<T>()`. It
handles GFP flag selection, null checking, and object construction, and returns
a typed `std::expected` so failures propagate naturally through the
`std::expected` chain.

```cpp
// include/kalloc.hpp
#pragma once
#include <expected>
#include "error.hpp"
#include <linux/slab.h>   // kmalloc / kfree
#include <linux/preempt.h>

// Select the correct GFP flag for the current CPU context.
// Must never be called with a size of zero.
[[nodiscard]] inline gfp_t current_gfp_flags() noexcept {
    return (in_atomic() || irqs_disabled() || in_nmi())
           ? GFP_ATOMIC
           : GFP_KERNEL;
}

// Allocate memory for one T, construct it with args, and return a pointer.
// Returns ErrorCode::AllocFail if kmalloc returns null.
// The caller owns the returned pointer and must free it with kfree_obj<T>().
template<typename T, typename... Args>
[[nodiscard]] std::expected<T*, ErrorCode> kalloc(Args&&... args) noexcept {
    void* mem = kmalloc(sizeof(T), current_gfp_flags());
    if (!mem) [[unlikely]]
        return std::unexpected(ErrorCode::AllocFail);
    return new (mem) T{static_cast<Args&&>(args)...};  // placement new only
}

// Destroy and free a pointer previously returned by kalloc<T>().
// Safe to call with nullptr.
template<typename T>
void kfree_obj(T* p) noexcept {
    if (p) {
        p->~T();
        kfree(p);
    }
}
```

**Usage pattern in business logic:**

```cpp
// Before (forbidden):
auto* buf = new RingBuffer{size};   // linker trap — will not compile

// After (required):
auto result = kalloc<RingBuffer>(size);
if (!result) return std::unexpected(result.error());
RingBuffer* buf = *result;
// ... use buf ...
kfree_obj(buf);
```

### 2.4 `kalloc_array<T>()` for Fixed-Size Arrays

For allocating arrays of trivially constructible types (raw buffers, POD arrays):

```cpp
template<typename T>
[[nodiscard]] std::expected<T*, ErrorCode> kalloc_array(size_t count) noexcept {
    static_assert(std::is_trivially_default_constructible_v<T>,
        "kalloc_array requires trivially constructible types; use kalloc() for others");
    if (count == 0 || count > (std::numeric_limits<size_t>::max() / sizeof(T)))
        return std::unexpected(ErrorCode::AllocFail);
    void* mem = kmalloc(sizeof(T) * count, current_gfp_flags());
    if (!mem) [[unlikely]]
        return std::unexpected(ErrorCode::AllocFail);
    return static_cast<T*>(mem);
}
```

### 2.5 Placement New for the Global Instance

The global `CppKernelModule` instance is constructed via placement new into a
static buffer. This is the **only** placement new call that happens outside of
`kalloc()`. It is permitted because the static buffer is always valid and
construction of the module object itself must not fail (Phase 1 — trivial by
design).

```cpp
alignas(CppKernelModule) static unsigned char g_module_buf[sizeof(CppKernelModule)];
static CppKernelModule* g_module = nullptr;

// In cpp_module_init:
g_module = new (g_module_buf) CppKernelModule{};  // placement new, cannot fail
```

### 2.6 `BUG()` and `BUG_ON()` — Explicitly Forbidden

`BUG()`, `BUG_ON()`, `WARN_ON()` with side effects, `panic()`, and any
equivalent that halts or panics the kernel must **never** be called by module
code.

The mock `linux/kernel.h` must still define `BUG()` (to `abort()`) so that any
accidental use is visible and loud in testing. The CI test suite must include a
static analysis or grep gate that fails if any call to `BUG` or `BUG_ON`
appears in `src/` or `include/`.

```cmake
# CMakeLists.txt — BUG() usage gate
add_custom_target(check_no_bug
    COMMAND grep -rn "BUG(" ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
            && echo "FAIL: BUG() found in module source" && exit 1
            || echo "OK: no BUG() in module source"
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)
```

---

## 3. Core Architecture

### 3.1 Two-Phase Initialization

- **Phase 1 — Construction**: Trivially safe. No allocations. No `kalloc()`. No
  `kmalloc()`. No hardware access. Prints `[CPP] Constructed`.
- **Phase 2 — Initialization**: Fallible. Returns
  `std::expected<void, ErrorCode>`. May call `kalloc()`. Any failure must be
  returned — never swallowed, never panicked.

### 3.2 The `CppKernelModule` Class

```cpp
#include <expected>
#include "error.hpp"

class CppKernelModule {
public:
    CppKernelModule();                    // Phase 1: trivial, no alloc
    [[nodiscard]] std::expected<void, ErrorCode> init();  // Phase 2: fallible
    ~CppKernelModule();                   // Teardown, frees resources
private:
    bool _initialized = false;
    // Any resources allocated in init() must be tracked here
    // and freed in the destructor unconditionally.
};
```

**`was_successful()` is removed.** The `std::expected` return from `init()` is
the authoritative signal. A separate boolean check is redundant and creates a
two-source-of-truth problem: if `init()` returns a value but `was_successful()`
returns false (or vice versa due to a bug), the module is in an undefined
state. Callers must check the `expected` return and act on it immediately.

### 3.3 Destructor Invariant

The destructor must be safe to call **at any point after construction**,
including after a partial `init()` that failed midway. Resources must be
tracked with clear initialized/null state so the destructor can free whatever
was successfully allocated without double-freeing or dereferencing null.

```cpp
// Pattern: null-initialized pointers, unconditional free in destructor
class CppKernelModule {
    RingBuffer* _ring = nullptr;   // set in init(), freed in destructor
    DmaBuffer*  _dma  = nullptr;   // set in init(), freed in destructor
public:
    ~CppKernelModule() {
        kfree_obj(_ring);   // safe with nullptr
        kfree_obj(_dma);    // safe with nullptr
        printk(KERN_INFO "[CPP] Destructed\n");
    }
};
```

This means `module_exit` can call the destructor unconditionally — even if
`init()` failed partway through — and the module will clean up whatever it
allocated.

### 3.4 `ErrorCode` Enum

```cpp
enum class ErrorCode : int {
    None        =  0,
    AllocFail   = -12,   // -ENOMEM
};
```

Values are inlined to avoid depending on `<errno.h>` macros before mock
headers are included. `to_errno(err)` must be a valid `module_init`
return for all non-`None` values.

---

## 4. Error Handling & Constraints

### 4.1 `[[nodiscard]]` Policy

Every function returning `std::expected<T,E>` or `ErrorCode` must be
`[[nodiscard]]`. `-Werror=unused-result` must be enabled across all compilers
in all modes. Discarding an `expected` return is a compile error, not a
runtime risk.

### 4.2 Forbidden Constructs

| Forbidden | Reason |
| --- | --- |
| `new T{}` (heap form) | Linker trap — use `kalloc<T>()` instead |
| `new (std::nothrow) T{}` | Bypasses trap; returns unchecked raw pointer |
| `delete p` | Use `kfree_obj(p)` instead |
| `BUG()`, `BUG_ON()`, `panic()` | Never panic — propagate `ErrorCode` |
| `std::vector`, `std::string`, `std::map` | Heap-heavy; use runtime allocator |
| Exceptions | `-fno-exceptions` across all modes |
| RTTI, `dynamic_cast`, `typeid` | `-fno-rtti` across all modes |
| Global ctors with side effects | `module_init` controls init order |
| Floating point | No FPU without kernel_fpu_begin/end guards |
| `thread_local` | No conventional TLS in kernel context |
| `#ifdef` in src/include for compiler | Belongs in `compat/` only |
| Ignoring `std::expected` returns | `[[nodiscard]]` + `-Werror` enforces |

### 4.3 G++12-Specific Warnings

```text
-Wall -Wextra -Werror
-Wno-interference-size   # GCC 12 spurious; remove at GCC 13
-Werror=unused-result
-Werror=return-type
```

---

## 5. Kernel Bridge — C11 Interface

All entry points must be `extern "C"` with signatures valid as C11. No C++
types, mangled names, or overloaded symbols may appear at the ABI boundary.

```cpp
extern "C" {
    int  cpp_module_init(void);   // 0 on success, negative errno on failure
    void cpp_module_exit(void);
}
```

### 5.1 `cpp_module_init` Logic

```text
1. Placement-new CppKernelModule into g_module_buf. Assign g_module.
   (Construction is trivial and cannot fail.)

2. Call g_module->init().

3. On std::unexpected result:
   a. Call g_module->~CppKernelModule() explicitly.
      (Destructor handles partial cleanup safely — see §3.3.)
   b. Set g_module = nullptr.
   c. Return static_cast<int>(err.error()).
      (Negative errno — kernel sees a clean init failure, no panic.)

4. Return 0.
```

Note: `was_successful()` is no longer called. The `expected` return from step 2
is the complete signal. If `init()` returns a value, the module is valid. If it
returns unexpected, the error code is the reason.

### 5.2 `cpp_module_exit` Logic

```text
1. Call g_module->~CppKernelModule() explicitly.
   (Safe even if init() failed partway — destructor null-safe per §3.3.)
2. Set g_module = nullptr.
```

---

## 6. Compatibility Layer (`compat/`)

### 6.1 Native `<expected>`

`std::expected<T,E>` is required natively from the selected toolchain.
Configuration must fail if `__cpp_lib_expected < 202202L`.

### 6.2 `compat/new_shim.hpp`

Placement new only. Must not pull in libc or libstdc++ headers.

```cpp
// compat/new_shim.hpp
#pragma once
#include <stddef.h>
inline void* operator new  (size_t, void* p) noexcept { return p; }
inline void  operator delete(void*, void*)   noexcept {}
```

### 6.3 `compat/` Inventory

| File | Purpose |
| --- | --- |
| `compat/new_shim.hpp` | Freestanding placement new |

Policy: `#ifdef` for compiler/stdlib version detection belongs exclusively in
`compat/`. Never in `src/` or `include/`.

---

## 7. Build System

### 7.1 Three-Mode CMake Matrix

```bash
cmake -DBUILD_MODE=host      # Host compiler, mock headers, runs all tests
cmake -DBUILD_MODE=platform  # G++12.5.x, mock headers, runs all tests
cmake -DBUILD_MODE=ci        # Same as platform; non-zero exit on any failure
```

`BUILD_MODE=platform` and `BUILD_MODE=ci` require a GCC 12.5.x compiler
(for example `-DCMAKE_CXX_COMPILER=/path/to/g++-12.5`).

No arch flags are injected by CMake. Additional target flags must be passed via
`EXTRA_CXX_FLAGS`.

### 7.2 Common Compile Flags (All Modes)

```cmake
add_compile_options(
    -std=c++23
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -fno-stack-protector
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -Wall -Wextra -Werror
    -Werror=unused-result
    -Werror=return-type
)
```

### 7.3 G++12-Specific Flags

```cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13")
    add_compile_options(-Wno-interference-size)
    # TODO: remove when minimum GCC version reaches 13
endif()
```

### 7.4 `std::expected` Availability Probe

```cmake
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-std=c++23")
check_cxx_source_compiles("
    #include <expected>
    #if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
    #  error missing
    #endif
    int main() { std::expected<int,int> e{1}; return e.value(); }
" HAS_NATIVE_EXPECTED)

if(HAS_NATIVE_EXPECTED)
    message(STATUS "std::expected: native")
else()
    message(FATAL_ERROR "Native std::expected is required")
endif()
```

### 7.5 Toolchain Version Enforcement

```cmake
if(BUILD_MODE STREQUAL "host")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
       CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.0")
        message(FATAL_ERROR "Host Clang must be 16+")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
       CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        message(FATAL_ERROR "Host GCC must be 13+")
    endif()
endif()

if(BUILD_MODE STREQUAL "platform" OR BUILD_MODE STREQUAL "ci")
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Platform mode requires GCC")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.5" OR
       CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13.0")
        message(FATAL_ERROR "Platform mode requires GCC 12.5.x")
    endif()
endif()
```

### 7.6 `BUG()` Usage Gate

```cmake
add_custom_target(check_no_bug
    COMMAND bash -c
        "grep -rn 'BUG(' ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include \
         && echo 'FAIL: BUG() found in module source' && exit 1 \
         || echo 'OK: no BUG() in module source'"
    VERBATIM
)
add_dependencies(kernel_module check_no_bug)
```

### 7.7 Linker

- All modes: `-Wl,--no-undefined` — catches missing `operator new` definition at
  link time, which is the intended behavior of the linker trap.
- Platform mode: `-static` by default.

---

## 8. Mock Headers (`mock_kernel/`)

### 8.1 `linux/kernel.h`

`BUG()` is defined (to `abort()`) so that any accidental call is loud and
visible in tests. The CI BUG-gate (§7.6) ensures it never appears in source,
but the definition remains as a safety net for mock test runs.

```c
#pragma once
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define printk(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define KERN_INFO  "[INFO] "
#define KERN_ERR   "[ERR]  "

typedef unsigned int gfp_t;
#define GFP_KERNEL  0x1u
#define GFP_ATOMIC  0x2u

/* Inspectable by test_allocator.cpp to verify GFP flag selection */
extern gfp_t __mock_last_gfp;
/* Set to true to make the next kmalloc call return null */
extern int   __mock_kmalloc_fail;

static inline void* kmalloc(size_t size, gfp_t flags) {
    __mock_last_gfp = flags;
    if (__mock_kmalloc_fail) { __mock_kmalloc_fail = 0; return NULL; }
    return malloc(size);
}
static inline void kfree(void* p) { free(p); }

#define __init
#define __exit

/* BUG() for visibility in tests — must never appear in src/ or include/ */
#define BUG() \
    do { \
        fprintf(stderr, "BUG() at %s:%d\n", __FILE__, __LINE__); \
        abort(); \
    } while (0)
#define BUG_ON(cond) do { if (cond) BUG(); } while (0)
```

> **`__mock_kmalloc_fail`** is new in Rev 4. It allows `test_allocator.cpp` and
> `test_init.cpp` to inject a single allocation failure without patching malloc,
> enabling clean testing of the `kalloc<T>()` error path and the `init()`
> partial-failure + cleanup path.

### 8.2 `linux/preempt.h`

```c
#pragma once
#include <stdint.h>

extern int __mock_preempt_count;
extern int __mock_irqs_disabled;
extern int __mock_in_nmi;

static inline int in_atomic(void)      { return __mock_preempt_count != 0; }
static inline int irqs_disabled(void)  { return __mock_irqs_disabled != 0; }
static inline int in_nmi(void)         { return __mock_in_nmi != 0; }
```

### 8.3 `linux/module.h`

```c
#pragma once

extern int  cpp_module_init(void);
extern void cpp_module_exit(void);

#define module_init(fn)
#define module_exit(fn)
#define MODULE_LICENSE(s)
#define MODULE_AUTHOR(s)
#define MODULE_DESCRIPTION(s)
```

### 8.4 `linux/errno.h`

```c
#pragma once
#define ENOMEM  12
#define EIO      5
#define EINVAL  22
```

---

## 9. Testing Requirements

### 9.1 Test Gate Policy

No platform build is valid unless all tests pass under G++12. `BUILD_MODE=ci`
enforces this as a hard gate.

### 9.2 Required Test Cases

**`tests/test_init.cpp` — Two-phase initialization and cleanup:**

| Test | Scenario | Expected outcome |
| --- | --- | --- |
| `test_happy_path` | Normal init | `init()` returns value; `g_module` valid |
| `test_alloc_fail_in_init` | kmalloc_fail=1 before init | AllocFail; cleanup |
| `test_partial_init_cleanup` | First alloc ok, second fails | No double-free |
| `test_exit_after_failed_init` | exit after failed init | Null-safe dtor |
| `test_destructor_print` | Full lifecycle | `[CPP] Destructed` once |
| `test_errno_mapping` | `to_errno(ErrorCode::AllocFail)` | Match Linux errno |

**`tests/test_allocator.cpp` — `kalloc<T>()` and GFP flag selection:**

| Test | Scenario | Expected outcome |
| --- | --- | --- |
| `test_kalloc_success` | Normal process context | __mock_last_gfp==GFP_KERNEL |
| `test_kalloc_atomic` | __mock_preempt_count=1 | __mock_last_gfp==GFP_ATOMIC |
| `test_kalloc_irqs_disabled` | irqs_disabled=1 | GFP_ATOMIC |
| `test_kalloc_nmi` | in_nmi=1 | GFP_ATOMIC |
| `test_gfp_priority_with_multiple_signals` | preempt+irq+nmi set | GFP_ATOMIC |
| `test_kalloc_fail` | kmalloc_fail=1 | unexpected(AllocFail) |
| `array_fail` | `kalloc_array` with kmalloc_fail=1 | unexpected(AllocFail) |
| `array_overflow_guard` | `kalloc_array` count overflow | AllocFail |
| `test_kfree_obj_null` | kfree_obj(nullptr) | No-op |
| `test_kfree_obj_valid` | kfree_obj(kalloc result) | Destructor + free |

**`tests/test_compat.cpp` — Compatibility layer:**

| Test | Scenario | Expected outcome |
| --- | --- | --- |
| `test_expected_value` | `expected<int,int>` value | .value() correct |
| `test_expected_error` | unexpected(AllocFail) | .error()==AllocFail |
| `test_expected_monadic` | .and_then/.or_else | Propagation |
| `test_nodiscard` | Discard [[nodiscard]] return | Compile must fail |
| `test_no_raw_new` | TU calls new Foo{} | Link must fail |

> **`test_no_raw_new`** is a negative *link* test (not compile). It verifies the
> linker trap. The test TU calls `new int{}` and the test asserts linking fails.

### 9.3 Running Tests

```bash
# Host mode
cmake -B build -DBUILD_MODE=host
cmake --build build
ctest --test-dir build -V

# Platform mode (G++12)
cmake -B build-platform -DBUILD_MODE=platform \
  -DCMAKE_CXX_COMPILER=/path/to/g++-12.5
cmake --build build-platform
ctest --test-dir build-platform -V

# CI gate
cmake -B build-ci -DBUILD_MODE=ci -DCMAKE_CXX_COMPILER=/path/to/g++-12.5
cmake --build build-ci
ctest --test-dir build-ci --output-on-failure
```

---

## 10. Directory Structure

```text
project/
├── CMakeLists.txt
├── cmake/
│   ├── cpp/
│   │   ├── CxxFlags.cmake
│   │   ├── CheckExpected.cmake       # native std::expected enforcement
│   │   └── FormatTargets.cmake
│   └── testing/
│       └── NegativeCompileTest.cmake # Negative compile/link test helpers
├── compat/
│   └── new_shim.hpp                   # Freestanding placement new
├── include/
│   ├── module.hpp                     # CppKernelModule declaration
│   ├── kalloc.hpp                 # kalloc<T>(), kfree_obj<T>(), current_gfp_flags()
│   └── error.hpp                     # ErrorCode enum
├── src/
│   ├── module.cpp                    # CppKernelModule implementation
│   ├── operator_new.cpp          # operator delete only; operator new absent
│   └── bridge.cpp                    # extern "C" entry points
├── mock_kernel/
│   └── linux/
│       ├── kernel.h                  # printk, kmalloc, kfree, BUG (mock)
│       ├── preempt.h             # in_atomic, irqs_disabled, in_nmi
│       ├── module.h                  # module_init/exit macros
│       └── errno.h                   # ENOMEM, EIO, EINVAL
└── tests/
    ├── test_init.cpp
    ├── test_allocator.cpp
    └── test_compat.cpp
```

---

## 11. Known Limitations & Future Work

| Item | Status | Notes |
| --- | --- | --- |
| Real Kbuild integration | Out of scope | __init/__exit, Kbuild Makefile |
| FPU guards | Not mocked | Stub kernel_fpu_begin/end |
| SMP / per-CPU data | Not addressed | this_cpu_ptr etc. |
| NMI allocator strictness | Partially modeled | GFP_ATOMIC insufficient |
| GCC 12 -Wno-interference-size | Workaround | Remove at GCC 13 |
