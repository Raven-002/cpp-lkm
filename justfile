# cpp-lkm task runner
#
# Usage:
#   just --list
#   just build
#   just test
#   just qa          # all format + lint checks
#   just qa-fix-*    # auto-fix for each qa target
#   just smoke

set dotenv-load := false
set ignore-comments := true
set positional-arguments := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

BUILD_DIR := "build"
BUILD_MODE := "host"

# Build the real .ko via Kbuild (default off in host mode in CMake)
BUILD_KO := "OFF"

KBUILD_DIR := ""

default:
  @just --list

clean:
  rm -rf "{{BUILD_DIR}}"

configure:
  if [[ -n "{{KBUILD_DIR}}" ]]; then \
    cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO="{{BUILD_KO}}" -DKBUILD_DIR="{{KBUILD_DIR}}"; \
  else \
    cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO="{{BUILD_KO}}"; \
  fi

build: configure
  cmake --build "{{BUILD_DIR}}" -j

test: build
  ctest --test-dir "{{BUILD_DIR}}" -V

# --- QA: format checks (read-only) ---
qa-format: qa-format-cpp qa-format-cmake

qa-format-cpp: configure
  cmake --build "{{BUILD_DIR}}" --target format-check

qa-format-cmake:
  scripts/cmake-format-check.sh

# --- QA: lint checks ---
qa-lint: qa-lint-cpp qa-lint-markdown
  - just qa-lint-cmake

qa-lint-cpp:
  if ! command -v clang-tidy >/dev/null 2>&1; then echo "clang-tidy not found. Install it (e.g. clang-tools-extra) to run lint."; exit 2; fi && \
  cmake -B "{{BUILD_DIR}}" -DBUILD_MODE="{{BUILD_MODE}}" -DBUILD_KO=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
  files="$(git ls-files 'src/*.cpp' 'include/*.h' 'tests/*.cpp' 'tests/*.h' 'compat/*.h' | grep -v 'tests/test_nodiscard\.cpp' || true)" && \
  if [[ -z "$files" ]]; then echo "No files found to lint."; exit 0; fi && \
  clang-tidy -p "{{BUILD_DIR}}" $files

qa-lint-markdown:
  (command -v markdownlint-cli2 >/dev/null 2>&1 && markdownlint-cli2 "docs/**/*.md" "README.md") || npx --yes markdownlint-cli2 "docs/**/*.md" "README.md"

# CMake lint (optional; skip if cmake-lint not installed)
qa-lint-cmake:
  scripts/cmake-lint.sh

# --- QA: run all checks ---
qa: qa-format qa-lint

# --- QA fix: auto-fix (apply formatters / fixers) ---
qa-fix: qa-fix-format qa-fix-lint

qa-fix-format: qa-fix-format-cpp qa-fix-format-cmake

qa-fix-format-cpp: configure
  cmake --build "{{BUILD_DIR}}" --target format

qa-fix-format-cmake:
  scripts/cmake-format-fix.sh

qa-fix-lint: qa-fix-lint-cpp
  - just qa-fix-lint-markdown
  - just qa-fix-lint-cmake

# C++ lint "fix" = format (clang-tidy has limited fixes)
qa-fix-lint-cpp: qa-fix-format-cpp

qa-fix-lint-markdown:
  (command -v markdownlint-cli2 >/dev/null 2>&1 && markdownlint-cli2 --fix "docs/**/*.md" "README.md") || npx --yes markdownlint-cli2 --fix "docs/**/*.md" "README.md"

# CMake lint fix = cmake-format -i
qa-fix-lint-cmake:
  scripts/cmake-format-fix.sh

# --- Kernel smoke testing ---
#
# Builds the .ko (BUILD_KO=ON), then insmod, dmesg, rmmod. Requires: sudo, kernel-devel,
# and SecureBoot policies permitting module loading.
#
# Override KBUILD_DIR, BUILD_DIR, or BUILD_MODE as needed:
#   just BUILD_DIR=build-ko smoke
#   just KBUILD_DIR=/path/to/kernel/build smoke

ko:
  just BUILD_KO=ON BUILD_DIR="{{BUILD_DIR}}" BUILD_MODE="{{BUILD_MODE}}" KBUILD_DIR="{{KBUILD_DIR}}" build

smoke: ko
  ko_path="{{BUILD_DIR}}/cpp_lkm.ko" && \
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

# Attempts to remove the module if it is loaded (useful after a failed smoke run).
smoke-clean:
  name="cpp_lkm" && \
  if lsmod | awk '{print $1}' | grep -qx "$name"; then sudo rmmod "$name"; else echo "$name not loaded"; fi
