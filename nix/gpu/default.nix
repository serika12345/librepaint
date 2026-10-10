# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later
{ nixpkgsRevision, system ? builtins.currentSystem }:
let
  pkgs = import (builtins.getFlake "github:NixOS/nixpkgs/${nixpkgsRevision}").outPath { inherit system; };
  dependency = pkgs.callPackage ./wgpu-native.nix { };
in
pkgs.buildEnv {
  name = "librepaint-gpu-dependencies";
  paths = [ dependency dependency.dev ];
}
