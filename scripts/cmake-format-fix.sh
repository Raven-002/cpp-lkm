#!/usr/bin/env bash
# Run cmake-format -i on all CMake sources (in-place fix).
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
for f in $files; do
  [[ -f "$f" ]] && uv run --group dev cmake-format -i "$f"
done
