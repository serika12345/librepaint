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

command -v ffmpeg >/dev/null 2>&1 || {
    printf 'ffmpeg is required\n' >&2
    exit 1
}
command -v gzip >/dev/null 2>&1 || {
    printf 'gzip is required\n' >&2
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

ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i color=c=white:s=1024x1024:r=1 \
    -i "$source_dir/librepaint-icon.png" \
    -filter_complex \
    '[1:v]format=rgba[fg];[0:v][fg]overlay=0:0:format=auto,format=rgb24[out]' \
    -map '[out]' -frames:v 1 -c:v png -pred mixed \
    "$temporary_dir/1024-apps-krita.png"

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

install -m 0644 "$temporary_dir/1024-apps-krita.png" \
    "$repo_root/krita/pics/branding/Next/1024-apps-krita.png"
install -m 0644 "$temporary_dir/sc-apps-krita.svgz" \
    "$repo_root/krita/pics/branding/Next/sc-apps-krita.svgz"
install -m 0644 "$temporary_dir/electrichearts_20250824A_kiki_4K.png" \
    "$repo_root/krita/data/splash/electrichearts_20250824A_kiki_4K.png"
install -m 0644 "$temporary_dir/electrichearts_20250824A_kiki_HD.jpg" \
    "$repo_root/krita/data/splash/electrichearts_20250824A_kiki_HD.jpg"

printf 'Installed LibrePaint icon and splash assets under compatibility filenames.\n'
