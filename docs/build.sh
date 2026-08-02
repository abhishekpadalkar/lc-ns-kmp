#!/usr/bin/env bash
# Build HTML docs. Optionally runs Doxygen first if available.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if command -v doxygen >/dev/null 2>&1; then
  echo "Running Doxygen..."
  doxygen Doxyfile
else
  echo "doxygen not found — building Sphinx without Breathe XML (C++ pages use hand-written API)."
fi

PYTHON="${PYTHON:-python3}"
if [[ -x "$ROOT/.venv/bin/python" ]]; then
  PYTHON="$ROOT/.venv/bin/python"
fi

"$PYTHON" -m sphinx -b html docs docs/_build/html "$@"
echo "Docs written to docs/_build/html/index.html"
