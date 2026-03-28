# cpp-lkm task runner
#
# Quick usage:
#   just --list
#   just setup-dev-env   # uv + bun dev toolchains (once per clone / after lock changes)
#   just build
#   just test
#   just qa          # all format + lint checks
#   just qa-fix-*    # auto-fix for each qa target
#   just smoke
#   just cmake-graph   # CMake target dependency graph (.dot + optional .svg)

set dotenv-load := false
set ignore-comments := true
set positional-arguments := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

# ------ Build Configuration ------
BUILD_DIR := "build"
# Kernel module (.ko) build tree. Keep this separate from host tests so CMake
# reconfigure does not flip BUILD_KO unexpectedly.
KO_BUILD_DIR := "build-ko"
BUILD_MODE := "host"
BUILD_KO := "OFF" # Build the real .ko via Kbuild (default off in host mode)
KBUILD_DIR := ""

# ------ Core Targets ------
default:
  @just --list

clean:
  rm -rf "{{BUILD_DIR}}" "{{KO_BUILD_DIR}}"

setup-dev-env:
  if ! command -v uv >/dev/null 2>&1; then echo "uv not found. Install from https://docs.astral.sh/uv/" >&2; exit 2; fi && \
  uv sync --group dev && \
  if [[ -n "${BUN_BIN:-}" && -x "${BUN_BIN}" ]]; then bun="${BUN_BIN}"; \
  elif [[ -x "${HOME}/.bun/bin/bun" ]]; then bun="${HOME}/.bun/bin/bun"; \
  elif command -v bun >/dev/null 2>&1; then bun="$(command -v bun)"; \
  else echo "bun not found. Install from https://bun.sh or set BUN_BIN (e.g. ~/.bun/bin/bun)" >&2; exit 2; fi && \
  "$bun" install

configure:
  if [[ -n "{{KBUILD_DIR}}" ]]; then \
    cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO="{{BUILD_KO}}" -DKBUILD_DIR="{{KBUILD_DIR}}"; \
  else \
    cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO="{{BUILD_KO}}"; \
  fi

build: configure
  cmake --build "{{BUILD_DIR}}" -j

# Regenerates Graphviz output for CMake targets (PUBLIC/INTERFACE/PRIVATE link edges).
# Requires configure first; outputs under BUILD_DIR. Install graphviz for SVG.
cmake-graph: configure
  cmake --graphviz="{{BUILD_DIR}}/cmake-deps.dot" -S . -B "{{BUILD_DIR}}" && \
  if command -v dot >/dev/null 2>&1; then \
    dot -Tsvg "{{BUILD_DIR}}/cmake-deps.dot" -o "{{BUILD_DIR}}/cmake-deps.svg" && \
    echo "Wrote {{BUILD_DIR}}/cmake-deps.dot and {{BUILD_DIR}}/cmake-deps.svg"; \
  else \
    echo "Wrote {{BUILD_DIR}}/cmake-deps.dot (install graphviz for SVG: dot -Tsvg …)"; \
  fi

test: build
  ctest --test-dir "{{BUILD_DIR}}" -V

# ------ QA: Format Checks (Read-Only) ------
qa-format: qa-format-cpp qa-format-cmake

qa-format-cpp: configure
  cmake --build "{{BUILD_DIR}}" --target format-check

qa-format-cmake:
  scripts/cmake-format-check.sh

# ------ QA: Lint Checks ------
qa-lint: qa-lint-cpp qa-lint-markdown
  - just qa-lint-cmake

qa-lint-cpp:
  if ! command -v clang-tidy >/dev/null 2>&1; then echo "clang-tidy not found. Install it (e.g. clang-tools-extra) to run lint."; exit 2; fi && \
  cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
  files="$(bash scripts/clang-tidy-files.sh || true)" && \
  if [[ -z "$files" ]]; then echo "No files found to lint."; exit 0; fi && \
  clang-tidy -p "{{BUILD_DIR}}" $files

qa-lint-markdown:
  bash scripts/markdownlint.sh

# CMake lint is optional (script skips if cmake-lint is unavailable).
qa-lint-cmake:
  scripts/cmake-lint.sh

# ------ QA: Run All Checks ------
qa: qa-format qa-lint

# ------ QA: Auto-Fix Targets ------
qa-fix: qa-fix-format qa-fix-lint

qa-fix-format: qa-fix-format-cpp qa-fix-format-cmake

qa-fix-format-cpp: configure
  cmake --build "{{BUILD_DIR}}" --target format

qa-fix-format-cmake:
  scripts/cmake-format-fix.sh

qa-fix-lint: qa-fix-lint-cpp
  - just qa-fix-lint-markdown
  - just qa-fix-lint-cmake

# C++ lint "fix" = format (clang-tidy has limited fixes).
qa-fix-lint-cpp: qa-fix-format-cpp

qa-fix-lint-markdown:
  bash scripts/markdownlint.sh --fix

# CMake lint fix = cmake-format -i.
qa-fix-lint-cmake:
  scripts/cmake-format-fix.sh

# ------ Kernel Smoke Testing ------
#
# Builds the .ko (BUILD_KO=ON), then insmod, dmesg, and rmmod.
# Requires sudo, kernel-devel, and SecureBoot policies permitting module load.
#
# Useful overrides:
#   just KO_BUILD_DIR=build-ko-custom smoke
#   just KBUILD_DIR=/path/to/kernel/build smoke

ko:
  just BUILD_KO=ON BUILD_DIR="{{KO_BUILD_DIR}}" BUILD_MODE="{{BUILD_MODE}}" KBUILD_DIR="{{KBUILD_DIR}}" build

smoke: ko
  ko_path="{{KO_BUILD_DIR}}/cpp_lkm.ko" && \
  if [[ ! -f "$ko_path" ]]; then echo "Missing $ko_path. Build failed?"; exit 1; fi && \
  name="cpp_lkm" && \
  before_epoch=$(date +%s) && \
  echo "== insmod ==" && \
  sudo insmod "$ko_path" || { echo "insmod failed"; exit 1; } && \
  since_sec=$(( $(date +%s) - before_epoch + 1 )) && \
  echo "== dmesg (since ${since_sec}s ago, filtered) ==" && \
  sudo dmesg --color=never -T --since "${since_sec} seconds ago" | sed -n '/\[CPP\]/p' && \
  echo "== rmmod ==" && \
  sudo rmmod "$name" || { echo "rmmod failed"; exit 1; } && \
  since_sec=$(( $(date +%s) - before_epoch + 1 )) && \
  echo "== dmesg (since ${since_sec}s ago, filtered) ==" && \
  sudo dmesg --color=never -T --since "${since_sec} seconds ago" | sed -n '/\[CPP\]/p'

# Attempts to remove the module if it is loaded (useful after failed smoke).
smoke-clean:
  name="cpp_lkm" && \
  if lsmod | awk '{print $1}' | grep -qx "$name"; then sudo rmmod "$name"; else echo "$name not loaded"; fi
