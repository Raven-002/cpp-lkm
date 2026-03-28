#!/usr/bin/env bash
# Run cmake-lint on all CMake sources.
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
# shellcheck disable=SC2086
uv run cmake-lint $files
