{
  lib,
  stdenv,
  runCommand,
  fetchurl,
  xz,
  autoPatchelfHook,
  zlib,
}:

let
  version = "1.97.1";
  release = "https://static.rust-lang.org/dist/2026-07-16";
  darwin = stdenv.hostPlatform.system == "aarch64-darwin";
  host = if darwin then "aarch64-apple-darwin" else "x86_64-unknown-linux-gnu";
  compiler = fetchurl {
    url = "${release}/rustc-${version}-${host}.tar.xz";
    hash =
      if darwin then
        "sha256-YHbK04zKuqJDJfJqdAgKNjomM6nNNMRzqJdyVdilk8s="
      else
        "sha256-mBnQoy1WvTOVhTGcgCYOMyd59VQf1mg4q34BbWyBSBk=";
  };
  standardLibrary = fetchurl {
    url = "${release}/rust-std-${version}-${host}.tar.xz";
    sha256 =
      if darwin then
        "a4895f5c6995e83cab8687e46b14324592398049def71ce75ca308c981cf200d"
      else
        "1c1e704ae80126b7de34f72ea2825f7fd01736dec20732faed47374b95282fba";
  };
  source = fetchurl {
    url = "${release}/rust-src-${version}.tar.xz";
    sha256 = "e9a1e616d04c6845895c827a178b9227f7c7199f3f4a80af81ab3aff7b80156b";
  };
in
assert lib.assertMsg (builtins.elem stdenv.hostPlatform.system [
  "aarch64-darwin"
  "x86_64-linux"
]) "The GPU cross compiler requires a supported pinned Rust host";
runCommand "rust-${if darwin then "ios" else "android"}-source-toolchain-${version}"
  {
    inherit version;
    nativeBuildInputs = [ xz ] ++ lib.optional (!darwin) autoPatchelfHook;
    buildInputs = lib.optionals (!darwin) [
      stdenv.cc.cc.lib
      zlib
    ];
    passthru.standardLibrarySource = source;
    meta = {
      description = "Matching Rust compiler, host runtime and sources for native GPU cross compilation";
      license = with lib.licenses; [
        mit
        asl20
      ];
      platforms = [
        "aarch64-darwin"
        "x86_64-linux"
      ];
    };
  }
  ''
    tar -xf ${compiler}
    tar -xf ${standardLibrary}
    tar -xf ${source}
    mkdir -p "$out"
    cp -R rustc-${version}-${host}/rustc/. "$out/"
    cp -R rust-std-${version}-${host}/rust-std-${host}/lib/. "$out/lib/"
    cp -R rust-src-${version}/rust-src/lib/. "$out/lib/"
    ${lib.optionalString (!darwin) ''autoPatchelf "$out"''}
  ''
