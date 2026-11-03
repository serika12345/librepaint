{
  androidAbi,
  androidNdkRoot,
  androidSdkRoot,
  pkgs,
  qtbase,
}:

let
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  sysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  platformLibDir = "${sysroot}/usr/lib/${androidTriple}/28";
  pluginName = "libplugins_platforms_qtforandroid_${androidAbi}.so";
in
assert qtbase.version == "6.11.1";
pkgs.stdenv.mkDerivation {
  pname = "qt-android-native-surface-${androidAbi}";
  inherit (qtbase) version src;
  patches = [ ./patches/qt-native-surface.patch ];
  strictDeps = true;
  dontPatchELF = true;
  dontStrip = true;
  nativeBuildInputs = [
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    pkgs.binutils
    pkgs.jdk17_headless
  ];
  configurePhase = ''
    runHook preConfigure
    mkdir native-surface
    cp ${./qt-native-surface/CMakeLists.txt} native-surface/CMakeLists.txt
    export ANDROID_HOME=${androidSdkRoot}
    export ANDROID_SDK_ROOT=${androidSdkRoot}
    export JAVA_HOME=${pkgs.jdk17_headless.home}
    cmake -S native-surface -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake \
      -DQT_HOST_PATH=${qtbase.hostQt} \
      -DANDROID_ABI=${androidAbi} -DANDROID_PLATFORM=android-28 \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_FIND_ROOT_PATH=${qtbase} \
      -DCMAKE_PREFIX_PATH=${qtbase} \
      -DCMAKE_MODULE_LINKER_FLAGS=-Wl,-z,max-page-size=16384 \
      -DEGL_INCLUDE_DIR=${sysroot}/usr/include \
      -DEGL_LIBRARY=${platformLibDir}/libEGL.so \
      -DGLESv2_INCLUDE_DIR=${sysroot}/usr/include \
      -DGLESv2_LIBRARY=${platformLibDir}/libGLESv2.so
    runHook postConfigure
  '';
  buildPhase = ''
    runHook preBuild
    cmake --build build --target QAndroidIntegrationPlugin --parallel "$NIX_BUILD_CORES"
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    install -Dm755 build/plugins/platforms/${pluginName} "$out/plugins/platforms/${pluginName}"
    mkdir -p "$out/share/licenses/qt-android-native-surface"
    cp -r LICENSES "$out/share/licenses/qt-android-native-surface/"
    runHook postInstall
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    readelf -d "$out/plugins/platforms/${pluginName}" | grep -F 'libQt6Gui_${androidAbi}.so'
    readelf -Ws "$out/plugins/platforms/${pluginName}" | grep -F 'qt_plugin_query_metadata_v2'
    if readelf -d "$out/plugins/platforms/${pluginName}" | grep -E '(RPATH|RUNPATH)'; then
      echo "Android platform plugin has a host runtime path" >&2
      exit 1
    fi
    runHook postInstallCheck
  '';
  meta = {
    description = "Qt-owned Android native surfaces for the LibrePaint GPU canvas";
    license = with pkgs.lib.licenses; [
      gpl2Only
      gpl3Only
    ];
    sourceProvenance = [ pkgs.lib.sourceTypes.fromSource ];
    platforms = [ "x86_64-linux" ];
  };
}
