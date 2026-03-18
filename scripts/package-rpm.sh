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

# Prepare source files for %install to copy
SRCDIR="$STAGING/SOURCES"
mkdir -p "$SRCDIR"
cp "$ARTIFACTS/waynaptics" "$SRCDIR/waynaptics"
cp "$REPO_DIR/dist/waynaptics.service" "$SRCDIR/waynaptics.service"

# Write spec file
cat > "$STAGING/waynaptics.spec" << 'SPEC'
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
mkdir -p %{buildroot}/opt/waynaptics/bin
mkdir -p %{buildroot}/etc/systemd/system
install -m 755 %{_sourcedir}/waynaptics %{buildroot}/opt/waynaptics/bin/waynaptics
install -m 644 %{_sourcedir}/waynaptics.service %{buildroot}/etc/systemd/system/waynaptics.service
touch %{buildroot}/etc/waynaptics.conf

%files
%attr(755, root, root) /opt/waynaptics/bin/waynaptics
%config(noreplace) /etc/waynaptics.conf
/etc/systemd/system/waynaptics.service

%post
systemctl daemon-reload
systemctl enable waynaptics.service
systemctl start waynaptics.service || true

%preun
if [ "$1" = "0" ]; then
  systemctl stop waynaptics.service || true
  systemctl disable waynaptics.service || true
fi
SPEC

# Build RPM
mkdir -p "$STAGING/rpmbuild"/{BUILD,RPMS,SRPMS,SPECS}
rpmbuild -bb \
  --define "_topdir $STAGING/rpmbuild" \
  --define "_sourcedir $SRCDIR" \
  "$STAGING/waynaptics.spec"

# Copy result
find "$STAGING/rpmbuild" -name '*.rpm' -exec cp {} "$ARTIFACTS/waynaptics.rpm" \;
echo "Built $ARTIFACTS/waynaptics.rpm"
