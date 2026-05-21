# FIT1045 HD Broadcast Messaging System

![CI](https://github.com/AH-dark/monash-fit1045-hd-project/actions/workflows/ci.yml/badge.svg)

`bcmd` is a C++23 broadcast messaging server and `bcli` is its command-line client. The
project is scaffolded for a hexagonal architecture with gRPC transport, Conan-managed native
dependencies, and Catch2-based tests.

## Quick Start

### Prerequisites
Run `./scripts/bootstrap.sh` to install Conan 2, CMake, Ninja, clang-format, clang-tidy, and grpcurl.

### Build
```bash
# Install dependencies (first run ~10 min for gRPC; cached thereafter)
conan install . --profile=conan_profiles/dev-macos --build=missing -of=build/debug

# Configure and build
cmake --preset debug
cmake --build --preset debug

# Run tests
ctest --preset debug --output-on-failure
```

### Run
```bash
# Generate dev TLS certificates
bash scripts/gen-dev-certs.sh

# Start server
./build/debug/bcmd --bind 0.0.0.0:50051 \
  --cert certs/dev/server.pem --key certs/dev/server.key

# Connect client (in another terminal)
./build/debug/bcli --server localhost:50051 \
  --ca certs/dev/ca.pem --username alice
```

See [docs/tls-setup.md](docs/tls-setup.md) for TLS details and [docs/architecture.md](docs/architecture.md) for system design.

## Repository Status

- Server binary: `bcmd`
- Client binary: `bcli`
- Package manager: Conan 2
- Build system: CMake 3.27+
- Primary local platform: macOS arm64

## Bootstrap

```bash
./scripts/bootstrap.sh
```

The bootstrap script installs Conan 2, detects a default Conan profile, and installs common C++
tooling such as CMake, Ninja, clang-format, clang-tidy, and grpcurl.

## Build

Generate Conan toolchain files first, then use the `debug` or `release` CMake presets.

```bash
conan install . --profile=conan_profiles/dev-macos --build=missing -of=build/debug
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

```bash
conan install . --profile=conan_profiles/release-linux --build=missing -of=build/release
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

## Development Commands

```bash
./scripts/format.sh
./scripts/lint.sh
```

## Remote

TODO: Add the `origin` remote once the repository has been provisioned.
