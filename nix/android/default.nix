{
  pkgs,
  source,
  androidAbi ? "arm64-v8a",
  dependencyRecipeRevision ? "0fd95c4efca2e5b5024121bb0ccae5b353956cd9",
  dependencyRecipeHash ? "sha256-HdV3KRydtw94mtVn1ZXGmKPH5Qro33UZEj3KtPc1lj8=",
  packageName ? "librepaint-android",
}:

let
  inherit (pkgs) lib;

  androidHost = import pkgs.path {
    localSystem = pkgs.stdenv.hostPlatform;
    config = {
      allowUnfree = true;
      android_sdk.accept_license = true;
    };
  };

  sdkComposition = androidHost.androidenv.composeAndroidPackages {
    buildToolsVersions = [ "35.0.0" ];
    includeCmake = false;
    includeEmulator = false;
    includeNDK = true;
    includeSystemImages = false;
    ndkVersion = "27.3.13750724";
    platformVersions = [ "35" ];
  };
  androidSdk = sdkComposition.androidsdk;
  androidSdkRoot = "${androidSdk}/libexec/android-sdk";
  androidNdkRoot = "${androidSdkRoot}/ndk-bundle";
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidPlatformLibDir = "${androidSysroot}/usr/lib/${androidTriple}/28";

  qtbase = import ./qtbase.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      ;
    pkgs = androidHost;
  };

  qtsvg = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [ ];
    moduleName = "qtsvg";
    pkgs = androidHost;
    requiredLibraries = [
      "Svg"
      "SvgWidgets"
    ];
    sourcePackage = androidHost.qt6.qtsvg;
  };

  qtshadertools = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [ ];
    moduleName = "qtshadertools";
    pkgs = androidHost;
    requiredLibraries = [ "ShaderTools" ];
    sourcePackage = androidHost.qt6.qtshadertools;
  };

  qtlanguageserver = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [ ];
    moduleName = "qtlanguageserver";
    pkgs = androidHost;
    requiredLibraries = [ ];
    requiredPaths = [ "lib/libQt6LanguageServer_${androidAbi}.a" ];
    sourcePackage = androidHost.qt6.qtlanguageserver;
  };

  qtdeclarative = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [
      qtlanguageserver
      qtshadertools
      qtsvg
    ];
    moduleName = "qtdeclarative";
    pkgs = androidHost;
    requiredLibraries = [
      "Qml"
      "Quick"
      "QuickControls2"
      "QuickWidgets"
    ];
    sourcePackage = androidHost.qt6.qtdeclarative;
  };

  qt5compat = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [
      qtdeclarative
      qtshadertools
    ];
    moduleName = "qt5compat";
    pkgs = androidHost;
    requiredLibraries = [ "Core5Compat" ];
    sourcePackage = androidHost.qt6.qt5compat;
  };

  qtimageformats = import ./qt-module.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      qtbase
      ;
    dependencies = [ ];
    moduleName = "qtimageformats";
    pkgs = androidHost;
    requiredLibraries = [ ];
    requiredPaths = [ "plugins/imageformats/libplugins_imageformats_qwebp_${androidAbi}.so" ];
    sourcePackage = androidHost.qt6.qtimageformats;
  };

  sourceDependencies = import ./dependencies.nix {
    inherit
      androidAbi
      androidNdkRoot
      qt5compat
      qtbase
      ;
    manifestFile = ../../packaging/ios/deps/dependencies.json;
    pkgs = androidHost;
  };
  sourceDependencyPrefix = androidHost.buildEnv {
    name = "librepaint-android-${androidAbi}-source-dependencies";
    paths = with sourceDependencies; [
      boost
      brotli
      eigen
      exiv2
      expat
      fftw
      fontconfig
      freetype
      fribidi
      giflib
      glib
      gsl
      harfbuzz
      imath
      immer
      json-c
      lager
      lcms2
      libdeflate
      libffi
      libhwy
      libintl
      libjpeg-turbo
      libjxl
      libmypaint
      libpng
      libtiff
      libunibreak
      libwebp
      openexr
      openjpeg
      pcre2
      xsimd
      zlib
      zug
    ];
    pathsToLink = [
      "/include"
      "/lib"
      "/share"
    ];
    ignoreCollisions = false;
  };
  kf6 = import ./kf6.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      sourceDependencies
      qtbase
      ;
    frameworkManifestFile = ../../packaging/ios/frameworks/frameworks.json;
    pkgs = androidHost;
  };
  kf6Prefix = androidHost.buildEnv {
    name = "librepaint-android-${androidAbi}-kf6";
    paths = builtins.attrValues kf6;
    pathsToLink = [
      "/include"
      "/lib"
      "/metatypes"
      "/share"
    ];
    ignoreCollisions = false;
  };
  applicationDependencies = import ./application-dependencies.nix {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      kf6
      qtbase
      ;
    dependencyRecipes = depsManagement;
    foundations = sourceDependencies;
    pkgs = androidHost;
  };
  applicationDependencyPrefix = androidHost.buildEnv {
    name = "librepaint-android-${androidAbi}-application-dependencies";
    paths = builtins.attrValues applicationDependencies;
    pathsToLink = [
      "/include"
      "/jar"
      "/lib"
      "/share"
    ];
    ignoreCollisions = false;
  };
  dependencyPrefix = androidHost.buildEnv {
    name = "librepaint-android-${androidAbi}-qt6-dependencies";
    paths = [
      qtbase
      qtsvg
      qtshadertools
      qtlanguageserver
      qtdeclarative
      qt5compat
      qtimageformats
      sourceDependencyPrefix
      sourceDependencies.quazip
      kf6Prefix
      applicationDependencyPrefix
    ];
    pathsToLink = [
      "/etc"
      "/include"
      "/jar"
      "/lib"
      "/libexec"
      "/metatypes"
      "/mkspecs"
      "/modules"
      "/plugins"
      "/qml"
      "/share"
      "/src"
      "/translations"
    ];
    ignoreCollisions = true;
  };

  gradleUnwrapped = androidHost.gradle-packages.mkGradle {
    version = "8.13";
    hash = "sha256-IPGxF2I3JUpvwgTYQ0GW+hGkz7OHVnUZxhVW6HEK7Xg=";
    defaultJava = androidHost.jdk17_headless;
  };
  gradle = gradleUnwrapped.wrapped;
  gradleLockProject = androidHost.stdenvNoCC.mkDerivation {
    pname = "librepaint-android-gradle-lock";
    version = "1";
    src = ./gradle-lock;

    nativeBuildInputs = [
      androidHost.jdk17_headless
      gradle
    ];

    dontConfigure = true;

    preBuild = ''
      export ANDROID_HOME=${androidSdkRoot}
      export ANDROID_SDK_ROOT=${androidSdkRoot}
      export ANDROID_USER_HOME="$NIX_BUILD_TOP/android-home"
      export GRADLE_USER_HOME="$NIX_BUILD_TOP/gradle-home"
      export HOME="$NIX_BUILD_TOP/home"
      export JAVA_HOME=${androidHost.jdk17_headless.home}
      mkdir -p "$ANDROID_USER_HOME" "$GRADLE_USER_HOME" "$HOME"

      if [[ -n "''${MITM_CACHE_HOST-}" ]]; then
        gradleTrustStore="$NIX_BUILD_TOP/gradle-mitm-keystore"
        gradleTrustStorePassword=librepaint
        ${androidHost.jdk17_headless}/bin/keytool -importcert -noprompt \
          -file "$MITM_CACHE_CA" -alias librepaint \
          -keystore "$gradleTrustStore" \
          -storepass "$gradleTrustStorePassword" >/dev/null
        export GRADLE_OPTS="''${GRADLE_OPTS-} -Dhttp.proxyHost=$MITM_CACHE_HOST -Dhttp.proxyPort=$MITM_CACHE_PORT -Dhttps.proxyHost=$MITM_CACHE_HOST -Dhttps.proxyPort=$MITM_CACHE_PORT -Djavax.net.ssl.trustStore=$gradleTrustStore -Djavax.net.ssl.trustStorePassword=$gradleTrustStorePassword"
      fi
    '';

    gradleUpdateScript = ''
      runHook preBuild
      gradle --no-daemon \
        --init-script ${pkgs.path}/pkgs/development/tools/build-managers/gradle/init-build.gradle \
        -Pandroid.aapt2FromMavenOverride=${androidSdkRoot}/build-tools/35.0.0/aapt2 \
        assembleRelease
    '';

    buildPhase = ''
      runHook preBuild
      gradle --no-daemon \
        --init-script ${pkgs.path}/pkgs/development/tools/build-managers/gradle/init-build.gradle \
        -Pandroid.aapt2FromMavenOverride=${androidSdkRoot}/build-tools/35.0.0/aapt2 \
        assembleRelease
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      lockApk="$(find build/outputs/apk/release -type f \
        -name '*-release-unsigned.apk' -print -quit)"
      test -s "$lockApk"
      install -Dm644 "$lockApk" "$out/gradle-lock.apk"
      runHook postInstall
    '';
  };
  androidGradleCache = gradle.fetchDeps {
    pkg = gradleLockProject;
    data = ./gradle-deps.json;
  };
  gradleWrapper = androidHost.writeShellScript "librepaint-android-gradle" ''
    gradleProjectDir="$(dirname "$0")"
    chmod -R u+w "$gradleProjectDir"
    cd "$gradleProjectDir"
    exec ${gradle}/bin/gradle \
      --no-daemon \
      --init-script ${pkgs.path}/pkgs/development/tools/build-managers/gradle/init-build.gradle \
      -Pandroid.aapt2FromMavenOverride=${androidSdkRoot}/build-tools/35.0.0/aapt2 \
      "$@"
  '';
  androidTestGradleWrapper = androidHost.writeShellScript "librepaint-android-test-gradle" ''
    set -euo pipefail

    cacheState="$(mktemp -d)"
    cachePid=""
    cleanup() {
      if [[ -n "$cachePid" ]]; then
        kill "$cachePid" 2>/dev/null || true
        wait "$cachePid" 2>/dev/null || true
      fi
      rm -rf -- "$cacheState"
    }
    trap cleanup EXIT

    ${androidHost.openssl}/bin/openssl genrsa -out "$cacheState/ca.key" 2048 >/dev/null 2>&1
    ${androidHost.openssl}/bin/openssl req -x509 -new -nodes \
      -key "$cacheState/ca.key" -sha256 -days 1 \
      -out "$cacheState/ca.cer" \
      -subj "/C=JP/ST=local/L=local/O=LibrePaint/OU=Android/CN=localhost"
    cacheHost=127.0.0.1
    cachePort="$(${androidHost.python3Packages.ephemeral-port-reserve}/bin/ephemeral-port-reserve "$cacheHost")"
    ${androidHost.mitm-cache}/bin/mitm-cache \
      -k "$cacheState/ca.key" -c "$cacheState/ca.cer" \
      -l"$cacheHost:$cachePort" replay ${androidGradleCache} \
      >"$cacheState/mitm-cache.log" 2>&1 &
    cachePid=$!
    cacheReady=0
    for _attempt in $(seq 1 50); do
      if ${androidHost.curl}/bin/curl --silent --output /dev/null \
          "http://$cacheHost:$cachePort"; then
        cacheReady=1
        break
      fi
      if ! kill -0 "$cachePid" 2>/dev/null; then
        cat "$cacheState/mitm-cache.log" >&2
        exit 1
      fi
      sleep 0.1
    done
    if [[ "$cacheReady" != 1 ]]; then
      cat "$cacheState/mitm-cache.log" >&2
      exit 1
    fi

    keyStore="$cacheState/keystore"
    keyStorePassword=librepaint
    ${androidHost.jdk17_headless}/bin/keytool -importcert -noprompt \
      -file "$cacheState/ca.cer" -alias librepaint \
      -keystore "$keyStore" -storepass "$keyStorePassword" >/dev/null

    execStatus=0
    ${gradle}/bin/gradle \
      --no-daemon \
      --init-script ${pkgs.path}/pkgs/development/tools/build-managers/gradle/init-build.gradle \
      -Pandroid.aapt2FromMavenOverride=${androidSdkRoot}/build-tools/35.0.0/aapt2 \
      -Dhttp.proxyHost="$cacheHost" -Dhttp.proxyPort="$cachePort" \
      -Dhttps.proxyHost="$cacheHost" -Dhttps.proxyPort="$cachePort" \
      -Djavax.net.ssl.trustStore="$keyStore" \
      -Djavax.net.ssl.trustStorePassword="$keyStorePassword" \
      "$@" || execStatus=$?
    exit "$execStatus"
  '';

  depsManagement = androidHost.fetchFromGitLab {
    domain = "invent.kde.org";
    owner = "packaging";
    repo = "krita-deps-management";
    rev = dependencyRecipeRevision;
    hash = dependencyRecipeHash;
  };

  requiredFeatures = [
    "FFTW3"
    "GIF"
    "GSL"
    "JPEG"
    "JPEGXL"
    "KSeExpr"
    "LibMyPaint"
    "Mlt7"
    "OpenEXR"
    "OpenJPEG"
    "QUAZIP"
    "Qt6Quick"
    "Qt6QuickControls2"
    "Qt6QuickWidgets"
    "TIFF"
  ];

  disabledFeatures = [
    "HEIF"
    "KDcrawQt6"
    "OpenColorIO"
    "Poppler"
    "PyQt6"
    "PythonLibrary"
    "SIP"
  ];

  nativeBuild = pkgs.stdenv.mkDerivation {
    pname = "${packageName}-native";
    version = "1.0.2";
    src = source;

    strictDeps = true;
    dontPatchELF = true;
    dontStrip = true;

    nativeBuildInputs = [
      androidHost.cmake
      androidHost.gettext
      androidHost.gawk
      androidHost.jdk17_headless
      androidHost.ninja
      androidHost.pkg-config
      androidHost.python3
      androidHost.unzip
    ];

    cmakeGenerator = "Ninja";
    cmakeBuildDir = "build";
    cmakeBuildType = "Release";
    cmakeFlags = [
      "-DBUILD_KRITA_QT_DESIGNER_PLUGINS:BOOL=OFF"
      "-DBUILD_TESTING:BOOL=OFF"
      "-DBUILD_WITH_QT6:BOOL=ON"
      "-DANDROIDDEPLOYQT_EXTRA_ARGS:STRING=--release"
      "-DCMAKE_BUILD_TYPE:STRING=Release"
      "-DCMAKE_INSTALL_BINDIR:PATH=bin"
      "-DCMAKE_INSTALL_DOCDIR:PATH=share/doc/krita"
      "-DCMAKE_INSTALL_INCLUDEDIR:PATH=include"
      "-DCMAKE_INSTALL_INFODIR:PATH=share/info"
      "-DCMAKE_INSTALL_LIBDIR:PATH=lib"
      "-DCMAKE_INSTALL_LIBEXECDIR:PATH=libexec"
      "-DCMAKE_INSTALL_LOCALEDIR:PATH=share/locale"
      "-DCMAKE_INSTALL_MANDIR:PATH=share/man"
      "-DCMAKE_INSTALL_SBINDIR:PATH=sbin"
      "-DCMAKE_AR:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
      "-DCMAKE_FIND_ROOT_PATH:PATH=${dependencyPrefix}"
      "-DCMAKE_MODULE_LINKER_FLAGS:STRING=-Wl,-z,max-page-size=16384"
      "-DCMAKE_PREFIX_PATH:PATH=${dependencyPrefix}"
      "-DCMAKE_RANLIB:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"
      "-DCMAKE_SHARED_LINKER_FLAGS:STRING=-Wl,-z,max-page-size=16384"
      "-DCMAKE_STRIP:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake"
      "-DECM_ADDITIONAL_FIND_ROOT_PATH:PATH=${dependencyPrefix}"
      "-DECM_DIR:PATH=${androidHost.kdePackages.extra-cmake-modules}/share/ECM/cmake"
      "-DEGL_INCLUDE_DIR:PATH=${androidSysroot}/usr/include"
      "-DEGL_LIBRARY:FILEPATH=${androidPlatformLibDir}/libEGL.so"
      "-DEXPAT_INCLUDE_DIR:PATH=${dependencyPrefix}/include"
      "-DEXPAT_LIBRARY:FILEPATH=${dependencyPrefix}/lib/libexpat.a"
      "-DFREETYPE_INCLUDE_DIR_freetype2:PATH=${dependencyPrefix}/include/freetype2"
      "-DFREETYPE_INCLUDE_DIR_ft2build:PATH=${dependencyPrefix}/include/freetype2"
      "-DFREETYPE_LIBRARY:FILEPATH=${dependencyPrefix}/lib/libfreetype.a"
      "-DFriBidi_INCLUDE_DIR:PATH=${dependencyPrefix}/include"
      "-DFriBidi_LIBRARY:FILEPATH=${dependencyPrefix}/lib/libfribidi.a"
      "-DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${androidHost.gettext}/bin/msgfmt"
      "-DGLESv2_INCLUDE_DIR:PATH=${androidSysroot}/usr/include"
      "-DGLESv2_LIBRARY:FILEPATH=${androidPlatformLibDir}/libGLESv2.so"
      "-DHarfBuzz_INCLUDE_DIR:PATH=${dependencyPrefix}/include/harfbuzz"
      "-DHarfBuzz_LIBRARY:FILEPATH=${dependencyPrefix}/lib/libharfbuzz.a"
      "-DHIDE_SAFE_ASSERTS:BOOL=ON"
      "-DKF6_HOST_TOOLING:PATH=${lib.getDev androidHost.kdePackages.kconfig}/lib/cmake"
      "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH:PATH=${dependencyPrefix}"
      "-DQT_HOST_PATH:PATH=${qtbase.hostQt}"
      "-DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON"
    ]
    ++ map (name: "-DCMAKE_REQUIRE_FIND_PACKAGE_${name}:BOOL=TRUE") requiredFeatures
    ++ map (name: "-DCMAKE_DISABLE_FIND_PACKAGE_${name}:BOOL=TRUE") disabledFeatures;

    preConfigure = ''
      export KRITA_INSTALL_PREFIX="$out"

      export ANDROID_ABI=${androidAbi}
      export ANDROID_HOME=${androidSdkRoot}
      export ANDROID_NDK_HOME=${androidNdkRoot}
      export ANDROID_NDK_ROOT=${androidNdkRoot}
      export ANDROID_SDK_ROOT=${androidSdkRoot}
      export ANDROID_USER_HOME="$NIX_BUILD_TOP/android-home"
      export GRADLE_USER_HOME="$NIX_BUILD_TOP/gradle-home"
      export HOME="$NIX_BUILD_TOP/home"
      export JAVA_HOME=${androidHost.jdk17_headless.home}
      export KDECI_ANDROID_ABI=${androidAbi}
      export KDECI_ANDROID_NDK_ROOT=${androidNdkRoot}
      export KDECI_ANDROID_SDK_ROOT=${androidSdkRoot}
      export KRITA_UNSTABLE_PACKAGE_SUFFIX=""
      export QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH='${qtdeclarative.hostQtModule};${qtlanguageserver.hostQtModule};${qtshadertools.hostQtModule};${qtsvg.hostQtModule}'
      export PATH="${qtbase.hostQt}/bin:$PATH"
      mkdir -p "$ANDROID_USER_HOME" "$GRADLE_USER_HOME" "$HOME"

      appendToVar cmakeFlags "-DCMAKE_INSTALL_PREFIX:PATH=$KRITA_INSTALL_PREFIX"
      appendToVar cmakeFlags "-DQT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH:PATH=$QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH"
    '';

    # CMAKE_REQUIRE_FIND_PACKAGE for WebP and libjpeg-turbo also makes their
    # nested config-mode probes required, preventing Krita's find-module
    # fallbacks from checking the requested components. Verify the actual
    # results directly instead, so neither feature can disappear silently.
    postConfigure = ''
      grep -Fq 'kritawebpimport' build.ninja
      grep -Fq 'kritawebpexport' build.ninja
      grep -Fxq '#define HAVE_JPEG_TURBO 1' config-jpeg.h
    '';

    postInstall = ''
      test -s "$out/lib/libkrita_${androidAbi}.so"
      test -s "$out/lib/kritawebpimport.so"
      test -s "$out/lib/kritajpegimport.so"
    '';

    meta = {
      description = "LibrePaint Android native libraries built from source with Qt 6";
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [
        fromSource
        binaryNativeCode
      ];
    };
  };

  incrementalDiscardedHooks = [
    "preUnpack"
    "postUnpack"
    "prePatch"
    "postPatch"
    "preConfigure"
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

  incrementalCmakeFlags = map (
    flag: if flag == "-DBUILD_TESTING:BOOL=OFF" then "-DBUILD_TESTING:BOOL=ON" else flag
  ) nativeBuild.cmakeFlags;
  androidCmakeFlagsFile = androidHost.writeText "librepaint-android-cmake-flags" (
    lib.concatStringsSep "\n" incrementalCmakeFlags + "\n"
  );
  androidConfigIdentity = builtins.hashString "sha256" (
    builtins.toJSON {
      inherit androidAbi androidNdkRoot androidSdkRoot;
      dependencyPrefix = toString dependencyPrefix;
      cmakeFlags = incrementalCmakeFlags;
    }
  );

  # Keep the SDK, dependency prefix, native build tools, and CMake contract in
  # the Nix closure while source files remain in the persistent worktree build.
  incrementalEnv = nativeBuild.overrideAttrs (
    old:
    {
      pname = "${packageName}-incremental-env";
      version = "1";
      src = null;
      patches = [ ];
      nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [ androidHost.ccache ];
      dontUnpack = true;
      phases = [ "installPhase" ];
      installPhase = ''
        mkdir -p "$out"
      '';
      doInstallCheck = false;
    }
    // lib.genAttrs incrementalDiscardedHooks (_hook: "")
  );

  packagingCmake = androidHost.replaceVars ./package.cmake { inherit androidAbi; };

  androidPackageSource = androidHost.runCommand "${packageName}-package-source" { } ''
    mkdir -p "$out"
    cp -a ${source}/packaging/android/apk "$out/apk"
    chmod -R u+w "$out/apk"
    install -m 0644 ${packagingCmake} "$out/CMakeLists.txt"
    install -m 0755 ${gradleWrapper} "$out/apk/gradlew"
    install -m 0644 ${qtbase}/src/android/templates/res/values/libs.xml \
      "$out/apk/res/values/libs.xml"
  '';

  librepaint = pkgs.stdenv.mkDerivation (finalAttrs: {
    pname = packageName;
    version = "1.0.2";
    src = androidPackageSource;

    strictDeps = true;
    dontPatchELF = true;
    dontStrip = true;

    nativeBuildInputs = [
      androidHost.cmake
      androidHost.gawk
      androidHost.jdk17_headless
      androidHost.ninja
      androidHost.python3
      androidHost.unzip
      gradle
    ];

    mitmCache = androidGradleCache;

    cmakeGenerator = "Ninja";
    cmakeBuildDir = "build";
    cmakeBuildType = "Release";
    cmakeFlags = [
      "-DANDROIDDEPLOYQT_EXTRA_ARGS:STRING=--release"
      "-DCMAKE_AR:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
      "-DCMAKE_FIND_ROOT_PATH:PATH=${dependencyPrefix};${nativeBuild}"
      "-DCMAKE_INSTALL_PREFIX:PATH=${nativeBuild}"
      "-DCMAKE_PREFIX_PATH:PATH=${dependencyPrefix};${nativeBuild}"
      "-DCMAKE_RANLIB:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"
      "-DCMAKE_STRIP:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake"
      "-DECM_ADDITIONAL_FIND_ROOT_PATH:PATH=${dependencyPrefix};${nativeBuild}"
      "-DEGL_INCLUDE_DIR:PATH=${androidSysroot}/usr/include"
      "-DEGL_LIBRARY:FILEPATH=${androidPlatformLibDir}/libEGL.so"
      "-DGLESv2_INCLUDE_DIR:PATH=${androidSysroot}/usr/include"
      "-DGLESv2_LIBRARY:FILEPATH=${androidPlatformLibDir}/libGLESv2.so"
      "-DNATIVE_PREFIX:PATH=${nativeBuild}"
      "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH:PATH=${dependencyPrefix}"
      "-DQT_HOST_PATH:PATH=${qtbase.hostQt}"
    ];

    preConfigure = ''
      export ANDROID_ABI=${androidAbi}
      export ANDROID_HOME=${androidSdkRoot}
      export ANDROID_NDK_HOME=${androidNdkRoot}
      export ANDROID_NDK_ROOT=${androidNdkRoot}
      export ANDROID_SDK_ROOT=${androidSdkRoot}
      export ANDROID_USER_HOME="$NIX_BUILD_TOP/android-home"
      export GRADLE_USER_HOME="$NIX_BUILD_TOP/gradle-home"
      export HOME="$NIX_BUILD_TOP/home"
      export JAVA_HOME=${androidHost.jdk17_headless.home}
      export KDECI_ANDROID_ABI=${androidAbi}
      export KDECI_ANDROID_NDK_ROOT=${androidNdkRoot}
      export KDECI_ANDROID_SDK_ROOT=${androidSdkRoot}
      export KRITA_INSTALL_PREFIX=${nativeBuild}
      export KRITA_UNSTABLE_PACKAGE_SUFFIX=""
      export QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH='${qtdeclarative.hostQtModule};${qtlanguageserver.hostQtModule};${qtshadertools.hostQtModule};${qtsvg.hostQtModule}'
      export PATH="${qtbase.hostQt}/bin:$PATH"
      mkdir -p "$ANDROID_USER_HOME" "$GRADLE_USER_HOME" "$HOME"

      androidExtraLibs=(
        "${qtdeclarative}/lib/libQt6QuickWidgets_${androidAbi}.so"
      )
      mltPluginDir="${applicationDependencies.mlt}/lib/mlt-7"
      test -d "$mltPluginDir"
      mltExtraLibDir="$NIX_BUILD_TOP/android-extra-libs"
      mkdir -p "$mltExtraLibDir"
      mltPluginCount=0
      while IFS= read -r -d "" plugin; do
        pluginName="''${plugin##*/libmlt}"
        stagedPlugin="$mltExtraLibDir/lib_mltplugin_$pluginName"
        ln -s "$plugin" "$stagedPlugin"
        androidExtraLibs+=("$stagedPlugin")
        mltPluginCount=$((mltPluginCount + 1))
      done < <(
        find "$mltPluginDir" -maxdepth 1 -type f \
          -name 'libmlt*.so' -print0 | sort -z
      )
      test "$mltPluginCount" -gt 0
      androidExtraLibsFlag="$(IFS=';'; echo "''${androidExtraLibs[*]}")"
      appendToVar cmakeFlags \
        "-DANDROID_EXTRA_LIBS:STRING=$androidExtraLibsFlag"
      appendToVar cmakeFlags "-DANDROID_APK_OUTPUT_DIR:PATH=$out"
    '';

    preBuild = ''
      mkdir -p "$out"
      test -n "''${MITM_CACHE_CA-}"

      # Gradle consumes one mutable runtime tree. Compose it from the immutable
      # source-built dependency and application outputs without making the
      # application derivation own or rebuild those dependencies.
      gradleInstallPrefix="$NIX_BUILD_TOP/gradle-install-prefix"
      mkdir -p "$gradleInstallPrefix"
      for runtimePrefix in ${dependencyPrefix} ${nativeBuild}; do
        for runtimeTree in lib share etc translations jar plugins qml; do
          if test -e "$runtimePrefix/$runtimeTree"; then
            mkdir -p "$gradleInstallPrefix/$runtimeTree"
            cp -aL --reflink=auto \
              "$runtimePrefix/$runtimeTree/." "$gradleInstallPrefix/$runtimeTree/"
            chmod -R u+w "$gradleInstallPrefix/$runtimeTree"
          fi
        done
      done
      export KRITA_INSTALL_PREFIX="$gradleInstallPrefix"
      export GRADLE_OPTS="''${GRADLE_OPTS-} -Dhttp.proxyHost=$MITM_CACHE_HOST -Dhttp.proxyPort=$MITM_CACHE_PORT -Dhttps.proxyHost=$MITM_CACHE_HOST -Dhttps.proxyPort=$MITM_CACHE_PORT -Djavax.net.ssl.trustStore=$MITM_CACHE_KEYSTORE -Djavax.net.ssl.trustStorePassword=$MITM_CACHE_KS_PWD"
    '';

    buildPhase = ''
      runHook preBuild
      if test -d android-build; then
        chmod -R u+w android-build
      fi
      cmake --build . --target create-apk-krita
      chmod -R u+w android-build
      cmake --build . --target create-aab-krita
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      releaseApk="$(find "$NIX_BUILD_TOP" -type f \
        -path '*/android-build/build/outputs/apk/release/*-release-unsigned.apk' \
        -print -quit)"
      test -n "$releaseApk"
      test -s "$releaseApk"
      install -m 0644 "$releaseApk" "$out/LibrePaint-${androidAbi}.apk"
      releaseAab="$(find "$NIX_BUILD_TOP" -type f \
        -path '*/android-build/build/outputs/bundle/release/*.aab' \
        -print -quit)"
      test -n "$releaseAab"
      test -s "$releaseAab"
      install -m 0644 "$releaseAab" "$out/LibrePaint-${androidAbi}.aab"
      ${source}/scripts/platform/audit-android-package \
        "$out/LibrePaint-${androidAbi}.apk" \
        ${androidAbi} \
        ${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf \
        ${androidSdkRoot}/build-tools/35.0.0/aapt2 \
        product
      ${source}/scripts/platform/audit-android-package \
        "$out/LibrePaint-${androidAbi}.aab" \
        ${androidAbi} \
        ${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf \
        ${androidSdkRoot}/build-tools/35.0.0/aapt2 \
        product
      runHook postInstall
    '';

    passthru = {
      gradleDepsUpdate = androidGradleCache.updateScript;
      inherit
        androidSdk
        dependencyPrefix
        depsManagement
        gradle
        nativeBuild
        ;
    };

    meta = {
      description = "LibrePaint Android ${androidAbi} APK and AAB built from source with Qt 6";
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [
        fromSource
        binaryNativeCode
        binaryBytecode
      ];
    };
  });

  devShell = androidHost.mkShell {
    name = "${packageName}-development";
    inputsFrom = [ incrementalEnv ];
    packages = [
      androidHost.ccache
      androidHost.nixfmt
    ];
    shellHook = ''
      export LIBREPAINT_ANDROID_INCREMENTAL_SHELL=1
      export LIBREPAINT_ANDROID_CMAKE_FLAGS_FILE=${androidCmakeFlagsFile}
      export LIBREPAINT_ANDROID_CONFIG_ID=${androidConfigIdentity}
      export LIBREPAINT_ANDROID_DEPENDENCY_PREFIX=${dependencyPrefix}
      export LIBREPAINT_ANDROID_GRADLE_WRAPPER=${androidTestGradleWrapper}
      export LIBREPAINT_ANDROID_PROFILE=${androidAbi}
      export ANDROID_ABI=${androidAbi}
      export ANDROID_HOME=${androidSdkRoot}
      export ANDROID_NDK_HOME=${androidNdkRoot}
      export ANDROID_NDK_ROOT=${androidNdkRoot}
      export ANDROID_SDK_ROOT=${androidSdkRoot}
      export CMAKE_FIND_ROOT_PATH=${dependencyPrefix}
      export CMAKE_PREFIX_PATH=${dependencyPrefix}
      export ECM_ADDITIONAL_FIND_ROOT_PATH=${dependencyPrefix}
      export JAVA_HOME=${androidHost.jdk17_headless.home}
      export KDECI_ANDROID_ABI=${androidAbi}
      export KDECI_ANDROID_NDK_ROOT=${androidNdkRoot}
      export KDECI_ANDROID_SDK_ROOT=${androidSdkRoot}
      export QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH='${qtdeclarative.hostQtModule};${qtlanguageserver.hostQtModule};${qtshadertools.hostQtModule};${qtsvg.hostQtModule}'
      export PATH=${androidSdkRoot}/platform-tools:${qtbase.hostQt}/bin:$PATH
      echo "LibrePaint Android Qt 6 environment (${androidAbi})"
      echo "  build: build-incremental ${
        if androidAbi == "arm64-v8a" then "android" else "android-${androidAbi}"
      } build"
    '';
  };
in
{
  androidDependencies = dependencyPrefix;
  androidSourceDependencies = sourceDependencyPrefix;
  androidKf6 = kf6Prefix;
  androidApplicationDependencies = applicationDependencyPrefix;
  inherit
    devShell
    incrementalEnv
    librepaint
    qt5compat
    qtbase
    qtdeclarative
    qtimageformats
    qtlanguageserver
    qtshadertools
    qtsvg
    ;
  inherit (sourceDependencies)
    boost
    eigen
    exiv2
    expat
    fftw
    fontconfig
    freetype
    fribidi
    giflib
    glib
    gsl
    harfbuzz
    imath
    immer
    json-c
    lager
    lcms2
    libdeflate
    libffi
    libhwy
    libintl
    libjpeg-turbo
    libjxl
    libmypaint
    libpng
    libtiff
    libunibreak
    libwebp
    openexr
    openjpeg
    pcre2
    quazip
    xsimd
    zlib
    zug
    ;
}
