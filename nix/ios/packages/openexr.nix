{
  imath-ios,
  lib,
  libdeflate-ios,
  mkIOSCMakePackage,
  openexr,
  packageSpec,
}:

assert lib.assertMsg (
  openexr.version == packageSpec.version
) "openexr source version ${openexr.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.dependencies == [
    "imath"
    "libdeflate"
  ]
) "openexr iOS manifest target dependencies changed";

mkIOSCMakePackage {
  pname = "openexr-ios";
  inherit (packageSpec) version;
  src = openexr.src;

  targetDependencies = [
    imath-ios
    libdeflate-ios
  ];
  cmakeFlags = packageSpec.cmake_args;

  requiredPaths = [
    "include/OpenEXR/ImfRgbaFile.h"
    "lib/libOpenEXR-3_5.a"
    "lib/cmake/OpenEXR/OpenEXRConfig.cmake"
  ];
  staticArchives = [ "lib/libOpenEXR-3_5.a" ];

  meta = {
    description = "Static OpenEXR cross-compiled for the Krita iPadOS target";
    inherit (openexr.meta) license;
  };
}
