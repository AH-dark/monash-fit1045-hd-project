#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy is required. Run ./scripts/bootstrap.sh first."
  exit 1
fi

if [[ ! -f compile_commands.json && ! -f build/debug/compile_commands.json ]]; then
  echo "compile_commands.json was not found. Configure the project before linting."
  exit 1
fi

build_dir="."
if [[ -f build/debug/compile_commands.json ]]; then
  build_dir="build/debug"
fi

find server client-cli shared \
  -type f \( -name "*.hpp" -o -name "*.cpp" \) \
  -print0 | xargs -0 clang-tidy -p "${build_dir}"
