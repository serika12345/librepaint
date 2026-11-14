# PyQt6 6.12.0 interpreter: an interpreter whose PyQt6 matches Qt 6.12.0.
{ lib, python }:
let
  py = python.override {
    packageOverrides = import ./pyqt6-overrides.nix { inherit lib; };
    self = py;
  };
in
py
