{
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  frameworks = import ./kde-frameworks.nix { inherit pkgs; };
  kseexpr = pkgs.kseexpr.override {
    kdePackages = pkgs.kdePackages // {
      inherit (frameworks) ki18n;
    };
  };
  python = pkgs.python3;
  pythonPackages = pkgs.python3Packages;
  pythonVersion = python.pythonVersion;
  runtimePyQt = pythonPackages.pyqt6.override {
    withLocation = false;
    withMultimedia = false;
    withPdf = false;
    withSerialPort = false;
    withSpeech = false;
    withWebSockets = false;
  };
  runtimePython = python.withPackages (packages: [
    runtimePyQt
    packages."pyqt6-sip"
  ]);
  # The standalone framework changes only when Python or PyQt changes; source
  # edits rebuild the application without rebuilding this runtime input.
  pythonRuntime =
    pkgs.runCommand "librepaint-python-framework-${pythonVersion}"
      {
        nativeBuildInputs = [ pkgs.darwin.cctools ];
        passthru = {
          inherit runtimePyQt runtimePython;
        };
      }
      ''
        versionDir="$out/Versions/${pythonVersion}"
        pythonLib="$versionDir/Python"
        pythonStdlib="$versionDir/lib/python${pythonVersion}"

        mkdir -p "$pythonStdlib/site-packages" "$versionDir/Resources"
        cp ${python}/lib/libpython${pythonVersion}.dylib "$pythonLib"
        chmod u+w "$pythonLib"
        install_name_tool \
          -id "@rpath/Python.framework/Versions/${pythonVersion}/Python" \
          "$pythonLib"

        cp -RL ${python}/lib/python${pythonVersion}/. "$pythonStdlib"
        chmod -R u+w "$pythonStdlib"
        rm -rf "$pythonStdlib/site-packages"
        mkdir -p "$pythonStdlib/site-packages"
        cp -RL ${runtimePython}/${python.sitePackages}/. \
          "$pythonStdlib/site-packages"

        mkdir -p "$versionDir/lib"
        ln -s ../Python "$versionDir/lib/libpython${pythonVersion}.dylib"
        ln -s "${pythonVersion}" "$out/Versions/Current"
        ln -s Versions/Current/Python "$out/Python"
        ln -s Versions/Current/Resources "$out/Resources"
      '';
  librepaint = import ./krita.nix {
    inherit
      pkgs
      source
      frameworks
      kseexpr
      pythonRuntime
      ;
  };
  macosDependencyMembers = librepaint.macosDependencyMembers;
  # Keep a source-independent reference to every direct input used by the
  # macOS app recipe. The manifest makes the members part of this output's
  # runtime closure without trying to merge their overlapping directory trees.
  macosDependencies =
    pkgs.runCommand "librepaint-macos-dependencies"
      {
        passthru.macosDependencyMembers = macosDependencyMembers;
      }
      ''
        mkdir -p "$out/nix-support"
        printf '%s\n' ${lib.escapeShellArgs (map toString macosDependencyMembers)} \
          > "$out/nix-support/macos-dependency-members"
      '';
  devShell = pkgs.mkShell {
    inputsFrom = [ librepaint ];
    packages = [ pkgs.nixfmt ];

    shellHook = ''
      echo "LibrePaint macOS development shell"
      echo "  build: nix build .#librepaint-macos"
    '';
  };
in
{
  inherit
    devShell
    frameworks
    kseexpr
    librepaint
    macosDependencies
    ;
}
