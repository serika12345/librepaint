{
  pkgs,
  versionsFile,
  dependencyManifestFile,
  qtManifestFile,
  frameworkManifestFile,
  pluginProfileFile,
  kritaSource,
}:

let
  lib = pkgs.lib;

  parseEnv =
    file:
    builtins.listToAttrs (
      map
        (
          line:
          let
            match = builtins.match "([A-Za-z_][A-Za-z0-9_]*)=(.*)" line;
          in
          {
            name = builtins.elemAt match 0;
            value = builtins.elemAt match 1;
          }
        )
        (
          builtins.filter (line: line != "" && builtins.match "[[:space:]]*#.*" line == null) (
            lib.splitString "\n" (builtins.readFile versionsFile)
          )
        )
    );

  versions = parseEnv versionsFile;
  dependencyManifest = builtins.fromJSON (builtins.readFile dependencyManifestFile);
  qtManifest = builtins.fromJSON (builtins.readFile qtManifestFile);
  frameworkManifest = builtins.fromJSON (builtins.readFile frameworkManifestFile);
  dependencyPackageNames = map (package: package.name) dependencyManifest.packages;
  dependencyByName =
    assert lib.assertMsg (dependencyManifest.schema == 1) "unsupported iOS dependency manifest schema";
    assert lib.assertMsg (
      dependencyPackageNames == [
        "zlib"
        "libdeflate"
        "expat"
        "libpng"
        "libjpeg-turbo"
        "libwebp"
        "libtiff"
        "openjpeg"
        "imath"
        "openexr"
        "libffi"
        "pcre2"
        "glib"
        "json-c"
        "libmypaint"
        "boost"
        "immer"
        "zug"
        "lager"
        "eigen"
        "xsimd"
        "lcms2"
        "exiv2"
        "freetype"
        "harfbuzz"
        "libunibreak"
        "fontconfig"
        "libintl"
        "fribidi"
        "giflib"
        "brotli"
        "libhwy"
        "libde265"
        "x265"
        "libaom"
        "libheif"
        "libjxl"
        "libraw"
        "yaml-cpp"
        "pystring"
        "minizip-ng"
        "opencolorio"
        "poppler"
        "libkdcraw"
        "kseexpr"
        "quazip"
      ]
    ) "iOS dependency manifest must contain exactly the pinned package sequence";
    builtins.listToAttrs (
      map (package: {
        name = package.name;
        value = package;
      }) dependencyManifest.packages
    );
  qtModuleNames = map (module: module.name) qtManifest.modules;
  qtModuleByName =
    assert lib.assertMsg (qtManifest.schema == 1) "unsupported iOS Qt manifest schema";
    assert lib.assertMsg (
      qtManifest.qt_version == versions.KRITA_IOS_QT_VERSION
    ) "iOS Qt manifest version does not match versions.env";
    assert lib.assertMsg (
      qtModuleNames == [
        "qtbase"
        "qtsvg"
        "qt5compat"
      ]
    ) "iOS Qt manifest must contain exactly the pinned Qt module sequence";
    builtins.listToAttrs (
      map (module: {
        name = module.name;
        value = module;
      }) qtManifest.modules
    );

  toolchain = import ./toolchain.nix {
    inherit lib versions;
  };

  kfHostTooling = import ./kf-host-tooling.nix {
    inherit pkgs versions;
  };

  host-kconfig-compiler = kfHostTooling.kconfigCompiler;
  kf6-host-tooling = kfHostTooling.kf6HostTooling;
  qttools-host-contract-check = kfHostTooling.qtToolsContractCheck;

  mkIOSCMakePackage = pkgs.callPackage ./mk-ios-cmake-package.nix {
    inherit toolchain;
  };

  mkIOSAutotoolsPackage = pkgs.callPackage ./mk-ios-autotools-package.nix {
    inherit toolchain;
  };

  mkIOSMesonPackage = pkgs.callPackage ./mk-ios-meson-package.nix {
    inherit toolchain;
  };

  mkIOSHeaderPackage = pkgs.callPackage ./mk-ios-header-package.nix { };

  mkCMakePackageVersion = pkgs.callPackage ./cmake-package-version.nix { };

  qt-xcrun-shim = pkgs.callPackage ./qt-xcrun-shim.nix {
    inherit toolchain;
  };

  zlib-ios = pkgs.callPackage ./packages/zlib.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.zlib;
  };

  libdeflate-ios = pkgs.callPackage ./packages/libdeflate.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.libdeflate;
  };

  libffi-ios = pkgs.callPackage ./packages/libffi.nix {
    inherit mkIOSAutotoolsPackage toolchain;
    packageSpec = dependencyByName.libffi;
  };

  pcre2-ios = pkgs.callPackage ./packages/pcre2.nix {
    inherit mkIOSAutotoolsPackage;
    packageSpec = dependencyByName.pcre2;
  };

  expat-ios = pkgs.callPackage ./packages/expat.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.expat;
  };

  libpng-ios = pkgs.callPackage ./packages/libpng.nix {
    inherit mkIOSCMakePackage toolchain zlib-ios;
    packageSpec = dependencyByName.libpng;
  };

  eigen-ios = pkgs.callPackage ./packages/eigen.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.eigen;
  };

  boost-ios = pkgs.callPackage ./packages/boost.nix {
    inherit mkIOSHeaderPackage;
    packageSpec = dependencyByName.boost;
  };

  boost-consumer-check = pkgs.callPackage ./tests/boost-consumer.nix {
    inherit boost-ios mkIOSCMakePackage toolchain;
  };

  immer-ios = pkgs.callPackage ./packages/immer.nix {
    inherit mkCMakePackageVersion mkIOSHeaderPackage;
    packageSpec = dependencyByName.immer;
  };

  immer-consumer-check = pkgs.callPackage ./tests/immer-consumer.nix {
    inherit immer-ios mkIOSCMakePackage toolchain;
  };

  zug-ios = pkgs.callPackage ./packages/zug.nix {
    inherit mkCMakePackageVersion mkIOSHeaderPackage;
    packageSpec = dependencyByName.zug;
  };

  zug-consumer-check = pkgs.callPackage ./tests/zug-consumer.nix {
    inherit mkIOSCMakePackage toolchain zug-ios;
  };

  lager-ios = pkgs.callPackage ./packages/lager.nix {
    inherit
      boost-ios
      mkCMakePackageVersion
      mkIOSHeaderPackage
      zug-ios
      ;
    packageSpec = dependencyByName.lager;
  };

  lager-consumer-check = pkgs.callPackage ./tests/lager-consumer.nix {
    inherit
      boost-ios
      lager-ios
      mkIOSCMakePackage
      toolchain
      zug-ios
      ;
  };

  xsimd-ios = pkgs.callPackage ./packages/xsimd.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.xsimd;
  };

  xsimd-consumer-check = pkgs.callPackage ./tests/xsimd-consumer.nix {
    inherit mkIOSCMakePackage toolchain xsimd-ios;
  };

  libunibreak-ios = pkgs.callPackage ./packages/libunibreak.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.libunibreak;
  };

  libunibreak-consumer-check = pkgs.callPackage ./tests/libunibreak-consumer.nix {
    inherit libunibreak-ios mkIOSCMakePackage toolchain;
  };

  libjpeg-turbo-ios = pkgs.callPackage ./packages/libjpeg-turbo.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.libjpeg-turbo;
  };

  libwebp-ios = pkgs.callPackage ./packages/libwebp.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.libwebp;
  };

  libtiff-ios = pkgs.callPackage ./packages/libtiff.nix {
    inherit
      libdeflate-ios
      libjpeg-turbo-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
    packageSpec = dependencyByName.libtiff;
  };

  openjpeg-ios = pkgs.callPackage ./packages/openjpeg.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.openjpeg;
  };

  imath-ios = pkgs.callPackage ./packages/imath.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.imath;
  };

  openexr-ios = pkgs.callPackage ./packages/openexr.nix {
    inherit imath-ios libdeflate-ios mkIOSCMakePackage;
    packageSpec = dependencyByName.openexr;
  };

  libjpeg-turbo-consumer-check = pkgs.callPackage ./tests/libjpeg-turbo-consumer.nix {
    inherit libjpeg-turbo-ios mkIOSCMakePackage toolchain;
  };

  exiv2-ios = pkgs.callPackage ./packages/exiv2.nix {
    inherit mkIOSCMakePackage toolchain zlib-ios;
    packageSpec = dependencyByName.exiv2;
  };

  exiv2-consumer-check = pkgs.callPackage ./tests/exiv2-consumer.nix {
    inherit
      exiv2-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
  };

  freetype-ios = pkgs.callPackage ./packages/freetype.nix {
    inherit
      libpng-ios
      mkIOSCMakePackage
      zlib-ios
      ;
    packageSpec = dependencyByName.freetype;
  };

  freetype-consumer-check = pkgs.callPackage ./tests/freetype-consumer.nix {
    inherit
      freetype-ios
      libpng-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
  };

  harfbuzz-ios = pkgs.callPackage ./packages/harfbuzz.nix {
    inherit freetype-ios mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.harfbuzz;
  };

  harfbuzz-consumer-check = pkgs.callPackage ./tests/harfbuzz-consumer.nix {
    inherit
      freetype-ios
      harfbuzz-ios
      libpng-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
  };

  fontconfig-ios = pkgs.callPackage ./packages/fontconfig.nix {
    inherit
      expat-ios
      freetype-ios
      libpng-ios
      mkIOSAutotoolsPackage
      toolchain
      zlib-ios
      ;
    packageSpec = dependencyByName.fontconfig;
  };

  fontconfig-consumer-check = pkgs.callPackage ./tests/fontconfig-consumer.nix {
    inherit
      expat-ios
      fontconfig-ios
      freetype-ios
      libpng-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
  };

  libintl-ios = pkgs.callPackage ./packages/libintl.nix {
    inherit mkIOSAutotoolsPackage toolchain;
    packageSpec = dependencyByName.libintl;
  };

  libintl-consumer-check = pkgs.callPackage ./tests/libintl-consumer.nix {
    inherit libintl-ios mkIOSCMakePackage toolchain;
  };

  glib-ios = pkgs.callPackage ./packages/glib.nix {
    inherit
      libffi-ios
      libintl-ios
      mkIOSMesonPackage
      pcre2-ios
      zlib-ios
      ;
    packageSpec = dependencyByName.glib;
  };

  json-c-ios = pkgs.callPackage ./packages/json-c.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName."json-c";
  };

  libmypaint-ios = pkgs.callPackage ./packages/libmypaint.nix {
    inherit glib-ios json-c-ios mkIOSAutotoolsPackage;
    packageSpec = dependencyByName.libmypaint;
  };

  fribidi-ios = pkgs.callPackage ./packages/fribidi.nix {
    inherit mkIOSMesonPackage toolchain;
    packageSpec = dependencyByName.fribidi;
  };

  fribidi-consumer-check = pkgs.callPackage ./tests/fribidi-consumer.nix {
    inherit fribidi-ios mkIOSCMakePackage toolchain;
  };

  giflib-ios = pkgs.callPackage ./packages/giflib.nix {
    inherit mkIOSAutotoolsPackage;
    packageSpec = dependencyByName.giflib;
  };

  brotli-ios = pkgs.callPackage ./packages/brotli.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.brotli;
  };

  libhwy-ios = pkgs.callPackage ./packages/libhwy.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.libhwy;
  };

  libde265-ios = pkgs.callPackage ./packages/libde265.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.libde265;
  };

  x265-ios = pkgs.callPackage ./packages/x265.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.x265;
  };

  libaom-ios = pkgs.callPackage ./packages/libaom.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName.libaom;
  };

  libheif-ios = pkgs.callPackage ./packages/libheif.nix {
    inherit
      libaom-ios
      libde265-ios
      mkIOSCMakePackage
      x265-ios
      ;
    packageSpec = dependencyByName.libheif;
  };

  libjxl-ios = pkgs.callPackage ./packages/libjxl.nix {
    inherit
      brotli-ios
      lcms2-ios
      libhwy-ios
      mkIOSCMakePackage
      ;
    packageSpec = dependencyByName.libjxl;
  };

  libraw-ios = pkgs.callPackage ./packages/libraw.nix {
    inherit lcms2-ios mkIOSAutotoolsPackage;
    packageSpec = dependencyByName.libraw;
  };

  yaml-cpp-ios = pkgs.callPackage ./packages/yaml-cpp.nix {
    inherit mkIOSCMakePackage;
    packageSpec = dependencyByName."yaml-cpp";
  };

  pystring-ios = pkgs.callPackage ./packages/pystring.nix {
    inherit mkIOSMesonPackage;
    packageSpec = dependencyByName.pystring;
  };

  minizip-ng-ios = pkgs.callPackage ./packages/minizip-ng.nix {
    inherit mkIOSCMakePackage zlib-ios;
    packageSpec = dependencyByName."minizip-ng";
  };

  opencolorio-ios = pkgs.callPackage ./packages/opencolorio.nix {
    inherit
      expat-ios
      imath-ios
      minizip-ng-ios
      mkIOSCMakePackage
      pystring-ios
      yaml-cpp-ios
      zlib-ios
      ;
    packageSpec = dependencyByName.opencolorio;
  };

  qtbase-ios = pkgs.callPackage ./packages/qtbase.nix {
    inherit
      freetype-ios
      harfbuzz-ios
      libpng-ios
      mkIOSCMakePackage
      toolchain
      zlib-ios
      ;
    packageSpec = qtModuleByName.qtbase;
    qtXcrunShim = qt-xcrun-shim;
  };

  qtsvg-ios = pkgs.callPackage ./packages/qtsvg.nix {
    inherit
      mkIOSCMakePackage
      qtbase-ios
      toolchain
      zlib-ios
      ;
    packageSpec = qtModuleByName.qtsvg;
    qtXcrunShim = qt-xcrun-shim;
  };

  qt5compat-ios = pkgs.callPackage ./packages/qt5compat.nix {
    inherit mkIOSCMakePackage qtbase-ios toolchain;
    packageSpec = qtModuleByName.qt5compat;
    qtXcrunShim = qt-xcrun-shim;
  };

  quazip-ios = pkgs.callPackage ./packages/quazip.nix {
    inherit
      mkIOSCMakePackage
      qt5compat-ios
      qtbase-ios
      toolchain
      zlib-ios
      ;
    packageSpec = dependencyByName.quazip;
    qtXcrunShim = qt-xcrun-shim;
  };

  poppler-ios = pkgs.callPackage ./packages/poppler.nix {
    inherit
      boost-ios
      fontconfig-ios
      freetype-ios
      kfHostTooling
      lcms2-ios
      libintl-ios
      libjpeg-turbo-ios
      mkIOSCMakePackage
      openjpeg-ios
      qtbase-ios
      zlib-ios
      ;
    packageSpec = dependencyByName.poppler;
    qtXcrunShim = qt-xcrun-shim;
  };

  libkdcraw-ios = pkgs.callPackage ./packages/libkdcraw.nix {
    inherit
      kfHostTooling
      libraw-ios
      mkIOSCMakePackage
      qtbase-ios
      ;
    libkdcraw = pkgs.kdePackages.libkdcraw;
    packageSpec = dependencyByName.libkdcraw;
    qtXcrunShim = qt-xcrun-shim;
  };

  lcms2-ios = pkgs.callPackage ./packages/lcms2.nix {
    inherit mkIOSCMakePackage toolchain;
    packageSpec = dependencyByName.lcms2;
  };

  mkIOSKFPackage = pkgs.callPackage ./mk-ios-kf-package.nix {
    frameworkDefaults = frameworkManifest.target_defaults;
    inherit mkIOSCMakePackage qtbase-ios toolchain;
    kfHostTooling = kfHostTooling;
    qtXcrunShim = qt-xcrun-shim;
  };

  kfPackages = import ./kf-packages.nix {
    inherit
      frameworkManifest
      libintl-ios
      mkIOSKFPackage
      pkgs
      versions
      ;
  };

  inherit (kfPackages)
    kcodecs-ios
    kcolorscheme-ios
    kcompletion-ios
    kconfig-ios
    kcoreaddons-ios
    kguiaddons-ios
    ki18n-ios
    kitemviews-ios
    kwidgetsaddons-ios
    ;

  kseexpr-ios = pkgs.callPackage ./packages/kseexpr.nix {
    inherit
      kfHostTooling
      ki18n-ios
      mkIOSCMakePackage
      qtbase-ios
      ;
    packageSpec = dependencyByName.kseexpr;
    qtXcrunShim = qt-xcrun-shim;
  };

  kf6-consumer-check = pkgs.callPackage ./tests/kf-consumer.nix {
    hostEcm = kfHostTooling.hostEcm;
    hostQt = kfHostTooling.hostQt;
    hostQtTools = kfHostTooling.hostQtTools;
    kf6HostTooling = kf6-host-tooling;
    qtXcrunShim = qt-xcrun-shim;
    inherit
      freetype-ios
      harfbuzz-ios
      kcodecs-ios
      kcolorscheme-ios
      kcompletion-ios
      kconfig-ios
      kcoreaddons-ios
      kguiaddons-ios
      ki18n-ios
      kitemviews-ios
      kwidgetsaddons-ios
      libpng-ios
      mkIOSCMakePackage
      qtbase-ios
      toolchain
      zlib-ios
      ;
  };

  baseIOSPackages = [
    zlib-ios
    libdeflate-ios
    expat-ios
    libpng-ios
    freetype-ios
    harfbuzz-ios
    fontconfig-ios
    lcms2-ios
    eigen-ios
    xsimd-ios
    libunibreak-ios
    libjpeg-turbo-ios
    libwebp-ios
    libtiff-ios
    openjpeg-ios
    imath-ios
    openexr-ios
    libffi-ios
    pcre2-ios
    glib-ios
    json-c-ios
    libmypaint-ios
    exiv2-ios
    boost-ios
    immer-ios
    zug-ios
    lager-ios
    libintl-ios
    fribidi-ios
    giflib-ios
    brotli-ios
    libhwy-ios
    libde265-ios
    x265-ios
    libaom-ios
    libheif-ios
    libjxl-ios
    libraw-ios
    yaml-cpp-ios
    pystring-ios
    minizip-ng-ios
    opencolorio-ios
  ];

  qtIOSPackages = [
    qtbase-ios
    qtsvg-ios
    qt5compat-ios
    quazip-ios
  ];

  kfIOSPackages = [
    kconfig-ios
    kwidgetsaddons-ios
    kcodecs-ios
    kcompletion-ios
    kcoreaddons-ios
    kguiaddons-ios
    ki18n-ios
    kitemviews-ios
    kcolorscheme-ios
  ];

  featureIOSPackages = [
    poppler-ios
    libkdcraw-ios
    kseexpr-ios
  ];

  allIOSPackages = baseIOSPackages ++ qtIOSPackages ++ kfIOSPackages ++ featureIOSPackages;

  krita-ios-app = pkgs.callPackage ./krita.nix {
    inherit
      boost-ios
      eigen-ios
      exiv2-ios
      expat-ios
      fontconfig-ios
      freetype-ios
      fribidi-ios
      harfbuzz-ios
      immer-ios
      kcodecs-ios
      kcolorscheme-ios
      kcompletion-ios
      kconfig-ios
      kcoreaddons-ios
      kguiaddons-ios
      ki18n-ios
      kitemviews-ios
      kritaSource
      pluginProfileFile
      kwidgetsaddons-ios
      lager-ios
      lcms2-ios
      libdeflate-ios
      giflib-ios
      libheif-ios
      libintl-ios
      libjpeg-turbo-ios
      libmypaint-ios
      json-c-ios
      libpng-ios
      libtiff-ios
      libunibreak-ios
      libwebp-ios
      libjxl-ios
      libkdcraw-ios
      mkIOSCMakePackage
      imath-ios
      openexr-ios
      openjpeg-ios
      opencolorio-ios
      poppler-ios
      qt5compat-ios
      qtbase-ios
      qtsvg-ios
      quazip-ios
      toolchain
      xsimd-ios
      zlib-ios
      zug-ios
      kseexpr-ios
      ;
    kfHostTooling = kfHostTooling;
    qtXcrunShim = qt-xcrun-shim;
  };

  # Package-specific phase hooks can retain source paths through their string
  # contexts even when `src` is removed and the phases are never executed.
  # Keep the common preConfigure toolchain setup, but discard every optional
  # hook through which the app recipe could capture its source in the future.
  incrementalDiscardedHooks = [
    "preUnpack"
    "postUnpack"
    "prePatch"
    "postPatch"
    "postConfigure"
    "preBuild"
    "postBuild"
    "preInstall"
    "postInstall"
    "preFixup"
    "postFixup"
    "preInstallCheck"
    "postInstallCheck"
  ];

  # Preserve the app derivation's exact native tools, target closure, CMake
  # flags, and toolchain identity without retaining or building a Krita source
  # snapshot. A recorded `nix develop --profile` of this derivation is the
  # stable environment for the repository-local incremental Ninja tree.
  krita-ios-incremental-env = krita-ios-app.overrideAttrs (
    _old:
    {
      pname = "krita-ios-incremental-env";
      version = "1";
      src = null;
      patches = [ ];
      dontUnpack = true;
      phases = [ "installPhase" ];
      installPhase = ''
        mkdir -p "$out"
      '';
      doInstallCheck = false;
    }
    // lib.genAttrs incrementalDiscardedHooks (_hook: "")
  );

  krita-ios-ipa = pkgs.callPackage ./ipa.nix {
    inherit krita-ios-app;
  };

  ipa-permissions-check = pkgs.callPackage ./tests/ipa-permissions.nix { };

  mkIOSAggregate =
    name: paths:
    pkgs.symlinkJoin {
      inherit name paths;
      passthru.iosAggregateMembers = paths;
      postBuild = ''
        mkdir -p "$out/nix-support"
        rm -f "$out/nix-support/propagated-build-inputs"
        printf '%s ' ${lib.escapeShellArgs (map toString paths)} \
          > "$out/nix-support/propagated-build-inputs"
        printf '\n' >> "$out/nix-support/propagated-build-inputs"
      '';
    };

  ios-base-dependencies = mkIOSAggregate "krita-ios-base-dependencies" baseIOSPackages;
  qt-ios-dependencies = mkIOSAggregate "krita-qt-ios-dependencies" (baseIOSPackages ++ qtIOSPackages);
  ios-dependencies = mkIOSAggregate "krita-ios-dependencies-bootstrap" allIOSPackages;
  kf6-ios-dependencies = ios-dependencies;
in
assert lib.assertMsg (
  krita-ios-incremental-env.src == null
  && krita-ios-incremental-env.patches == [ ]
  && lib.all (hook: (krita-ios-incremental-env.${hook} or null) == "") incrementalDiscardedHooks
) "incremental iOS environment must discard the app source and package-specific phase hooks";
assert lib.assertMsg (
  builtins.length allIOSPackages == 58
  && builtins.length (lib.unique (map toString allIOSPackages)) == 58
) "final iOS dependency aggregate must contain exactly 58 unique outputs";
assert lib.assertMsg (
  map (package: package.pname) allIOSPackages == [
    "zlib-ios"
    "libdeflate-ios"
    "expat-ios"
    "libpng-ios"
    "freetype-ios"
    "harfbuzz-ios"
    "fontconfig-ios"
    "lcms2-ios"
    "eigen-ios"
    "xsimd-ios"
    "libunibreak-ios"
    "libjpeg-turbo-ios"
    "libwebp-ios"
    "libtiff-ios"
    "openjpeg-ios"
    "imath-ios"
    "openexr-ios"
    "libffi-ios"
    "pcre2-ios"
    "glib-ios"
    "json-c-ios"
    "libmypaint-ios"
    "exiv2-ios"
    "boost-ios"
    "immer-ios"
    "zug-ios"
    "lager-ios"
    "libintl-ios"
    "fribidi-ios"
    "giflib-ios"
    "brotli-ios"
    "libhwy-ios"
    "libde265-ios"
    "x265-ios"
    "libaom-ios"
    "libheif-ios"
    "libjxl-ios"
    "libraw-ios"
    "yaml-cpp-ios"
    "pystring-ios"
    "minizip-ng-ios"
    "opencolorio-ios"
    "qtbase-ios"
    "qtsvg-ios"
    "qt5compat-ios"
    "quazip-ios"
    "kconfig-ios"
    "kwidgetsaddons-ios"
    "kcodecs-ios"
    "kcompletion-ios"
    "kcoreaddons-ios"
    "kguiaddons-ios"
    "ki18n-ios"
    "kitemviews-ios"
    "kcolorscheme-ios"
    "poppler-ios"
    "libkdcraw-ios"
    "kseexpr-ios"
  ]
) "final iOS dependency aggregate package order changed";
{
  kseexpr-source = pkgs.kseexpr.src;

  inherit
    boost-consumer-check
    boost-ios
    brotli-ios
    eigen-ios
    exiv2-consumer-check
    exiv2-ios
    expat-ios
    fribidi-consumer-check
    fribidi-ios
    giflib-ios
    fontconfig-consumer-check
    fontconfig-ios
    freetype-consumer-check
    freetype-ios
    harfbuzz-consumer-check
    harfbuzz-ios
    host-kconfig-compiler
    ios-base-dependencies
    ios-dependencies
    immer-consumer-check
    immer-ios
    imath-ios
    ipa-permissions-check
    lager-consumer-check
    lager-ios
    lcms2-ios
    kf6-host-tooling
    kf6-consumer-check
    kf6-ios-dependencies
    kcodecs-ios
    kcolorscheme-ios
    kcompletion-ios
    kconfig-ios
    kcoreaddons-ios
    kguiaddons-ios
    ki18n-ios
    kitemviews-ios
    krita-ios-app
    krita-ios-incremental-env
    krita-ios-ipa
    kseexpr-ios
    libaom-ios
    libde265-ios
    libheif-ios
    libhwy-ios
    openexr-ios
    openjpeg-ios
    opencolorio-ios
    poppler-ios
    pystring-ios
    libintl-consumer-check
    libintl-ios
    libdeflate-ios
    libjpeg-turbo-consumer-check
    libjpeg-turbo-ios
    libjxl-ios
    libkdcraw-ios
    libraw-ios
    libtiff-ios
    libunibreak-consumer-check
    libunibreak-ios
    libwebp-ios
    libpng-ios
    qt5compat-ios
    qt-ios-dependencies
    qtbase-ios
    qtsvg-ios
    qt-xcrun-shim
    qttools-host-contract-check
    quazip-ios
    minizip-ng-ios
    toolchain
    xsimd-consumer-check
    xsimd-ios
    x265-ios
    yaml-cpp-ios
    zlib-ios
    zug-consumer-check
    zug-ios
    kwidgetsaddons-ios
    glib-ios
    json-c-ios
    libffi-ios
    libmypaint-ios
    pcre2-ios
    ;

  librepaint-ios-app = krita-ios-app;
  librepaint-ios-incremental-env = krita-ios-incremental-env;
  librepaint-ios-ipa = krita-ios-ipa;
}
