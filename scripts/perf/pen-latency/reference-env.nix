# SPDX-License-Identifier: GPL-3.0-or-later
#
# Python with tkinter for reference_app.py, the X11 reference window of the
# capture-based latency measurement. nixpkgs comes from the repository's
# flake.lock without evaluating the repository as a flake, so the source tree
# is not copied to the Nix store.
#   nix-build scripts/perf/pen-latency/reference-env.nix -o <out-link>
# A host target names <out-link>/bin/python3. The out-link is a garbage
# collection root; a store path that no root refers to disappears at the next
# collection and the target then exits before the measurement.
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
in
pkgs.python3.withPackages (p: [ p.tkinter ])
