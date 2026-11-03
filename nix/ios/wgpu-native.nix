{
  lib,
  stdenv,
  stdenvNoCC,
  writeShellScript,
  runCommand,
  cargo,
  rustPlatform,
  python3,
  gawk,
  gnugrep,
  wgpu-native,
  rustToolchain,
  qtXcrunShim,
  toolchain,
}:

let
  native = import ../gpu/wgpu-native.nix { inherit wgpu-native; };
  vendor = import ../gpu/wgpu-native-vendor.nix {
    inherit
      runCommand
      rustPlatform
      rustToolchain
      wgpu-native
      ;
  };
  inherit (vendor) standardLibraryVendor;
  hostLinker = writeShellScript "wgpu-native-ios-host-linker" ''
    unset SDKROOT IPHONEOS_DEPLOYMENT_TARGET
    exec ${stdenv.cc}/bin/cc "$@"
  '';
  archiveInspection = import ./inspect-ios-archive.nix { inherit gawk toolchain; };
in
stdenvNoCC.mkDerivation {
  pname = "wgpu-native-ios";
  inherit (native) version src patches;
  strictDeps = true;
  dontStrip = true;
  dontConfigure = true;
  disallowedReferences = [
    rustToolchain
    cargo
    wgpu-native.cargoDeps
    standardLibraryVendor
    vendor
  ];
  nativeBuildInputs = [
    cargo
    stdenv.cc
    rustToolchain
    rustPlatform.bindgenHook
    qtXcrunShim
    python3
    gawk
    gnugrep
  ];
  __impureHostDeps = toolchain.impureHostDeps;
  DEVELOPER_DIR = toolchain.developerDir;
  SDKROOT = toolchain.sdkRoot;
  IPHONEOS_DEPLOYMENT_TARGET = toolchain.deploymentTarget;
  SOURCE_DATE_EPOCH = "1";
  ZERO_AR_DATE = "1";
  WGPU_NATIVE_VERSION = wgpu-native.version;
  RUSTC_BOOTSTRAP = "1";
  RUSTC = "${rustToolchain}/bin/rustc";
  CC_aarch64_apple_ios = toolchain.cc;
  AR_aarch64_apple_ios = toolchain.ar;
  CARGO_TARGET_AARCH64_APPLE_IOS_LINKER = toolchain.cc;
  CARGO_TARGET_AARCH64_APPLE_DARWIN_LINKER = hostLinker;
  CARGO_PROFILE_RELEASE_LTO = "fat";
  CARGO_PROFILE_RELEASE_CODEGEN_UNITS = "1";

  preBuild = ''
    ${python3}/bin/python3 - <<'PY'
    import plistlib, re, subprocess
    with open("${toolchain.xcodeVersionPlist}", "rb") as file:
        xcode = plistlib.load(file)
    with open("${toolchain.sdkVersionPlist}", "rb") as file:
        sdk = plistlib.load(file)
    clang = subprocess.check_output(["${toolchain.cc}", "--version"], text=True).splitlines()[0]
    actual = [xcode["CFBundleShortVersionString"], xcode["ProductBuildVersion"],
              sdk["CFBundleShortVersionString"], sdk["ProductBuildVersion"],
              re.search(r"Apple clang version (\S+)", clang).group(1),
              re.search(r"\(clang-([^\)]+)\)", clang).group(1)]
    expected = ${
      builtins.toJSON [
        toolchain.xcodeVersion
        toolchain.xcodeBuildVersion
        toolchain.sdkVersion
        toolchain.sdkBuildVersion
        toolchain.clangVersion
        toolchain.clangBuildVersion
      ]
    }
    if actual != expected:
        raise SystemExit("The iOS GPU dependency requires the pinned Xcode, SDK and Apple Clang identities")
    PY
    substituteInPlace Cargo.toml --replace-fail 'crate-type = ["cdylib", "staticlib"]' 'crate-type = ["staticlib"]'
    export CARGO_TARGET_AARCH64_APPLE_IOS_RUSTFLAGS="--remap-path-prefix=$NIX_BUILD_TOP=/build --remap-path-prefix=${rustToolchain}=/build/rust --remap-path-prefix=${wgpu-native.cargoDeps}=/build/wgpu-vendor --remap-path-prefix=${standardLibraryVendor}=/build/std-vendor --remap-path-prefix=${vendor}=/build/vendor"
    export CARGO_HOME="$NIX_BUILD_TOP/cargo"
    mkdir -p "$CARGO_HOME"
    cat > "$CARGO_HOME/config.toml" <<EOF_CONFIG
    [source.crates-io]
    replace-with = "vendored-sources"
    [source.vendored-sources]
    directory = "${vendor}"
    EOF_CONFIG
  '';
  buildPhase = ''
    runHook preBuild
    cargo build --release --offline --locked --target aarch64-apple-ios \
      --no-default-features --features metal,wgsl -Zbuild-std=std,panic_abort --jobs "$NIX_BUILD_CORES"
    runHook postBuild
  '';
  installPhase = ''
    mkdir -p "$out/lib" "$out/include/webgpu" "$out/share/licenses/wgpu-native"
    cp target/aarch64-apple-ios/release/libwgpu_native.a "$out/lib/"
    cp ffi/wgpu.h ffi/webgpu-headers/webgpu.h "$out/include/webgpu/"
    cp LICENSE.MIT LICENSE.APACHE "$out/share/licenses/wgpu-native/"
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    ${archiveInspection}
    archive="$out/lib/libwgpu_native.a"
    test "$(${toolchain.lipo} -archs "$archive")" = ${toolchain.architecture}
    members="$(${toolchain.ar} -t "$archive" | grep -v '^__.SYMDEF' | wc -l | tr -d ' ')"
    inspect_ios_archive_members "$archive" "$members"
    printf '#include <webgpu/wgpu.h>\nint main(void) { return (wgpuGetVersion() >> 24) != 27; }\n' > consumer.c
    ${toolchain.cc} -target arm64-apple-ios${toolchain.deploymentTarget} -isysroot ${toolchain.sdkRoot} \
      -I"$out/include" consumer.c "$archive" -framework Metal -framework Foundation -framework QuartzCore -lobjc -o consumer
  '';
  passthru.iosToolchainIdentity = toolchain.identity;
  meta = {
    description = "Metal and WGSL native WebGPU implementation built with the pinned iOS target environment";
    inherit (wgpu-native.meta) homepage license;
    platforms = [ "aarch64-darwin" ];
  };
}
