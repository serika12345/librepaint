# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build tools and libraries for wgpu_embed_check (build.sh) on Linux and
# macOS. nixpkgs comes from the repository's flake.lock without evaluating
# the repository as a flake, so the source tree is not copied to the Nix
# store. Every package comes from the binary cache of that nixpkgs.
#   nix-shell scripts/perf/wgpu-embed/env.nix --run '...'
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
  qt = pkgs.qt6;
  lib = pkgs.lib;
  linux = pkgs.stdenv.hostPlatform.isLinux;
in
pkgs.mkShell {
  packages = [
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    qt.qtbase
    pkgs.wgpu-native
    pkgs.wgpu-native.dev
  ] ++ lib.optionals linux [
    qt.qtwayland
    pkgs.wayland
    pkgs.libx11
  ];
  WGPU_NATIVE_INCLUDE = "${pkgs.wgpu-native.dev}/include";
  WGPU_NATIVE_LIB = "${pkgs.wgpu-native}/lib/libwgpu_native${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}";
  # The launcher that build.sh writes sets these for the measured process:
  # the platform plugins of qtbase (and qtwayland on Linux), and on Linux the
  # Vulkan loader that wgpu opens at run time next to the system's GPU drivers.
  WGPU_EMBED_QT_PLUGIN_PATH = lib.makeSearchPath "lib/qt-6/plugins" ([ qt.qtbase ] ++ lib.optional linux qt.qtwayland);
  WGPU_EMBED_LIBRARY_PATH = lib.optionalString linux
    (lib.makeLibraryPath [ pkgs.vulkan-loader pkgs.wayland pkgs.libGL ] + ":/run/opengl-driver/lib");
}
