---
title: "Installation"
weight: 1
---

# Installation Guide

## System Requirements

- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- vcpkg (optional, for dependency management)

## Build from Source

### 1. Clone Repository

```bash
git clone https://github.com/Okysu/NovaMark.git
cd NovaMark
```

### 2. Install Dependencies

Using vcpkg:

```bash
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Install dependencies
./vcpkg/vcpkg install
```

### 3. Build Project

```bash
# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release
```

### 4. Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## Verify Installation

```bash
./build/src/cli/nova-cli --version
```

The output should display version information.

## Next Steps

- [Quick Start](../quickstart/) - Create your first game
