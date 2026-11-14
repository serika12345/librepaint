{
  lib,
  libaom-ios,
  libde265-ios,
  libheif,
  mkIOSCMakePackage,
  packageSpec,
  x265-ios,
}:

assert lib.assertMsg (
  libheif.version == packageSpec.version
) "libheif source version ${libheif.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [
    "libde265"
    "x265"
    "libaom"
  ]
) "libheif iOS manifest target dependencies changed";

mkIOSCMakePackage {
  pname = "libheif-ios";
  inherit (packageSpec) version;
  src = libheif.src;

  targetDependencies = [
    libde265-ios
    x265-ios
    libaom-ios
  ];
  cmakeFlags = packageSpec.cmake_args;
  enableTargetPkgConfig = true;

  requiredPaths = packageSpec.required_paths ++ packageSpec.artifacts;
  inspectAllAppleObjects = true;

  # The static libheif package exports its AOM dependency as the imported
  # target AOM::aom without any find_dependency for it. Replace both
  # occurrences with the static archive path, matching x265 and libde265.
  postInstall = ''
    libheif_cmake_config="$out/lib/cmake/libheif/libheif-config.cmake"
    libheif_aom_pattern='\$<LINK_ONLY:AOM::aom>'
    libheif_aom_count="$(
      grep -o -F "$libheif_aom_pattern" "$libheif_cmake_config" | grep -c 'AOM::aom' || true
    )"
    if test "$libheif_aom_count" -ne 2; then
      echo "error: expected 2 '$libheif_aom_pattern' entries in $libheif_cmake_config; found $libheif_aom_count" >&2
      exit 1
    fi
    substituteInPlace "$libheif_cmake_config" \
      --replace-fail "$libheif_aom_pattern" '${libaom-ios}/lib/libaom.a'
    if grep -F 'AOM::aom' "$libheif_cmake_config"; then
      echo "error: $libheif_cmake_config still references AOM::aom" >&2
      exit 1
    fi
  '';

  postInstallCheck = ''
    libheif_cmake_config="$out/lib/cmake/libheif/libheif-config.cmake"
    if grep -F -q 'AOM::aom' "$libheif_cmake_config"; then
      echo "error: $libheif_cmake_config still references AOM::aom" >&2
      exit 1
    fi
    if ! grep -F -q '${libaom-ios}/lib/libaom.a' "$libheif_cmake_config"; then
      echo "error: $libheif_cmake_config does not reference ${libaom-ios}/lib/libaom.a" >&2
      exit 1
    fi
  '';

  meta = {
    description = "Static HEIF/AVIF codec cross-compiled for the Krita iPadOS target";
    inherit (libheif.meta) license;
  };
}
