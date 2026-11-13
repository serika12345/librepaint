# PyQt6 6.12.0 trial: an interpreter whose PyQt6 matches Qt 6.12.0.
{ lib, python }:
let
  py = python.override {
    packageOverrides = self: super: {
      # PyQt6 6.12.0 matches Qt 6.12.0; nixpkgs still has 6.11.0.  Remove this file when nixpkgs
      # packages PyQt6 6.12.0 (it also needs sip 6.17 and PyQt-builder 1.20).
      sip = super.sip.overridePythonAttrs (_: {
        version = "6.17.3";
        src = self.fetchPypi {
          pname = "sip";
          version = "6.17.3";
          hash = "sha256-47XC0UKalvLh/CW2nUzrIyvIt/I5XoZMkEzsKJBWQrU=";
        };
        patches = [ ];
      });

      # PyQt6 6.12.0 matches Qt 6.12.0; nixpkgs still has 6.11.0.  Remove this file when nixpkgs
      # packages PyQt6 6.12.0 (it also needs sip 6.17 and PyQt-builder 1.20).
      pyqt-builder = super.pyqt-builder.overridePythonAttrs (_: {
        version = "1.21.0";
        src = self.fetchPypi {
          pname = "pyqt_builder";
          version = "1.21.0";
          hash = "sha256-Zz76JyN6gf/vx9mmQZVlYAoSm09M9NNYyJo0izmzpFQ=";
        };
      });

      # PyQt6 6.12.0 matches Qt 6.12.0; nixpkgs still has 6.11.0.  Remove this file when nixpkgs
      # packages PyQt6 6.12.0 (it also needs sip 6.17 and PyQt-builder 1.20).
      pyqt6-sip = super.pyqt6-sip.overridePythonAttrs (_: {
        version = "13.13.1";
        src = self.fetchPypi {
          pname = "pyqt6_sip";
          version = "13.13.1";
          hash = "sha256-g8PJy5O2xjGvcEgIT7DyNLYQoB7b+bK0TE7kvaQZfv4=";
        };
      });

      # PyQt6 6.12.0 matches Qt 6.12.0; nixpkgs still has 6.11.0.  Remove this file when nixpkgs
      # packages PyQt6 6.12.0 (it also needs sip 6.17 and PyQt-builder 1.20).
      pyqt6 = super.pyqt6.overridePythonAttrs (old: {
        version = "6.12.0";
        src = self.fetchPypi {
          pname = "pyqt6";
          version = "6.12.0";
          hash = "sha256-0+Ahtxa66IAiRCL50biU0tU95+YOYNvEP4YHI/nMMAs=";
        };
        # 6.12.0 no longer ships the two enumeration values this patch removes.
        patches = lib.filter (
          patch:
          !(lib.hasInfix "qt-6.12.patch" (patch.name or ""))
          && !(lib.hasInfix "qt-6.12.patch" (patch.url or ""))
        ) (old.patches or [ ]);
      });
    };
    self = py;
  };
in
py
