#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
diagram_dir="$repo_root/docs/architecture"
temporary_dir="$(mktemp -d)"
trap 'rm -rf "$temporary_dir"' EXIT

cd "$repo_root"

shellcheck scripts/docs/*.sh
markdownlint-cli2 "docs/architecture/*.md"
lychee --offline --no-progress docs/architecture/*.md
env -u DEBUG d2 fmt --check "$diagram_dir"/*.d2

for source in "$diagram_dir"/*.d2; do
    output="$temporary_dir/$(basename "${source%.d2}.svg")"
    tracked="${source%.d2}.svg"

    env -u DEBUG d2 --layout dagre "$source" "$output"

    if ! cmp --silent "$tracked" "$output"; then
        echo "Generated diagram is stale: ${tracked#"$repo_root"/}" >&2
        echo "Run scripts/docs/render-architecture.sh inside nix develop .#docs." >&2
        exit 1
    fi
done
