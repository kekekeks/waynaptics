#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ARTIFACTS="$REPO_DIR/artifacts"
INSTALL_DIR="$ARTIFACTS/install"

if [ ! -d "$INSTALL_DIR/daemon" ]; then
  echo "Error: $INSTALL_DIR/daemon not found. Run compile-inner.sh first." >&2
  exit 1
fi

docker run --rm \
  -v "$REPO_DIR:/io" \
  archlinux:latest \
  bash /io/scripts/package-arch-inner.sh
