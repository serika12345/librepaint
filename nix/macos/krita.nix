{
  pkgs,
  source,
  frameworks,
  kseexpr,
}:

let
  inherit (pkgs) lib;
  qt = pkgs.qt6Packages;
  ffmpeg = pkgs.ffmpeg;
  mltOpenCV = pkgs.opencv4.override { ffmpeg-headless = ffmpeg; };
  frei0r = pkgs.frei0r.override { opencv = mltOpenCV; };
  mlt = qt.mlt.override {
    inherit ffmpeg frei0r;
  };
  nativeBuildInputs = [
    pkgs.cmake
    pkgs.gettext
    pkgs.ninja
    pkgs.pkg-config
    pkgs.python3
    pkgs.kdePackages.extra-cmake-modules
    qt.qttools
    qt.wrapQtAppsHook
  ];
  buildInputs = [
    pkgs.boost
    pkgs.eigen_5
    pkgs.exiv2
    ffmpeg
    pkgs.fftw
    pkgs.fontconfig
    frei0r
    pkgs.freetype
    pkgs.fribidi
    pkgs.giflib
    pkgs.gsl
    pkgs.harfbuzz
    pkgs.immer
    pkgs.lager
    pkgs.lcms2
    pkgs.libheif
    pkgs.libjpeg_turbo
    pkgs.libjxl
    pkgs.libmypaint
    pkgs.libpng
    pkgs.libraw
    pkgs.libtiff
    pkgs.libunibreak
    pkgs.libwebp
    pkgs.opencolorio
    pkgs.openexr
    pkgs.openjpeg
    pkgs.SDL2
    pkgs.xsimd
    pkgs.zlib
    pkgs.zug
    kseexpr
    mlt
    qt.poppler
    qt.qt5compat
    qt.qtbase
    qt.qtdeclarative
    qt.qtsvg
    qt.quazip
    frameworks.kcodecs
    frameworks.kcolorscheme
    frameworks.kcompletion
    frameworks.kconfig
    frameworks.kcoreaddons
    frameworks.kguiaddons
    frameworks.ki18n
    frameworks.kitemviews
    frameworks.libkdcraw
    frameworks.kwidgetsaddons
  ];
  deploymentTarget = pkgs.stdenv.hostPlatform.darwinMinVersion;
in
pkgs.stdenv.mkDerivation {
  pname = "librepaint-macos";
  version = "1.0.2";

  src = source;
  strictDeps = true;

  inherit nativeBuildInputs buildInputs;

  postPatch = ''
    substituteInPlace plugins/impex/jp2/jp2_converter.cc \
      --replace-fail '<openjpeg.h>' '<${pkgs.openjpeg.incDir}/openjpeg.h>'
  '';

  cmakeBuildType = "Release";
  cmakeFlags = [
    "-DBUILD_KRITA_QT_DESIGNER_PLUGINS=OFF"
    "-DBUILD_TESTING=OFF"
    "-DBUILD_WITH_QT6=ON"
    "-DCMAKE_DISABLE_FIND_PACKAGE_PythonLibrary=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6WaylandClient=TRUE"
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=${deploymentTarget}"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Mlt7=TRUE"
    "-DKRITA_MACOS_USE_XCODE_TOOLS=OFF"
    "-DMACOS_MINIMUM_VERSION=${deploymentTarget}"
  ];

  qtWrapperArgs = [
    "--set FONTCONFIG_FILE ${pkgs.fontconfig.out}/etc/fonts/fonts.conf"
  ];

  preConfigure = ''
    if "$CC" --version | grep -q Apple; then
      echo "The macOS recipe requires the nixpkgs LLVM toolchain." >&2
      exit 1
    fi

    buildPrefix=/build/librepaint
    export NIX_CFLAGS_COMPILE="''${NIX_CFLAGS_COMPILE-} -ffile-prefix-map=$NIX_BUILD_TOP=$buildPrefix -fmacro-prefix-map=$NIX_BUILD_TOP=$buildPrefix -fdebug-prefix-map=$NIX_BUILD_TOP=$buildPrefix"
  '';

  postConfigure = ''
    grep -Fx "#define KRITA_BUILD_DIR \"$PWD\"" KoConfig.h
    substituteInPlace KoConfig.h \
      --replace-fail "#define KRITA_BUILD_DIR \"$PWD\"" \
      '#define KRITA_BUILD_DIR "/build/librepaint"'
  '';

  postInstall = ''
    app="$out/bin/LibrePaint.app"
    test -x "$app/Contents/MacOS/LibrePaint"
    test -f "$app/Contents/Resources/LibrePaint.icns"
    rm -f "$out/bin/krita_version"
    rm -rf "$out/share/applications" "$out/share/metainfo"
    ${pkgs.python3}/bin/python3 - "$app/Contents/Info.plist" <<'PY'
    import plistlib
    import sys

    with open(sys.argv[1], "rb") as handle:
        info = plistlib.load(handle)

    expected = {
        "CFBundleDisplayName": "LibrePaint",
        "CFBundleExecutable": "LibrePaint",
        "CFBundleIdentifier": "local.librepaint.macos",
        "CFBundleName": "LibrePaint",
    }
    for key, value in expected.items():
        actual = info.get(key)
        if actual != value:
            raise SystemExit(f"{key} is {actual!r}; expected {value!r}")
    PY
  '';

  postFixup = ''
    app="$out/bin/LibrePaint.app"
    mkdir -p "$app/Contents/PlugIns"
    ln -s ${mlt}/lib/mlt "$app/Contents/PlugIns/mlt"
    ln -s ${frei0r}/lib/frei0r-1 "$app/Contents/PlugIns/frei0r-1"
    ln -s ${mlt}/share/mlt "$app/Contents/Resources/mlt"
    ln -s ${lib.getExe ffmpeg} "$app/Contents/MacOS/ffmpeg"
    ln -s ${lib.getExe' ffmpeg "ffprobe"} "$app/Contents/MacOS/ffprobe"

    if grep -r -a -F -q "$NIX_BUILD_TOP" "$out"; then
      echo "Nix build root embedded in the macOS output:" >&2
      grep -r -a -F -l "$NIX_BUILD_TOP" "$out" >&2
      exit 1
    fi
  '';

  enableParallelBuilding = true;

  passthru.macosDependencyMembers = nativeBuildInputs ++ buildInputs;

  meta = {
    description = "LibrePaint digital painting application for macOS";
    homepage = "https://github.com/serika12345/librepaint";
    license = lib.licenses.gpl3Only;
    mainProgram = "LibrePaint.app/Contents/MacOS/LibrePaint";
    platforms = [ "aarch64-darwin" ];
  };
}
