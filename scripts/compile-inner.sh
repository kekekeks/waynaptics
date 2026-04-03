#!/bin/bash
set -e
set -x

# Install build dependencies
apt-get update
apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libevdev-dev libglib2.0-dev libx11-dev \
  qt6-base-dev

# Build with config tool
BUILD_DIR=/tmp/build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake /io -DCMAKE_BUILD_TYPE=Release -DBUILD_CONFIG_TOOL=ON
make -j"$(nproc)"

# Install into staging area with component-based install
INSTALL_DIR=/io/artifacts/install
rm -rf "$INSTALL_DIR"

# Install daemon component
DESTDIR="$INSTALL_DIR/daemon" cmake --install . --prefix /usr --component daemon

# Install config component
DESTDIR="$INSTALL_DIR/config" cmake --install . --prefix /usr --component config

# Full install for tarball
DESTDIR="$INSTALL_DIR/full" cmake --install . --prefix /usr

chown -R "${HOST_UID:-0}:${HOST_GID:-0}" /io/artifacts
