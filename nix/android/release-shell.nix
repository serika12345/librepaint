{ pkgs }:

let
  androidHost = import pkgs.path {
    localSystem = pkgs.stdenv.hostPlatform;
    config = {
      allowUnfree = true;
      android_sdk.accept_license = true;
    };
  };
  androidSdk =
    (androidHost.androidenv.composeAndroidPackages {
      buildToolsVersions = [ "35.0.0" ];
      includeCmake = false;
      includeEmulator = false;
      includeNDK = false;
      includeSystemImages = false;
      platformVersions = [ "35" ];
    }).androidsdk;
in
androidHost.mkShellNoCC {
  name = "librepaint-android-release";
  packages = with androidHost; [
    androidSdk
    binutils
    gawk
    gh
    jdk17_headless
    python3
    unzip
  ];
  shellHook = ''
    export ANDROID_RELEASE_BUILD_TOOLS=${androidSdk}/libexec/android-sdk/build-tools/35.0.0
  '';
}
