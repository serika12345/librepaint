{ pkgs }:

let
  kritaDependencies = pkgs.fetchzip {
    url = "https://invent.kde.org/packaging/krita-deps-management/-/archive/b318061744709ea5051739dd7b30351cb8285fd1/krita-deps-management-b318061744709ea5051739dd7b30351cb8285fd1.tar.gz";
    hash = "sha256-D4hSa2xSZCwWQs1CHbrasU04XkHdT3IK68N9ovHiMpI=";
  };
  patchDirectory = "${kritaDependencies}/ext_googleangle";
  chromiumZlib = pkgs.fetchzip {
    url = "https://github.com/dimula73/google-zlib/archive/a6d209ab932df0f1c9d5b7dc67cfa74e8a3272c0.tar.gz";
    hash = "sha256-7uhJBUCXQqJ8tL0deF1tGUWx2X5yg/wHNNrzPoHufyI=";
  };
in
pkgs.stdenv.mkDerivation {
  pname = "krita-angle";
  version = "chromium-5005";

  src = pkgs.fetchFromGitHub {
    owner = "google";
    repo = "angle";
    rev = "f2280c0c5f935dccbaf528343d474c8fcdebe63a";
    hash = "sha256-GTUkDokI1l6iiINwl/KfnenAUwkAqyr3hOSkyOF3weM=";
  };

  patches = [
    "${patchDirectory}/0001-D3D-Initialize-storage-after-generating-mipmap-image.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0001-ANGLE-Use-pixel-sizes-in-the-XAML-swap-chain.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0002-ANGLE-Add-support-for-querying-platform-device.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0003-ANGLE-Fix-Windows-Store-D3D-Trim-and-Level-9-require.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0004-ANGLE-fix-usage-of-shared-handles-for-WinRT-applicat.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0005-ANGLE-Fix-initialization-of-zero-sized-window.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0006-ANGLE-winrt-Do-full-screen-update-if-the-the-window-.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0007-Revert-Fix-scanForWantedComponents-not-ignoring-attr.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0008-ANGLE-Disable-multisampling-to-avoid-crash-in-Qt-app.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0009-ANGLE-Dynamically-load-D3D-compiler-from-a-list.patch"
    "${patchDirectory}/01-patches_qt5.12.12/0010-ANGLE-clean-up-displays-on-dll-unload.patch"
    "${patchDirectory}/02-patches_krita/0011-Fix-MinGW-build.patch"
    "${patchDirectory}/02-patches_krita/0012-Fix-Clang-MinGW-target-build.patch"
    "${patchDirectory}/02-patches_krita/0013-Add-CMake-build-file.patch"
    "${patchDirectory}/02-patches_krita/0014-CMake-Do-not-exclude-EGL-headers.patch"
    "${patchDirectory}/02-patches_krita/0015-Implement-openGL-surface-color-space-selection-in-An.patch"
    "${patchDirectory}/02-patches_krita/0016-Reduce-flickering-when-resizing-window.patch"
    "${patchDirectory}/02-patches_krita/0017-Replace-extern-thread_local-to-avoid-GCC-mingw-w64-b.patch"
    "${patchDirectory}/02-patches_krita/0019-Add-resource-files-for-DLL-identification.patch"
    "${patchDirectory}/0001-D3D11-Fix-uniform-setting-for-mat3-followed-by-float.diff"
    "${patchDirectory}/0001-Add-a-switch-to-enable-debugging-output.patch"
    ./angle-windows-native-tls.patch
  ];

  postPatch = ''
    substituteInPlace include/GLSLANG/ShaderVars.h \
      --replace-fail '#include <array>' $'#include <array>\n#include <cstdint>'

    cp ${patchDirectory}/angle_commit.h.in angle_commit.h
    substituteInPlace angle_commit.h \
      --replace-fail '#cmakedefine ANGLE_COMMIT_HASH "@ANGLE_COMMIT_HASH@"' '#define ANGLE_COMMIT_HASH "f2280c0c5f93+krita_qt5"' \
      --replace-fail '#cmakedefine ANGLE_COMMIT_HASH_SIZE @ANGLE_COMMIT_HASH_SIZE@' '#define ANGLE_COMMIT_HASH_SIZE 22' \
      --replace-fail '#cmakedefine ANGLE_COMMIT_DATE "@ANGLE_COMMIT_DATE@"' '#define ANGLE_COMMIT_DATE "2022-05-03"'
    cp angle_commit.h src/common/angle_commit.h

    mkdir -p third_party/zlib
    cp -a ${chromiumZlib}/. third_party/zlib/
    chmod -R u+w third_party/zlib
  '';

  nativeBuildInputs = with pkgs.buildPackages; [
    cmake
    ninja
  ];

  buildInputs = [ pkgs.zlib ];

  cmakeFlags = [
    "-DBUILD_SHARED_LIBS=ON"
  ];

  dontStrip = true;
}
