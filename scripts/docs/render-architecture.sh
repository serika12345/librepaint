#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
diagram_dir="$repo_root/docs/architecture"

for source in "$diagram_dir"/*.d2; do
    env -u DEBUG d2 --layout dagre "$source" "${source%.d2}.svg"
done
