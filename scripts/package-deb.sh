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

# Directory structure
mkdir -p "$STAGING/opt/waynaptics/bin"
mkdir -p "$STAGING/etc/systemd/system"
mkdir -p "$STAGING/etc"
mkdir -p "$STAGING/DEBIAN"

# Files
cp "$ARTIFACTS/waynaptics" "$STAGING/opt/waynaptics/bin/waynaptics"
chmod 755 "$STAGING/opt/waynaptics/bin/waynaptics"

cp "$REPO_DIR/dist/waynaptics.service" "$STAGING/etc/systemd/system/waynaptics.service"

touch "$STAGING/etc/waynaptics.conf"

# DEBIAN/control
cat > "$STAGING/DEBIAN/control" << EOF
Package: waynaptics
Version: 1.0.0
Section: utils
Priority: optional
Architecture: amd64
Depends: libevdev2, libglib2.0-0
Maintainer: waynaptics
Description: Synaptics touchpad to PS/2 mouse emulator via uinput
 Converts X11 xf86-input-synaptics driver into a standalone mouse emulator.
EOF

# Mark config file
cat > "$STAGING/DEBIAN/conffiles" << EOF
/etc/waynaptics.conf
EOF

# postinst: enable and start service
cat > "$STAGING/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e
systemctl daemon-reload
systemctl enable waynaptics.service
systemctl start waynaptics.service || true
EOF
chmod 755 "$STAGING/DEBIAN/postinst"

# prerm: stop and disable service
cat > "$STAGING/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e
systemctl stop waynaptics.service || true
systemctl disable waynaptics.service || true
EOF
chmod 755 "$STAGING/DEBIAN/prerm"

# Build
dpkg-deb --build "$STAGING" "$ARTIFACTS/waynaptics.deb"
echo "Built $ARTIFACTS/waynaptics.deb"
