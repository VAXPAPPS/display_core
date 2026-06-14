#!/usr/bin/env bash
# Script to build Debian package (.deb) for SettingsX
# Usage: ./build_deb.sh

set -e

# Package metadata
PKG_NAME="settingsx"
PKG_VERSION="0.1.0"
PKG_ARCH="amd64"
PKG_DIR="${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}"
STAGE_DIR="build/debian/${PKG_DIR}"

echo "=== Building SettingsX ==="
make clean
make

echo "=== Creating Debian directory structure ==="
mkdir -p "${STAGE_DIR}/DEBIAN"
mkdir -p "${STAGE_DIR}/usr/bin"
mkdir -p "${STAGE_DIR}/usr/share/applications"
mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${STAGE_DIR}/usr/share/settingsx"
mkdir -p "${STAGE_DIR}/usr/share/vsysinfo"

echo "=== Copying files to staging ==="
# Binary executable
cp settingsx "${STAGE_DIR}/usr/bin/settingsx"

# Application Icon in system icon paths and application paths
cp assets/settingsx.svg "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/settingsx.svg"
cp assets/settingsx.svg "${STAGE_DIR}/usr/share/settingsx/settingsx.svg"
cp assets/settingsx.svg "${STAGE_DIR}/usr/share/vsysinfo/settingsx.svg"

# Desktop launcher entry
cp assets/settingsx.desktop "${STAGE_DIR}/usr/share/applications/settingsx.desktop"

echo "=== Creating control file ==="
cat <<EOF > "${STAGE_DIR}/DEBIAN/control"
Package: ${PKG_NAME}
Version: ${PKG_VERSION}
Section: utils
Priority: optional
Architecture: ${PKG_ARCH}
Maintainer: VAXP OS Developer <developer@vaxp.org>
Depends: libgtk-3-0, libglib2.0-0, libxrandr2, libx11-6, libpulse0, libwayland-client0
Description: SettingsX system settings manager
 SettingsX is a lightweight, elegant settings daemon and control panel for VAXP OS.
EOF

echo "=== Creating postinst maintenance script ==="
cat <<EOF > "${STAGE_DIR}/DEBIAN/postinst"
#!/bin/sh
set -e
if [ "\$1" = "configure" ]; then
    # Update desktop launcher cache
    update-desktop-database -q || true
    # Update system icon cache
    gtk-update-icon-cache -tf /usr/share/icons/hicolor || true
fi
EOF
chmod 755 "${STAGE_DIR}/DEBIAN/postinst"

echo "=== Creating postrm maintenance script ==="
cat <<EOF > "${STAGE_DIR}/DEBIAN/postrm"
#!/bin/sh
set -e
if [ "\$1" = "remove" ] || [ "\$1" = "purge" ]; then
    # Update desktop launcher cache
    update-desktop-database -q || true
    # Update system icon cache
    gtk-update-icon-cache -tf /usr/share/icons/hicolor || true
fi
EOF
chmod 755 "${STAGE_DIR}/DEBIAN/postrm"

echo "=== Packaging to Debian archive ==="
dpkg-deb --build "build/debian/${PKG_DIR}" "./${PKG_DIR}.deb"

echo "=== Debian Package built successfully! ==="
echo "Package path: ${PKG_DIR}.deb"
