# cpp-lkm task runner
#
# Quick usage:
#   just --list
#   just setup-dev-env   # uv + bun dev toolchains (once per clone / after lock changes)
#   just build
#   just test
#   just qa              # all format + lint checks
#   just qa-fix          # auto-fix for each qa target
#   just smoke
#   just cmake-graph     # CMake target dependency graph (.dot + optional .svg)

set dotenv-load := false
set ignore-comments := true
set positional-arguments := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

# ------ Build Configuration ------
# Preset names match CMakePresets.json configurePresets.
# Use CMAKE_PRESET=platform just build  to switch the active preset.
CMAKE_PRESET := "host"
KBUILD_DIR := ""

KO_PATH := "builds/host-ko/my_kernel_module.ko"
MODULE_NAME := "my_kernel_module"

# ------ Core Targets ------
default:
  @just --list

clean:
  rm -rf builds

setup-dev-env:
  #!/usr/bin/env bash
  set -euo pipefail
  if ! command -v uv >/dev/null 2>&1; then
    echo "uv not found. Install from https://docs.astral.sh/uv/" >&2
    exit 2
  fi
  uv sync --group dev
  if [[ -n "${BUN_BIN:-}" && -x "${BUN_BIN}" ]]; then
    bun="${BUN_BIN}"
  elif [[ -x "${HOME}/.bun/bin/bun" ]]; then
    bun="${HOME}/.bun/bin/bun"
  elif command -v bun >/dev/null 2>&1; then
    bun="$(command -v bun)"
  else
    echo "bun not found. Install from https://bun.sh or set BUN_BIN (e.g. ~/.bun/bin/bun)" >&2
    exit 2
  fi
  "$bun" install

configure:
  cmake --preset "{{CMAKE_PRESET}}"

build: configure
  cmake --build --preset "{{CMAKE_PRESET}}"

# Regenerates Graphviz output for CMake targets (PUBLIC/INTERFACE/PRIVATE link edges).
# Requires configure first; outputs under builds/host. Install graphviz for SVG.
cmake-graph:
  #!/usr/bin/env bash
  set -euo pipefail
  cmake --preset "{{CMAKE_PRESET}}"
  cmake --graphviz="builds/host-ko/cmake-deps.dot" -S . -B builds/host-ko
  if command -v dot >/dev/null 2>&1; then
    dot -Tsvg "builds/host-ko/cmake-deps.dot" -o "builds/host-ko/cmake-deps.svg"
    echo "Wrote builds/host-ko/cmake-deps.dot and builds/host-ko/cmake-deps.svg"
  else
    echo "Wrote builds/host-ko/cmake-deps.dot (install graphviz for SVG: dot -Tsvg …)"
  fi

test: build
  ctest --preset "{{CMAKE_PRESET}}"

# ------ QA: Format Checks (Read-Only) ------
qa-format: qa-format-cpp qa-format-cmake

qa-format-cpp: configure
  cmake --build --preset "{{CMAKE_PRESET}}" --target format-check

qa-format-cmake:
  @scripts/cmake-format-check.sh

# ------ QA: Lint Checks ------
qa-lint: qa-lint-cpp qa-lint-markdown
  - just qa-lint-cmake

qa-lint-cpp: configure
  #!/usr/bin/env bash
  set -euo pipefail
  if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy not found. Install it (e.g. clang-tools-extra) to run lint."
    exit 2
  fi
  files="$(bash scripts/clang-tidy-files.sh || true)"
  if [[ -z "$files" ]]; then
    echo "No files found to lint."
    exit 0
  fi
  clang-tidy -p "builds/{{CMAKE_PRESET}}" $files

qa-lint-markdown:
  @bash scripts/markdownlint.sh

# CMake lint is optional (script skips if cmake-lint is unavailable).
qa-lint-cmake:
  @scripts/cmake-lint.sh

# ------ QA: Run All Checks ------
qa: qa-format qa-lint

# ------ QA: Auto-Fix Targets ------
qa-fix: qa-fix-format qa-fix-lint

qa-fix-format: qa-fix-format-cpp qa-fix-format-cmake

qa-fix-format-cpp: configure
  cmake --build --preset "{{CMAKE_PRESET}}" --target format

qa-fix-format-cmake:
  @scripts/cmake-format-fix.sh

qa-fix-lint: qa-fix-lint-cpp
  - just qa-fix-lint-markdown
  - just qa-fix-lint-cmake

# C++ lint "fix" = format (clang-tidy has limited fixes).
qa-fix-lint-cpp: qa-fix-format-cpp

qa-fix-lint-markdown:
  @bash scripts/markdownlint.sh --fix

# CMake lint fix = cmake-format -i.
qa-fix-lint-cmake:
  @scripts/cmake-format-fix.sh

# ------ Kernel Smoke Testing ------
#
# Builds the .ko (BUILD_KO=ON), then insmod, dmesg, and rmmod.
# Requires sudo, kernel-devel, and SecureBoot policies permitting module load.
#
# Useful overrides:
#   just KBUILD_DIR=/path/to/kernel/build smoke

ko:
  #!/usr/bin/env bash
  set -euo pipefail
  if [[ -n "{{KBUILD_DIR}}" ]]; then
    cmake --preset host-ko -D KBUILD_DIR="{{KBUILD_DIR}}"
  else
    cmake --preset host-ko
  fi
  cmake --build --preset host-ko

smoke: ko
  #!/usr/bin/env bash
  set -euo pipefail
  ko_path="{{KO_PATH}}"
  name="{{MODULE_NAME}}"
  if [[ ! -f "$ko_path" ]]; then
    echo "Missing $ko_path. Build failed?"
    exit 1
  fi
  before_epoch=$(date +%s)
  echo "== insmod =="
  sudo insmod "$ko_path"
  since_sec=$(( $(date +%s) - before_epoch + 1 ))
  echo "== dmesg (since ${since_sec}s ago, filtered) =="
  sudo dmesg --color=always -T --since "${since_sec} seconds ago" | sed -n '/\[CPP\]/p'
  echo "== rmmod =="
  sudo rmmod "$name"
  since_sec=$(( $(date +%s) - before_epoch + 1 ))
  echo "== dmesg (since ${since_sec}s ago, filtered) =="
  sudo dmesg --color=always -T --since "${since_sec} seconds ago" | sed -n '/\[CPP\]/p'

# Attempts to remove the module if it is loaded (useful after failed smoke).
smoke-clean:
  #!/usr/bin/env bash
  set -euo pipefail
  name="{{MODULE_NAME}}"
  if lsmod | awk '{print $1}' | grep -qx "$name"; then
    sudo rmmod "$name"
  else
    echo "$name not loaded"
  fi
