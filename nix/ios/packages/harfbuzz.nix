{
  lib,
  freetype-ios,
  harfbuzz,
  mkIOSCMakePackage,
  packageSpec,
  toolchain,
}:

assert lib.assertMsg (
  harfbuzz.version == packageSpec.version
) "HarfBuzz source version ${harfbuzz.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [ "freetype" ]
) "HarfBuzz iOS manifest target dependencies must be exactly [ freetype ]";

mkIOSCMakePackage {
  pname = "harfbuzz-ios";
  inherit (packageSpec) version;
  src = harfbuzz.src;

  # FreeType is the only direct target dependency. The common builder adds its
  # propagated zlib/libpng closure to the target-only CMake search roots.
  targetDependencies = [ freetype-ios ];

  # Upstream's CMake build does not infer this project-specific switch from
  # CMAKE_SYSTEM_NAME=iOS. It is required to export the three CoreText
  # frameworks instead of attempting to use macOS ApplicationServices.
  cmakeFlags = [
    "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE"
  ]
  ++ packageSpec.cmake_args;

  requiredPaths = [
    "include/harfbuzz/hb.h"
    "include/harfbuzz/hb-coretext.h"
    "include/harfbuzz/hb-features.h"
    "include/harfbuzz/hb-ft.h"
    "include/harfbuzz/hb-version.h"
    "lib/libharfbuzz.a"
    "lib/cmake/harfbuzz/harfbuzzConfig.cmake"
    "lib/cmake/harfbuzz/harfbuzzConfig-release.cmake"
    "lib/pkgconfig/harfbuzz.pc"
    "nix-support/propagated-build-inputs"
  ];

  staticArchives = [ "lib/libharfbuzz.a" ];

  postInstall = ''
    config="$out/lib/cmake/harfbuzz/harfbuzzConfig.cmake"

    # The community-maintained upstream CMake export records a raw FreeType
    # archive, an unresolved Threads target, and absolute SDK framework paths.
    # Preserve the relocatable HarfBuzz export while expressing all of those
    # requirements as consumer-side CMake dependencies/link items.
    substituteInPlace "$config" \
      --replace-fail "${freetype-ios}/lib/libfreetype.a" "Freetype::Freetype" \
      --replace-fail "${toolchain.sdkRoot}/System/Library/Frameworks/CoreFoundation.framework" "-framework CoreFoundation" \
      --replace-fail "${toolchain.sdkRoot}/System/Library/Frameworks/CoreText.framework" "-framework CoreText" \
      --replace-fail "${toolchain.sdkRoot}/System/Library/Frameworks/CoreGraphics.framework" "-framework CoreGraphics"

    {
      echo 'include(CMakeFindDependencyMacro)'
      echo 'find_dependency(Threads)'
      echo 'find_dependency(Freetype CONFIG)'
      echo
      cat "$config"
    } > "$config.with-dependencies"
    mv "$config.with-dependencies" "$config"
  '';

  postInstallCheck = ''
    features="$out/include/harfbuzz/hb-features.h"
    for feature in CORETEXT FREETYPE; do
      grep -Eq "^#define[[:space:]]+HB_HAS_$feature[[:space:]]+1([[:space:]]|$)" "$features"
    done
    for feature in \
      CAIRO \
      DIRECTWRITE \
      GDI \
      GLIB \
      GOBJECT \
      GRAPHITE \
      ICU \
      RASTER \
      SUBSET \
      UNISCRIBE \
      VECTOR \
      WASM; do
      if grep -Eq "^#define[[:space:]]+HB_HAS_$feature([[:space:]]|$)" "$features"; then
        echo "error: HarfBuzz enabled unsupported feature: $feature" >&2
        exit 1
      fi
    done

    for forbidden_output in \
      "$out/lib/libharfbuzz-gobject.a" \
      "$out/lib/libharfbuzz-icu.a" \
      "$out/lib/libharfbuzz-raster.a" \
      "$out/lib/libharfbuzz-subset.a" \
      "$out/lib/libharfbuzz-vector.a"; do
      if test -e "$forbidden_output"; then
        echo "error: HarfBuzz installed a disabled optional library: $forbidden_output" >&2
        exit 1
      fi
    done

    config="$out/lib/cmake/harfbuzz/harfbuzzConfig.cmake"
    grep -Fxq 'find_dependency(Threads)' "$config"
    grep -Fxq 'find_dependency(Freetype CONFIG)' "$config"
    grep -Fq 'Freetype::Freetype' "$config"
    if grep -Fq '${freetype-ios}/lib/libfreetype.a' "$config"; then
      echo "error: HarfBuzz CMake export retained its raw FreeType archive" >&2
      exit 1
    fi
    for framework in CoreFoundation CoreText CoreGraphics; do
      grep -Fq -- "-framework $framework" "$config"
    done

    pc="$out/lib/pkgconfig/harfbuzz.pc"
    grep -Eq '^Requires\.private:.*freetype2[[:space:]]*>=[[:space:]]*20\.0\.14' "$pc"
    for framework in CoreFoundation CoreText CoreGraphics; do
      grep -Eq -- "^Libs\.private:.*-framework[[:space:]]+$framework([[:space:]]|$)" "$pc"
    done
  '';

  meta = {
    description = "Static HarfBuzz cross-compiled for the pinned Krita iPadOS target";
    inherit (harfbuzz.meta) license;
  };
}
