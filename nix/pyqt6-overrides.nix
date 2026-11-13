# PyQt6 6.12.0 overrides, shared by the Linux, macOS and Windows interpreters.
# PyQt6 6.12.0 matches Qt 6.12.0; nixpkgs still has 6.11.0.  Remove this file when nixpkgs
# packages PyQt6 6.12.0 (it also needs sip 6.17 and PyQt-builder 1.20).
{ lib }:
self: super: {
  sip = super.sip.overridePythonAttrs (_: {
    version = "6.17.3";
    src = self.fetchPypi {
      pname = "sip";
      version = "6.17.3";
      hash = "sha256-47XC0UKalvLh/CW2nUzrIyvIt/I5XoZMkEzsKJBWQrU=";
    };
    patches = [ ];
  });

  pyqt-builder = super.pyqt-builder.overridePythonAttrs (_: {
    version = "1.21.0";
    src = self.fetchPypi {
      pname = "pyqt_builder";
      version = "1.21.0";
      hash = "sha256-Zz76JyN6gf/vx9mmQZVlYAoSm09M9NNYyJo0izmzpFQ=";
    };
  });

  pyqt6-sip = super.pyqt6-sip.overridePythonAttrs (_: {
    version = "13.13.1";
    src = self.fetchPypi {
      pname = "pyqt6_sip";
      version = "13.13.1";
      hash = "sha256-g8PJy5O2xjGvcEgIT7DyNLYQoB7b+bK0TE7kvaQZfv4=";
    };
  });

  # PyQt6 6.12.0 is not cached, so build only the bindings LibrePaint's Python plugins import
  # (QtCore, QtGui, QtWidgets, QtXml, QtQml); this also keeps Qt WebEngine and Qt Multimedia
  # (FFmpeg 9) out of the PyQt6 closure.
  pyqt6 = (super.pyqt6.override {
    withLocation = false;
    withMultimedia = false;
    withPdf = false;
    withSerialPort = false;
    withSpeech = false;
    withWebSockets = false;
  }).overridePythonAttrs (old: {
    version = "6.12.0";
    src = self.fetchPypi {
      pname = "pyqt6";
      version = "6.12.0";
      hash = "sha256-0+Ahtxa66IAiRCL50biU0tU95+YOYNvEP4YHI/nMMAs=";
    };
    # Qt 6.12 drops operator!= for QLocale in C++20 mode, which GCC 16 uses by default;
    # the generated PyQt6 6.12.0 code calls it explicitly.
    env = (old.env or { }) // {
      NIX_CFLAGS_COMPILE = toString [ (old.env.NIX_CFLAGS_COMPILE or "") "-std=gnu++17" ];
    };
    # 6.12.0 no longer ships the two enumeration values this patch removes.
    patches = lib.filter (
      patch:
      !(lib.hasInfix "qt-6.12.patch" (patch.name or ""))
      && !(lib.hasInfix "qt-6.12.patch" (patch.url or ""))
    ) (old.patches or [ ]);
    nativeBuildInputs = lib.filter (
      input: !(builtins.elem (input.pname or "") [ "qtquick3d" "qtquicktimeline" "qtwebchannel" ])
    ) old.nativeBuildInputs;
    buildInputs = lib.filter (
      input: !(builtins.elem (input.pname or "") [ "qtquick3d" "qtquicktimeline" "qtwebchannel" ])
    ) old.buildInputs;
    pythonImportsCheck = [
      "PyQt6"
      "PyQt6.QtCore"
      "PyQt6.QtGui"
      "PyQt6.QtWidgets"
      "PyQt6.QtXml"
      "PyQt6.QtQml"
    ];
  });
}
