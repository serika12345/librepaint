{
  lib,
  stdenv,
  stdenvNoCC,
  runCommand,
  cargo,
  rustPlatform,
  patchelf,
  python3,
  rustToolchain,
  androidAbi,
  androidNdkRoot,
  wgpu-native,
}:
let
  version = "27.0.4.0";
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
  triple = if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  machine = if androidAbi == "arm64-v8a" then "AArch64" else "Advanced Micro Devices X86-64";
  tools = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
  sysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  targetVariable = lib.replaceStrings [ "-" ] [ "_" ] triple;
  cargoVariable = lib.toUpper targetVariable;
in
assert lib.assertMsg (builtins.elem androidAbi [
  "arm64-v8a"
  "x86_64"
]) "Unsupported Android GPU ABI";
assert lib.assertMsg (
  wgpu-native.version == version
) "Native GPU C API versions must agree across platforms";
stdenvNoCC.mkDerivation {
  pname = "wgpu-native-android-${androidAbi}";
  inherit version;
  inherit (native) src patches;
  nativeBuildInputs = [
    cargo
    stdenv.cc
    rustToolchain
    rustPlatform.bindgenHook
    patchelf
    python3
  ];
  dontConfigure = true;
  dontStrip = true;
  dontPatchELF = true;
  strictDeps = true;
  WGPU_NATIVE_VERSION = version;
  RUSTC_BOOTSTRAP = "1";
  RUSTC = "${rustToolchain}/bin/rustc";
  "CC_${targetVariable}" = "${tools}/${triple}28-clang";
  "AR_${targetVariable}" = "${tools}/llvm-ar";
  "CARGO_TARGET_${cargoVariable}_LINKER" = "${tools}/${triple}28-clang";
  CARGO_PROFILE_RELEASE_LTO = "fat";
  CARGO_PROFILE_RELEASE_CODEGEN_UNITS = "1";
  disallowedReferences = [
    rustToolchain
    cargo
    wgpu-native.cargoDeps
    standardLibraryVendor
    vendor
  ];
  preBuild = ''
    substituteInPlace Cargo.toml --replace-fail 'crate-type = ["cdylib", "staticlib"]' 'crate-type = ["cdylib"]'
    export CARGO_TARGET_${cargoVariable}_RUSTFLAGS="--remap-path-prefix=$NIX_BUILD_TOP=/build --remap-path-prefix=${rustToolchain}=/build/rust --remap-path-prefix=${wgpu-native.cargoDeps}=/build/wgpu-vendor --remap-path-prefix=${standardLibraryVendor}=/build/std-vendor --remap-path-prefix=${vendor}=/build/vendor -C link-arg=-Wl,-z,max-page-size=16384"
    export BINDGEN_EXTRA_CLANG_ARGS="--target=${triple}28 --sysroot=${sysroot}"
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
    cargo build --release --offline --locked --target ${triple} \
      --no-default-features --features vulkan,wgsl -Zbuild-std=std,panic_abort --jobs "$NIX_BUILD_CORES"
    runHook postBuild
  '';
  installPhase = ''
    mkdir -p "$out/lib" "$out/include/webgpu" "$out/share/licenses/wgpu-native"
    cp target/${triple}/release/libwgpu_native.so "$out/lib/"
    cp ffi/wgpu.h ffi/webgpu-headers/webgpu.h "$out/include/webgpu/"
    cp ${wgpu-native.src}/LICENSE.MIT ${wgpu-native.src}/LICENSE.APACHE "$out/share/licenses/wgpu-native/"
    patchelf --page-size 16384 --set-soname libwgpu_native.so "$out/lib/libwgpu_native.so"
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    python3 - "$out/lib/libwgpu_native.so" <<'PY'
    import subprocess, sys
    library = sys.argv[1]
    def readelf(flag):
        return subprocess.check_output(["${tools}/llvm-readelf", flag, library], text=True)
    machine = next(line.split(":", 1)[1].strip() for line in readelf("-h").splitlines()
                   if line.strip().startswith("Machine:"))
    if machine != "${machine}":
        raise SystemExit("The GPU runtime has the wrong Android ABI")
    dynamic = readelf("-d")
    if "Library soname: [libwgpu_native.so]" not in dynamic:
        raise SystemExit("The Android GPU runtime must use a relocatable library name")
    allowed = {"libandroid.so", "libdl.so", "libm.so", "libc.so"}
    for line in dynamic.splitlines():
        if "(NEEDED)" in line and line.split("[")[1].split("]")[0] not in allowed:
            raise SystemExit("The GPU runtime has an undeclared platform dependency: " + line)
    for line in readelf("-l").splitlines():
        if line.strip().startswith("LOAD ") and int(line.split()[-1], 16) < 16384:
            raise SystemExit("The GPU runtime requires 16 KiB page alignment")
    PY
    printf '#include <webgpu/wgpu.h>\nint main(void) { return (wgpuGetVersion() >> 24) != 27; }\n' > consumer.c
    ${tools}/${triple}28-clang -I"$out/include" consumer.c "$out/lib/libwgpu_native.so" \
      -Wl,--no-allow-shlib-undefined -Wl,-z,max-page-size=16384 -o consumer
    ${tools}/llvm-readelf -d consumer | grep -F 'Shared library: [libwgpu_native.so]'
  '';
  meta = {
    description = "Native WebGPU runtime built from pinned source for Android ${androidAbi}";
    inherit (wgpu-native.meta) homepage license;
    platforms = [ "x86_64-linux" ];
  };
}
