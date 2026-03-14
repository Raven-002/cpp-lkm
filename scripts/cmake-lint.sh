#!/usr/bin/env bash
# Run cmake-lint on all CMake sources.
set -euo pipefail
if ! command -v cmake-lint >/dev/null 2>&1; then
  echo "cmake-lint not found. Install with: pip install 'cmakelang[YAML]'" >&2
  exit 2
fi
cd "$(git rev-parse --show-toplevel)"
files=$(git ls-files 'CMakeLists.txt' 'cmake/*.cmake' 'cmake/*/*.cmake' 2>/dev/null || true)
if [[ -z "$files" ]]; then
  exit 0
fi
# shellcheck disable=SC2086
cmake-lint $files
