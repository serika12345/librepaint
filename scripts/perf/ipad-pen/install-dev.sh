#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Signs a LibrePaint IPA with an Xcode development provisioning profile and
# installs it on a connected iPad with devicectl, for measurements on a device
# that has no AltStore. The profile is the newest unexpired one in Xcode's
# profile directory whose application identifier ends with the bundle
# identifier (IPAD_DEV_BUNDLE_ID, default local.librepaint.ipad.xcodedev); the
# IPA's bundle identifier is replaced by it, so an installed copy under that
# identifier keeps its data. Xcode creates such a profile when an app with
# that identifier is built for the device with automatic signing.
#
# Usage: install-dev.sh IPA DEVICE_ID
#   IPA comes from KRITA_IOS_IPA_ONLY=1 build-incremental ios deploy.
set -euo pipefail
[ "$#" -eq 2 ] || { echo "usage: $0 IPA DEVICE_ID" >&2; exit 2; }
ipa=$(cd "$(dirname "$1")" && pwd)/$(basename "$1") device=$2
bundle_id=${IPAD_DEV_BUNDLE_ID:-local.librepaint.ipad.xcodedev}
profiles="$HOME/Library/Developer/Xcode/UserData/Provisioning Profiles"
work=$(mktemp -d "${TMPDIR:-/tmp}/librepaint-dev-install.XXXXXX")
trap 'rm -rf "$work"' EXIT

profile=""
newest=""
for candidate in "$profiles"/*.mobileprovision; do
    [ -f "$candidate" ] || continue
    security cms -D -i "$candidate" > "$work/profile.plist" 2> /dev/null || continue
    app_id=$(plutil -extract Entitlements.application-identifier raw -o - "$work/profile.plist" 2> /dev/null || true)
    expires=$(plutil -extract ExpirationDate raw -o - "$work/profile.plist" 2> /dev/null || true)
    [[ "$app_id" == *".$bundle_id" ]] || continue
    [[ "$expires" > "$(date -u +%Y-%m-%dT%H:%M:%SZ)" ]] || continue
    if [[ -z "$newest" || "$expires" > "$newest" ]]; then profile=$candidate newest=$expires; fi
done
[ -n "$profile" ] || { echo "error: no unexpired provisioning profile for $bundle_id" >&2; exit 1; }
security cms -D -i "$profile" > "$work/profile.plist"
plutil -extract Entitlements xml1 -o "$work/entitlements.plist" "$work/profile.plist"
team=$(plutil -extract TeamIdentifier.0 raw -o - "$work/profile.plist")
identity=$(security find-identity -v -p codesigning | awk -v team="$team" '/Apple Development/ { print $2; exit }')
[ -n "$identity" ] || { echo "error: no Apple Development signing identity" >&2; exit 1; }
echo "profile expires $newest"

(cd "$work" && unzip -q "$ipa")
app="$work/Payload/LibrePaint.app"
chmod -R u+w "$app"
plutil -replace CFBundleIdentifier -string "$bundle_id" "$app/Info.plist"
cp "$profile" "$app/embedded.mobileprovision"
# Nested code first, then the application with the profile's entitlements.
find "$app" -depth \( -name '*.dylib' -o -name '*.framework' -o -name '*.appex' \) -print0 |
    while IFS= read -r -d '' nested; do codesign --force --sign "$identity" "$nested"; done
codesign --force --sign "$identity" --entitlements "$work/entitlements.plist" "$app"
xcrun devicectl device install app --device "$device" "$app"
