{
  androidAbi,
  androidNdkRoot,
  pkgs,
}:

{
  pname,
  version,
  src,
  cmakeFlags ? [ ],
  cmakeSourceDir ? ".",
  cmakeToolchainFile ? "${androidNdkRoot}/build/cmake/android.toolchain.cmake",
  dependencies ? [ ],
  nativeBuildInputs ? [ ],
  patches ? [ ],
  preConfigureCommands ? "",
  postInstall ? "",
  requiredPaths ? [ ],
  sourceRoot ? null,
  meta ? { },
}:

let
  inherit (pkgs) lib;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  dependencyPrefixPath = lib.concatStringsSep ";" (map toString dependencies);
  dependencyPkgConfigPath = lib.concatMapStringsSep ":" (
    dependency: "${dependency}/lib/pkgconfig:${dependency}/share/pkgconfig"
  ) dependencies;
in
assert lib.assertMsg (builtins.elem androidAbi [
  "arm64-v8a"
  "x86_64"
]) "Android source dependencies support only the product and diagnostic ABIs";
pkgs.stdenv.mkDerivation (
  {
    inherit
      patches
      pname
      src
      version
      ;
    strictDeps = true;
    dontPatchELF = true;
    dontStrip = true;

    nativeBuildInputs = [
      pkgs.cmake
      pkgs.ninja
      pkgs.pkg-config
    ]
    ++ nativeBuildInputs;

    configurePhase = ''
        runHook preConfigure

        export ANDROID_NDK_HOME=${androidNdkRoot}
        export ANDROID_NDK_ROOT=${androidNdkRoot}
        export PKG_CONFIG_LIBDIR=${dependencyPkgConfigPath}
        export PKG_CONFIG_PATH=
        export PKG_CONFIG_SYSROOT_DIR=

      ${preConfigureCommands}
      cmake -S ${cmakeSourceDir} -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=${cmakeToolchainFile} \
          -DANDROID_ABI=${androidAbi} \
          -DANDROID_PLATFORM=android-28 \
          -DANDROID_STL=c++_shared \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
          -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE \
        "-DCMAKE_FIND_ROOT_PATH=${dependencyPrefixPath}" \
          -DCMAKE_INSTALL_PREFIX="$out" \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384 \
          -DCMAKE_MODULE_LINKER_FLAGS=-Wl,-z,max-page-size=16384 \
          ${lib.escapeShellArgs cmakeFlags}

        runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild
      cmake --build build --parallel "$NIX_BUILD_CORES"
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      cmake --install build
      ${postInstall}
      runHook postInstall
    '';

    doInstallCheck = true;
    installCheckPhase = ''
      runHook preInstallCheck
      ${lib.concatMapStringsSep "\n" (path: ''
        test -e "$out/${path}"
      '') requiredPaths}
      while IFS= read -r -d "" library; do
        ${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf \
          -h "$library" | grep -F 'Machine:' >/dev/null
      done < <(find "$out" -type f -name '*.so' -print0)
      runHook postInstallCheck
    '';

    propagatedBuildInputs = dependencies;

    passthru = {
      inherit androidAbi;
      androidTargetTriple = androidTriple;
    };

    meta = meta // {
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [ fromSource ];
    };
  }
  // lib.optionalAttrs (sourceRoot != null) { inherit sourceRoot; }
)
