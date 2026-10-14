{
  lib,
  runCommand,
  fetchurl,
  xz,
}:

let
  version = "1.97.1";
  release = "https://static.rust-lang.org/dist/2026-07-16";
  compiler = fetchurl {
    url = "${release}/rustc-${version}-aarch64-apple-darwin.tar.xz";
    hash = "sha256-YHbK04zKuqJDJfJqdAgKNjomM6nNNMRzqJdyVdilk8s=";
  };
  standardLibrary = fetchurl {
    url = "${release}/rust-std-${version}-aarch64-apple-darwin.tar.xz";
    sha256 = "a4895f5c6995e83cab8687e46b14324592398049def71ce75ca308c981cf200d";
  };
  source = fetchurl {
    url = "${release}/rust-src-${version}.tar.xz";
    sha256 = "e9a1e616d04c6845895c827a178b9227f7c7199f3f4a80af81ab3aff7b80156b";
  };
in
runCommand "rust-ios-source-toolchain-${version}"
  {
    inherit version;
    nativeBuildInputs = [ xz ];
    passthru.standardLibrarySource = source;
    meta = {
      description = "Matching Rust compiler, host runtime and sources for iOS cross compilation";
      license = with lib.licenses; [
        mit
        asl20
      ];
      platforms = [ "aarch64-darwin" ];
    };
  }
  ''
    tar -xf ${compiler}
    tar -xf ${standardLibrary}
    tar -xf ${source}
    mkdir -p "$out"
    cp -R rustc-${version}-aarch64-apple-darwin/rustc/. "$out/"
    cp -R rust-std-${version}-aarch64-apple-darwin/rust-std-aarch64-apple-darwin/lib/. "$out/lib/"
    cp -R rust-src-${version}/rust-src/lib/. "$out/lib/"
  ''
