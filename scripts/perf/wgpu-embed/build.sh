#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds wgpu_embed_check into OUT_DIR (default build/perf/wgpu-embed) and
# writes OUT_DIR/wgpu-embed-check, a launcher that sets the Qt plugin path
# and the Vulkan loader path of env.nix and runs the program. Measurement
# targets name the launcher.
#
# Usage: build.sh [OUT_DIR]
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
out=${1:-$root/build/perf/wgpu-embed}
mkdir -p "$out"

nix-shell "$here/env.nix" --run \
  "cmake -S '$here' -B '$out/cmake' -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo > '$out/cmake.log' && cmake --build '$out/cmake'"
paths=$(nix-shell "$here/env.nix" --run 'printf "%s\n%s\n" "$WGPU_EMBED_QT_PLUGIN_PATH" "$WGPU_EMBED_LIBRARY_PATH"')
plugins=$(sed -n 1p <<< "$paths")
libraries=$(sed -n 2p <<< "$paths")

cat > "$out/wgpu-embed-check" <<EOF
#!/usr/bin/env bash
export QT_PLUGIN_PATH='$plugins'
export LD_LIBRARY_PATH='$libraries'\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}
exec '$out/cmake/wgpu_embed_check' "\$@"
EOF
chmod +x "$out/wgpu-embed-check"
echo "launcher: $out/wgpu-embed-check"
