# SPDX-License-Identifier: GPL-3.0-or-later
#
# wgpu-native for Windows (x86_64-w64-mingw32), cross-built from the pinned
# nixpkgs for wgpu_embed_check only; the product does not depend on it. The
# binary cache has no build of it, so the first nix-build compiles it (about
# 15 minutes on the Linux development host).
#   nix-build scripts/perf/wgpu-embed/wgpu-native-windows.nix -o build/perf/wgpu-native-mingw
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
in
pkgs.pkgsCross.mingwW64.wgpu-native
