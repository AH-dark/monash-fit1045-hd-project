#!/usr/bin/env bash
set -euo pipefail

install_macos_tools() {
  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required on macOS. Install it from https://brew.sh/ and rerun this script."
    exit 1
  fi

  brew install python cmake ninja llvm grpcurl
}

install_linux_tools() {
  if ! command -v apt-get >/dev/null 2>&1; then
    echo "Unsupported Linux distribution: apt-get is required by this bootstrap script."
    exit 1
  fi

  sudo apt-get update
  sudo apt-get install -y python3 python3-pip cmake ninja-build clang-format clang-tidy grpcurl
}

case "$(uname -s)" in
  Darwin)
    install_macos_tools
    ;;
  Linux)
    install_linux_tools
    ;;
  *)
    echo "Unsupported platform: $(uname -s)"
    exit 1
    ;;
esac

python3 - <<'PY'
import sys

if sys.version_info < (3, 10):
    raise SystemExit("Python 3.10 or newer is required")
PY

python3 -m pip install "conan==2.*"
conan profile detect --force

echo "Bootstrap complete."
