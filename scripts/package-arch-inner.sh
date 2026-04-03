#!/bin/bash
set -e

INSTALL_DIR="/io/artifacts/install"
ARTIFACTS="/io/artifacts"

VERSION="1.0.0"
ARCH="x86_64"

build_pkg() {
  local PKGNAME="$1"
  local PKGDESC="$2"
  local DEPENDS="$3"
  local INSTALL_SRC="$4"
  local EXTRA_DIR="$5"

  local STAGING
  STAGING="$(mktemp -d)"

  cp -a "$INSTALL_SRC"/* "$STAGING"/

  # Extra files
  if [ -n "$EXTRA_DIR" ] && [ -d "$EXTRA_DIR" ]; then
    cp -a "$EXTRA_DIR"/* "$STAGING"/
  fi

  local SIZE
  SIZE=$(du -sk "$STAGING" | cut -f1)

  cat > "$STAGING/.PKGINFO" << EOF
pkgname = $PKGNAME
pkgver = $VERSION-1
pkgdesc = $PKGDESC
url = https://github.com/kekekeks/waynaptics
builddate = $(date +%s)
packager = waynaptics build system
size = $((SIZE * 1024))
arch = $ARCH
EOF

  for dep in $DEPENDS; do
    echo "depend = $dep" >> "$STAGING/.PKGINFO"
  done

  # Generate .MTREE
  cd "$STAGING"
  LANG=C bsdtar -czf .MTREE --format=mtree \
    --options='!all,use-set,type,uid,gid,mode,time,size,md5,sha256,link' \
    $(find . -not -name '.PKGINFO' -not -name '.MTREE' -not -name '.')

  # Build package
  local PKGFILE="$ARTIFACTS/${PKGNAME}-${VERSION}-1-${ARCH}.pkg.tar.zst"
  bsdtar -cf - .PKGINFO .MTREE * | zstd -f -o "$PKGFILE"
  echo "Built $PKGFILE"

  rm -rf "$STAGING"
}

# Prepare extra files for daemon (config file)
DAEMON_EXTRA="$(mktemp -d)"
mkdir -p "$DAEMON_EXTRA/etc"
cp /io/dist/waynaptics.conf "$DAEMON_EXTRA/etc/waynaptics.conf"

# --- waynaptics daemon package ---
build_pkg "waynaptics" \
  "Synaptics touchpad to PS/2 mouse emulator via uinput" \
  "libevdev glib2" \
  "$INSTALL_DIR/daemon" \
  "$DAEMON_EXTRA"

rm -rf "$DAEMON_EXTRA"

# --- waynaptics-config package ---
if [ -d "$INSTALL_DIR/config" ]; then
  build_pkg "waynaptics-config" \
    "Configuration utility for waynaptics touchpad daemon" \
    "waynaptics qt6-base" \
    "$INSTALL_DIR/config" \
    ""
fi
