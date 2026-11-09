# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tools for mutter_capture.py: Python with PyGObject and numpy, GStreamer
# and its PipeWire source. nixpkgs comes from the repository's flake.lock
# without evaluating the repository as a flake, so the source tree is not
# copied to the Nix store.
#   nix-shell scripts/perf/pen-latency/linux-env.nix --run '...'
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
  gst = pkgs.gst_all_1;
in
pkgs.mkShell {
  packages = [
    (pkgs.python3.withPackages (p: [ p.pygobject3 p.numpy ]))
    gst.gstreamer
    gst.gst-plugins-base
    pkgs.pipewire
    pkgs.gobject-introspection
  ];
  GST_PLUGIN_SYSTEM_PATH_1_0 = pkgs.lib.makeSearchPath "lib/gstreamer-1.0" [
    gst.gstreamer.out
    gst.gst-plugins-base
    pkgs.pipewire
  ];
}
