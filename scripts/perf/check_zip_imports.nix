# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tools for check_zip_imports.py: Python with pefile. nixpkgs comes from the
# repository's flake.lock without evaluating the repository as a flake, so
# the source tree is not copied to the Nix store.
#   nix-shell scripts/perf/check_zip_imports.nix --run '...'
let
  lock = builtins.fromJSON (builtins.readFile ../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
in
pkgs.mkShell {
  packages = [
    (pkgs.python3.withPackages (p: [ p.pefile ]))
  ];
}
