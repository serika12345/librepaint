#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 --deployment --incremental-build-dir path [--force-nix-gc]" >&2
    exit 2
}

deployment_mode=0
force_nix_gc=0
incremental_build_dir=""
while (( $# > 0 )); do
    case "$1" in
        --deployment)
            deployment_mode=1
            ;;
        --force-nix-gc)
            force_nix_gc=1
            ;;
        --incremental-build-dir)
            shift
            (( $# > 0 )) || usage
            incremental_build_dir="$1"
            ;;
        *)
            usage
            ;;
    esac
    shift
done

(( deployment_mode )) || usage
[[ -n "$incremental_build_dir" ]] || usage

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
deploy_dir="$repo_root/build-ios/deploy"
keep_ipas="${KRITA_IOS_KEEP_IPAS:-3}"
minimum_free_gib="${KRITA_IOS_GC_MIN_FREE_GIB:-20}"

if ! [[ "$keep_ipas" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: KRITA_IOS_KEEP_IPAS must be a positive integer" >&2
    exit 2
fi
if ! [[ "$minimum_free_gib" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: KRITA_IOS_GC_MIN_FREE_GIB must be a positive integer" >&2
    exit 2
fi

# IPA files are reproducible deployment artifacts. Keep the newest few, while
# retaining logs and screenshots used for device debugging. Treat the legacy
# Krita prefix and the current LibrePaint prefix as one history. Sorting by the
# shared bundle-version suffix keeps the existing name/date policy independent
# of the product-name migration; the full path is a deterministic tie-breaker.
ipa_files=()
if [[ -d "$deploy_dir" ]]; then
    while IFS=$'\t' read -r _ ipa; do
        ipa_files+=("$ipa")
    done < <(
        find "$deploy_dir" -maxdepth 1 -type f \
            \( -name 'Krita-iPad-*.ipa' -o -name 'LibrePaint-iPad-*.ipa' \) \
            -print \
            | while IFS= read -r ipa; do
                ipa_name="${ipa##*/}"
                ipa_sort_key="${ipa_name#LibrePaint-iPad-}"
                if [[ "$ipa_sort_key" == "$ipa_name" ]]; then
                    ipa_sort_key="${ipa_name#Krita-iPad-}"
                fi
                printf '%s\t%s\n' "$ipa_sort_key" "$ipa"
            done \
            | LC_ALL=C sort -t $'\t' -k1,1 -k2,2
    )
fi

prune_count=$((${#ipa_files[@]} - keep_ipas))
if (( prune_count > 0 )); then
    for ((index = 0; index < prune_count; ++index)); do
        echo "removing old IPA: ${ipa_files[index]}"
        rm -- "${ipa_files[index]}"
    done
fi

# This script is intentionally deployment-only. During dependency pinning and
# the final clean bootstrap, do not preserve the legacy dev shell, host tools,
# or cache-deployment closure. bootstrap-ios-dependencies.sh owns that phase.
#
# Once normal deployment has resumed, protect everything needed by the active
# iPad build before collecting dead Nix store paths. This prevents a cleanup
# from turning the next incremental build into a dependency download/rebuild.
nix_roots="$repo_root/build-ios/nix-roots"
mkdir -p "$nix_roots"
if [[ "$incremental_build_dir" != /* ]]; then
    incremental_build_dir="$repo_root/$incremental_build_dir"
fi
if [[ ! -d "$incremental_build_dir" ]]; then
    echo "error: incremental build tree does not exist: $incremental_build_dir" >&2
    exit 1
fi
incremental_build_dir="$(cd "$incremental_build_dir" && pwd -P)"
if [[ "$incremental_build_dir" == "/" || "$incremental_build_dir" == "$repo_root" ]]; then
    echo "error: unsafe incremental build tree: $incremental_build_dir" >&2
    exit 1
fi
marker_path="$incremental_build_dir/.krita-ios-incremental-config"
if [[ ! -f "$marker_path" ]] \
    || ! grep -Fxq 'schema=1' "$marker_path" \
    || ! grep -Eq '^fingerprint=[0-9a-f]{64}$' "$marker_path"; then
    echo "error: build tree is not owned by the incremental workflow: $incremental_build_dir" >&2
    exit 1
fi

graph_files=(
    "$incremental_build_dir/CMakeCache.txt"
    "$incremental_build_dir/build.ninja"
    "$incremental_build_dir/CMakeFiles/rules.ninja"
)
for graph_file in "${graph_files[@]}"; do
    if [[ ! -f "$graph_file" ]]; then
        echo "error: incremental build graph is incomplete: $graph_file" >&2
        exit 1
    fi
done
cmake_home="$(
    sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' \
        "$incremental_build_dir/CMakeCache.txt"
)"
if [[ "$cmake_home" != "$repo_root" ]]; then
    echo "error: incremental build tree belongs to '$cmake_home', not '$repo_root'" >&2
    exit 1
fi

# Root the exact store paths recorded by CMake/Ninja without evaluating the
# dirty Git flake. Evaluating the flake here would copy the full Krita source
# into the store after every edit-and-deploy cycle.
closure_paths="$(mktemp "${TMPDIR:-/tmp}/krita-ios-build-closure.XXXXXX")"
closure_references="$(mktemp "${TMPDIR:-/tmp}/krita-ios-build-references.XXXXXX")"
trap 'rm -f "$closure_paths" "$closure_references"' EXIT
if ! grep -Eho '/nix/store/[0-9a-z]{32}-[A-Za-z0-9+._?=-]+' \
    "${graph_files[@]}" | LC_ALL=C sort -u >"$closure_paths"; then
    echo "error: failed to extract Nix store paths from the incremental graph" >&2
    exit 1
fi
if [[ ! -s "$closure_paths" ]]; then
    echo "error: incremental build graph contains no Nix store paths" >&2
    exit 1
fi
while IFS= read -r store_path; do
    if ! nix-store --check-validity "$store_path" 2>/dev/null; then
        echo "error: incremental build input is absent from the Nix store: $store_path" >&2
        exit 1
    fi
done <"$closure_paths"

build_closure_root="$(
    KRITA_IOS_CACHE_PHASE=deployment \
    KRITA_IOS_BUILD_CLOSURE_PATHS="$closure_paths" \
        nix eval --impure --raw --file "$repo_root/nix/ios/build-closure-root.nix"
)"
nix-store --query --references "$build_closure_root" \
    | LC_ALL=C sort -u >"$closure_references"
if ! cmp -s "$closure_paths" "$closure_references"; then
    echo "error: iOS build closure root is incomplete; Nix GC disabled" >&2
    exit 1
fi
# Replace the last known-good build closure only after proving the complete
# direct reference set. The referenced store paths retain their own closures.
nix-store --realise "$build_closure_root" \
    --add-root "$nix_roots/ios-dependencies-build" >/dev/null

available_kib="$(df -Pk "$repo_root" | awk 'NR == 2 { print $4 }')"
minimum_free_kib=$((minimum_free_gib * 1024 * 1024))
if (( ! force_nix_gc && available_kib >= minimum_free_kib )); then
    echo "Nix GC skipped: free space is above ${minimum_free_gib} GiB"
    exit 0
fi

echo "collecting unreachable Nix store paths..."
nix-store --gc
