#!/usr/bin/env bash
set -euo pipefail

# Configure (Release build, default install prefix /usr/local)
cmake -S . -B build/linux-x64 -DCMAKE_BUILD_TYPE=Release

# Build with all CPU cores
cmake --build build/linux-x64 -j"$(nproc)"
