{
  androidAbi,
  androidNdkRoot,
  androidSdkRoot,
  dependencies ? [ ],
  moduleName,
  pkgs,
  qtbase,
  requiredLibraries,
  requiredPaths ? [ ],
  sourcePackage,
}:

let
  inherit (pkgs) lib;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidPlatformLibDir = "${androidSysroot}/usr/lib/${androidTriple}/28";
  dependencyPrefixPath = lib.concatStringsSep ";" (map toString dependencies);
  hostModule = pkgs.qt6.${moduleName};
  hostDependencyModules = map (dependency: dependency.hostQtModule) dependencies;
  hostPrefixPath = lib.concatStringsSep ";" ([ hostModule ] ++ hostDependencyModules);
in
assert lib.assertMsg (
  sourcePackage.version == qtbase.version
) "Qt Android modules must use the same version as Qtbase";
pkgs.stdenv.mkDerivation {
  pname = "${moduleName}-android-${androidAbi}";
  inherit (sourcePackage) version src;

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
    export LDFLAGS="''${LDFLAGS-} -Wl,-z,max-page-size=16384"
    export QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH='${hostPrefixPath}'
    export QT_ADDITIONAL_PACKAGES_PREFIX_PATH='${dependencyPrefixPath}'

    mkdir build
    cd build
    moduleConfigure="$NIX_BUILD_TOP/qt-configure-module"
    cp ${qtbase}/bin/qt-configure-module "$moduleConfigure"
    chmod u+w "$moduleConfigure"
    substituteInPlace "$moduleConfigure" \
      --replace-fail 'script_dir_path=`dirname $0`' 'script_dir_path=${qtbase}/bin' \
      --replace-fail 'script_dir_path=`(cd "$script_dir_path"; pwd)`' ':' \
      --replace-fail \
        'cp "$script_dir_path/../lib/cmake/Qt6/qt-configure-module-flags.txt" "$opttmpfilepath"' \
        'cp "$script_dir_path/../lib/cmake/Qt6/qt-configure-module-flags.txt" "$opttmpfilepath"; chmod u+w "$opttmpfilepath"'
    "$moduleConfigure" .. -- \
      -DBUILD_TESTING=OFF \
      -DANDROID_PLATFORM=android-28 \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$out" \
      -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-z,max-page-size=16384" \
      -DEGL_INCLUDE_DIR=${androidSysroot}/usr/include \
      -DEGL_LIBRARY=${androidPlatformLibDir}/libEGL.so \
      -DGLESv2_INCLUDE_DIR=${androidSysroot}/usr/include \
      -DGLESv2_LIBRARY=${androidPlatformLibDir}/libGLESv2.so \
      "-DQT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH=${hostPrefixPath}" \
      "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=${dependencyPrefixPath}" \
      -DQT_BUILD_EXAMPLES=OFF \
      -DQT_BUILD_STANDALONE_EXAMPLES=OFF \
      -DQT_BUILD_STANDALONE_TESTS=OFF \
      -DQT_BUILD_TESTS=OFF \
      -DQT_HOST_PATH=${qtbase.hostQt} \
      -DQT_INTERNAL_BUILD_STANDALONE_PARTS=OFF

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

    ${lib.concatMapStringsSep "\n" (library: ''
      test -f "$out/lib/libQt6${library}_${androidAbi}.so"
    '') requiredLibraries}
    ${lib.concatMapStringsSep "\n" (path: ''
      test -e "$out/${path}"
    '') requiredPaths}
    if grep -R -a -l -E 'org\.qtproject\.qt5|libQt5' "$out"; then
      echo "${moduleName} Android output contains a Qt 5 reference" >&2
      exit 1
    fi

    runHook postInstallCheck
  '';

  propagatedBuildInputs = [ qtbase ] ++ dependencies;

  passthru = {
    inherit androidAbi moduleName qtbase;
    hostQtModule = hostModule;
  };

  meta = {
    description = "Qt ${sourcePackage.version} ${moduleName} built from source for Android ${androidAbi}";
    inherit (sourcePackage.meta) homepage license;
    platforms = [ "x86_64-linux" ];
    sourceProvenance = with lib.sourceTypes; [ fromSource ];
  };
}
