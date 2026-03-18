#!/bin/bash
set -e

set -x

# Install build dependencies
apt-get update
apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libevdev-dev libglib2.0-dev libx11-dev

# Build
cd /tmp
cmake /io -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

# Copy result
mkdir -p /io/artifacts
cp src/waynaptics/waynaptics /io/artifacts/waynaptics
chown "${HOST_UID:-0}:${HOST_GID:-0}" /io/artifacts/waynaptics
