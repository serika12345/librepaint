{
  expat,
  lib,
  mkIOSCMakePackage,
  packageSpec,
  pkg-config,
  toolchain,
}:

assert lib.assertMsg (
  expat.version == packageSpec.version
) "Expat source version ${expat.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [ ]
) "Expat iOS manifest must not have target dependencies";

mkIOSCMakePackage {
  pname = "expat-ios";
  inherit (packageSpec) version;
  src = expat.src;

  cmakeFlags = packageSpec.cmake_args;

  requiredPaths = [
    "include/expat.h"
    "include/expat_config.h"
    "include/expat_external.h"
    "lib/libexpat.a"
    "lib/cmake/expat-${packageSpec.version}/expat-config.cmake"
    "lib/cmake/expat-${packageSpec.version}/expat-config-version.cmake"
    "lib/cmake/expat-${packageSpec.version}/expat-release.cmake"
    "lib/cmake/expat-${packageSpec.version}/expat.cmake"
    "lib/pkgconfig/expat.pc"
  ];

  staticArchives = [ "lib/libexpat.a" ];

  postInstallCheck = ''
    if test -e "$out/bin/xmlwf"; then
      echo "error: the target-only Expat output contains the xmlwf executable" >&2
      exit 1
    fi
    if find "$out/lib" -maxdepth 1 -name 'libexpat*.dylib' -print -quit | grep -q .; then
      echo "error: the static-only Expat output contains a dynamic library" >&2
      exit 1
    fi

    pkg_config="${pkg-config}/bin/pkg-config"
    pc_version="$(
      PKG_CONFIG_PATH= \
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
      PKG_CONFIG_SYSROOT_DIR= \
        "$pkg_config" --modversion expat
    )"
    if test "$pc_version" != "${packageSpec.version}"; then
      echo "error: expat.pc reports version '$pc_version'; expected ${packageSpec.version}" >&2
      exit 1
    fi

    pc_cflags="$(
      PKG_CONFIG_PATH= \
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
      PKG_CONFIG_SYSROOT_DIR= \
        "$pkg_config" --static --cflags expat
    )"
    for expected_flag in "-I$out/include"; do
      if ! grep -F -- "$expected_flag" <<<"$pc_cflags" >/dev/null; then
        echo "error: expat.pc cflags omit $expected_flag: $pc_cflags" >&2
        exit 1
      fi
    done

    pc_libs="$(
      PKG_CONFIG_PATH= \
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig" \
      PKG_CONFIG_SYSROOT_DIR= \
        "$pkg_config" --static --libs expat
    )"
    for expected_flag in "-L$out/lib" "-lexpat" "-lm"; do
      if ! grep -F -- "$expected_flag" <<<"$pc_libs" >/dev/null; then
        echo "error: expat.pc static libs omit $expected_flag: $pc_libs" >&2
        exit 1
      fi
    done

    consumer_build="$NIX_BUILD_TOP/expat-consumer"
    cmake \
      -S ${../tests/expat-consumer} \
      -DKRITA_IOS_EXPAT_VERSION=${packageSpec.version} \
      -B "$consumer_build" \
      -G Ninja \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_SYSTEM_PROCESSOR=${toolchain.architecture} \
      -DCMAKE_OSX_ARCHITECTURES=${toolchain.architecture} \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${toolchain.deploymentTarget} \
      -DCMAKE_OSX_SYSROOT=${toolchain.sdkRoot} \
      -DCMAKE_C_COMPILER=${toolchain.cc} \
      -DCMAKE_AR=${toolchain.ar} \
      -DCMAKE_RANLIB=${toolchain.ranlib} \
      -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
      -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE \
      -DCMAKE_PREFIX_PATH="$out" \
      -DCMAKE_FIND_ROOT_PATH="$out" \
      -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
    cmake --build "$consumer_build" --parallel

    link_commands="$(ninja -C "$consumer_build" -t commands krita-ios-expat-consumer)"
    if ! grep -Fq -- "$out/lib/libexpat.a" <<<"$link_commands"; then
      echo "error: Expat consumer did not link the target archive" >&2
      exit 1
    fi

    consumer="$consumer_build/krita-ios-expat-consumer.app/krita-ios-expat-consumer"
    test "$(${toolchain.lipo} -archs "$consumer")" = "${toolchain.architecture}"
    consumer_metadata="$(${toolchain.vtool} -show-build "$consumer")"
    grep -Eq 'platform[[:space:]]+IOS([[:space:]]|$)' <<<"$consumer_metadata"
    grep -Eq 'minos[[:space:]]+${toolchain.deploymentTarget}([[:space:]]|$)' <<<"$consumer_metadata"
    grep -Eq 'sdk[[:space:]]+${toolchain.sdkVersion}([[:space:]]|$)' <<<"$consumer_metadata"
  '';

  meta = {
    description = "Static Expat cross-compiled for the pinned Krita iPadOS target";
    license = lib.licenses.mit;
  };
}
