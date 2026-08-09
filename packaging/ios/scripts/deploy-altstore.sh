#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 [--skip-build] [CoreDevice identifier]" >&2
    exit 2
}

skip_build=0
if (( $# > 0 )) && [[ "$1" == "--skip-build" ]]; then
    skip_build=1
    shift
fi
if (( $# > 1 )); then
    usage
fi

scripts_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$scripts_dir" rev-parse --show-toplevel)"
ipa_permissions="$repo_root/nix/ios/ipa-permissions.py"
device_id="${1:-${KRITA_IOS_DEVICE:-}}"

# The default entry point is the guarded persistent build. The helper calls
# this script back with --skip-build after it has completed the exact tree.
if (( ! skip_build )); then
    if [[ -n "$device_id" ]]; then
        exec "$scripts_dir/build-krita-incremental.sh" deploy "$device_id"
    else
        exec "$scripts_dir/build-krita-incremental.sh" deploy
    fi
fi

if [[ -z "${KRITA_IOS_BUILD_DIR:-}" ]]; then
    echo "error: --skip-build requires the exact KRITA_IOS_BUILD_DIR selected by the build workflow" >&2
    echo "run this script without --skip-build for a guarded incremental build and deployment" >&2
    exit 2
fi

build_dir="$KRITA_IOS_BUILD_DIR"
if [[ "$build_dir" != /* ]]; then
    build_dir="$repo_root/$build_dir"
fi
if [[ ! -d "$build_dir" ]]; then
    echo "error: incremental build tree does not exist: $build_dir" >&2
    exit 1
fi
build_dir="$(cd "$build_dir" && pwd -P)"
if [[ ! -f "$build_dir/.krita-ios-incremental-config" ]]; then
    echo "error: build tree is not owned by the incremental workflow: $build_dir" >&2
    exit 1
fi
python3 "$repo_root/packaging/ios/scripts/replace-brand-art-with-white.py" \
    --audit-ios-classification
python3 "$repo_root/packaging/ios/scripts/audit-default-resource-bundle.py"
app_path="$build_dir/bin/krita.app"
binary="$app_path/krita"
archive_dir="$build_dir/lib"
python3 "$repo_root/packaging/ios/scripts/audit-static-dependency-resources.py" \
    --binary "$binary" \
    --build-ninja "$build_dir/build.ninja"

if [[ -z "$device_id" ]]; then
    device_id="$(xcrun devicectl list devices | awk '
        /(connected|available)/ {
            for (field = 1; field <= NF; field++) {
                if (length($field) == 36 && $field ~ /^[[:xdigit:]-]+$/) {
                    print $field
                    exit
                }
            }
        }
    ')"
fi
if [[ -z "$device_id" ]]; then
    echo "error: no available CoreDevice found" >&2
    exit 1
fi

"$scripts_dir/inspect-apple-binary.sh" device "$binary"
"$scripts_dir/inspect-static-resources.sh" "$binary" "$archive_dir"
plutil -lint "$app_path/Info.plist"
for document_key in UIFileSharingEnabled LSSupportsOpeningDocumentsInPlace; do
    if [[ "$(plutil -extract "$document_key" raw -o - "$app_path/Info.plist" 2>/dev/null || true)" != "true" ]]; then
        echo "error: iPadOS document access is not enabled in Info.plist: $document_key" >&2
        exit 1
    fi
done
document_types="$(plutil -extract CFBundleDocumentTypes xml1 -o - "$app_path/Info.plist" 2>/dev/null || true)"
for document_type in org.krita.kra org.krita.openraster public.png public.jpeg; do
    if ! grep -Fq "$document_type" <<<"$document_types"; then
        echo "error: iPadOS document type is missing from Info.plist: $document_type" >&2
        exit 1
    fi
done

if ! pgrep -x AltServer >/dev/null; then
    echo "error: AltServer is not running" >&2
    exit 1
fi

apps_output="$(xcrun devicectl device info apps --device "$device_id")"
altstore_bundle_id="$(awk '$1 == "AltStore" { print $2; exit }' <<<"$apps_output")"
if [[ -z "$altstore_bundle_id" ]]; then
    echo "error: AltStore is not installed on the selected device" >&2
    exit 1
fi

stage_dir="$(mktemp -d "${TMPDIR:-/tmp}/krita-altstore.XXXXXX")"
server_pid=""
cleanup() {
    if [[ -n "$server_pid" ]]; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    # Validation deliberately rejects symlinks and special files before it
    # changes any modes. Make only real directories below this exact mktemp
    # root writable so a failed preflight cannot strand a read-only stage.
    python3 "$ipa_permissions" make-tree-removable "$stage_dir" \
        >/dev/null 2>&1 || true
    rm -rf "$stage_dir" || true
}
trap cleanup EXIT

mkdir -p "$stage_dir/Payload"
COPYFILE_DISABLE=1 /bin/cp -RX \
    "$app_path" "$stage_dir/Payload/krita.app"

# The source may be an immutable Nix output.  Make the private staging copy
# writable before merging runtime data; the source application is untouched.
python3 "$ipa_permissions" \
    normalize-app "$stage_dir/Payload/krita.app"

cmake_command="$(awk -F= '$1 == "CMAKE_COMMAND:INTERNAL" { print $2; exit }' "$build_dir/CMakeCache.txt")"
if [[ -z "$cmake_command" || ! -x "$cmake_command" ]]; then
    echo "error: could not find the CMake executable used to configure the build" >&2
    exit 1
fi

runtime_prefix="$stage_dir/runtime"
runtime_install_log="$stage_dir/runtime-install.log"
if ! "$cmake_command" -DCMAKE_INSTALL_PREFIX="$runtime_prefix" \
    -P "$build_dir/krita/data/cmake_install.cmake" >"$runtime_install_log" 2>&1; then
    cat "$runtime_install_log" >&2
    exit 1
fi
if ! "$cmake_command" -DCMAKE_INSTALL_PREFIX="$runtime_prefix" \
    -P "$build_dir/plugins/cmake_install.cmake" >>"$runtime_install_log" 2>&1; then
    cat "$runtime_install_log" >&2
    exit 1
fi

# Desktop service metadata has no consumer on iPadOS. Remove only this exact
# generated directory before the runtime tree is inspected and staged.
rm -rf -- "$runtime_prefix/share/applications"

# These core action registries are installed by krita/CMakeLists.txt rather
# than the krita/data subtree. Stage them without running the complete Krita
# install script, which would also duplicate and mutate the application bundle.
core_actions_dir="$runtime_prefix/share/krita/actions"
"$cmake_command" -E make_directory "$core_actions_dir"
"$cmake_command" -E copy_if_different \
    "$repo_root/krita/krita.action" \
    "$repo_root/krita/kritamenu.action" \
    "$core_actions_dir"

if [[ ! -d "$runtime_prefix/share" ]]; then
    echo "error: the iPadOS runtime data install produced no share directory" >&2
    exit 1
fi
mkdir -p "$stage_dir/Payload/krita.app/share"
COPYFILE_DISABLE=1 /bin/cp -RX \
    "$runtime_prefix/share/." "$stage_dir/Payload/krita.app/share"
"$scripts_dir/inspect-runtime-data.sh" \
    "$stage_dir/Payload/krita.app" "$runtime_prefix"

bundle_version="${KRITA_IOS_BUNDLE_VERSION:-$(date -u +%Y%m%d%H%M%S)}"
plutil -replace CFBundleVersion -string "$bundle_version" "$stage_dir/Payload/krita.app/Info.plist"

# Normalize again after every staged mutation so CMake install modes and
# plutil's replacement behavior cannot leak into the completed IPA.
python3 "$ipa_permissions" \
    normalize-app "$stage_dir/Payload/krita.app"
chmod 0755 "$stage_dir/Payload"

output_dir="$repo_root/build-ios/deploy"
mkdir -p "$output_dir"
ipa_name="LibrePaint-iPad-${bundle_version}.ipa"
ipa_path="$output_dir/$ipa_name"
staged_ipa="$stage_dir/$ipa_name"
entry_list="$stage_dir/ipa-entries"
(
    cd "$stage_dir"
    {
        find Payload -type d -exec printf '%s/\n' {} \;
        find Payload -type f -print
    } | LC_ALL=C sort > "$entry_list"
    COPYFILE_DISABLE=1 ZIPOPT= ZIP= zip -nw -MM -X -9 -q \
        "$staged_ipa" -@ < "$entry_list"
)
unzip -tq "$staged_ipa"
python3 "$ipa_permissions" check-ipa "$staged_ipa" \
    --staged-app "$stage_dir/Payload/krita.app"
mv -f "$staged_ipa" "$ipa_path"

network_interface="$(route -n get default | awk '$1 == "interface:" { print $2; exit }')"
host_ip="$(ipconfig getifaddr "$network_interface" 2>/dev/null || true)"
if [[ -z "$host_ip" ]]; then
    echo "error: could not determine the Mac's default-route IPv4 address" >&2
    exit 1
fi

port="${KRITA_IOS_DEPLOY_PORT:-8765}"
while lsof -nP -iTCP:"$port" -sTCP:LISTEN >/dev/null 2>&1; do
    port="$((port + 1))"
done

server_log="$stage_dir/http-server.log"
python3 -m http.server "$port" --bind "$host_ip" --directory "$output_dir" >"$server_log" 2>&1 &
server_pid="$!"
sleep 1
if ! kill -0 "$server_pid" 2>/dev/null; then
    echo "error: failed to start local IPA server" >&2
    cat "$server_log" >&2
    exit 1
fi

download_url="http://${host_ip}:${port}/${ipa_name}"
encoded_url="http%3A%2F%2F${host_ip}%3A${port}%2F${ipa_name}"
install_url="altstore-classic://install?url=${encoded_url}"

echo "device:         $device_id"
echo "AltStore:       $altstore_bundle_id"
echo "bundle version: $bundle_version"
echo "IPA:            $ipa_path"
echo "download URL:   $download_url"
echo "Opening AltStore installation deep link..."

xcrun devicectl device process launch \
    --device "$device_id" \
    --terminate-existing \
    --payload-url "$install_url" \
    "$altstore_bundle_id"

echo "Waiting for AltStore to download the IPA..."
downloaded=0
for _ in {1..300}; do
    if grep -Fq "GET /${ipa_name} HTTP/1.1\" 200" "$server_log"; then
        downloaded=1
        break
    fi
    sleep 1
done
if (( ! downloaded )); then
    echo "error: AltStore did not download the IPA within 5 minutes" >&2
    cat "$server_log" >&2
    exit 1
fi

echo "IPA downloaded; waiting for the signed app to be installed..."
installed_bundle_id=""
for _ in {1..300}; do
    installed_line="$(xcrun devicectl device info apps --device "$device_id" \
        | awk '$2 ~ /^local\.librepaint\.ipad/ { print; exit }')"
    installed_version="$(awk '{ print $4 }' <<<"$installed_line")"
    if [[ "$installed_version" == "$bundle_version" ]]; then
        installed_bundle_id="$(awk '{ print $2 }' <<<"$installed_line")"
        break
    fi
    sleep 1
done
if [[ -z "$installed_bundle_id" ]]; then
    echo "error: the signed app was not installed within 5 minutes" >&2
    exit 1
fi

echo "installed: $installed_bundle_id ($bundle_version)"
xcrun devicectl device process launch --device "$device_id" --terminate-existing "$installed_bundle_id"

launch_log="$output_dir/LibrePaint-iPad-${bundle_version}-krita.log"
sleep "${KRITA_IOS_LAUNCH_SETTLE_SECONDS:-5}"
if xcrun devicectl device copy from \
    --device "$device_id" \
    --domain-type appDataContainer \
    --domain-identifier "$installed_bundle_id" \
    --source "Library/Application Support/krita.log" \
    --destination "$launch_log" >/dev/null; then
    echo "startup log:    $launch_log"
else
    echo "warning: could not collect the LibrePaint startup log" >&2
fi

"$scripts_dir/maintain-build-cache.sh" \
    --deployment \
    --incremental-build-dir "$build_dir"
