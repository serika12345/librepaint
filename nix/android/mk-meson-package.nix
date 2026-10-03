{
  androidAbi,
  androidNdkRoot,
  pkgs,
}:

{
  pname,
  version,
  src,
  mesonFlags ? [ ],
  dependencies ? [ ],
  nativeBuildInputs ? [ ],
  patches ? [ ],
  preConfigureCommands ? "",
  postInstall ? "",
  requiredPaths ? [ ],
  meta ? { },
}:

let
  inherit (pkgs) lib;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  cpuFamily = if androidAbi == "arm64-v8a" then "aarch64" else "x86_64";
  toolBin = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  dependencyPkgConfigPath = lib.concatMapStringsSep ":" (
    dependency: "${dependency}/lib/pkgconfig:${dependency}/share/pkgconfig"
  ) dependencies;
  dependencyIncludeArgs = map (dependency: "-I${dependency}/include") dependencies;
  dependencyLibraryArgs = map (dependency: "-L${dependency}/lib") dependencies;
  mesonList = values: lib.concatMapStringsSep ", " (value: "'${value}'") values;
in
pkgs.stdenv.mkDerivation {
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
    pkgs.meson
    pkgs.ninja
    pkgs.pkg-config
    pkgs.python3
  ]
  ++ nativeBuildInputs;

  configurePhase = ''
    runHook preConfigure

    patchShebangs .
    export PKG_CONFIG_LIBDIR=${dependencyPkgConfigPath}
    export PKG_CONFIG_PATH=
    export PKG_CONFIG_SYSROOT_DIR=
    ${preConfigureCommands}
    cat > android-cross.ini <<'EOF'
    [binaries]
    c = '${toolBin}/${androidTriple}28-clang'
    cpp = '${toolBin}/${androidTriple}28-clang++'
    ar = '${toolBin}/llvm-ar'
    strip = '${toolBin}/llvm-strip'
    pkg-config = '${pkgs.pkg-config}/bin/pkg-config'

    [host_machine]
    system = 'android'
    cpu_family = '${cpuFamily}'
    cpu = '${cpuFamily}'
    endian = 'little'

    [properties]
    sys_root = '${androidSysroot}'

    [built-in options]
    c_args = [${mesonList ([ "-fPIC" ] ++ dependencyIncludeArgs)}]
    cpp_args = [${mesonList ([ "-fPIC" ] ++ dependencyIncludeArgs)}]
    c_link_args = [${mesonList ([ "-Wl,-z,max-page-size=16384" ] ++ dependencyLibraryArgs)}]
    cpp_link_args = [${mesonList ([ "-Wl,-z,max-page-size=16384" ] ++ dependencyLibraryArgs)}]
    EOF

    meson setup build \
      --cross-file android-cross.ini \
      --buildtype release \
      --default-library static \
      --prefix "$out" \
      ${lib.escapeShellArgs mesonFlags}

    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild
    meson compile -C build -j "$NIX_BUILD_CORES"
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    meson install -C build
    ${postInstall}
    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    ${lib.concatMapStringsSep "\n" (path: ''
      test -e "$out/${path}"
    '') requiredPaths}
    runHook postInstallCheck
  '';

  propagatedBuildInputs = dependencies;

  passthru = { inherit androidAbi; };

  meta = meta // {
    platforms = [ "x86_64-linux" ];
    sourceProvenance = with lib.sourceTypes; [ fromSource ];
  };
}
