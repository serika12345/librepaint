{
  androidAbi,
  androidNdkRoot,
  androidSdkRoot,
  dependencyRecipes,
  foundations,
  kf6,
  pkgs,
  qtbase,
}:

let
  inherit (pkgs) lib;
  mkCMakePackage = import ./mk-cmake-package.nix {
    inherit androidAbi androidNdkRoot pkgs;
  };
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  ffmpegArch = if androidAbi == "arm64-v8a" then "aarch64" else "x86_64";
  toolBin = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidPlatformLibDir = "${androidSysroot}/usr/lib/${androidTriple}/28";

  sdlSource = pkgs.fetchurl {
    url = "https://github.com/sh-zam/SDL-android-glue/archive/8babf290ece99a58a5354d52775bf79063cf7cf5.tar.gz";
    hash = "sha256-LwOafmKYfFuCvHvkmxxszBOO5w2fLZMrQMy6EnoIAWg=";
  };
  sdl2 = mkCMakePackage {
    pname = "sdl2-android-${androidAbi}";
    version = "2.32.10-krita-8babf29";
    src = sdlSource;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DSDL_ANDROID_AUDIO_STRIPPED=ON"
      "-DSDL_HAPTIC=OFF"
      "-DSDL_JOYSTICK=OFF"
      "-DSDL_VIDEO=OFF"
      "-DSDL_SENSOR=OFF"
      "-DSDL_MISC=OFF"
      "-DSDL_POWER=OFF"
      "-DSDL_LOCALE=OFF"
      "-DSDL_FILESYSTEM=OFF"
      "-DINSTALL_JARS_PATH=${builtins.placeholder "out"}/jar"
      "-DANDROID_SDK=${androidSdkRoot}"
      "-DANDROID_JAVA_PLATFORM_API=android-35"
    ];
    nativeBuildInputs = [ pkgs.jdk17_headless ];
    requiredPaths = [
      "include/SDL2/SDL.h"
      "lib/libSDL2.so"
      "jar/SDL2Android.jar"
    ];
    meta = {
      description = "Krita SDL2 Android audio glue built from pinned source";
      license = lib.licenses.zlib;
    };
  };

  unwindstackSource = pkgs.fetchurl {
    url = "https://github.com/getsentry/libunwindstack-ndk/archive/284202fb1e42dbeba6598e26ced2e1ec404eecd1.tar.gz";
    hash = "sha256-CifZGsRS5M+raWfMkjpT1TvUmtAXthagltkaI4K8us4=";
  };
  unwindstack = mkCMakePackage {
    pname = "unwindstack-android-${androidAbi}";
    version = "2022-01-13-284202f";
    src = unwindstackSource;
    cmakeSourceDir = "cmake";
    patches = [
      "${dependencyRecipes}/ext_libunwindstack-ndk/0001-Add-install-property-and-cmake-config-file.patch"
    ];
    requiredPaths = [
      "include/unwindstack/Unwinder.h"
      "lib/libunwindstack.a"
      "lib/cmake/unwindstack/unwindstack-config.cmake"
    ];
    meta = {
      description = "libunwindstack NDK port built from pinned source";
      license = lib.licenses.asl20;
    };
  };

  ffmpeg = pkgs.stdenv.mkDerivation {
    pname = "ffmpeg-android-${androidAbi}";
    inherit (pkgs.ffmpeg) version src;
    strictDeps = true;
    dontPatchELF = true;
    dontStrip = true;
    nativeBuildInputs = [
      pkgs.makeWrapper
      pkgs.pkg-config
      pkgs.python3
      pkgs.yasm
    ]
    ++ lib.optionals (androidAbi == "x86_64") [ pkgs.nasm ];
    configurePhase = ''
      runHook preConfigure
      export AR=${toolBin}/llvm-ar
      export CC=${toolBin}/${androidTriple}28-clang
      export CXX=${toolBin}/${androidTriple}28-clang++
      export NM=${toolBin}/llvm-nm
      export RANLIB=${toolBin}/llvm-ranlib
      export STRIP=${toolBin}/llvm-strip
      export PKG_CONFIG_LIBDIR=${foundations.zlib}/lib/pkgconfig:${foundations.libwebp}/lib/pkgconfig
      export PKG_CONFIG_PATH=
      export PKG_CONFIG_SYSROOT_DIR=
      ./configure \
        --prefix="$out" \
        --target-os=android \
        --arch=${ffmpegArch} \
        --enable-cross-compile \
        --sysroot=${androidSysroot} \
        --cc="$CC" \
        --cxx="$CXX" \
        --ar="$AR" \
        --nm="$NM" \
        --ranlib="$RANLIB" \
        --strip="$STRIP" \
        --pkg-config=${pkgs.pkg-config}/bin/pkg-config \
        --pkg-config-flags=--static \
        --enable-shared \
        --disable-static \
        --enable-pic \
        --disable-autodetect \
        --disable-doc \
        --disable-htmlpages \
        --disable-manpages \
        --disable-podpages \
        --disable-txtpages \
        --disable-programs \
        --disable-avdevice \
        --enable-avcodec \
        --enable-avfilter \
        --enable-avformat \
        --enable-avutil \
        --enable-swresample \
        --enable-swscale \
        --enable-network \
        --enable-zlib \
        --enable-libwebp \
        --extra-ldflags=-Wl,-z,max-page-size=16384
      runHook postConfigure
    '';
    buildPhase = ''
      runHook preBuild
      make -j"$NIX_BUILD_CORES"
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      make install
      runHook postInstall
    '';
    doInstallCheck = true;
    installCheckPhase = ''
      for module in avcodec avfilter avformat avutil swresample swscale; do
        test -e "$out/lib/lib$module.so"
        test -f "$out/lib/pkgconfig/lib$module.pc"
      done
    '';
    propagatedBuildInputs = [
      foundations.libwebp
      foundations.zlib
    ];
    passthru = { inherit androidAbi; };
    meta = {
      description = "FFmpeg built from pinned source for Android ${androidAbi}";
      inherit (pkgs.ffmpeg.meta) license;
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [ fromSource ];
    };
  };

  mltSource = pkgs.fetchurl {
    url = "https://github.com/mltframework/mlt/releases/download/v7.38.0/mlt-7.38.0.tar.gz";
    sha256 = "b8f0a23c89e9250edc5038d745537c382367bf2ad3dad5d5c7cd13b0fe1c4144";
  };
  mlt = mkCMakePackage {
    pname = "mlt-android-${androidAbi}";
    version = "7.38.0";
    src = mltSource;
    patches = [
      "${dependencyRecipes}/ext_mlt/0002-MLT-check-pointers-before-dereferencing.patch"
      "${dependencyRecipes}/ext_mlt/0004-Android-Add-an-option-to-skip-non-plugins-in-reposit.patch"
      "${dependencyRecipes}/ext_mlt/0011-android-Fix-export-table-for-FILE-arguments-with-NDK.patch"
      "${dependencyRecipes}/ext_mlt/0013-Remove-dependency-on-libavdevice.patch"
    ];
    dependencies = [
      ffmpeg
      sdl2
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DMOD_AVFORMAT=ON"
      "-DMOD_DECKLINK=OFF"
      "-DMOD_FREI0R=OFF"
      "-DMOD_GDK=OFF"
      "-DMOD_JACKRACK=OFF"
      "-DMOD_KDENLIVE=OFF"
      "-DMOD_MOVIT=OFF"
      "-DMOD_NORMALIZE=OFF"
      "-DMOD_OLDFILM=OFF"
      "-DMOD_OPENCV=OFF"
      "-DMOD_OPENFX=OFF"
      "-DMOD_PLUS=ON"
      "-DMOD_PLUSGPL=OFF"
      "-DMOD_QT=OFF"
      "-DMOD_QT6=OFF"
      "-DMOD_RESAMPLE=OFF"
      "-DMOD_RTAUDIO=OFF"
      "-DMOD_RUBBERBAND=OFF"
      "-DMOD_SDL2=ON"
      "-DMOD_SOX=OFF"
      "-DMOD_SPATIALAUDIO=OFF"
      "-DMOD_VIDSTAB=OFF"
      "-DMOD_VORBIS=OFF"
      "-DMOD_XINE=OFF"
      "-DMOD_XML=OFF"
      "-DUSE_LV2=OFF"
      "-DUSE_VST2=OFF"
    ];
    requiredPaths = [
      "include/mlt-7/framework/mlt.h"
      "lib/libmlt-7.so"
      "lib/mlt-7/libmltavformat.so"
    ];
    meta = {
      description = "MLT multimedia runtime built from pinned source for Android ${androidAbi}";
      license = lib.licenses.gpl3Plus;
    };
  };

  kseexpr = mkCMakePackage {
    pname = "kseexpr-android-${androidAbi}";
    inherit (pkgs.kseexpr) version src;
    patches = pkgs.kseexpr.patches or [ ];
    cmakeToolchainFile = "${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake";
    dependencies = [
      qtbase
      kf6.ki18n
    ];
    nativeBuildInputs = [
      pkgs.bison
      pkgs.flex
      pkgs.gettext
    ];
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=ON"
      "-DENABLE_LLVM_BACKEND=OFF"
      "-DENABLE_QT6=ON"
      "-DBUILD_TESTS=OFF"
      "-DBUILD_DEMOS=OFF"
      "-DBUILD_DOC=OFF"
      "-DBUILD_UTILS=OFF"
      "-DBUILD_TRANSLATIONS=OFF"
      "-DUSE_PREGENERATED_FILES=OFF"
      "-DECM_DIR=${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake"
      "-DEGL_INCLUDE_DIR=${androidSysroot}/usr/include"
      "-DEGL_LIBRARY=${androidPlatformLibDir}/libEGL.so"
      "-DGETTEXT_MSGFMT_EXECUTABLE=${pkgs.gettext}/bin/msgfmt"
      "-DGLESv2_INCLUDE_DIR=${androidSysroot}/usr/include"
      "-DGLESv2_LIBRARY=${androidPlatformLibDir}/libGLESv2.so"
      "-DQT_HOST_PATH=${qtbase.hostQt}"
    ];
    requiredPaths = [
      "lib/cmake/KSeExpr/kseexpr-config.cmake"
      "lib/libKSeExpr.so"
      "lib/libKSeExprUI.so"
    ];
    meta = {
      description = "KSeExpr built from pinned source with Qt 6 for Android ${androidAbi}";
      inherit (pkgs.kseexpr.meta) license;
    };
  };
in
{
  inherit
    ffmpeg
    kseexpr
    mlt
    sdl2
    unwindstack
    ;
}
