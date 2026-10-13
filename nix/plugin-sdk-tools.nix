{ pkgs }:

let
  inherit (pkgs) lib;
  source = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../scripts/plugin-sdk
      ../scripts/tests/test_plugin_sdk.py
      ../sdk/cmake/LibrePaintSDKConfig.cmake
      ../cmake/modules/KritaAddBrokenUnitTest.cmake
      ../cmake/modules/KritaTestSuite.cmake
      ../COPYING
      ../COPYING-CMAKE-SCRIPTS
    ];
  };
  commands = [
    pkgs.cmake
    pkgs.gettext
    pkgs.ninja
  ]
  ++ lib.optional pkgs.stdenv.hostPlatform.isDarwin pkgs.darwin.cctools
  ++ lib.optional pkgs.stdenv.hostPlatform.isLinux pkgs.patchelf;
in
pkgs.stdenvNoCC.mkDerivation {
  pname = "librepaint-plugin-sdk-tools";
  version = "1";
  src = source;
  strictDeps = true;
  nativeBuildInputs = [
    pkgs.makeWrapper
    pkgs.python3
  ];
  dontConfigure = true;
  dontBuild = true;
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    python3 -m unittest discover -s scripts/tests -p test_plugin_sdk.py
    runHook postCheck
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin" "$out/libexec" "$out/share/librepaint-sdk/sdk/cmake" \
      "$out/share/librepaint-sdk/cmake/modules" "$out/share/licenses/librepaint-plugin-sdk-tools"
    cp scripts/plugin-sdk "$out/libexec/plugin-sdk"
    cp sdk/cmake/LibrePaintSDKConfig.cmake "$out/share/librepaint-sdk/sdk/cmake/"
    cp cmake/modules/KritaAddBrokenUnitTest.cmake cmake/modules/KritaTestSuite.cmake \
      "$out/share/librepaint-sdk/cmake/modules/"
    cp COPYING COPYING-CMAKE-SCRIPTS "$out/share/licenses/librepaint-plugin-sdk-tools/"
    makeWrapper ${pkgs.python3}/bin/python3 "$out/bin/plugin-sdk" \
      --add-flags "$out/libexec/plugin-sdk" \
      --set LIBREPAINT_SDK_DATA "$out/share/librepaint-sdk" \
      --prefix PATH : ${lib.makeBinPath commands}
    runHook postInstall
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/plugin-sdk" --help
    runHook postInstallCheck
  '';
  meta = {
    description = "Package LibrePaint SDKs and rebuild external plugins";
    license = lib.licenses.gpl2Plus;
    platforms = lib.platforms.darwin ++ lib.platforms.linux;
    mainProgram = "plugin-sdk";
  };
}
