{
  lib,
  runCommand,
  fetchurl,
  unzip,
}:

let
  version = "27.0.4.0";
  archive = fetchurl {
    url = "https://github.com/gfx-rs/wgpu-native/releases/download/v${version}/wgpu-windows-x86_64-gnu-release.zip";
    hash = "sha256-wMLbzvPGqZM6Ghv3y9quvtYaM8gzussCaWYvkVNr6L0=";
  };
in
runCommand "wgpu-native-windows-x86_64-gnu-${version}"
  {
    inherit version;
    outputs = [
      "out"
      "dev"
    ];
    nativeBuildInputs = [ unzip ];
    meta = {
      description = "Pinned upstream native WebGPU runtime for 64-bit MinGW Windows";
      homepage = "https://github.com/gfx-rs/wgpu-native";
      license = with lib.licenses; [
        mit
        asl20
      ];
    };
  }
  ''
    unzip -q ${archive}
    mkdir -p "$out/bin" "$out/share/wgpu-native" "$dev/include" "$dev/lib" "$dev/nix-support"
    cp -R include/webgpu "$dev/include/"
    cp -R wgpu-native-meta "$out/share/wgpu-native/"
    cp lib/wgpu_native.dll "$out/bin/"
    cp lib/libwgpu_native.dll.a "$dev/lib/"
    printf '%s\n' "$out" > "$dev/nix-support/propagated-build-inputs"
  ''
