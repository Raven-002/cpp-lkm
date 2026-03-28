# Architecture Document

## Repository Layout

```
cpp-lkm/                   # Reusable framework subproject
  CMakeLists.txt           # Exposes cpp_lkm_create_kernel_interface() and
                           # cpp_lkm_add_ko_target(); builds cpp_lkm_runtime.
  cmake/
    CppLkmTargets.cmake    # Public CMake API (the two functions)
    CppLkmFlags.cmake      # Freestanding C++23 compiler flags
    CppLkmKernelDir.cmake  # Kernel build-dir resolution
    CppLkmKernelHeaders.cmake  # Kernel include paths + KBUILD_MODNAME
    CppLkmKernelAbi.cmake  # x86_64 ABI flags (-mcmodel=kernel, retpoline)
    CppLkmBridgeGen.cmake  # Bridge source generation (ko builds)
    CppLkmKbuild.cmake     # Kbuild staging + .ko custom target
    templates/
      module_bridge.cpp.in  # C++ bridge template (instantiated per module)
      linux_entry.c.in      # Kbuild C entry point template
  include/cpp_lkm/
    common/error.hpp        # ErrorCode, Result<T>, to_errno()
    runtime/kernel_api.h   # C ABI between C++ code and the kernel bridge
    runtime/kalloc.hpp     # kalloc<T>(), kalloc_array<T>(), kfree_obj()
    runtime/kernel_module.hpp  # IKernelModule interface
    runtime/module_entry.h # cpp_module_init / cpp_module_exit declarations
  src/
    operator_new.cpp       # Linker trap: defines operator delete, omits new

include/my_module/         # Project-specific public headers
  my_kernel_module.hpp     # MyKernelModule : public IKernelModule
  userspace_device.hpp     # UserspaceDevice (miscdevice abstraction)

src/                       # Project-specific implementation
  my_kernel_module.cpp     # MyKernelModule implementation
  userspace_device.cpp     # UserspaceDevice + C trampolines

compat/
  gsl_owner.hpp            # gsl::owner<T*> ownership annotation
  new_shim.hpp             # Freestanding placement new (only permitted form)

cmake/
  cpp/
    CheckExpected.cmake    # Verify native std::expected
    FormatTargets.cmake    # format / format-check / cmake-format targets
  testing/
    CheckNoBug.cmake           # BUG() usage gate (run at configure time)
    GenModuleBridge.cmake      # Host-mode bridge generation + linking
    NegativeCompileTest.cmake  # add_negative_compile_test macro
    NegativeCompileTest.cmake  # add_negative_link_test macro
    RunNegativeCompileTest.cmake
    RunNegativeLinkTest.cmake
  kernel/
    ToolchainCheck.cmake   # Compiler version enforcement per BUILD_MODE

tests/
  support/
    mock_kernel_bridge.cpp # Host-mode C bridge stubs (malloc, printf, chardev)
    mock_globals.hpp       # Externally-controllable mock state
    mock_chardev.hpp       # cpp_mock_chardev_simulate_{read,write}
  test_init.cpp
  test_allocator.cpp
  test_compat.cpp
  test_userspace_device.cpp
  test_nodiscard.cpp        # Negative compile fixture
  test_negative_new.cpp     # Negative link fixture
```

## Public CMake API

### `cpp_lkm_create_kernel_interface(<target> [MODULE_NAME <n>] [KDIR <path>] [ABI_MODE ko])`

Creates an `INTERFACE` target that carries:
- Freestanding C++23 flags (`-ffreestanding`, `-fno-exceptions`, `-fno-rtti`, …)
- Framework include directories (`cpp-lkm/include/`, repo root for `compat/`)
- `__KERNEL__`, `MODULE`, `KBUILD_MODNAME=<name>` compile definitions
- Kernel header paths (from `KDIR` arg, `CPP_LKM_KDIR` cache var, or `uname -r`)
- x86_64 ABI flags (`-mcmodel=kernel`, retpoline/thunk) **only** when `ABI_MODE ko`

### `cpp_lkm_add_ko_target(TARGET <lib> MODULE_NAME <n> MODULE_OBJECT <C> MODULE_HEADER <h> [ALL])`

Produces `<n>_ko` custom target and `<n>.ko` in the build dir. Internally:
1. Generates `module_bridge.cpp` from the template (instantiates `MODULE_OBJECT`).
2. Builds `<n>.bridge` OBJECT target.
3. Generates `linux_entry.c` (Kbuild entry point, `MODULE_LICENSE`, metadata).
4. Stages archives and generated Kbuild `Makefile` into `kbuild_<n>/`.
5. Invokes `make -C <kdir> M=<stage-dir> modules`.

## Two-Phase Initialization

The module uses a strictly defined two-phase initialization pattern:

1. **Phase 1 (Construction)**: The `MODULE_OBJECT` constructor is called via
   placement-new into a static buffer. This phase is trivial, performs zero
   allocations, and cannot fail.
2. **Phase 2 (Initialization)**: The `init()` method is called. This phase is
   fallible and may allocate internal resources, and returns
   `std::expected<void, ErrorCode>`. Any failures must be handled gracefully.

## IKernelModule Interface

Every kernel module implementation must inherit from `IKernelModule`:

```cpp
class IKernelModule {
public:
    virtual ~IKernelModule() = default;
    [[nodiscard]] virtual Result<void> init() = 0;
protected:
    IKernelModule() = default;
};
```

The generated `module_bridge.cpp` holds the static storage buffer (sized and
aligned to the concrete `MODULE_OBJECT` type) and calls `init()` / the
destructor. This is the **only** place that names the concrete type.

## Generated Bridge

`cpp_lkm_add_ko_target()` (and `cpp_lkm_gen_host_bridge()` for host tests)
calls `configure_file()` on `cpp-lkm/cmake/templates/module_bridge.cpp.in`,
substituting:
- `@CPP_LKM_MODULE_OBJECT@` → the class name passed as `MODULE_OBJECT`
- `@CPP_LKM_MODULE_HEADER@` → the header path passed as `MODULE_HEADER`

The resulting `module_bridge.cpp` compiles into the `<n>.bridge` OBJECT target
and provides `cpp_module_init` / `cpp_module_exit` with the correct concrete type.

## Context-Aware Allocator (`kalloc`)

Standard `new` is disabled (see Linker Trap section). All dynamic memory
allocations happen via `kalloc<T>()`, which returns a
`std::expected<T*, ErrorCode>`.
`kalloc` automagically queries the current CPU execution context (e.g.,
interrupts disabled, NMI, or atomic context) and selects the right `kmalloc`
bitmask (`GFP_KERNEL` or `GFP_ATOMIC`).

## The `operator new` Linker Trap

Global heap-form `operator new` and `new[]` are intentionally **declared but
not defined** in `cpp-lkm/src/operator_new.cpp`.
If any developer accidentally uses `new Foo{}` instead of `kalloc<Foo>()`, the
code will compile, but the linker will fail with an undefined reference to
`operator new`, catching the mistake at build time.

## The C/C++ Kernel Bridge

Kernel code is written in C. This module exposes its functionality through
exported C functions, mapped to `extern "C"` wrappers.

**Dual-Implementation Symmetry:**

- Generated `linux_entry.c` (from `linux_entry.c.in`) — The real bridge
  compiled by Kbuild. Contains `MODULE_LICENSE`, `module_init`/`module_exit`,
  and forwards to `cpp_module_init`/`cpp_module_exit`.
- `src/runtime/src/linux_bridge.c` no longer exists in the project root;
  its role (wrapping `kmalloc`, `printk`, chardev) is now part of the framework's
  `cpp_lkm/runtime/kernel_api.h` declaration surface. The actual Kbuild C
  implementation lives in the framework source.
- `tests/support/mock_kernel_bridge.cpp` — The host-mode counterpart. It
  forwards `cpp_kmalloc` to `malloc()` (with fail-injection capabilities),
  `cpp_printk` to `printf()`, etc.

When a new kernel API feature is needed, declare it in
`cpp-lkm/include/cpp_lkm/runtime/kernel_api.h`, implement it in the kernel
bridge (the real `.c` file in the Kbuild tree), and add a mock in
`tests/support/mock_kernel_bridge.cpp`.

## Extending the Module Implementation (`src/`)

1. **Types and files**: Add members, private helpers, or subsystems to
   `MyKernelModule` (or add new classes under `src/` and `include/my_module/`).
2. **Build system**: Add new `.cpp` sources to the `my_kernel_module` STATIC
   library in the root `CMakeLists.txt`.
3. **Initialization contract**: Keep Phase 1 construction trivial (no
   allocation); put fallible setup in `init()`; free everything in the
   destructor via `kfree_obj()`.
4. **Kernel surface**: Do not call Linux internals directly from C++ code.
   Declare needed operations in `cpp-lkm/include/cpp_lkm/runtime/kernel_api.h`
   and implement them in the kernel bridge + mock.
5. **Bridge**: Do not modify `module_bridge.cpp` — it is generated. Only change
   the `MODULE_OBJECT` / `MODULE_HEADER` arguments to `cpp_lkm_add_ko_target`.
