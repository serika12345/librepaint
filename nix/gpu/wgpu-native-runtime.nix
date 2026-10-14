{
  lib,
  stdenv,
  runCommand,
  wgpu-native,
  vulkan-loader,
  patchelf,
}:

if !stdenv.hostPlatform.isLinux then
  wgpu-native
else
  # Preserve the cached Rust library while supplying its runtime-loaded Vulkan dependency.
  runCommand "wgpu-native-runtime-${wgpu-native.version}"
    {
      inherit (wgpu-native) version meta;
      outputs = [
        "out"
        "dev"
      ];
      nativeBuildInputs = [ patchelf ];
    }
    ''
      mkdir -p "$out/lib" "$dev/nix-support"
      install -m755 ${wgpu-native}/lib/libwgpu_native.so "$out/lib/"
      patchelf --add-rpath ${lib.makeLibraryPath [ vulkan-loader ]} "$out/lib/libwgpu_native.so"
      ln -s ${lib.getDev wgpu-native}/include "$dev/include"
      printf '%s\n' "$out" > "$dev/nix-support/propagated-build-inputs"
    ''
