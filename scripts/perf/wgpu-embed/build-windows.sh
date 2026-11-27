#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Cross-builds wgpu_embed_check for Windows on the Linux development host,
# with the MinGW toolchain and Qt of the product's Windows build environment
# (the librepaint-windows shell) and wgpu-native from wgpu-native-windows.nix.
# OUT_DIR/deploy receives the program with every DLL it loads, found by
# following the import tables through the Nix store, and the Qt platform
# plugin; copy that directory to the Windows device.
#
# Usage: build-windows.sh [OUT_DIR]   (default build/perf/wgpu-embed-windows)
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
out=${1:-$root/build/perf/wgpu-embed-windows}

if [ "${LIBREPAINT_WINDOWS_INCREMENTAL_SHELL:-}" != 1 ]; then
  mkdir -p "$out"
  nix-build "$here/wgpu-native-windows.nix" -o "$out/wgpu-native" > /dev/null
  # nix-build names the link of the dev output with a -dev suffix.
  nix-build "$here/wgpu-native-windows.nix" -A dev -o "$out/wgpu-native" > /dev/null
  exec "$root/scripts/platform/enter-incremental-environment" windows librepaint-windows \
    LIBREPAINT_WINDOWS_INCREMENTAL_SHELL "$0" "$out"
fi

export WGPU_NATIVE_INCLUDE=$out/wgpu-native-dev/include
export WGPU_NATIVE_LIB=$out/wgpu-native/lib/libwgpu_native.dll.a
cmake -S "$here" -B "$out/cmake" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER="$CXX" > "$out/cmake.log"
cmake --build "$out/cmake"

deploy=$out/deploy
rm -rf "$deploy"
mkdir -p "$deploy/platforms"
cp "$out/cmake/wgpu_embed_check.exe" "$deploy/"
qtbase=$(sed -n 's/^Qt6Core_DIR:PATH=\(.*\)\/lib\/cmake\/Qt6Core$/\1/p' "$out/cmake/CMakeCache.txt")
cp "$qtbase/lib/qt-6/plugins/platforms/qwindows.dll" "$deploy/platforms/"
chmod -R u+w "$deploy"

# Every DLL that the program or the plugin imports and that a store path of
# the build environment provides; system DLLs are left to Windows.
search=$(printf '%s\n' $(echo "${PATH//:/ }" | tr ' ' '\n' | sed 's|/bin$||') "$qtbase" "$out/wgpu-native" \
  $(env | sed -n 's/^[A-Za-z_]*=\(\/nix\/store\/[^:]*\).*/\1/p') | sort -u)
pending=("$deploy/wgpu_embed_check.exe" "$deploy/platforms/qwindows.dll")
while [ "${#pending[@]}" -gt 0 ]; do
  file=${pending[0]}
  pending=("${pending[@]:1}")
  for dll in $(x86_64-w64-mingw32-objdump -p "$file" | sed -n 's/^\s*DLL Name: //p'); do
    [ -e "$deploy/$dll" ] && continue
    found=$(for dir in $search; do ls "$dir"/bin/"$dll" "$dir"/lib/"$dll" 2> /dev/null || true; done | head -1)
    [ -n "$found" ] || continue
    cp "$found" "$deploy/"
    chmod u+w "$deploy/$dll"
    pending+=("$deploy/$dll")
  done
done
ls "$deploy"
