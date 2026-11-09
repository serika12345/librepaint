# SPDX-License-Identifier: GPL-3.0-or-later
#
# The PyQt6 that LibrePaint's Python plugins load, used both to build the
# Python bindings and at run time. The plugins import QtCore, QtGui,
# QtWidgets, QtXml and QtQml. The nixpkgs switches below leave out the
# bindings whose Qt modules are heavy and unused (Qt WebEngine behind
# QtPdf, Location, Multimedia, Speech, SerialPort, WebSockets). PyQt6 builds a
# binding only for the Qt modules it finds, so removing Qt Quick 3D, Quick
# Timeline and WebChannel from its inputs keeps those modules out of the build.
{ pythonPackages }:
let
  unusedQtModules = [
    "qtquick3d"
    "qtquicktimeline"
    "qtwebchannel"
  ];
  used = input: !(builtins.elem (input.pname or "") unusedQtModules);
in
(pythonPackages.pyqt6.override {
  withLocation = false;
  withMultimedia = false;
  withPdf = false;
  withSerialPort = false;
  withSpeech = false;
  withWebSockets = false;
}).overrideAttrs
  (old: {
    nativeBuildInputs = builtins.filter used old.nativeBuildInputs;
    buildInputs = builtins.filter used old.buildInputs;
  })
