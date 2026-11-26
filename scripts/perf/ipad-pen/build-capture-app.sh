#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds IPadCapture.app around ipad_capture.mm. macOS keeps the camera
# permission per signed application, so the application is signed with the
# first Apple Development identity of the keychain (or IPAD_CAPTURE_IDENTITY):
# a rebuilt binary keeps the permission. An ad-hoc signature would ask again
# after every build.
#
# Usage: build-capture-app.sh OUT_DIR    (writes OUT_DIR/IPadCapture.app)
set -euo pipefail
[ "$#" -eq 1 ] || { echo "usage: $0 OUT_DIR" >&2; exit 2; }
here=$(cd "$(dirname "$0")" && pwd)
app="$1/IPadCapture.app"
mkdir -p "$app/Contents/MacOS"
cat > "$app/Contents/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>ipad_capture</string>
<key>CFBundleIdentifier</key><string>local.librepaint.ipad-capture</string>
<key>CFBundleName</key><string>IPadCapture</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>LSUIElement</key><true/>
<key>NSCameraUsageDescription</key><string>Captures the screen of the connected iPad to measure pen latency.</string>
</dict></plist>
PLIST
clang++ -std=c++17 -fobjc-arc -O2 -framework Foundation -framework AVFoundation -framework CoreMedia \
  -framework CoreMediaIO -framework CoreVideo "$here/ipad_capture.mm" -o "$app/Contents/MacOS/ipad_capture"
identity=${IPAD_CAPTURE_IDENTITY:-$(security find-identity -v -p codesigning | awk '/Apple Development/ { print $2; exit }')}
codesign --force --sign "${identity:--}" "$app"
echo "$app"
