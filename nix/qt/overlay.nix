# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds every platform against the same Qt release. The pinned nixpkgs
# provides Qt 6.11.1, which lacks two fixes LibrePaint depends on:
# QTBUG-147854 (GLX swap interval lost on a second context, which made each
# Linux X11 buffer swap block for a frame) and QTBUG-147602 (QImage::toCGImage
# releasing the colour space, which crashed macOS when a document opened).
# Qt 6.11.3 has both and matches the Windows Qt fork branch for-krita/6.11.3.
#
# The overlay replaces the release sources of all Qt modules and keeps the
# nixpkgs build expressions. Remove it when the pinned nixpkgs provides a Qt
# release that the Windows fork also supports.
{ nixpkgs }:
final: previous:
let
  srcs = import ./srcs.nix {
    inherit (final) fetchurl;
    mirror = "mirror://qt";
  };
in
{
  # Keep .override on the result: nixpkgs packages such as PyQt call
  # qt6.override, which a plain overrideScope result does not provide.
  qt6 = final.lib.makeOverridable (
    args:
    (previous.qt6.override args).overrideScope (
      qtFinal: qtPrevious: {
        inherit srcs;
        qtModule = qtFinal.callPackage "${nixpkgs}/pkgs/development/libraries/qt-6/qtModule.nix" {
          inherit srcs;
          inherit (qtFinal) darwinVersionInputs;
        };
        qtbase = qtPrevious.qtbase.overrideAttrs {
          inherit (srcs.qtbase) src version;
        };
        # Qt Multimedia only needs Qt Quick 3D for its spatial audio QML types,
        # which LibrePaint does not use. nixpkgs already builds it without Qt
        # Quick 3D for Windows; doing the same everywhere keeps Qt Quick 3D out
        # of the build.
        qtmultimedia = qtPrevious.qtmultimedia.overrideAttrs (old: {
          propagatedBuildInputs = builtins.filter (
            input: (input.pname or "") != "qtquick3d"
          ) old.propagatedBuildInputs;
        }) // {
          inherit (qtPrevious.qtmultimedia) meta;
        };
        # nixpkgs backports two qtdeclarative fixes that Qt 6.11.3 already
        # contains; applying them again fails. qtModule.nix attaches meta to
        # the derivation after mkDerivation, and overrideAttrs returns a plain
        # derivation, so meta has to be carried over for consumers that read it
        # (for example the Android Qt modules).
        qtdeclarative = qtPrevious.qtdeclarative.overrideAttrs (old: {
          patches = builtins.filter (
            patch:
            builtins.match ".*(8a2c82be6ad90e3f2a0760d8bab1e3a8cdb2473a|9d4d376726a6ce15c429128dc65b927e411e40da).*" (toString patch) == null
          ) (old.patches or [ ]);
        }) // {
          inherit (qtPrevious.qtdeclarative) meta;
        };
      }
    )
  ) { };
}
