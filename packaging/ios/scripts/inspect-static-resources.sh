#!/usr/bin/env bash
set -euo pipefail

if (( $# != 2 )); then
    echo "usage: $0 <Mach-O binary> <archive directory>" >&2
    exit 2
fi

binary="$1"
archive_dir="$2"

if [[ ! -f "$binary" ]]; then
    echo "error: binary does not exist: $binary" >&2
    exit 1
fi
if [[ ! -d "$archive_dir" ]]; then
    echo "error: archive directory does not exist: $archive_dir" >&2
    exit 1
fi

inspection_dir="$(mktemp -d "${TMPDIR:-/tmp}/krita-ios-resources.XXXXXX")"
trap 'rm -rf "$inspection_dir"' EXIT

archive_symbols="$inspection_dir/archive-symbols.txt"
binary_symbols="$inspection_dir/binary-symbols.txt"
archive_resources="$inspection_dir/archive-resources.txt"
binary_resources="$inspection_dir/binary-resources.txt"

find "$archive_dir" -type f -name '*.a' -print0 \
    | xargs -0 nm -A 2>/dev/null \
    | grep 'qInitResources_' >"$archive_symbols" || true
nm "$binary" 2>/dev/null | grep 'qInitResources_' >"$binary_symbols" || true

sed -E 's/.*qInitResources_([[:alnum:]_]+)v$/\1/' "$archive_symbols" | sort -u >"$archive_resources"
sed -E 's/.*qInitResources_([[:alnum:]_]+)v$/\1/' "$binary_symbols" | sort -u >"$binary_resources"

missing=0
while IFS= read -r resource; do
    [[ -n "$resource" ]] || continue
    if ! grep -Fxq "$resource" "$binary_resources"; then
        echo "error: static Qt resource is missing from final binary: $resource" >&2
        grep "qInitResources_${resource}v$" "$archive_symbols" >&2 || true
        missing=1
    fi
done <"$archive_resources"

if (( missing )); then
    exit 1
fi

if grep -Fxq "sponsors" "$binary_resources"; then
    echo "error: retired sponsor artwork is still embedded in the final binary" >&2
    exit 1
fi

archive_count="$(wc -l <"$archive_resources" | tr -d ' ')"
echo "static Qt resources retained: $archive_count"
sed 's/^/  /' "$archive_resources"
