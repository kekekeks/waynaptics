#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

mkdir -p "$REPO_DIR/artifacts"

exec docker run -t --rm \
  -v "$REPO_DIR:/io" \
  -e HOST_UID="$(id -u)" \
  -e HOST_GID="$(id -g)" \
  debian:11 \
  bash /io/scripts/compile-inner.sh
