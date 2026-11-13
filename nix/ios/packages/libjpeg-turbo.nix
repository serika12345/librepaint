{
  lib,
  libjpeg_turbo,
  mkIOSCMakePackage,
  packageSpec,
  pkg-config,
  toolchain,
}:

let
  expectedCMakeArgs = [
    "-DENABLE_SHARED=OFF"
    "-DENABLE_STATIC=ON"
    "-DWITH_TOOLS=OFF"
    "-DWITH_TESTS=OFF"
    "-DWITH_JAVA=OFF"
    "-DWITH_SIMD=ON"
    "-DREQUIRE_SIMD=ON"
    "-DWITH_TURBOJPEG=ON"
    "-DWITH_JPEG8=ON"
    "-DBUILD=19800101"
  ];
in
assert lib.assertMsg (libjpeg_turbo.version == packageSpec.version)
  "libjpeg-turbo source version ${libjpeg_turbo.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [ ]
) "libjpeg-turbo iOS manifest unexpectedly gained target dependencies";
assert lib.assertMsg (
  packageSpec.cmake_args == expectedCMakeArgs
) "libjpeg-turbo iOS manifest CMake arguments must preserve the audited static SIMD build contract";

mkIOSCMakePackage {
  pname = "libjpeg-turbo-ios";
  inherit (packageSpec) version;
  src = libjpeg_turbo.src;

  cmakeFlags = packageSpec.cmake_args;

  requiredPaths = [
    "include/jconfig.h"
    "include/jerror.h"
    "include/jmorecfg.h"
    "include/jpeglib.h"
    "include/turbojpeg.h"
    "lib/libjpeg.a"
    "lib/libturbojpeg.a"
    "lib/cmake/libjpeg-turbo/libjpeg-turboConfig.cmake"
    "lib/cmake/libjpeg-turbo/libjpeg-turboConfigVersion.cmake"
    "lib/cmake/libjpeg-turbo/libjpeg-turboTargets.cmake"
    "lib/cmake/libjpeg-turbo/libjpeg-turboTargets-release.cmake"
    "lib/pkgconfig/libjpeg.pc"
    "lib/pkgconfig/libturbojpeg.pc"
  ];

  staticArchives = [
    "lib/libjpeg.a"
    "lib/libturbojpeg.a"
  ];

  postInstallCheck = ''
    if test -d "$out/bin"; then
      echo "error: the library-only libjpeg-turbo output contains tools" >&2
      exit 1
    fi
    if find "$out/lib" -maxdepth 1 \( \
      -name '*.dylib' -o -name '*.so' -o -name '*.so.*' \
    \) -print -quit | grep -q .; then
      echo "error: the static-only libjpeg-turbo output contains a dynamic library" >&2
      exit 1
    fi

    config_header="$out/include/jconfig.h"
    grep -Eq '^#define[[:space:]]+LIBJPEG_TURBO_VERSION[[:space:]]+${lib.escapeRegex packageSpec.version}([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+LIBJPEG_TURBO_VERSION_NUMBER[[:space:]]+3001004([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+JPEG_LIB_VERSION[[:space:]]+80([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+WITH_SIMD[[:space:]]+1([[:space:]]|$)' "$config_header"

    targets="$out/lib/cmake/libjpeg-turbo/libjpeg-turboTargets.cmake"
    targets_release="$out/lib/cmake/libjpeg-turbo/libjpeg-turboTargets-release.cmake"
    grep -Fq 'add_library(libjpeg-turbo::jpeg-static STATIC IMPORTED)' "$targets"
    grep -Fq 'add_library(libjpeg-turbo::turbojpeg-static STATIC IMPORTED)' "$targets"
    grep -Fq 'IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/lib/libjpeg.a"' "$targets_release"
    grep -Fq 'IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/lib/libturbojpeg.a"' "$targets_release"

    pkg_config=${lib.getExe pkg-config}
    for module in libjpeg libturbojpeg; do
      pc_version="$(
        PKG_CONFIG_PATH= \
        PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
        PKG_CONFIG_SYSROOT_DIR= \
          "$pkg_config" --modversion "$module"
      )"
      if test "$pc_version" != "${packageSpec.version}"; then
        echo "error: $module.pc reports version '$pc_version'; expected ${packageSpec.version}" >&2
        exit 1
      fi

      pc_cflags="$(
        PKG_CONFIG_PATH= \
        PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
        PKG_CONFIG_SYSROOT_DIR= \
          "$pkg_config" --cflags "$module"
      )"
      if ! grep -Fq -- "-I$out/include" <<<"$pc_cflags"; then
        echo "error: $module.pc cflags omit the target include directory: $pc_cflags" >&2
        exit 1
      fi
    done

    jpeg_libs="$(
      PKG_CONFIG_PATH= \
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
      PKG_CONFIG_SYSROOT_DIR= \
        "$pkg_config" --static --libs libjpeg
    )"
    turbojpeg_libs="$(
      PKG_CONFIG_PATH= \
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
      PKG_CONFIG_SYSROOT_DIR= \
        "$pkg_config" --static --libs libturbojpeg
    )"
    for expected_flag in "-L$out/lib" -ljpeg; do
      if ! grep -Fq -- "$expected_flag" <<<"$jpeg_libs"; then
        echo "error: libjpeg.pc static libs omit $expected_flag: $jpeg_libs" >&2
        exit 1
      fi
    done
    for expected_flag in "-L$out/lib" -lturbojpeg; do
      if ! grep -Fq -- "$expected_flag" <<<"$turbojpeg_libs"; then
        echo "error: libturbojpeg.pc static libs omit $expected_flag: $turbojpeg_libs" >&2
        exit 1
      fi
    done

    expected_neon_members=(
      jcgray-neon.c.o
      jcphuff-neon.c.o
      jcsample-neon.c.o
      jdmerge-neon.c.o
      jdsample-neon.c.o
      jfdctfst-neon.c.o
      jidctred-neon.c.o
      jquanti-neon.c.o
      jccolor-neon.c.o
      jidctint-neon.c.o
      jidctfst-neon.c.o
      jchuff-neon.c.o
      jdcolor-neon.c.o
      jfdctint-neon.c.o
      jsimd.c.o
    )
    for archive in "$out/lib/libjpeg.a" "$out/lib/libturbojpeg.a"; do
      archive_members="$(${toolchain.ar} -t "$archive")"
      for member in "''${expected_neon_members[@]}"; do
        if ! grep -Fxq "$member" <<<"$archive_members"; then
          echo "error: $archive omits required NEON object $member" >&2
          exit 1
        fi
      done

      if ! grep -a -Fq 'libjpeg-turbo version ${packageSpec.version} (build 19800101)' "$archive"; then
        echo "error: $archive does not contain the deterministic libjpeg-turbo build identity" >&2
        exit 1
      fi
    done
  '';

  meta = {
    description = "Static SIMD-enabled libjpeg-turbo for the pinned Krita iPadOS target";
    inherit (libjpeg_turbo.meta) license;
  };
}
