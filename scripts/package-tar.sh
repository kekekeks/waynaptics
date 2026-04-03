#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ARTIFACTS="$REPO_DIR/artifacts"
INSTALL_DIR="$ARTIFACTS/install"

if [ ! -d "$INSTALL_DIR/full" ]; then
  echo "Error: $INSTALL_DIR/full not found. Run compile-inner.sh first." >&2
  exit 1
fi

STAGING="$(mktemp -d)"
trap "rm -rf '$STAGING'" EXIT

cp -a "$INSTALL_DIR/full"/* "$STAGING"/

# Include config file
mkdir -p "$STAGING/etc"
cp "$REPO_DIR/dist/waynaptics.conf" "$STAGING/etc/waynaptics.conf"

tar -czf "$ARTIFACTS/waynaptics.tar.gz" -C "$STAGING" .
echo "Built $ARTIFACTS/waynaptics.tar.gz"
