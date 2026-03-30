# Consumer kernel API (`cpp_*` bridge)

This directory is **project-owned tier (c)** in the build: C headers and sources for APIs consumed
by module code through `<kernel_api/kernel_api.h>` (logging/chardev/assert and any project-specific
extensions).

| Tier | What | Build |
|------|------|--------|
| **(a)** | `linux_entry.c` — `module_init` / `module_exit` | Kbuild only |
| **(b)** | `module_bridge.cpp` — `cpp_module_init` / `cpp_module_exit` | CMake → `.a` for Kbuild |
| **(c)** | This tree — `kernel_api/*.h` + `src/*.c` | CMake `kernel_api_iface`; **`.c` compiled only by Kbuild** for the real `.ko` |
| **(d)** | `src/kernel_module/` — module logic | CMake `STATIC` → `.a` for Kbuild |

## Layout

- `include/kernel_api/` — Public C ABI: `types.h`, `memory.h`, `chardev.h`, umbrella `kernel_api.h`, …
- `src/*.c` — Implementations (`cpp_printk`, chardev, assert, …). Not compiled by the host toolchain
  for the module; `cpp_lkm_add_ko_target()` copies them into the Kbuild stage directory.

## CMake

- [`CMakeLists.txt`](CMakeLists.txt) defines **`kernel_api_iface`** (`INTERFACE`) for `<kernel_api/…>`
  includes and **`kernel_api_sources`** as the source-holder target for staged `.c` files.
- The top-level project links this into the kernel interface target for module-level includes:

  ```cmake
  target_link_libraries(my_kernel_module.iface INTERFACE kernel_api_iface)
  ```

## Kbuild

- Pass `KBUILD_SOURCE_TARGETS kernel_api_sources` to `cpp_lkm_add_ko_target()`.
- Pass `KBUILD_INCLUDE_DIRS <path-to-include>` so staged `.c` files resolve
  `#include "kernel_api/..."`.

`cpp-lkm` runtime now owns its own minimal memory/context bridge under
`cpp-lkm/include/cpp_lkm/runtime/kernel_api/` and stages its required C sources automatically.

Host tests link `tests/support/mock_kernel_bridge.cpp` instead of these `.c` files.

Add a new API by extending `include/kernel_api/*.h`, implementing `src/*.c`, updating the mock, and
refreshing the umbrella header if you add a new header file.
