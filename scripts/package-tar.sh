#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ARTIFACTS="$REPO_DIR/artifacts"
STAGING="$(mktemp -d)"

trap "rm -rf '$STAGING'" EXIT

# Ensure binary exists
if [ ! -f "$ARTIFACTS/waynaptics" ]; then
  echo "Error: $ARTIFACTS/waynaptics not found. Run compile-hbb.sh first." >&2
  exit 1
fi

cp "$ARTIFACTS/waynaptics" "$STAGING/waynaptics"
cp "$REPO_DIR/dist/waynaptics.service" "$STAGING/waynaptics.service"
touch "$STAGING/waynaptics.conf"

tar -czf "$ARTIFACTS/waynaptics.tar.gz" -C "$STAGING" waynaptics waynaptics.service waynaptics.conf
echo "Built $ARTIFACTS/waynaptics.tar.gz"
