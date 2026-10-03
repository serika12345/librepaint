{
  androidAbi,
  androidNdkRoot,
  androidSdkRoot,
  pkgs,
}:

let
  inherit (pkgs) lib;
  hostQt = pkgs.qt6.qtbase;
  qtbase = pkgs.qt6.qtbase;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidPlatformLibDir = "${androidSysroot}/usr/lib/${androidTriple}/28";
in
assert lib.assertMsg (
  pkgs.stdenv.hostPlatform.system == "x86_64-linux"
) "Qt for Android must be built on the pinned x86_64 Linux host";
assert lib.assertMsg (builtins.elem androidAbi [
  "arm64-v8a"
  "x86_64"
]) "The Android Qt build supports only the product and diagnostic ABIs";
pkgs.stdenv.mkDerivation {
  pname = "qtbase-android-${androidAbi}";
  inherit (qtbase) version src;

  strictDeps = true;
  dontPatchELF = true;
  dontStrip = true;

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.jdk17_headless
    pkgs.ninja
    pkgs.perl
    pkgs.pkg-config
    pkgs.python3
  ];

  configurePhase = ''
    runHook preConfigure

    export ANDROID_HOME=${androidSdkRoot}
    export ANDROID_NDK_HOME=${androidNdkRoot}
    export ANDROID_NDK_ROOT=${androidNdkRoot}
    export ANDROID_SDK_ROOT=${androidSdkRoot}
    export JAVA_HOME=${pkgs.jdk17_headless.home}

    mkdir build
    cd build
    ../configure \
      -prefix "$out" \
      -qt-host-path ${hostQt} \
      -android-abis ${androidAbi} \
      -android-sdk ${androidSdkRoot} \
      -android-ndk ${androidNdkRoot} \
      -android-javac-source 8 \
      -android-javac-target 8 \
      -opensource \
      -confirm-license \
      -release \
      -shared \
      -opengl es2 \
      -nomake examples \
      -nomake tests \
      -- \
      -DANDROID_PLATFORM=android-28 \
      -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384 \
      -DEGL_INCLUDE_DIR=${androidSysroot}/usr/include \
      -DEGL_LIBRARY=${androidPlatformLibDir}/libEGL.so \
      -DGLESv2_INCLUDE_DIR=${androidSysroot}/usr/include \
      -DGLESv2_LIBRARY=${androidPlatformLibDir}/libGLESv2.so \
      -DQT_BUILD_EXAMPLES=OFF \
      -DQT_BUILD_TESTS=OFF

    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild
    cmake --build . --parallel "$NIX_BUILD_CORES"
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    cmake --install .
    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck

    test -x "$out/bin/qt-cmake"
    test -x "${hostQt}/bin/androiddeployqt"
    test -f "$out/lib/cmake/Qt6/qt.toolchain.cmake"
    test -f "$out/src/android/templates/AndroidManifest.xml"
    test -f "$out/src/android/templates/res/values/libs.xml"
    test -f "$out/src/android/java/src/org/qtproject/qt/android/bindings/QtActivity.java"
    test -f "$out/lib/libQt6Core_${androidAbi}.so"
    test -f "$out/lib/libQt6Gui_${androidAbi}.so"
    test -f "$out/lib/libQt6Widgets_${androidAbi}.so"
    test -f "$out/lib/libQt6Test_${androidAbi}.so"
    test -f "$out/jar/Qt6Android.jar"

    grep -Fq 'QT_VERSION = ${qtbase.version}' "$out/mkspecs/qconfig.pri"
    grep -Fxq 'DEFAULT_ANDROID_ABIS = ${androidAbi}' "$out/mkspecs/qdevice.pri"
    grep -Fq 'org.qtproject.qt.android' \
      "$out/src/android/java/src/org/qtproject/qt/android/bindings/QtActivity.java"
    if grep -R -a -l -E 'org\.qtproject\.qt5|libQt5' "$out"; then
      echo "Qtbase Android output contains a Qt 5 reference" >&2
      exit 1
    fi

    runHook postInstallCheck
  '';

  passthru = {
    inherit
      androidAbi
      androidNdkRoot
      androidSdkRoot
      hostQt
      ;
    androidQtModule = "qtbase";
  };

  meta = {
    description = "Qt ${qtbase.version} base modules built from source for Android ${androidAbi}";
    inherit (qtbase.meta) homepage license;
    platforms = [ "x86_64-linux" ];
    sourceProvenance = with lib.sourceTypes; [
      fromSource
      binaryNativeCode
    ];
  };
}
