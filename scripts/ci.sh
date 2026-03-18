#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Compiling with Holy Build Box ==="
bash "$SCRIPT_DIR/compile-hbb.sh"

echo "=== Building Debian package ==="
bash "$SCRIPT_DIR/package-deb.sh"

echo "=== Building RPM package ==="
bash "$SCRIPT_DIR/package-rpm.sh"

echo "=== Building tarball ==="
bash "$SCRIPT_DIR/package-tar.sh"

echo "=== Done ==="
ls -lh "$SCRIPT_DIR/../artifacts/"
