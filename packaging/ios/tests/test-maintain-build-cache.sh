#!/usr/bin/env bash
set -euo pipefail

fail() {
    echo "error: $*" >&2
    exit 1
}

assert_exists() {
    [[ -e "$1" ]] || fail "expected path to remain: $1"
}

assert_missing() {
    [[ ! -e "$1" ]] || fail "expected path to be pruned: $1"
}

test_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
source_script="$test_dir/../scripts/maintain-build-cache.sh"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/maintain-build-cache-test.XXXXXX")"
if [[ -z "$test_root" || "$test_root" == "/" ]]; then
    fail "unsafe temporary test directory: $test_root"
fi
trap 'rm -rf -- "$test_root"' EXIT

fixture_repo="$test_root/repo"
fixture_script_dir="$fixture_repo/packaging/ios/scripts"
deploy_dir="$fixture_repo/build-ios/deploy"
mkdir -p "$fixture_script_dir" "$deploy_dir"
git -C "$fixture_repo" init -q
cp "$source_script" "$fixture_script_dir/maintain-build-cache.sh"

# Interleave legacy and current names so the test proves that the timestamp
# suffix, rather than the product-name prefix, determines which IPAs are kept.
touch \
    "$deploy_dir/LibrePaint-iPad-20260801000000.ipa" \
    "$deploy_dir/Krita-iPad-20260802000000.ipa" \
    "$deploy_dir/LibrePaint-iOS-20260803000000.ipa" \
    "$deploy_dir/LibrePaint-iOS-20260804000000.ipa"

# Debug evidence and unrelated packages must never be included in IPA pruning.
touch \
    "$deploy_dir/Krita-iPad-20260802000000-krita.log" \
    "$deploy_dir/LibrePaint-iOS-20260803000000-librepaint.log" \
    "$deploy_dir/device-screenshot.png" \
    "$deploy_dir/Other-iPad-20260805000000.ipa"

# The cache script prunes IPAs before validating the incremental build tree.
# An intentionally absent tree stops the isolated fixture before all Nix work.
set +e
output="$({
    KRITA_IOS_KEEP_IPAS=2 \
        "$fixture_script_dir/maintain-build-cache.sh" \
        --deployment --incremental-build-dir missing
} 2>&1)"
status=$?
set -e

[[ "$status" -eq 1 ]] || fail "expected missing build tree to stop the fixture"
[[ "$output" == *"incremental build tree does not exist"* ]] \
    || fail "cache script did not stop at the expected pre-Nix check"

assert_missing "$deploy_dir/LibrePaint-iPad-20260801000000.ipa"
assert_missing "$deploy_dir/Krita-iPad-20260802000000.ipa"
assert_exists "$deploy_dir/LibrePaint-iOS-20260803000000.ipa"
assert_exists "$deploy_dir/LibrePaint-iOS-20260804000000.ipa"

assert_exists "$deploy_dir/Krita-iPad-20260802000000-krita.log"
assert_exists "$deploy_dir/LibrePaint-iOS-20260803000000-librepaint.log"
assert_exists "$deploy_dir/device-screenshot.png"
assert_exists "$deploy_dir/Other-iPad-20260805000000.ipa"

echo "maintain-build-cache IPA pruning test passed"
