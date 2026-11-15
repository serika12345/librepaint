{
  exiv2,
  lib,
  mkIOSCMakePackage,
  packageSpec,
  pkg-config,
  toolchain,
  zlib-ios,
}:

let
  versionParts = lib.splitVersion packageSpec.version;
  versionMajor = builtins.elemAt versionParts 0;
  versionMinor = builtins.elemAt versionParts 1;
  versionPatch = builtins.elemAt versionParts 2;
  expectedCMakeArgs = [
    "-DBUILD_SHARED_LIBS=OFF"
    "-DBUILD_TESTING=OFF"
    "-DEXIV2_BUILD_EXIV2_COMMAND=OFF"
    "-DEXIV2_BUILD_SAMPLES=OFF"
    "-DEXIV2_BUILD_UNIT_TESTS=OFF"
    "-DEXIV2_BUILD_FUZZ_TESTS=OFF"
    "-DEXIV2_BUILD_DOC=OFF"
    "-DEXIV2_ENABLE_PNG=ON"
    "-DEXIV2_ENABLE_BMFF=ON"
    "-DEXIV2_ENABLE_LENSDATA=ON"
    "-DEXIV2_ENABLE_FILESYSTEM_ACCESS=ON"
    "-DEXIV2_ENABLE_WEBREADY=OFF"
    "-DEXIV2_ENABLE_CURL=OFF"
    "-DEXIV2_ENABLE_XMP=OFF"
    "-DEXIV2_ENABLE_EXTERNAL_XMP=OFF"
    "-DEXIV2_ENABLE_INIH=OFF"
    "-DEXIV2_ENABLE_BROTLI=OFF"
    "-DEXIV2_ENABLE_VIDEO=OFF"
    "-DEXIV2_ENABLE_NLS=OFF"
    "-DBUILD_WITH_STACK_PROTECTOR=OFF"
  ];
in
assert lib.assertMsg (
  exiv2.version == packageSpec.version
) "Exiv2 source version ${exiv2.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [ "zlib" ]
) "Exiv2 iOS manifest target dependencies must be exactly [ zlib ]";
assert lib.assertMsg (packageSpec.cmake_args == expectedCMakeArgs)
  "Exiv2 iOS manifest CMake arguments must preserve the audited library-only optional-feature contract";

mkIOSCMakePackage {
  pname = "exiv2-ios";
  inherit (packageSpec) version;
  src = exiv2.src;

  targetDependencies = [ zlib-ios ];

  cmakeFlags = [ "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE" ] ++ packageSpec.cmake_args;

  requiredPaths = [
    "include/exiv2/basicio.hpp"
    "include/exiv2/bmffimage.hpp"
    "include/exiv2/exif.hpp"
    "include/exiv2/exiv2.hpp"
    "include/exiv2/exiv2lib_export.h"
    "include/exiv2/exv_conf.h"
    "include/exiv2/image.hpp"
    "include/exiv2/jpgimage.hpp"
    "include/exiv2/pngimage.hpp"
    "include/exiv2/types.hpp"
    "include/exiv2/version.hpp"
    "lib/libexiv2.a"
    "lib/cmake/exiv2/exiv2Config.cmake"
    "lib/cmake/exiv2/exiv2ConfigVersion.cmake"
    "lib/cmake/exiv2/exiv2Targets.cmake"
    "lib/cmake/exiv2/exiv2Targets-release.cmake"
    "lib/pkgconfig/exiv2.pc"
    "nix-support/propagated-build-inputs"
  ];

  staticArchives = [ "lib/libexiv2.a" ];

  postInstall = ''
    pc="$out/lib/pkgconfig/exiv2.pc"
    targets="$out/lib/cmake/exiv2/exiv2Targets.cmake"

    # Upstream writes CMAKE_INSTALL_PREFIX literally into the pkg-config file.
    # Derive it from the installed metadata location so the package can also be
    # consumed from a materialized cache prefix without retaining its old path.
    substituteInPlace "$pc" \
      --replace-fail "prefix=$out" 'prefix=''${pcfiledir}/../..' \
      --replace-fail 'Libs.private: ' 'Libs.private: -liconv'

    # CMake's static-library try_compile classifies the SDK Iconv as built in,
    # although final iOS executables still need the SDK's portable -liconv link
    # item. Export that contract without embedding the external SDK path.
    substituteInPlace "$targets" \
      --replace-fail \
        'INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ZLIB::ZLIB>"' \
        'INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ZLIB::ZLIB>;\$<LINK_ONLY:-liconv>"'

    # The command-line tool is disabled; its installed manual is not useful in
    # a target library package and needlessly enlarges the aggregate prefix.
    rm -rf "$out/share/man"
  '';

  postInstallCheck = ''
    if test -d "$out/bin"; then
      echo "error: the library-only Exiv2 output contains executables" >&2
      exit 1
    fi
    if test -e "$out/share/man"; then
      echo "error: the library-only Exiv2 output contains command documentation" >&2
      exit 1
    fi
    if find "$out/lib" -maxdepth 1 \( \
      -name '*.dylib' -o -name '*.so' -o -name '*.so.*' \
    \) -print -quit | grep -q .; then
      echo "error: the static-only Exiv2 output contains a dynamic library" >&2
      exit 1
    fi

    config_header="$out/include/exiv2/exv_conf.h"
    for macro in \
      EXV_ENABLE_BMFF \
      EXV_ENABLE_FILESYSTEM \
      EXV_HAVE_ICONV \
      EXV_HAVE_LENSDATA \
      EXV_HAVE_LIBZ; do
      if ! grep -Eq "^#define[[:space:]]+$macro([[:space:]]|$)" "$config_header"; then
        echo "error: Exiv2 configuration omits required feature macro $macro" >&2
        exit 1
      fi
    done
    for macro in \
      EXV_ENABLE_INIH \
      EXV_ENABLE_NLS \
      EXV_ENABLE_VIDEO \
      EXV_ENABLE_WEBREADY \
      EXV_HAVE_BROTLI \
      EXV_HAVE_XMP_TOOLKIT \
      EXV_USE_CURL; do
      if grep -Eq "^#define[[:space:]]+$macro([[:space:]]|$)" "$config_header"; then
        echo "error: Exiv2 unexpectedly enabled optional feature macro $macro" >&2
        exit 1
      fi
    done
    grep -Eq '^#define[[:space:]]+EXV_PACKAGE_VERSION[[:space:]]+"${lib.escapeRegex packageSpec.version}"([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+EXIV2_MAJOR_VERSION[[:space:]]+\(${versionMajor}U\)([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+EXIV2_MINOR_VERSION[[:space:]]+\(${versionMinor}U\)([[:space:]]|$)' "$config_header"
    grep -Eq '^#define[[:space:]]+EXIV2_PATCH_VERSION[[:space:]]+\(${versionPatch}U\)([[:space:]]|$)' "$config_header"

    archive_members="$(${toolchain.ar} -t "$out/lib/libexiv2.a")"
    for member in \
      exif.cpp.o \
      image.cpp.o \
      jpgimage.cpp.o \
      pngchunk_int.cpp.o \
      pngimage.cpp.o; do
      if ! grep -Fxq "$member" <<<"$archive_members"; then
        echo "error: libexiv2.a omits required metadata/image object $member" >&2
        exit 1
      fi
    done

    cmake_dir="$out/lib/cmake/exiv2"
    config="$cmake_dir/exiv2Config.cmake"
    targets="$cmake_dir/exiv2Targets.cmake"
    targets_release="$cmake_dir/exiv2Targets-release.cmake"
    grep -Fq 'get_filename_component(PACKAGE_PREFIX_DIR "''${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)' "$config"
    grep -Fq 'find_dependency(ZLIB REQUIRED)' "$config"
    grep -Fq 'include("''${CMAKE_CURRENT_LIST_DIR}/exiv2Targets.cmake")' "$config"
    grep -Fq 'add_library(exiv2lib ALIAS Exiv2::exiv2lib)' "$config"
    grep -Fq 'add_library(Exiv2::exiv2lib STATIC IMPORTED)' "$targets"
    grep -Fq 'INTERFACE_INCLUDE_DIRECTORIES "''${_IMPORT_PREFIX}/include"' "$targets"
    grep -Fq 'INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ZLIB::ZLIB>;\$<LINK_ONLY:-liconv>"' "$targets"
    grep -Fq 'IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/lib/libexiv2.a"' "$targets_release"
    grep -Fq 'if(FALSE) # if(EXV_HAVE_LIBICONV)' "$config"
    if grep -Eq 'INTERFACE_LINK_LIBRARIES ".*Iconv' "$targets"; then
      echo "error: Exiv2 CMake target unexpectedly propagates Iconv" >&2
      exit 1
    fi

    pc="$out/lib/pkgconfig/exiv2.pc"
    grep -Fxq 'prefix=''${pcfiledir}/../..' "$pc"
    grep -Eq '^Version:[[:space:]]+${lib.escapeRegex packageSpec.version}([[:space:]]|$)' "$pc"
    grep -Eq '^Requires\.private:[[:space:]]+zlib([[:space:]]|$)' "$pc"
    grep -Eq '^Libs:[[:space:]]+-L\$\{libdir\}[[:space:]]+-lexiv2([[:space:]]|$)' "$pc"
    grep -Eq '^Libs\.private:[[:space:]]+-liconv([[:space:]]|$)' "$pc"
    grep -Eq '^Cflags:[[:space:]]+-I\$\{includedir\}([[:space:]]|$)' "$pc"
    if grep -Ei '^Requires(\.private)?:.*iconv' "$pc"; then
      echo "error: exiv2.pc unexpectedly requires a host Iconv package" >&2
      exit 1
    fi

    pkg_config=${lib.getExe pkg-config}
    pc_environment=(
      PKG_CONFIG_PATH=
      PKG_CONFIG_LIBDIR="$out/lib/pkgconfig:${zlib-ios}/lib/pkgconfig"
      PKG_CONFIG_SYSROOT_DIR=
    )
    pc_version="$(env "''${pc_environment[@]}" "$pkg_config" --modversion exiv2)"
    if test "$pc_version" != "${packageSpec.version}"; then
      echo "error: exiv2.pc reports version '$pc_version'; expected ${packageSpec.version}" >&2
      exit 1
    fi
    pc_prefix="$(env "''${pc_environment[@]}" "$pkg_config" --variable=prefix exiv2)"
    if test "$(realpath "$pc_prefix")" != "$out"; then
      echo "error: exiv2.pc resolves prefix '$pc_prefix' outside $out" >&2
      exit 1
    fi
    pc_libs="$(env "''${pc_environment[@]}" "$pkg_config" --static --libs exiv2)"
    for expected_flag in \
      -lexiv2 \
      "-L${zlib-ios}/lib" \
      -lz \
      -liconv; do
      if ! grep -Fq -- "$expected_flag" <<<"$pc_libs"; then
        echo "error: exiv2.pc static libs omit $expected_flag: $pc_libs" >&2
        exit 1
      fi
    done

    metadata_paths=("$cmake_dir" "$pc")
    for forbidden_path in "$out" "${zlib-ios}" "${exiv2.src}"; do
      if grep -R -a -l -F "$forbidden_path" "''${metadata_paths[@]}"; then
        echo "error: Exiv2 package metadata embeds forbidden path: $forbidden_path" >&2
        exit 1
      fi
    done
    if grep -R -a -l -F '${exiv2.src}' "$out"; then
      echo "error: Exiv2 output contains its immutable source path" >&2
      exit 1
    fi
  '';

  meta = {
    description = "Static Exiv2 metadata library for the pinned Krita iPadOS target";
    inherit (exiv2.meta) license;
  };
}
