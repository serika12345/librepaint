#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat >&2 <<EOF
usage: $0 --confirm-pinning-complete

Run this once, after every iOS dependency recipe has been pinned and committed.
It releases only the known repository-local legacy GC roots, performs a full
Nix GC, builds the final .#ios-dependencies aggregate from the normal Git flake,
runs the complete KF6 link/code-generation check, and roots that aggregate only
after both builds succeed.
EOF
    exit 2
}

if (( $# != 1 )) || [[ "$1" != "--confirm-pinning-complete" ]]; then
    usage
fi

repo_root="$(git rev-parse --show-toplevel)"
if [[ "$PWD" != "$repo_root" ]]; then
    echo "error: run this script from the repository root: $repo_root" >&2
    exit 1
fi

if [[ -n "$(git status --porcelain=v1 --untracked-files=all)" ]]; then
    echo "error: commit or remove all visible worktree changes before the clean bootstrap" >&2
    exit 1
fi

nix_roots="$repo_root/build-ios/nix-roots"

check_store_link() {
    local root_path="$1"
    local target

    if [[ ! -e "$root_path" && ! -L "$root_path" ]]; then
        return
    fi
    if [[ ! -L "$root_path" ]]; then
        echo "error: expected a GC-root symlink: $root_path" >&2
        exit 1
    fi

    target="$(readlink "$root_path")"
    if [[ "$target" != /nix/store/* ]]; then
        echo "error: GC-root candidate points outside /nix/store: $root_path -> $target" >&2
        exit 1
    fi
}

check_profile_link() {
    local root_path="$1"
    local target

    if [[ ! -e "$root_path" && ! -L "$root_path" ]]; then
        return
    fi
    if [[ ! -L "$root_path" ]]; then
        echo "error: expected a dev-shell profile symlink: $root_path" >&2
        exit 1
    fi

    target="$(readlink "$root_path")"
    if [[ ! "$target" =~ ^dev-shell-[0-9]+-link$ ]]; then
        echo "error: unexpected dev-shell profile target: $root_path -> $target" >&2
        exit 1
    fi
}

# Remove only roots created by the previous dependency/deployment workflow.
# Validate the complete set before removing the first symlink, so an unknown
# entry cannot leave the legacy root set only partly released.
legacy_store_roots=(
    "$nix_roots/host-qttools"
    "$nix_roots/ios-dependencies-build"
    "$nix_roots/ios-dependencies"
    "$repo_root/build-ios/result-ios-dependencies"
    "$repo_root/result"
    "$repo_root/result-1"
)
if [[ -d "$nix_roots" ]]; then
    while IFS= read -r -d '' root_candidate; do
        root_name="$(basename "$root_candidate")"
        case "$root_name" in
            dev-shell)
                ;;
            host-qttools|ios-dependencies-build|ios-dependencies)
                ;;
            dev-shell-*-link)
                if [[ ! "$root_name" =~ ^dev-shell-[0-9]+-link$ ]]; then
                    echo "error: unexpected dev-shell generation name: $root_candidate" >&2
                    exit 1
                fi
                legacy_store_roots+=("$root_candidate")
                ;;
            *)
                echo "error: unrecognized entry in $nix_roots: $root_candidate" >&2
                exit 1
                ;;
        esac
    done < <(find "$nix_roots" -mindepth 1 -maxdepth 1 -print0)
fi

check_profile_link "$nix_roots/dev-shell"
for root_candidate in "${legacy_store_roots[@]}"; do
    check_store_link "$root_candidate"
done

if [[ -e "$nix_roots/dev-shell" || -L "$nix_roots/dev-shell" ]]; then
    echo "releasing legacy GC profile: $nix_roots/dev-shell -> $(readlink "$nix_roots/dev-shell")"
    rm -- "$nix_roots/dev-shell"
fi
for root_candidate in "${legacy_store_roots[@]}"; do
    if [[ -e "$root_candidate" || -L "$root_candidate" ]]; then
        echo "releasing legacy GC root: $root_candidate -> $(readlink "$root_candidate")"
        rm -- "$root_candidate"
    fi
done

echo "collecting all Nix store paths not protected by external GC roots..."
nix-store --gc

echo "checking the committed Git flake without building..."
nix flake check --no-build
nix eval --raw .#ios-dependencies.drvPath >/dev/null

echo "bootstrapping the final iOS dependency aggregate..."
aggregate_list="$(mktemp "${TMPDIR:-/tmp}/krita-ios-bootstrap-aggregate.XXXXXX")"
trap 'rm -f "$aggregate_list"' EXIT
if ! nix build --no-link --print-out-paths .#ios-dependencies > "$aggregate_list"; then
    echo "error: clean bootstrap build failed; no aggregate root was created" >&2
    exit 1
fi
aggregate_paths=()
while IFS= read -r store_path; do
    [[ -n "$store_path" ]] && aggregate_paths+=("$store_path")
done < "$aggregate_list"

if (( ${#aggregate_paths[@]} != 1 )); then
    echo "error: expected one aggregate output, got ${#aggregate_paths[@]}" >&2
    exit 1
fi
aggregate="${aggregate_paths[0]}"
if [[ "$aggregate" != /nix/store/* ]] || ! nix-store --check-validity "$aggregate"; then
    echo "error: bootstrap returned an invalid aggregate path: $aggregate" >&2
    exit 1
fi

echo "checking the bootstrapped KF6 target closure with a complete iOS link..."
if ! nix build --no-link .#kf6-consumer-check; then
    echo "error: KF6 integration check failed; no aggregate root was created" >&2
    exit 1
fi

# Root only the successfully realised final aggregate. Build-time closure roots
# are recreated later by deployment maintenance, never by the clean bootstrap.
mkdir -p "$nix_roots"
final_root="$nix_roots/ios-dependencies"
if [[ -e "$final_root" || -L "$final_root" ]]; then
    echo "error: final aggregate root unexpectedly exists: $final_root" >&2
    exit 1
fi
nix-store --realise "$aggregate" --add-root "$final_root" >/dev/null

if [[ "$(readlink "$final_root")" != "$aggregate" ]]; then
    echo "error: final aggregate root does not reference the bootstrap output" >&2
    exit 1
fi

echo "clean bootstrap succeeded: $aggregate"
echo "final aggregate root:      $final_root"
echo "normal deployments may now run build-librepaint-incremental.sh bootstrap/deploy"
