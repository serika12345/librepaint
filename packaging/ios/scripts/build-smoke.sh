#!/usr/bin/env bash
set -euo pipefail

if (( $# != 1 )); then
    echo "usage: $0 <device|simulator>" >&2
    exit 2
fi

mode="$1"
case "$mode" in
    device)
        platform=DEVICE
        sdk=iphoneos
        ;;
    simulator)
        platform=SIMULATOR
        sdk=iphonesimulator
        ;;
    *)
        echo "error: mode must be device or simulator" >&2
        exit 2
        ;;
esac

repo_root="$(git rev-parse --show-toplevel)"
scripts_dir="$repo_root/packaging/ios/scripts"
build_dir="$repo_root/build-ios/smoke/$mode"
configuration=RelWithDebInfo

"$scripts_dir/check-host.sh"

"$scripts_dir/run-logged.sh" "smoke-$mode-configure" \
    cmake -S "$repo_root/packaging/ios/smoke" -B "$build_dir" -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="$repo_root/packaging/ios/cmake/KritaIOSPlatform.cmake" \
    -DKRITA_IOS_PLATFORM="$platform"

"$scripts_dir/run-logged.sh" "smoke-$mode-build" \
    cmake --build "$build_dir" --config "$configuration" -- \
    -sdk "$sdk" CODE_SIGNING_ALLOWED=NO SDK_STAT_CACHE_ENABLE=NO

binary="$build_dir/$configuration-$sdk/KritaIOSSmoke.app/KritaIOSSmoke"
"$scripts_dir/inspect-apple-binary.sh" "$mode" "$binary"
echo "smoke app: ${binary%/KritaIOSSmoke}"
