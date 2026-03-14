#!/usr/bin/env bash
# Run cmake-format --check on all CMake sources. Exit non-zero if any need formatting.
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
fail=0
for f in $files; do
  if [[ -f "$f" ]] && ! cmake-format --check "$f"; then
    fail=1
  fi
done
exit $fail
