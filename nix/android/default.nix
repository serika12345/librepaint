{
  pkgs,
  source,
  androidAbi ? "arm64-v8a",
  artifactLockFile ? ./upstream-artifacts.json,
  dependencyRecipeRevision ? "7830a5fdfd698ac6012dceca6a8ef7bf4916e67e",
  dependencyRecipeHash ? "sha256-ABYJ44hFWPwq0Wx6ZcmrioRA+o1pnlp67++vx8TxkZk=",
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

  gradleUnwrapped = androidHost.gradle-packages.mkGradle {
    version = "8.13";
    hash = "sha256-IPGxF2I3JUpvwgTYQ0GW+hGkz7OHVnUZxhVW6HEK7Xg=";
    defaultJava = androidHost.jdk17_headless;
  };
  gradle = gradleUnwrapped.wrapped;
  androidGradleCache = gradle.fetchDeps {
    pkg = {
      pname = "librepaint-android-gradle";
    };
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

  artifactLock = lib.importJSON artifactLockFile;
  artifactNames = map (artifact: artifact.name) artifactLock.packages;
  artifacts = map (
    artifact:
    androidHost.fetchurl {
      name = "${artifact.name}-${artifact.version}.tar";
      url = "https://invent.kde.org/api/v4/projects/${toString artifactLock.project}/packages/generic/${artifact.name}/${artifact.version}/archive.tar";
      inherit (artifact) sha256;
    }
  ) artifactLock.packages;

  dependencyPrefix =
    assert lib.assertMsg (
      artifactLock.platform == "Android/${androidAbi}/Qt5/Shared"
    ) "The Android artifact lock must select the requested upstream Qt 5 shared profile";
    assert lib.assertMsg (
      artifactLock.dependencyRecipeRevision == dependencyRecipeRevision
    ) "The Android artifact lock and dependency recipe revision must match";
    assert lib.assertMsg (
      builtins.length artifacts == 62
    ) "The upstream Android profile must contain exactly 62 packages";
    assert lib.assertMsg (
      builtins.length artifactNames == builtins.length (lib.unique artifactNames)
    ) "The upstream Android artifact lock contains duplicate package names";
    androidHost.runCommand "librepaint-android-${androidAbi}-qt5-dependencies"
      {
        nativeBuildInputs = [
          androidHost.binutils
          androidHost.gnutar
          androidHost.patchelf
        ];
        dontPatchELF = true;
        dontStrip = true;
        passthru = {
          inherit artifactLock artifacts;
        };
      }
      ''
        mkdir -p "$out"
        for archive in ${lib.escapeShellArgs (map toString artifacts)}; do
          tar --extract --file="$archive" --directory="$out" --no-same-owner
        done

        while IFS= read -r -d "" configFile; do
          sed -i -E \
            "s#/builds/packaging/krita-deps-management/ext_[^/[:space:]\"';]*/_install#$out#g" \
            "$configFile"
        done < <(grep -IRIlZ '/builds/packaging/krita-deps-management/ext_.*/_install' "$out")

        # The registry contains Android libraries plus the Linux tools needed
        # by qmake/androiddeployqt. Patch only dynamically linked x86_64 host
        # executables; touching arm64 Android libraries would corrupt the APK.
        while IFS= read -r -d "" candidate; do
          if readelf -h "$candidate" 2>/dev/null \
              | grep -q 'Machine:.*Advanced Micro Devices X86-64'; then
            if patchelf --print-interpreter "$candidate" >/dev/null 2>&1; then
              patchelf --set-interpreter ${pkgs.stdenv.cc.bintools.dynamicLinker} "$candidate"
              patchelf --set-rpath ${lib.escapeShellArg "$ORIGIN/../lib:${lib.makeLibraryPath [ pkgs.stdenv.cc.cc ]}"} "$candidate"
            elif patchelf --print-rpath "$candidate" >/dev/null 2>&1; then
              patchelf --set-rpath ${lib.escapeShellArg "$ORIGIN:${lib.makeLibraryPath [ pkgs.stdenv.cc.cc ]}"} "$candidate"
            fi
          fi
        done < <(find "$out/bin" "$out/libexec" "$out/lib" -type f -print0 2>/dev/null)
        for hostToolDir in "$out/bin" "$out/libexec"; do
          if [ -d "$hostToolDir" ]; then
            patchShebangs "$hostToolDir"
          fi
        done

        substituteInPlace "$out/mkspecs/qdevice.pri" \
          --replace-fail '/opt/android-tooling/sdk' '${androidSdkRoot}' \
          --replace-fail '/opt/android-tooling/android-ndk-r27d' '${androidNdkRoot}'
        printf '[Paths]\nPrefix=..\n' > "$out/bin/qt.conf"
        test "$("$out/bin/qmake" -query QT_INSTALL_PREFIX)" = "$out"
        grep -Fq '${androidSdkRoot}' "$out/mkspecs/qdevice.pri"
        grep -Fq '${androidNdkRoot}' "$out/mkspecs/qdevice.pri"
        if grep -IRIl '/builds/packaging/krita-deps-management/ext_.*/_install' "$out"; then
          echo "An upstream CI install prefix remains in the dependency configuration" >&2
          exit 1
        fi
        test -x "$out/bin/androiddeployqt"
        test -f "$out/lib/libQt5Core_${androidAbi}.so"
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
    "Qt5Quick"
    "Qt5QuickControls2"
    "Qt5QuickWidgets"
    "TIFF"
  ];

  disabledFeatures = [
    "HEIF"
    "KF5KDcraw"
    "OpenColorIO"
    "Poppler"
    "PyQt5"
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
      androidHost.jdk17_headless
      androidHost.ninja
      androidHost.pkg-config
      androidHost.python3
    ];

    cmakeGenerator = "Ninja";
    cmakeBuildDir = "build";
    cmakeBuildType = "Release";
    cmakeFlags = [
      "-DBUILD_KRITA_QT_DESIGNER_PLUGINS:BOOL=OFF"
      "-DBUILD_TESTING:BOOL=OFF"
      "-DBUILD_WITH_QT6:BOOL=OFF"
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
      "-DCMAKE_PREFIX_PATH:PATH=${dependencyPrefix}"
      "-DCMAKE_RANLIB:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"
      "-DCMAKE_STRIP:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${depsManagement}/tools/android-toolchain-krita.cmake"
      "-DECM_ADDITIONAL_FIND_ROOT_PATH:PATH=${dependencyPrefix}"
      "-DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${androidHost.gettext}/bin/msgfmt"
      "-DHIDE_SAFE_ASSERTS:BOOL=OFF"
    ]
    ++ map (name: "-DCMAKE_REQUIRE_FIND_PACKAGE_${name}:BOOL=TRUE") requiredFeatures
    ++ map (name: "-DCMAKE_DISABLE_FIND_PACKAGE_${name}:BOOL=TRUE") disabledFeatures;

    preConfigure = ''
      export KRITA_INSTALL_PREFIX="$out"
      cp -a ${dependencyPrefix} "$KRITA_INSTALL_PREFIX"
      chmod -R u+w "$KRITA_INSTALL_PREFIX"

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
      export PATH="$KRITA_INSTALL_PREFIX/bin:$PATH"
      mkdir -p "$ANDROID_USER_HOME" "$GRADLE_USER_HOME" "$HOME"

      appendToVar cmakeFlags "-DCMAKE_INSTALL_PREFIX:PATH=$KRITA_INSTALL_PREFIX"
      appendToVar cmakeFlags "-DCMAKE_FIND_ROOT_PATH:PATH=$KRITA_INSTALL_PREFIX"
      appendToVar cmakeFlags "-DCMAKE_PREFIX_PATH:PATH=$KRITA_INSTALL_PREFIX"
      appendToVar cmakeFlags "-DECM_ADDITIONAL_FIND_ROOT_PATH:PATH=$KRITA_INSTALL_PREFIX"
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
      test -s "$out/lib/kritawebpimport_${androidAbi}.so"
      test -s "$out/lib/kritajpegimport_${androidAbi}.so"
    '';

    meta = {
      description = "LibrePaint Android native prefix built with the pinned upstream Qt 5 dependency snapshot";
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
    flag:
    if flag == "-DBUILD_TESTING:BOOL=OFF" then "-DBUILD_TESTING:BOOL=ON" else flag
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
    install -m 0644 ${nativeBuild}/src/android/templates/res/values/libs.xml \
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
      androidHost.jdk17_headless
      androidHost.ninja
      androidHost.python3
      gradle
    ];

    mitmCache = gradle.fetchDeps {
      pkg = finalAttrs.finalPackage;
      data = ./gradle-deps.json;
      bwrapFlags = ''--ro-bind "$PWD" "$PWD" --dir /bin --symlink ${androidHost.bash}/bin/bash /bin/sh'';
    };

    # The Android Gradle plugin resolves release lint tooling lazily, so the
    # dependency updater must exercise the real release packaging target.
    gradleUpdateScript = ''
      runHook preBuild
      cmake --build . --target create-apk-krita
    '';

    cmakeGenerator = "Ninja";
    cmakeBuildDir = "build";
    cmakeBuildType = "Release";
    cmakeFlags = [
      "-DANDROIDDEPLOYQT_EXTRA_ARGS:STRING=--release"
      "-DCMAKE_AR:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
      "-DCMAKE_FIND_ROOT_PATH:PATH=${nativeBuild}"
      "-DCMAKE_INSTALL_PREFIX:PATH=${nativeBuild}"
      "-DCMAKE_PREFIX_PATH:PATH=${nativeBuild}"
      "-DCMAKE_RANLIB:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"
      "-DCMAKE_STRIP:FILEPATH=${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${depsManagement}/tools/android-toolchain-krita.cmake"
      "-DECM_ADDITIONAL_FIND_ROOT_PATH:PATH=${nativeBuild}"
      "-DNATIVE_PREFIX:PATH=${nativeBuild}"
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
      export PATH="${nativeBuild}/bin:$PATH"
      mkdir -p "$ANDROID_USER_HOME" "$GRADLE_USER_HOME" "$HOME"

      androidExtraLibs=(
        "${nativeBuild}/lib/libQt5QuickWidgets_${androidAbi}.so"
        "${nativeBuild}/lib/libharfbuzz.so"
      )
      test -d "${nativeBuild}/lib/mlt-7"
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
        find "${nativeBuild}/lib/mlt-7" -maxdepth 1 -type f \
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

      # Upstream packages the application from a mutable install prefix.
      # Recreate that boundary for Gradle without copying host tools and
      # development files that are irrelevant to the APK.
      gradleInstallPrefix="$NIX_BUILD_TOP/gradle-install-prefix"
      mkdir -p "$gradleInstallPrefix"
      for runtimeTree in lib share etc translations jar; do
        cp -a --reflink=auto \
          "${nativeBuild}/$runtimeTree" "$gradleInstallPrefix/"
      done
      chmod -R u+w "$gradleInstallPrefix"

      # The shared prefix also carries a few x86_64 libraries for Linux host
      # tools. Gradle copies every top-level .so, so keep those out of the
      # arm64-v8a JNI directory while retaining every Android library.
      hostRuntimeLibDir="$NIX_BUILD_TOP/host-runtime-libs"
      mkdir -p "$hostRuntimeLibDir"
      hostRuntimeLibCount=0
      while IFS= read -r -d "" runtimeLibrary; do
        if ${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf \
          -h "$runtimeLibrary" 2>/dev/null \
          | grep -q 'Machine:.*Advanced Micro Devices X86-64'; then
          mv "$runtimeLibrary" "$hostRuntimeLibDir/"
          hostRuntimeLibCount=$((hostRuntimeLibCount + 1))
        fi
      done < <(
        find "$gradleInstallPrefix/lib" -maxdepth 1 -type f \
          -print0 | sort -z
      )
      test "$hostRuntimeLibCount" -gt 0
      while IFS= read -r -d "" runtimeSymlink; do
        if test ! -e "$runtimeSymlink"; then
          mv "$runtimeSymlink" "$hostRuntimeLibDir/"
        fi
      done < <(
        find "$gradleInstallPrefix/lib" -maxdepth 1 -type l \
          -print0 | sort -z
      )
      export KRITA_INSTALL_PREFIX="$gradleInstallPrefix"

      mapfile -d "" kritaModulePlugins < <(
        find "${nativeBuild}/lib" -maxdepth 1 -type f \
          -name 'krita*_${androidAbi}.so' -print0 | sort -z
      )
      test "''${#kritaModulePlugins[@]}" -gt 0
      (IFS=';'; printf '%s' "''${kritaModulePlugins[*]}") > module-plugins
      export GRADLE_OPTS="''${GRADLE_OPTS-} -Dhttp.proxyHost=$MITM_CACHE_HOST -Dhttp.proxyPort=$MITM_CACHE_PORT -Dhttps.proxyHost=$MITM_CACHE_HOST -Dhttps.proxyPort=$MITM_CACHE_PORT -Djavax.net.ssl.trustStore=$MITM_CACHE_KEYSTORE -Djavax.net.ssl.trustStorePassword=$MITM_CACHE_KS_PWD"
    '';

    buildPhase = ''
      runHook preBuild
      cmake --build . --target create-apk-krita
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      releaseApk="$(find "$NIX_BUILD_TOP" -type f \
        -path '*/krita_build_apk/build/outputs/apk/release/*-release-unsigned.apk' \
        -print -quit)"
      test -n "$releaseApk"
      test -s "$releaseApk"
      install -m 0644 "$releaseApk" "$out/LibrePaint-${androidAbi}.apk"
      runHook postInstall
    '';

    passthru = {
      gradleDepsUpdate = finalAttrs.mitmCache.updateScript;
      inherit
        androidSdk
        dependencyPrefix
        depsManagement
        gradle
        nativeBuild
        ;
    };

    meta = {
      description = "LibrePaint Android ${androidAbi} APK built with the pinned upstream Qt 5 dependency snapshot";
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
      export PATH=${androidSdkRoot}/platform-tools:${dependencyPrefix}/bin:$PATH
      echo "LibrePaint Android Qt 5 environment (${androidAbi})"
      echo "  build: build-incremental ${
        if androidAbi == "arm64-v8a" then "android" else "android-${androidAbi}"
      } build"
    '';
  };
in
{
  androidDependencies = dependencyPrefix;
  inherit devShell incrementalEnv librepaint;
}
