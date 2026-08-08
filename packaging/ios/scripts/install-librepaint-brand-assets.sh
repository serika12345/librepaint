#!/bin/sh
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../.." && pwd)
source_dir="$repo_root/packaging/ios/brand-assets"

require_source()
{
    source_path=$1
    expected_sha256=$2
    actual_sha256=$(shasum -a 256 "$source_path" | awk '{print $1}')
    if [ "$actual_sha256" != "$expected_sha256" ]; then
        printf 'source hash mismatch: %s\n' "$source_path" >&2
        exit 1
    fi
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 || {
        printf '%s is required\n' "$1" >&2
        exit 1
    }
}

require_command ffmpeg
require_command gzip
require_command python3
require_command xcrun

xcrun --find actool >/dev/null 2>&1 || {
    printf 'Xcode actool is required\n' >&2
    exit 1
}

require_source "$source_dir/librepaint-icon.svg" \
    2db493d76e8941413762647bbf7d2599ae7e25d4d005eafcf9efced6d0df9c3c
require_source "$source_dir/librepaint-icon.png" \
    5a7e1f43503083c8d6005bce405937b4d2aa0db6a87845a8215804afc596fba0
require_source "$source_dir/librepaint-logo.svg" \
    3bdd573cf384ba67e5e0940c2cb39fa9ca26db87fdcc2c26c3e0533c0a0413fd
require_source "$source_dir/librepaint-logo.png" \
    9650fe80133e0c34d9be036cdc247c6dcc583554d4e1ca363e75a4c19d239dad

temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/librepaint-brand-assets.XXXXXX")
cleanup()
{
    rm -r -- "$temporary_dir"
}
trap cleanup EXIT HUP INT TERM

mkdir -p \
    "$temporary_dir/krita.icon/Assets" \
    "$temporary_dir/macos-icon"

# Keep alpha for desktop, document, and Android icons. iOS uses the separate
# opaque 1024-pixel rendering below because app icons may not contain alpha.
for icon_size in 16 22 24 32 48 64 128 256 512 1024; do
    ffmpeg -hide_banner -loglevel error -y \
        -i "$source_dir/librepaint-icon.png" \
        -vf "scale=${icon_size}:${icon_size}:flags=lanczos,format=rgba" \
        -frames:v 1 -c:v png -pred mixed \
        "$temporary_dir/icon-${icon_size}.png"
done

ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i color=c=white:s=1024x1024:r=1 \
    -i "$source_dir/librepaint-icon.png" \
    -filter_complex \
    '[1:v]format=rgba[fg];[0:v][fg]overlay=0:0:format=auto,format=rgb24[out]' \
    -map '[out]' -frames:v 1 -c:v png -pred mixed \
    "$temporary_dir/1024-apps-krita.png"

ffmpeg -hide_banner -loglevel error -y \
    -i "$temporary_dir/1024-apps-krita.png" \
    -vf 'scale=256:256:flags=lanczos,format=rgb24' \
    -frames:v 1 -c:v png -pred mixed \
    "$temporary_dir/bundle-preview.png"

gzip -n -9 -c "$source_dir/librepaint-icon.svg" \
    > "$temporary_dir/sc-apps-krita.svgz"

ffmpeg -hide_banner -loglevel error -y \
    -i "$source_dir/librepaint-logo.png" \
    -vf 'scale=3840:2160:flags=lanczos,crop=3840:1920:0:120,format=rgb24' \
    -frames:v 1 -c:v png -pred mixed \
    "$temporary_dir/electrichearts_20250824A_kiki_4K.png"

ffmpeg -hide_banner -loglevel error -y \
    -i "$source_dir/librepaint-logo.png" \
    -vf 'scale=1920:1080:flags=lanczos,crop=1920:960:0:60,format=yuvj444p' \
    -frames:v 1 -q:v 2 \
    "$temporary_dir/electrichearts_20250824A_kiki_HD.jpg"

# Reuse the splash artwork in the upper half of the 2x DMG background, leaving
# the lower half clear for the Finder install icons.
ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i color=c=white:s=1400x870:r=1 \
    -i "$temporary_dir/electrichearts_20250824A_kiki_4K.png" \
    -filter_complex \
    '[1:v]scale=900:450:flags=lanczos[brand];[0:v][brand]overlay=250:0:format=auto,format=rgb24[out]' \
    -map '[out]' -frames:v 1 -c:v png -pred mixed -dpi 144 \
    "$temporary_dir/krita_dmgBG.png"

install -m 0644 "$source_dir/librepaint-icon.svg" \
    "$temporary_dir/krita.icon/Assets/librepaint-icon.svg"
install -m 0644 "$repo_root/krita/pics/branding/Next/krita.icon/icon.json" \
    "$temporary_dir/krita.icon/icon.json"

xcrun actool "$temporary_dir/krita.icon" \
    --compile "$temporary_dir/macos-icon" \
    --platform macosx \
    --minimum-deployment-target 12.0 \
    --app-icon krita \
    --output-partial-info-plist "$temporary_dir/macos-icon/icon.plist" \
    --output-format human-readable-text \
    --standalone-icon-behavior all

for icon_size in 16 22 24 32 48 64 128 256 512; do
    install -m 0644 "$temporary_dir/icon-${icon_size}.png" \
        "$repo_root/krita/pics/branding/Next/${icon_size}-apps-krita.png"
done
install -m 0644 "$temporary_dir/1024-apps-krita.png" \
    "$repo_root/krita/pics/branding/Next/1024-apps-krita.png"
install -m 0644 "$temporary_dir/sc-apps-krita.svgz" \
    "$repo_root/krita/pics/branding/Next/sc-apps-krita.svgz"
install -m 0644 "$source_dir/librepaint-icon.svg" \
    "$repo_root/krita/pics/branding/Next/krita.icon/Assets/librepaint-icon.svg"

for icon_size in 16 22 24 32 48 64 128 256 512 1024; do
    install -m 0644 "$temporary_dir/icon-${icon_size}.png" \
        "$repo_root/krita/pics/mimetypes/${icon_size}-mimetypes-application-x-krita.png"
done
install -m 0644 "$temporary_dir/icon-128.png" "$repo_root/pics/krita.png"

for relative_path in \
    pics/16_dark_application-x-krita.svg \
    pics/16_light_application-x-krita.svg \
    pics/22_dark_application-x-krita.svg \
    pics/22_light_application-x-krita.svg \
    pics/32_dark_application-x-krita.svg \
    pics/32_light_application-x-krita.svg \
    pics/64_dark_application-x-krita.svg \
    pics/64_light_application-x-krita.svg \
    pics/16_dark_application-x-krz.svg \
    pics/16_light_application-x-krz.svg \
    pics/22_dark_application-x-krz.svg \
    pics/22_light_application-x-krz.svg \
    pics/32_dark_application-x-krz.svg \
    pics/32_light_application-x-krz.svg \
    pics/64_dark_application-x-krz.svg \
    pics/64_light_application-x-krz.svg \
    krita/pics/svg/dark_krita_log.svg \
    krita/pics/svg/light_krita_log.svg \
    krita/pics/Breeze-dark/dark_application-pdf.svg \
    krita/pics/Breeze-light/light_application-pdf.svg
do
    install -m 0644 "$source_dir/librepaint-icon.svg" "$repo_root/$relative_path"
done

install -m 0644 "$temporary_dir/macos-icon/krita.icns" \
    "$repo_root/packaging/macos/KritaIcon.icns"
install -m 0644 "$temporary_dir/krita_dmgBG.png" \
    "$repo_root/packaging/macos/krita_dmgBG.png"
install -m 0644 "$temporary_dir/macos-icon/krita.icns" \
    "$repo_root/krita/pics/mimetypes/krita-kra.icns"
install -m 0644 "$temporary_dir/macos-icon/krita.icns" \
    "$repo_root/krita/pics/mimetypes/krz/krita-krz.icns"

for density_and_size in mdpi:48 hdpi:72 xhdpi:96 xxhdpi:144 xxxhdpi:192; do
    density=${density_and_size%:*}
    icon_size=${density_and_size#*:}
    icon_directory="$repo_root/packaging/android/apk/res/mipmap-${density}"
    mkdir -p "$icon_directory"
    ffmpeg -hide_banner -loglevel error -y \
        -i "$source_dir/librepaint-icon.png" \
        -vf "scale=${icon_size}:${icon_size}:flags=lanczos,format=rgba" \
        -frames:v 1 -c:v libwebp -lossless 1 -compression_level 6 \
        "$icon_directory/ic_launcher.webp"
done

python3 - \
    "$script_dir/replace-brand-art-with-white.py" \
    "$temporary_dir/bundle-preview.png" \
    "$repo_root/krita/data/bundles/Krita_3_Default_Resources.bundle" \
    "$repo_root/krita/data/bundles/Krita_4_Default_Resources.bundle" \
    "$repo_root/krita/data/bundles/Krita_Artists_SeExpr_examples.bundle" \
    "$repo_root/krita/data/bundles/RGBA_brushes.bundle" <<'PY'
import importlib.util
from pathlib import Path
import sys

module_path = Path(sys.argv[1])
spec = importlib.util.spec_from_file_location("librepaint_white_asset_tools", module_path)
if spec is None or spec.loader is None:
    raise RuntimeError(f"cannot load bundle helper: {module_path}")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

replacement = Path(sys.argv[2]).read_bytes()
for bundle_path in map(Path, sys.argv[3:]):
    module.patch_bundle_preview(bundle_path, replacement)
PY

install -m 0644 "$temporary_dir/electrichearts_20250824A_kiki_4K.png" \
    "$repo_root/krita/data/splash/electrichearts_20250824A_kiki_4K.png"
install -m 0644 "$temporary_dir/electrichearts_20250824A_kiki_HD.jpg" \
    "$repo_root/krita/data/splash/electrichearts_20250824A_kiki_HD.jpg"

printf 'Installed LibrePaint icons, bundle previews, and splash assets under compatibility filenames.\n'
