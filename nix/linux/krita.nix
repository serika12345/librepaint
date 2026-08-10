{
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  pythonPackages = pkgs.python3Packages;

  # Keep this list aligned with nixpkgs' krita-unwrapped recipe.  It is shared
  # with linux-dependencies so that the source-independent closure can be
  # fetched or built before the LibrePaint source changes.
  nativeBuildInputs = [
    pkgs.cmake
    pkgs.kdePackages.extra-cmake-modules
    pkgs.pkg-config
    pythonPackages.sip
    pkgs.qt6.wrapQtAppsHook
  ];

  buildInputs = [
    pkgs.boost
    pkgs.libraw
    pkgs.fftw
    pkgs.eigen_5
    pkgs.exiv2
    pkgs.fribidi
    pkgs.lcms2
    pkgs.gsl
    pkgs.openexr
    pkgs.lager
    pkgs.libaom
    pkgs.libheif

    pkgs.giflib
    pkgs.libjxl
    pkgs.openjpeg
    pkgs.opencolorio
    pkgs.xsimd
    pkgs.curl
    pkgs.immer
    pkgs.kseexpr
    pkgs.libmypaint
    pkgs.libunibreak
    pkgs.libwebp
    pkgs.SDL2
    pkgs.zug
    pythonPackages.pyqt6
    pkgs.qt6.qtmultimedia
    pkgs.qt6.qttools
  ]
  ++ (with pkgs.kdePackages; [
    breeze-icons
    karchive
    kcompletion
    kconfig
    kcoreaddons
    kcrash
    kguiaddons
    ki18n
    kio
    kitemmodels
    kitemviews
    kwidgetsaddons
    kwindowsystem
    mlt
    poppler
    quazip
    libkdcraw
  ]);

  pythonPath = pythonPackages.makePythonPath [
    pythonPackages.sip
    pythonPackages.setuptools
  ];

  librepaintUnwrapped = pkgs.stdenv.mkDerivation {
    pname = "librepaint-linux-unwrapped";
    version = "1.0.2";

    src = source;

    inherit nativeBuildInputs buildInputs;

    # Krita's CMake helpers replace PYTHONPATH while probing SIP.  Keep the
    # Nix Python modules visible, as in nixpkgs' krita-unwrapped recipe.
    postPatch = ''
      substituteInPlace cmake/modules/FindSIP.cmake \
        --replace-fail 'PYTHONPATH=''${_pyqt5_python_path}' 'PYTHONPATH=${pythonPath}'
      substituteInPlace cmake/modules/SIPMacros.cmake \
        --replace-fail 'PYTHONPATH=''${_krita_python_path}' 'PYTHONPATH=${pythonPath}'

      substituteInPlace plugins/impex/jp2/jp2_converter.cc \
        --replace-fail '<openjpeg.h>' '<${pkgs.openjpeg.incDir}/openjpeg.h>'
    '';

    cmakeBuildType = "RelWithDebInfo";
    cmakeFlags = [
      "-DALLOW_UNSTABLE=QT6"
      "-DBUILD_KRITA_QT_DESIGNER_PLUGINS=ON"
      "-DBUILD_WITH_QT6=ON"
      "-DENABLE_UPDATERS=OFF"
    ];

    passthru.linuxDependencyMembers =
      nativeBuildInputs
      ++ buildInputs
      ++ [
        pkgs.wrapGAppsHook3
      ];

    meta = {
      description = "LibrePaint digital painting application for Linux";
      homepage = "https://github.com/serika12345/librepaint";
      license = lib.licenses.gpl3Only;
      mainProgram = "krita";
      platforms = lib.platforms.linux;
    };
  };

  # The upstream Krita wrapper includes G'MIC by default.  Override only its
  # Krita build input so its ABI follows this source build while retaining the
  # source, flags, and dependency recipe from nixpkgs.
  librepaintGmic = pkgs.krita-plugin-gmic.override {
    krita-unwrapped = librepaintUnwrapped;
  };
in
{
  inherit librepaintGmic librepaintUnwrapped;

  librepaint = pkgs.symlinkJoin {
    pname = "librepaint-linux";
    inherit (librepaintUnwrapped) version buildInputs;

    meta = librepaintUnwrapped.meta // {
      mainProgram = "LibrePaint";
    };

    nativeBuildInputs = librepaintUnwrapped.nativeBuildInputs ++ [
      pkgs.wrapGAppsHook3
    ];

    paths = [
      librepaintUnwrapped
      librepaintGmic
    ];

    # This is intentionally the same runtime wrapper as nixpkgs' Krita
    # package.  It keeps the desktop, GLib, Qt, Python, and plugin discovery
    # environment intact without copying dependency trees into the output.
    postBuild = ''
      mv "$out/bin/krita" "$out/bin/LibrePaint"
      substituteInPlace "$out/share/applications/org.kde.krita.desktop" \
        --replace-fail "Exec=krita %F" "Exec=LibrePaint %F"
      substituteInPlace "$out/share/metainfo/org.kde.krita.appdata.xml" \
        --replace-fail "<binary>krita</binary>" "<binary>LibrePaint</binary>"

      gappsWrapperArgsHook
      wrapQtApp "$out/bin/LibrePaint" \
        "''${gappsWrapperArgs[@]}" \
        --prefix PYTHONPATH : "$PYTHONPATH" \
        --set KRITA_PLUGIN_PATH "$out/lib/kritaplugins"
    '';

    passthru = {
      inherit librepaintGmic librepaintUnwrapped;
      linuxDependencyMembers = librepaintUnwrapped.linuxDependencyMembers;
    };
  };
}
