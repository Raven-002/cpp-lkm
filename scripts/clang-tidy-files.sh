#!/usr/bin/env bash
# Print clang-tidy target files (space-separated), excluding negative tests.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

git ls-files 'src/*.cpp' 'include/*.hpp' 'tests/*.cpp' 'tests/*.hpp' 'compat/*.hpp' \
  | grep -Ev 'tests/test_nodiscard\.cpp|tests/test_negative_new\.cpp' \
  | while IFS= read -r f; do
      [[ -f "$f" ]] && printf '%s\n' "$f"
    done
