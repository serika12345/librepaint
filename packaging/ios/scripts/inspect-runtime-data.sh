#!/usr/bin/env bash
set -euo pipefail

if (( $# != 2 )); then
    echo "usage: $0 APP_PATH RUNTIME_PREFIX" >&2
    exit 2
fi

app_path="$1"
runtime_prefix="$2"
scripts_dir="$(cd "$(dirname "$0")" && pwd -P)"
expected_share="$runtime_prefix/share"
bundles_dir="$app_path/share/krita/bundles"
actions_dir="$app_path/share/krita/actions"
touch_ui_action="$actions_dir/iostouchui.action"
license_dir="$app_path/share/doc/librepaint/non-code-licenses"

if [[ ! -d "$app_path" ]]; then
    echo "error: application bundle not found: $app_path" >&2
    exit 1
fi
if [[ ! -d "$expected_share" ]]; then
    echo "error: staged runtime share directory not found: $expected_share" >&2
    exit 1
fi

# File-list comparison does not expose an empty directory, and a dangling
# symlink does not satisfy -e. Reject both forms in the generated runtime tree
# and in the application copy so desktop-only metadata cannot slip into iPadOS.
for applications_dir in \
    "$expected_share/applications" \
    "$app_path/share/applications"; do
    if [[ -e "$applications_dir" || -L "$applications_dir" ]]; then
        echo "error: desktop application metadata was packaged for iPadOS: $applications_dir" >&2
        exit 1
    fi
done

expected_list="$(mktemp "${TMPDIR:-/tmp}/krita-runtime-expected.XXXXXX")"
actual_list="$(mktemp "${TMPDIR:-/tmp}/krita-runtime-actual.XXXXXX")"
cleanup() {
    rm -f "$expected_list" "$actual_list"
}
trap cleanup EXIT

(
    cd "$runtime_prefix"
    find share -type f -print | LC_ALL=C sort
) >"$expected_list"
(
    cd "$app_path"
    find share -type f -print | LC_ALL=C sort
) >"$actual_list"

if ! diff -u "$expected_list" "$actual_list"; then
    echo "error: application runtime data does not match the generated install tree" >&2
    exit 1
fi

bundle_count="$(find "$bundles_dir" -maxdepth 1 -type f -name '*.bundle' | wc -l | tr -d ' ')"
profile_count="$(find "$app_path/share/color/icc/krita" -type f \( -name '*.icc' -o -name '*.icm' \) | wc -l | tr -d ' ')"
action_count="$(find "$actions_dir" -type f -name '*.action' | wc -l | tr -d ' ')"

if (( bundle_count != 1 )) || [[ ! -f "$bundles_dir/Krita_4_Default_Resources.bundle" ]]; then
    echo "error: expected exactly the audited Krita_4_Default_Resources.bundle; found $bundle_count bundle(s)" >&2
    exit 1
fi
for notice in \
    CC-BY-3.0.txt \
    CC-BY-SA-3.0.txt \
    CC-BY-SA-4.0.txt \
    CC0-1.0.txt \
    GPL-2.0-or-later.txt \
    GPL-3.0-only.txt \
    GPL-3.0-or-later.txt \
    LGPL-2.0-or-later.txt \
    LGPL-3.0-only.txt \
    LGPL-3.0-or-later.txt \
    LicenseRef-ICC-License.txt \
    default-resource-bundle-licenses.json \
    librepaint-brand-assets.md \
    non-code-licenses.md \
    qtbase-icc-attribution.json \
    retained-functional-assets.md \
    static-dependency-resources.json \
    white-brand-assets.json \
    bundles/README \
    profiles/elles-icc-profiles/plain-text-README-for-elles-well-behaved-icc-profiles.txt \
    profiles/ycbcr-icc-profiles/LICENSE-PROFILES.txt; do
    if [[ ! -s "$license_dir/$notice" ]]; then
        echo "error: non-code asset notice was not packaged: $notice" >&2
        exit 1
    fi
done
python3 "$scripts_dir/audit-default-resource-bundle.py" \
    --bundle "$bundles_dir/Krita_4_Default_Resources.bundle" \
    --manifest "$license_dir/default-resource-bundle-licenses.json" \
    --skip-external-notice-check
python3 "$scripts_dir/audit-static-dependency-resources.py" \
    --manifest "$license_dir/static-dependency-resources.json"
if (( profile_count != 31 )); then
    echo "error: expected exactly 31 audited ICC color profiles; found $profile_count" >&2
    exit 1
fi
for excluded_profile in \
    scRGB.icm \
    cmyk.icm \
    krita25_lcms-builtin-sRGB_g100-truegamma.icc; do
    if [[ -e "$app_path/share/color/icc/krita/$excluded_profile" ]]; then
        echo "error: ambiguous ICC profile was packaged: $excluded_profile" >&2
        exit 1
    fi
done
if (( action_count == 0 )); then
    echo "error: no LibrePaint action definitions were packaged" >&2
    exit 1
fi
for core_action in krita.action kritamenu.action; do
    if [[ ! -s "$actions_dir/$core_action" ]]; then
        echo "error: core LibrePaint action registry was not packaged: $core_action" >&2
        exit 1
    fi
done
if [[ ! -s "$touch_ui_action" ]]; then
    echo "error: iPadOS touch UI action registry was not packaged" >&2
    exit 1
fi

if ! grep -q '<Action name="copy_merged">' "$actions_dir/kritamenu.action"; then
    echo "error: packaged LibrePaint menu registry is incomplete" >&2
    exit 1
fi
if ! grep -q '<Action name="view_show_ios_touch_ui">' "$touch_ui_action"; then
    echo "error: packaged iPadOS touch UI action registry is incomplete" >&2
    exit 1
fi

preset_bundle=""
while IFS= read -r bundle; do
    if unzip -Z1 "$bundle" | grep '^paintoppresets/' >/dev/null; then
        preset_bundle="$(basename "$bundle")"
        break
    fi
done < <(find "$bundles_dir" -maxdepth 1 -type f -name '*.bundle' | LC_ALL=C sort)

if [[ -z "$preset_bundle" ]]; then
    echo "error: no packaged resource bundle contains brush presets" >&2
    exit 1
fi

runtime_file_count="$(wc -l <"$actual_list" | tr -d ' ')"
echo "iPadOS runtime data retained: $runtime_file_count files"
echo "  resource bundles: $bundle_count ($preset_bundle contains brush presets)"
echo "  ICC profiles:     $profile_count"
echo "  action files:     $action_count (core menu registries present)"
