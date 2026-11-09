# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the KDE Frameworks without their Python bindings. LibrePaint links
# the C++ libraries only. The bindings pull PySide6, which pulls Qt WebEngine
# and Qt WebView into every build that compiles Qt itself, as the Qt 6.11.3
# overlay does. nixpkgs enables the bindings through the hasPythonBindings
# argument of mkKdeDerivation, which adds the "python" output, shiboken6, a
# Python environment and PySide6. This overlay removes those inputs and turns
# the CMake option off.
final: previous:
let
  inherit (final) lib;
  bindingInput =
    input:
    let
      name = lib.getName input;
    in
    lib.elem name [
      "shiboken6"
      "pyside6"
    ]
    || (lib.hasPrefix "python3" name && lib.hasSuffix "env" name);
  withoutBindings =
    package:
    let
      result = package.overrideAttrs (old: {
        outputs = lib.remove "python" old.outputs;
        nativeBuildInputs = lib.filter (input: !bindingInput input) old.nativeBuildInputs;
        buildInputs = lib.filter (input: !bindingInput input) old.buildInputs;
        cmakeFlags = (old.cmakeFlags or [ ]) ++ [ "-DBUILD_PYTHON_BINDINGS=OFF" ];
      });
    in
    # mkKdeDerivation attaches meta after mkDerivation; overrideAttrs drops it.
    result // { inherit (package) meta; };
in
{
  kdePackages = previous.kdePackages.overrideScope (
    _kdeFinal: kdePrevious:
    lib.genAttrs [
      "kcalendarcore"
      "kcoreaddons"
      "kguiaddons"
      "knotifications"
      "kstatusnotifieritem"
      "kunitconversion"
      "kwidgetsaddons"
      "kxmlgui"
    ] (name: withoutBindings kdePrevious.${name})
  );
}
