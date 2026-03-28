#!/usr/bin/env bash
# Run markdownlint-cli2 via the bun-managed package.json scripts, or pass explicit paths.
# Usage:
#   scripts/markdownlint.sh              # lint default globs (see package.json)
#   scripts/markdownlint.sh --fix        # fix default globs
#   scripts/markdownlint.sh path1.md ... # lint listed files (e.g. pre-commit)
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
cd "$root"

resolve_bun() {
  if [[ -n "${BUN_BIN:-}" && -x "${BUN_BIN}" ]]; then
    echo "${BUN_BIN}"
    return
  fi
  if [[ -x "${HOME}/.bun/bin/bun" ]]; then
    echo "${HOME}/.bun/bin/bun"
    return
  fi
  if command -v bun >/dev/null 2>&1; then
    command -v bun
    return
  fi
  echo "bun not found. Install from https://bun.sh or set BUN_BIN (e.g. ~/.bun/bin/bun)." >&2
  exit 2
}

BUN="$(resolve_bun)"

ensure_deps() {
  if [[ ! -x node_modules/.bin/markdownlint-cli2 ]]; then
    "$BUN" install
  fi
}

if [[ "${1:-}" == "--fix" ]]; then
  ensure_deps
  exec "$BUN" run fix:md
elif [[ $# -gt 0 ]]; then
  ensure_deps
  exec node_modules/.bin/markdownlint-cli2 "$@"
else
  ensure_deps
  exec "$BUN" run lint:md
fi
