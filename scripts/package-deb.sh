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

# --- waynaptics daemon package ---
STAGING="$(mktemp -d)"
trap "rm -rf '$STAGING'" EXIT

cp -a "$INSTALL_DIR/daemon"/* "$STAGING"/

mkdir -p "$STAGING/etc"
cp "$REPO_DIR/dist/waynaptics.conf" "$STAGING/etc/waynaptics.conf"

mkdir -p "$STAGING/DEBIAN"
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

cat > "$STAGING/DEBIAN/conffiles" << EOF
/etc/waynaptics.conf
EOF

cat > "$STAGING/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e
systemctl daemon-reload
systemctl enable waynaptics.service
systemctl start waynaptics.service || true
EOF
chmod 755 "$STAGING/DEBIAN/postinst"

cat > "$STAGING/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e
systemctl stop waynaptics.service || true
systemctl disable waynaptics.service || true
EOF
chmod 755 "$STAGING/DEBIAN/prerm"

dpkg-deb --build "$STAGING" "$ARTIFACTS/waynaptics.deb"
echo "Built $ARTIFACTS/waynaptics.deb"
rm -rf "$STAGING"

# --- waynaptics-config package ---
if [ -d "$INSTALL_DIR/config" ]; then
  STAGING="$(mktemp -d)"

  cp -a "$INSTALL_DIR/config"/* "$STAGING"/

  mkdir -p "$STAGING/DEBIAN"
  cat > "$STAGING/DEBIAN/control" << EOF
Package: waynaptics-config
Version: 1.0.0
Section: utils
Priority: optional
Architecture: amd64
Depends: waynaptics, libqt6widgets6, libqt6network6
Maintainer: waynaptics
Description: Configuration utility for waynaptics touchpad daemon
 Qt6 graphical tool for configuring waynaptics synaptics touchpad settings.
EOF

  dpkg-deb --build "$STAGING" "$ARTIFACTS/waynaptics-config.deb"
  echo "Built $ARTIFACTS/waynaptics-config.deb"
  rm -rf "$STAGING"
fi
