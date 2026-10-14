{
  lib,
  stdenvNoCC,
  fetchurl,
  unzip,
  patchelf,
  python3,
  androidAbi,
  androidNdkRoot,
  wgpu-native,
}:
let
  version = "27.0.4.0";
  architecture = if androidAbi == "arm64-v8a" then "aarch64" else "x86_64";
  triple = if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  machine = if androidAbi == "arm64-v8a" then "AArch64" else "Advanced Micro Devices X86-64";
  hash =
    if androidAbi == "arm64-v8a" then
      "80a93ecfcb14d07f6cadb8bc9bdcf78974f3668b27a89f52a1b006cd14e985a0"
    else
      "01dfab96efcb980f04da1814c3d0300c06bd3cadb3ee65cc7f1be17ff904523a";
  tools = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
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
  src = fetchurl {
    url = "https://github.com/gfx-rs/wgpu-native/releases/download/v${version}/wgpu-android-${architecture}-release.zip";
    sha256 = hash;
  };
  nativeBuildInputs = [
    unzip
    patchelf
    python3
  ];
  dontUnpack = true;
  dontConfigure = true;
  dontBuild = true;
  dontStrip = true;
  dontPatchELF = true;
  strictDeps = true;
  installPhase = ''
    unzip -q "$src"
    mkdir -p "$out/lib" "$out/include" "$out/share/licenses/wgpu-native" "$out/share/wgpu-native"
    cp lib/libwgpu_native.so "$out/lib/"
    cp -R include/webgpu "$out/include/"
    cp -R wgpu-native-meta "$out/share/wgpu-native/"
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
    description = "Pinned native WebGPU runtime for Android ${androidAbi}";
    inherit (wgpu-native.meta) homepage license;
    platforms = [ "x86_64-linux" ];
    sourceProvenance = [ lib.sourceTypes.binaryNativeCode ];
  };
}
