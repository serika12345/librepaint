{
  boost,
  lib,
  mkIOSHeaderPackage,
  packageSpec,
}:

let
  boostParts = map lib.toInt (lib.splitVersion packageSpec.version);
  boostLibVersion = "${toString (builtins.elemAt boostParts 0)}_${toString (builtins.elemAt boostParts 1)}";
  boostVersionNumber = toString (
    builtins.elemAt boostParts 0 * 100000
    + builtins.elemAt boostParts 1 * 100
    + builtins.elemAt boostParts 2
  );
  boostConfig = ''
    get_filename_component(_BOOST_PREFIX "''${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
    set(Boost_FOUND TRUE)
    set(Boost_VERSION "${packageSpec.version}")
    set(Boost_VERSION_STRING "${packageSpec.version}")
    if(NOT TARGET Boost::headers)
      add_library(Boost::headers INTERFACE IMPORTED)
      set_target_properties(Boost::headers PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "''${_BOOST_PREFIX}/include")
    endif()
    if(NOT TARGET Boost::boost)
      add_library(Boost::boost INTERFACE IMPORTED)
      set_target_properties(Boost::boost PROPERTIES INTERFACE_LINK_LIBRARIES Boost::headers)
    endif()
    if(NOT TARGET Boost::disable_autolinking)
      add_library(Boost::disable_autolinking INTERFACE IMPORTED)
      set_target_properties(Boost::disable_autolinking PROPERTIES INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_NO_LIB)
    endif()
    unset(_BOOST_PREFIX)
  '';
  boostConfigVersion = ''
    set(PACKAGE_VERSION "${packageSpec.version}")
    if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
      set(PACKAGE_VERSION_COMPATIBLE FALSE)
    else()
      set(PACKAGE_VERSION_COMPATIBLE TRUE)
      if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
      endif()
    endif()
  '';
in
assert lib.assertMsg (
  boost.version == packageSpec.version
) "Boost source version ${boost.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.build_system == "boost_headers"
) "Boost iOS manifest must retain the header-only build system";
assert lib.assertMsg (
  packageSpec.dependencies == [ ]
) "Boost iOS manifest unexpectedly gained target dependencies";
assert lib.assertMsg (
  packageSpec.cmake_args == [ ]
) "Boost iOS manifest unexpectedly gained CMake arguments";

mkIOSHeaderPackage {
  pname = "boost-ios";
  inherit (packageSpec) version;
  src = boost.src;

  headerTrees = [
    {
      source = "boost";
      destination = "include/boost";
    }
  ];

  generatedFiles = {
    "lib/cmake/Boost-${packageSpec.version}/BoostConfig.cmake" = boostConfig;
    "lib/cmake/Boost-${packageSpec.version}/BoostConfigVersion.cmake" = boostConfigVersion;
  };

  requiredPaths = [
    "include/boost/version.hpp"
    "include/boost/circular_buffer.hpp"
    "include/boost/mp11.hpp"
    "lib/cmake/Boost-${packageSpec.version}/BoostConfig.cmake"
    "lib/cmake/Boost-${packageSpec.version}/BoostConfigVersion.cmake"
  ];

  postInstallCheck = ''
    version_header="$out/include/boost/version.hpp"
    grep -Eq '^#define[[:space:]]+BOOST_VERSION[[:space:]]+${boostVersionNumber}([[:space:]]|$)' "$version_header"
    grep -Eq '^#define[[:space:]]+BOOST_LIB_VERSION[[:space:]]+"${boostLibVersion}"([[:space:]]|$)' "$version_header"

    config="$out/lib/cmake/Boost-${packageSpec.version}/BoostConfig.cmake"
    version_config="$out/lib/cmake/Boost-${packageSpec.version}/BoostConfigVersion.cmake"
    grep -Fq 'get_filename_component(_BOOST_PREFIX "''${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)' "$config"
    grep -Fq 'add_library(Boost::headers INTERFACE IMPORTED)' "$config"
    grep -Fq 'INTERFACE_INCLUDE_DIRECTORIES "''${_BOOST_PREFIX}/include"' "$config"
    grep -Fq 'add_library(Boost::boost INTERFACE IMPORTED)' "$config"
    grep -Fq 'INTERFACE_LINK_LIBRARIES Boost::headers' "$config"
    grep -Fq 'add_library(Boost::disable_autolinking INTERFACE IMPORTED)' "$config"
    grep -Fq 'INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_NO_LIB' "$config"
    grep -Fxq 'unset(_BOOST_PREFIX)' "$config"
    grep -Fxq 'set(PACKAGE_VERSION "${packageSpec.version}")' "$version_config"

    if grep -Fq '${boost.src}' "$config" "$version_config"; then
      echo "error: Boost CMake metadata embeds the source path" >&2
      exit 1
    fi
  '';

  meta = {
    description = "Boost headers and relocatable CMake metadata for Krita's iPadOS build";
    inherit (boost.meta) license;
  };
}
