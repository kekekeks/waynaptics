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

STAGING="$(mktemp -d)"
trap "rm -rf '$STAGING'" EXIT

# Copy daemon files into buildroot-daemon
DAEMON_ROOT="$STAGING/daemon-root"
mkdir -p "$DAEMON_ROOT"
cp -a "$INSTALL_DIR/daemon"/* "$DAEMON_ROOT"/
mkdir -p "$DAEMON_ROOT/etc"
cp "$REPO_DIR/dist/waynaptics.conf" "$DAEMON_ROOT/etc/waynaptics.conf"

# Determine if config tool is available
HAS_CONFIG=0
if [ -d "$INSTALL_DIR/config" ]; then
  HAS_CONFIG=1
  CONFIG_ROOT="$STAGING/config-root"
  mkdir -p "$CONFIG_ROOT"
  cp -a "$INSTALL_DIR/config"/* "$CONFIG_ROOT"/
fi

# Write spec file for daemon
cat > "$STAGING/waynaptics.spec" << SPEC
Name:    waynaptics
Version: 1.0.0
Release: 1
Summary: Synaptics touchpad to PS/2 mouse emulator via uinput
License: MIT
Group:   System Environment/Daemons
Requires: libevdev, glib2

%description
Converts X11 xf86-input-synaptics driver into a standalone mouse emulator.

%install
cp -a $DAEMON_ROOT/* %{buildroot}/

%files
%attr(755, root, root) /usr/bin/waynaptics
%config(noreplace) /etc/waynaptics.conf
/usr/lib/systemd/system/waynaptics.service

%pre
getent passwd waynaptics >/dev/null 2>&1 || useradd --system --no-create-home --shell /usr/sbin/nologin waynaptics

%post
install -d -o waynaptics -g waynaptics -m 0755 /var/lib/waynaptics
systemctl daemon-reload
systemctl enable waynaptics.service
systemctl start waynaptics.service || true

%preun
if [ "\$1" = "0" ]; then
  systemctl stop waynaptics.service || true
  systemctl disable waynaptics.service || true
fi
SPEC

mkdir -p "$STAGING/rpmbuild"/{BUILD,RPMS,SRPMS,SPECS,SOURCES}
rpmbuild -bb \
  --define "_topdir $STAGING/rpmbuild" \
  "$STAGING/waynaptics.spec"

find "$STAGING/rpmbuild/RPMS" -name '*.rpm' -exec cp {} "$ARTIFACTS/waynaptics.rpm" \;
echo "Built $ARTIFACTS/waynaptics.rpm"

# Config tool RPM
if [ "$HAS_CONFIG" = "1" ]; then
  cat > "$STAGING/waynaptics-config.spec" << SPEC
Name:    waynaptics-config
Version: 1.0.0
Release: 1
Summary: Configuration utility for waynaptics touchpad daemon
License: MIT
Group:   Applications/System
Requires: waynaptics, qt6-qtbase

%description
Qt6 graphical tool for configuring waynaptics synaptics touchpad settings.

%install
cp -a $CONFIG_ROOT/* %{buildroot}/

%files
%attr(755, root, root) /usr/bin/waynaptics-config
/usr/share/applications/waynaptics-config.desktop
SPEC

  rm -rf "$STAGING/rpmbuild"/{BUILD,RPMS,SRPMS}
  mkdir -p "$STAGING/rpmbuild"/{BUILD,RPMS,SRPMS}
  rpmbuild -bb \
    --define "_topdir $STAGING/rpmbuild" \
    "$STAGING/waynaptics-config.spec"

  find "$STAGING/rpmbuild/RPMS" -name '*.rpm' -exec cp {} "$ARTIFACTS/waynaptics-config.rpm" \;
  echo "Built $ARTIFACTS/waynaptics-config.rpm"
fi
