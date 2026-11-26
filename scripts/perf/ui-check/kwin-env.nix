# SPDX-License-Identifier: GPL-3.0-or-later
#
# The compositor side of the KDE Plasma UI check (UI_CHECK_SESSION=kwin of
# run-ui-check.sh): KWin, the D-Bus session bus it needs, a font
# configuration, Python with PyGObject for kwin_screenshot.py, and pywayland
# with KWin's fake input protocol for kwin_click.py. nixpkgs
# comes from the repository's flake.lock without evaluating the repository as
# a flake, so the source tree is not copied to the Nix store.
#   nix-build scripts/perf/ui-check/kwin-env.nix -o build/perf/kwin-env
let
  lock = builtins.fromJSON (builtins.readFile ../../../flake.lock);
  node = lock.nodes.${lock.nodes.${lock.root}.inputs.nixpkgs}.locked;
  pkgs = import (fetchTarball {
    url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    sha256 = node.narHash;
  }) { };
  python = pkgs.python3.withPackages (p: [
    p.pygobject3
    p.pywayland
  ]);
  # Python bindings of KWin's fake input protocol for kwin_click.py.
  fakeInput = pkgs.runCommand "kwin-fake-input-pywayland" { nativeBuildInputs = [ pkgs.pkg-config pkgs.wayland-scanner ]; } ''
    mkdir -p $out
    ${python}/bin/python -m pywayland.scanner \
      -i ${pkgs.kdePackages.plasma-wayland-protocols}/share/plasma-wayland-protocols/fake-input.xml -o $out
  '';
in
pkgs.buildEnv {
  name = "librepaint-kwin-ui-check";
  paths = [
    pkgs.kdePackages.kwin
    pkgs.dbus
    pkgs.busybox
    pkgs.glib.out
    python
  ];
  pathsToLink = [ "/bin" "/etc" "/lib" "/libexec" "/share" ];
  postBuild = ''
    ln -s ${pkgs.mesa} $out/opengl-driver
    ln -s ${fakeInput} $out/fake-input
    ln -s ${pkgs.makeFontsConf { fontDirectories = [ pkgs.dejavu_fonts ]; }} $out/fonts.conf
  '';
}
