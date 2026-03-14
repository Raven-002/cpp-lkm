#!/usr/bin/env bash
# Run cmake-format -i on all CMake sources (in-place fix).
set -euo pipefail
if ! command -v cmake-format >/dev/null 2>&1; then
  echo "cmake-format not found. Install with: pip install 'cmakelang[YAML]'" >&2
  exit 2
fi
cd "$(git rev-parse --show-toplevel)"
files=$(git ls-files 'CMakeLists.txt' 'cmake/*.cmake' 'cmake/*/*.cmake' 2>/dev/null || true)
if [[ -z "$files" ]]; then
  exit 0
fi
for f in $files; do
  [[ -f "$f" ]] && cmake-format -i "$f"
done
