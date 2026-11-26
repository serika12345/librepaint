# SPDX-License-Identifier: GPL-3.0-or-later
#
# The X server side of the Xorg UI check (run-ui-check-xorg.sh): the Xorg
# server with the dummy video driver and the libinput input driver, a window
# manager, and the tools that authorize, screenshot and click. opengl-driver
# links Mesa, which the container mounts at /run/opengl-driver so that the
# server offers GLX with software rendering, as a desktop X server offers GLX. nixpkgs comes
# from the repository's flake.lock without evaluating the repository as a
# flake, so the source tree is not copied to the Nix store.
#   nix-build scripts/perf/ui-check/xorg-env.nix -o build/perf/xorg-env
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
in
pkgs.buildEnv {
  name = "librepaint-xorg-ui-check";
  paths = [
    pkgs.xorg.xorgserver
    pkgs.xorg.xf86videodummy
    pkgs.xorg.xf86inputlibinput
    pkgs.xorg.xauth
    pkgs.xorg.xwd
    pkgs.openbox
    pkgs.xdotool
    pkgs.imagemagick
    pkgs.busybox
  ];
  pathsToLink = [ "/bin" "/etc" "/lib/xorg/modules" "/share" ];
  postBuild = "ln -s ${pkgs.mesa} $out/opengl-driver";
}
