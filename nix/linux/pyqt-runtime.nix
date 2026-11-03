{
  runCommand,
  python3,
  python3Packages,
  patchelf,
}:

let
  inherit (python3Packages) pyqt6 pyqt6-sip dbus-python;
  site = python3.sitePackages;
  support = python3Packages.makePythonPath [
    pyqt6-sip
    dbus-python
  ];
in
runCommand "librepaint-pyqt6-runtime-${pyqt6.version}"
  {
    nativeBuildInputs = [ patchelf ];
    disallowedReferences = [ pyqt6 ];
  }
  ''
    mkdir -p "$out/${site}"
    cp -R ${pyqt6}/${site}/. "$out/${site}/"
    chmod -R u+w "$out"
    for module in QtLocation QtPositioning QtMultimedia QtMultimediaWidgets QtPdf QtPdfWidgets QtSerialPort QtTextToSpeech QtWebSockets; do
      rm -rf "$out/${site}/PyQt6/$module"*
    done
    find "$out" -type d -name __pycache__ -prune -exec rm -rf {} +
    while IFS= read -r -d $'\0' module; do
      patchelf --shrink-rpath "$module"
    done < <(find "$out" -type f -name '*.so' -print0)

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$out/${site}:${support}" ${python3.interpreter} - <<'PY'
    import importlib.util
    from PyQt6 import QtCore, QtGui, QtWidgets, QtQml, QtQuick, QtSvg, QtPrintSupport
    for module in ("QtLocation", "QtPositioning", "QtMultimedia", "QtMultimediaWidgets", "QtPdf", "QtPdfWidgets", "QtSerialPort", "QtTextToSpeech", "QtWebSockets"):
        assert importlib.util.find_spec("PyQt6." + module) is None, module
    PY
  ''
