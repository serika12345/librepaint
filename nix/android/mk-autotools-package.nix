{
  androidAbi,
  androidNdkRoot,
  pkgs,
}:

{
  pname,
  version,
  src,
  configureFlags ? [ ],
  configureCache ? { },
  dependencies ? [ ],
  buildTargets ? [ ],
  installTargets ? [ [ "install" ] ],
  nativeBuildInputs ? [ ],
  patches ? [ ],
  autoreconf ? false,
  preConfigure ? "",
  postInstall ? "",
  requiredPaths ? [ ],
  sourceSubdir ? ".",
  meta ? { },
}:

let
  inherit (pkgs) lib;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  toolBin = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
  dependencyPkgConfigPath = lib.concatMapStringsSep ":" (
    dependency: "${dependency}/lib/pkgconfig:${dependency}/share/pkgconfig"
  ) dependencies;
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
    pkgs.autoconf
    pkgs.automake
    pkgs.libtool
    pkgs.pkg-config
  ]
  ++ nativeBuildInputs;

  configurePhase = ''
    runHook preConfigure

    export AR=${toolBin}/llvm-ar
    export CC=${toolBin}/${androidTriple}28-clang
    export CXX=${toolBin}/${androidTriple}28-clang++
    export LD=${toolBin}/ld.lld
    export NM=${toolBin}/llvm-nm
    export RANLIB=${toolBin}/llvm-ranlib
    export STRIP=${toolBin}/llvm-strip
    export CFLAGS="''${CFLAGS-} -fPIC"
    export CXXFLAGS="''${CXXFLAGS-} -fPIC"
    export LDFLAGS="''${LDFLAGS-} -Wl,-z,max-page-size=16384"
    export PKG_CONFIG_LIBDIR=${dependencyPkgConfigPath}
    export PKG_CONFIG_PATH=
    export PKG_CONFIG_SYSROOT_DIR=
    ${lib.concatStringsSep "\n" (
      lib.mapAttrsToList (name: value: "export ${name}=${lib.escapeShellArg value}") configureCache
    )}

    ${preConfigure}
    ${lib.optionalString autoreconf "autoreconf -fiv"}
    cd ${lib.escapeShellArg sourceSubdir}
    ./configure \
      --build=x86_64-unknown-linux-gnu \
      --host=${androidTriple} \
      --prefix="$out" \
      --disable-shared \
      --enable-static \
      ${lib.escapeShellArgs configureFlags}

    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild
    ${lib.optionalString (buildTargets == [ ]) ''make -j"$NIX_BUILD_CORES"''}
    ${lib.concatMapStringsSep "\n" (target: ''
      make -j"$NIX_BUILD_CORES" ${lib.escapeShellArgs target}
    '') buildTargets}
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    ${lib.concatMapStringsSep "\n" (target: "make ${lib.escapeShellArgs target}") installTargets}
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
