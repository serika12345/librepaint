{
  lib,
  src,
  excludeAndroidPackage ? false,
}:

lib.cleanSourceWith {
  name = "librepaint-source";
  inherit src;
  filter =
    path: _type:
    let
      relativePath = lib.removePrefix "${toString src}/" (toString path);
      topLevel = builtins.head (lib.splitString "/" relativePath);
    in
    !(builtins.elem topLevel [
      ".cache"
      ".direnv"
      ".git"
      ".github"
      ".gitlab"
      "AGENTS.md"
      "build-ios"
      "docs"
      "flake.lock"
      "flake.nix"
      "logs"
      "nix"
      "result"
    ])
    && !(builtins.elem relativePath [
      "packaging/macos/fonts.conf"
      "packaging/macos/macos-apptodmg.py"
      "packaging/macos/macos-bundle-launcher.c"
      "packaging/macos/macos-deploy.py"
      "packaging/macos/qt.conf"
      "scripts/platform/audit-linux-appimage.py"
      "scripts/platform/audit-macos-bundle.py"
      "scripts/platform/audit-windows-package.py"
      "scripts/platform/check-macos-release-dmg"
      "scripts/platform/check-release-assets"
      "scripts/platform/sign-android-draft-release"
      "scripts/platform/verify-android-release-apk.py"
      "scripts/tests/test_android_release_apk.py"
      "scripts/tests/test_linux_release_appimage.py"
      "scripts/tests/test_macos_release_bundle.py"
      "scripts/tests/test_windows_release_package.py"
      "scripts/plugin-sdk"
      "scripts/verify-plugin-sdk"
      "scripts/tests/test_plugin_sdk.py"
    ])
    && !(builtins.any
      (directory: relativePath == directory || lib.hasPrefix "${directory}/" relativePath)
      [
        "sdk/cmake"
        "sdk/contracts"
      ]
    )
    && !(
      excludeAndroidPackage
      && (relativePath == "packaging/android/apk" || lib.hasPrefix "packaging/android/apk/" relativePath)
    )
    && !(lib.hasPrefix "README" topLevel);
}
