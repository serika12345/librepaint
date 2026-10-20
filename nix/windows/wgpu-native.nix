{
  lib,
  runCommand,
  wgpu-native,
}:

let
  native = import ../gpu/wgpu-native.nix { inherit wgpu-native; };
in
runCommand "wgpu-native-windows-x86_64-gnu-${native.version}"
  {
    inherit (native) version;
    outputs = [
      "out"
      "dev"
    ];
    meta = {
      description = "Native WebGPU runtime built from pinned source for 64-bit MinGW Windows";
      homepage = "https://github.com/gfx-rs/wgpu-native";
      license = with lib.licenses; [
        mit
        asl20
      ];
    };
  }
  ''
    mkdir -p "$out/bin" "$out/share/licenses/wgpu-native" "$dev/include" "$dev/lib" "$dev/nix-support"
    cp -R ${lib.getDev native}/include/webgpu "$dev/include/"
    cp ${native}/bin/wgpu_native.dll "$out/bin/"
    cp ${native}/lib/libwgpu_native.dll.a "$dev/lib/"
    cp ${native.src}/LICENSE.MIT ${native.src}/LICENSE.APACHE "$out/share/licenses/wgpu-native/"
    printf '%s\n' "$out" > "$dev/nix-support/propagated-build-inputs"
  ''
