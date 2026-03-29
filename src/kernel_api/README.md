# Consumer kernel API (`cpp_*` bridge)

This directory is **tier (c)** in the build: C headers and sources that implement the symbols used
by C++ through `cpp_lkm/runtime/kernel_api.h` (a thin shim to `<kernel_api/kernel_api.h>`).

| Tier | What | Build |
|------|------|--------|
| **(a)** | `linux_entry.c` — `module_init` / `module_exit` | Kbuild only |
| **(b)** | `module_bridge.cpp` — `cpp_module_init` / `cpp_module_exit` | CMake → `.a` for Kbuild |
| **(c)** | This tree — `kernel_api/*.h` + `src/*.c` | CMake `kernel_api_iface`; **`.c` compiled only by Kbuild** for the real `.ko` |
| **(d)** | `src/kernel_module/` — module logic | CMake `STATIC` → `.a` for Kbuild |

## Layout

- `include/kernel_api/` — Public C ABI: `types.h`, `memory.h`, `chardev.h`, umbrella `kernel_api.h`, …
- `src/*.c` — Implementations (`cpp_printk`, `cpp_kmalloc`, chardev, …). Not compiled by the host
  toolchain for the module; `cpp_lkm_add_ko_target()` copies them into the Kbuild stage directory.

## CMake

- [`CMakeLists.txt`](CMakeLists.txt) defines **`kernel_api_iface`** (`INTERFACE`): include directory for
  `<kernel_api/…>`. A **`kernel_api_sources`** custom target lists the `.c` files for the IDE only
  (those files must not be compiled with the host toolchain; Kbuild compiles them for the `.ko`).
- The top-level project must link this into **`cpp_lkm_runtime`** and the kernel interface target so
  the shim and `kalloc.hpp` resolve `<kernel_api/kernel_api.h>`:

  ```cmake
  target_link_libraries(cpp_lkm_runtime PUBLIC kernel_api_iface)
  target_link_libraries(my_kernel_module.iface INTERFACE kernel_api_iface)
  ```

## Kbuild

- Pass `KERNEL_API_SRC_DIR` to `cpp_lkm_add_ko_target()` (this `src/` directory).
- `KERNEL_API_INCLUDE_DIR` defaults to `../include` next to that `src/` so Kbuild gets
  `ccflags-y += -I…/include` for `#include "kernel_api/…"`.

Host tests link `tests/support/mock_kernel_bridge.cpp` instead of these `.c` files.

Add a new API by extending `include/kernel_api/*.h`, implementing `src/*.c`, updating the mock, and
refreshing the umbrella header if you add a new header file.
