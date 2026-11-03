{
  runCommand,
  rustPlatform,
  rustToolchain,
  wgpu-native,
}:

let
  standardLibraryVendor = rustPlatform.fetchCargoVendor {
    name = "librepaint-gpu-rust-std-${rustToolchain.version}";
    src = rustToolchain.standardLibrarySource;
    cargoRoot = "rust-src/lib/rustlib/src/rust/library";
    hash = "sha256-hU9x09KB7TacKV9lQBCGwbE9SO7lHfPDi8JMjRXE1qk=";
  };
in
runCommand "wgpu-native-cross-vendor-${wgpu-native.version}"
  { passthru = { inherit standardLibraryVendor; }; }
  ''
    mkdir -p "$out"
    for crate in ${wgpu-native.cargoDeps}/source-registry-0/* ${standardLibraryVendor}/source-registry-0/*; do
      name="''${crate##*/}"
      if test -e "$out/$name"; then
        cmp "$out/$name/.cargo-checksum.json" "$crate/.cargo-checksum.json"
      else
        ln -s "$crate" "$out/$name"
      fi
    done
  ''
