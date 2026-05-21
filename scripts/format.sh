#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required. Run ./scripts/bootstrap.sh first."
  exit 1
fi

find server client-cli shared \
  -type f \( -name "*.hpp" -o -name "*.cpp" \) \
  -print0 | xargs -0 clang-format -i
