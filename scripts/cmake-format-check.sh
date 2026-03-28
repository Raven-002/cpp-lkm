#!/usr/bin/env bash
# Run cmake-format --check on all CMake sources. Exit non-zero if any need formatting.
set -euo pipefail
if ! command -v uv >/dev/null 2>&1; then
  echo "uv not found. Install from https://docs.astral.sh/uv/ then run: uv sync --group dev" >&2
  exit 2
fi
cd "$(git rev-parse --show-toplevel)"
files=$(
  git ls-files 'CMakeLists.txt' '**/CMakeLists.txt' 'cmake/**/*.cmake' 2>/dev/null || true
)
if [[ -z "$files" ]]; then
  exit 0
fi
fail=0
for f in $files; do
  if [[ -f "$f" ]] && ! uv run cmake-format --check "$f"; then
    fail=1
  fi
done
exit $fail
