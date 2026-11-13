{
  lib,
  freetype-ios,
  gnugrep,
  harfbuzz-ios,
  libpng-ios,
  mkIOSCMakePackage,
  ninja,
  packageSpec,
  qt6Packages,
  qtXcrunShim,
  toolchain,
  zlib-ios,
}:

let
  qtbase = qt6Packages.qtbase;
  hostQt = qt6Packages.qtbase;

  staticArchives = packageSpec.artifacts.static_archives;
  standaloneObjects = packageSpec.artifacts.standalone_objects;
  requiredPaths = packageSpec.artifacts.required_paths ++ staticArchives ++ standaloneObjects;

  cacheStringLocks = packageSpec.configure.cache_string_locks;
  cacheBooleanLocks = packageSpec.configure.cache_boolean_locks;
  commonCacheStringLocks = {
    CMAKE_BUILD_TYPE = "Release";
    CMAKE_OSX_ARCHITECTURES = toolchain.architecture;
    CMAKE_OSX_DEPLOYMENT_TARGET = toolchain.deploymentTarget;
    CMAKE_OSX_SYSROOT = "iphoneos";
    CMAKE_SYSTEM_NAME = "iOS";
  };
  qtCacheStringLocks = removeAttrs cacheStringLocks (builtins.attrNames commonCacheStringLocks);

  cmakeBoolean = value: if value then "ON" else "OFF";
  cacheBooleanFlags = lib.mapAttrsToList (name: value: "-D${name}=${cmakeBoolean value}") (
    removeAttrs cacheBooleanLocks [ "QT_WILL_BUILD_TOOLS" ]
  );
  qtCacheStringFlags = lib.mapAttrsToList (name: value: "-D${name}=${value}") qtCacheStringLocks;
  inputFlags = lib.mapAttrsToList (
    name: value: "-D${name}=${value}"
  ) packageSpec.configure.input_locks;
  enabledFeatureFlags = map (
    feature: "-DFEATURE_${feature}=ON"
  ) packageSpec.configure.feature_locks.enabled;
  disabledFeatureFlags = map (
    feature: "-DFEATURE_${feature}=OFF"
  ) packageSpec.configure.feature_locks.disabled;

  cacheStringCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_string ${lib.escapeShellArg name} ${lib.escapeShellArg value}
    '') cacheStringLocks
  );
  cacheBooleanCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_boolean ${lib.escapeShellArg name} ${lib.escapeShellArg (cmakeBoolean value)}
    '') cacheBooleanLocks
  );
  inputCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_string ${lib.escapeShellArg name} ${lib.escapeShellArg value}
    '') packageSpec.configure.input_locks
  );
  enabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} ON
  '') packageSpec.configure.feature_locks.enabled;
  disabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} OFF
  '') packageSpec.configure.feature_locks.disabled;
in
assert lib.assertMsg (packageSpec.name == "qtbase") "Qt package manifest entry must be qtbase";
assert lib.assertMsg (
  packageSpec.version == qtbase.version
) "Qt target source version ${qtbase.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.version == hostQt.version
) "Qt host version ${hostQt.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.source.flake_attr == "source-qtbase"
) "Qtbase source manifest must use the source-qtbase flake output";
assert lib.assertMsg (
  packageSpec.source.archive_name == "qtbase-everywhere-src-${packageSpec.version}.tar.xz"
) "Qtbase source archive name does not match its pinned version";
assert lib.assertMsg (
  packageSpec.source.archive_sha256
  == "a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307"
) "Qtbase source archive hash differs from the audited Qt 6.12.0 archive";
assert lib.assertMsg (
  packageSpec.host_tools.flake_attr == "host-qtbase"
) "Qtbase host-tool manifest must use the host-qtbase flake output";
assert lib.assertMsg (
  packageSpec.host_tools.version == hostQt.version
) "Qtbase host-tool manifest version must match the host Qt derivation";
assert lib.assertMsg (
  packageSpec.dependencies.target_packages == [
    "zlib"
    "libpng"
    "freetype"
    "harfbuzz"
  ]
) "Qtbase target dependencies must be exactly zlib, libpng, freetype, and harfbuzz";
assert lib.assertMsg (
  packageSpec.dependencies.qt_modules == [ ]
) "Qtbase must not depend on another target Qt module";
assert lib.assertMsg (
  packageSpec.configure.languages == [
    "ASM"
    "C"
    "CXX"
    "OBJC"
    "OBJCXX"
  ]
) "Qtbase language manifest must cover every pinned Apple compiler";
assert lib.assertMsg (
  packageSpec.host_tools.cmake_packages == [
    "Qt6HostInfo"
    "Qt6CoreTools"
    "Qt6GuiTools"
    "Qt6WidgetsTools"
  ]
) "Qtbase host CMake package contract changed";
assert lib.assertMsg (
  packageSpec.host_tools.executables == [
    "libexec/syncqt"
    "libexec/moc"
    "libexec/cmake_automoc_parser"
    "libexec/rcc"
  ]
) "Qtbase host executable contract changed";
assert lib.assertMsg (
  packageSpec.configure.sdk_lock == {
    allow_absolute_path = false;
    cmake_cache_value = "iphoneos";
    name = "iphoneos";
  }
) "Qtbase must preserve a logical iphoneos SDK in installed configuration";
assert lib.assertMsg (
  lib.getAttrs (builtins.attrNames commonCacheStringLocks) cacheStringLocks == commonCacheStringLocks
) "Qtbase common CMake cache locks disagree with the pinned iOS toolchain";
assert lib.assertMsg (
  qtCacheStringLocks == { QT_QMAKE_TARGET_MKSPEC = "macx-ios-clang"; }
) "Qtbase has an unsupported project-specific string cache lock";
assert lib.assertMsg (
  packageSpec.artifacts.exact_static_archive_set && packageSpec.artifacts.exact_standalone_object_set
) "Qtbase output sets must both be exact";
assert lib.assertMsg (
  builtins.length staticArchives == packageSpec.artifacts.expected_counts.static_archives
) "Qtbase static archive count does not match its manifest";
assert lib.assertMsg (
  builtins.length standaloneObjects == packageSpec.artifacts.expected_counts.standalone_objects
) "Qtbase standalone object count does not match its manifest";
assert lib.assertMsg (
  lib.unique staticArchives == staticArchives && lib.unique standaloneObjects == standaloneObjects
) "Qtbase output manifest contains duplicate artifacts";
assert lib.assertMsg (
  lib.intersectLists packageSpec.configure.feature_locks.enabled packageSpec.configure.feature_locks.disabled
  == [ ]
) "Qtbase feature manifest enables and disables the same feature";
assert lib.assertMsg (
  packageSpec.artifacts.forbidden.path_globs == [
    "**/*.app"
    "**/*.dylib"
    "**/*.framework"
    "**/*.so"
    "**/*.so.*"
    "doc/**"
    "examples/**"
    "sbom/**"
    "tests/**"
  ]
) "Qtbase forbidden output-glob contract changed";
assert lib.assertMsg (
  packageSpec.artifacts.forbidden.reference_literals == [ toolchain.xcodeApp ]
  && packageSpec.artifacts.forbidden.reference_environment_values == [ "NIX_BUILD_TOP" ]
) "Qtbase forbidden reference contract changed";
mkIOSCMakePackage {
  pname = "qtbase-ios";
  inherit (packageSpec) version;
  src = qtbase.src;

  targetDependencies = [
    zlib-ios
    libpng-ios
    freetype-ios
    harfbuzz-ios
  ];

  appleSdkResolver = qtXcrunShim;
  enableFullAppleToolchain = true;
  inspectAllAppleObjects = true;
  tryCompileTargetType = null;
  nativeBuildInputs = [ gnugrep ];

  cmakeFlags = [
    "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE"
    "-DQT_APPLE_SDK=iphoneos"
    "-DQT_HOST_PATH:PATH=${hostQt}"
    "-DQT_HOST_PATH_CMAKE_DIR:PATH=${hostQt}/lib/cmake"
    "-DQT_XCRUN:FILEPATH=${qtXcrunShim}/bin/xcrun"
  ]
  ++ qtCacheStringFlags
  ++ cacheBooleanFlags
  ++ inputFlags
  ++ enabledFeatureFlags
  ++ disabledFeatureFlags;

  inherit requiredPaths;
  staticArchives = [ ];

  postUnpack = ''
    source_archive=${lib.escapeShellArg (toString qtbase.src)}
    case "$source_archive" in
      *-${packageSpec.source.archive_name}) ;;
      *)
        echo "error: unexpected Qtbase source archive name: $source_archive" >&2
        exit 1
        ;;
    esac
    actual_source_sha256="$(sha256sum "$source_archive" | cut -d ' ' -f 1)"
    if test "$actual_source_sha256" != "${packageSpec.source.archive_sha256}"; then
      echo "error: Qtbase source hash is $actual_source_sha256; expected ${packageSpec.source.archive_sha256}" >&2
      exit 1
    fi
  '';

  postConfigure = ''
    check_cache_string() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      if test "$actual" != "$expected"; then
        echo "error: Qt cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    check_cache_boolean() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt boolean cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      case "$actual" in
        1 | ON | TRUE | YES) actual=ON ;;
        0 | OFF | FALSE | NO) actual=OFF ;;
        *)
          echo "error: Qt cache $name has non-boolean value '$actual'" >&2
          exit 1
          ;;
      esac
      if test "$actual" != "$expected"; then
        echo "error: Qt cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    ${cacheStringCheckScript}
    ${cacheBooleanCheckScript}
    ${inputCheckScript}
    ${enabledFeatureCheckScript}
    ${disabledFeatureCheckScript}

    check_cache_string CMAKE_C_COMPILER ${lib.escapeShellArg toolchain.cc}
    check_cache_string CMAKE_CXX_COMPILER ${lib.escapeShellArg toolchain.cxx}
    check_cache_string CMAKE_ASM_COMPILER ${lib.escapeShellArg toolchain.cc}
    check_cache_string CMAKE_OBJC_COMPILER ${lib.escapeShellArg toolchain.cc}
    check_cache_string CMAKE_OBJCXX_COMPILER ${lib.escapeShellArg toolchain.cxx}
    check_cache_string CMAKE_AR ${lib.escapeShellArg toolchain.ar}
    check_cache_string CMAKE_RANLIB ${lib.escapeShellArg toolchain.ranlib}
    check_cache_string CMAKE_STRIP ${lib.escapeShellArg toolchain.strip}
    check_cache_string CMAKE_LINKER ${lib.escapeShellArg toolchain.ld}
    check_cache_string CMAKE_NM ${lib.escapeShellArg toolchain.nm}
    check_cache_string CMAKE_INSTALL_NAME_TOOL ${lib.escapeShellArg toolchain.installNameTool}
    check_cache_string CMAKE_OTOOL ${lib.escapeShellArg toolchain.otool}
    check_cache_string CMAKE_TAPI ${lib.escapeShellArg toolchain.tapi}
    check_cache_string CMAKE_MAKE_PROGRAM ${lib.escapeShellArg "${ninja}/bin/ninja"}
    check_cache_string QT_HOST_PATH ${lib.escapeShellArg (toString hostQt)}
    check_cache_string QT_HOST_PATH_CMAKE_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake"}
    check_cache_string QT_XCRUN ${lib.escapeShellArg "${qtXcrunShim}/bin/xcrun"}

    for relative_tool in ${lib.escapeShellArgs packageSpec.host_tools.executables}; do
      if ! test -x "${hostQt}/$relative_tool"; then
        echo "error: pinned host Qt tool is missing or not executable: $relative_tool" >&2
        exit 1
      fi
    done
    for package in ${lib.escapeShellArgs packageSpec.host_tools.cmake_packages}; do
      if ! test -f "${hostQt}/lib/cmake/$package/$package"Config.cmake; then
        echo "error: pinned host Qt CMake package is missing: $package" >&2
        exit 1
      fi
    done

    xcrun_log="$NIX_BUILD_TOP/qt-xcrun-shim.log"
    if ! test -s "$xcrun_log"; then
      echo "error: Qt/CMake did not call the pinned xcrun shim" >&2
      exit 1
    fi
    if grep -Ev '^(-sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-version|xcodebuild -version)$' "$xcrun_log"; then
      echo "error: Qt/CMake made an unsupported xcrun invocation" >&2
      exit 1
    fi
    grep -Fxq -- '-sdk iphoneos --show-sdk-path' "$xcrun_log"
    grep -Fxq -- '--sdk iphoneos --show-sdk-path' "$xcrun_log"
    grep -Fxq -- '--sdk iphoneos --show-sdk-version' "$xcrun_log"
    grep -Fxq 'xcodebuild -version' "$xcrun_log"
  '';

  postInstall = ''
    build_internals="$out/lib/cmake/Qt6BuildInternals/QtBuildInternalsExtra.cmake"
    libjpeg_pri="$out/mkspecs/modules/qt_ext_libjpeg.pri"
    pcre2_pri="$out/mkspecs/modules/qt_ext_pcre2.pri"

    source_tree_count="$(sed -n 's/^set(QT_SOURCE_TREE "\(.*\)" CACHE PATH$/\1/p' "$build_internals" | wc -l | tr -d ' ')"
    if test "$source_tree_count" -ne 1; then
      echo "error: expected one installed QT_SOURCE_TREE; found $source_tree_count" >&2
      exit 1
    fi
    qt_source_tree="$(sed -n 's/^set(QT_SOURCE_TREE "\(.*\)" CACHE PATH$/\1/p' "$build_internals")"
    case "$qt_source_tree" in
      "$NIX_BUILD_TOP"/*) ;;
      *)
        echo "error: unexpected installed Qt source tree: $qt_source_tree" >&2
        exit 1
        ;;
    esac

    substituteInPlace "$build_internals" \
      --replace-fail \
        "set(QT_SOURCE_TREE \"$qt_source_tree\" CACHE PATH" \
        'set(QT_SOURCE_TREE "''${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/.qtbase-source-unavailable" CACHE PATH'
    substituteInPlace "$libjpeg_pri" \
      --replace-fail \
        "QMAKE_INCDIR_LIBJPEG = $qt_source_tree/src/3rdparty/libjpeg/src" \
        'QMAKE_INCDIR_LIBJPEG = $$[QT_INSTALL_HEADERS/get]/QtJpeg'
    substituteInPlace "$pcre2_pri" \
      --replace-fail \
        "QMAKE_INCDIR_PCRE2 = $qt_source_tree/src/3rdparty/pcre2/src" \
        'QMAKE_INCDIR_PCRE2 ='

    test "$(grep -Fxc 'set(QT_SOURCE_TREE "''${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/.qtbase-source-unavailable" CACHE PATH' "$build_internals")" -eq 1
    test "$(grep -Fxc 'QMAKE_INCDIR_LIBJPEG = $$[QT_INSTALL_HEADERS/get]/QtJpeg' "$libjpeg_pri")" -eq 1
    test "$(grep -Fxc 'QMAKE_INCDIR_PCRE2 =' "$pcre2_pri")" -eq 1
  '';

  postInstallCheck = ''
    expected_archives="$NIX_BUILD_TOP/qtbase-archives.expected"
    actual_archives="$NIX_BUILD_TOP/qtbase-archives.actual"
    printf '%s\n' ${lib.escapeShellArgs staticArchives} | LC_ALL=C sort > "$expected_archives"
    find "$out" -name '*.a' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_archives"
    if ! cmp -s "$expected_archives" "$actual_archives"; then
      echo "error: Qtbase static archive set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_archives" >&2
      echo "actual:" >&2
      cat "$actual_archives" >&2
      exit 1
    fi

    expected_objects="$NIX_BUILD_TOP/qtbase-objects.expected"
    actual_objects="$NIX_BUILD_TOP/qtbase-objects.actual"
    printf '%s\n' ${lib.escapeShellArgs standaloneObjects} | LC_ALL=C sort > "$expected_objects"
    find "$out" -name '*.o' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_objects"
    if ! cmp -s "$expected_objects" "$actual_objects"; then
      echo "error: Qtbase standalone object set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_objects" >&2
      echo "actual:" >&2
      cat "$actual_objects" >&2
      exit 1
    fi

    for relative_object in ${lib.escapeShellArgs (staticArchives ++ standaloneObjects)}; do
      if ! test -f "$out/$relative_object" || test -L "$out/$relative_object"; then
        echo "error: Qtbase Apple object is not a regular non-symlink file: $relative_object" >&2
        exit 1
      fi
    done

    for relative_path in ${lib.escapeShellArgs packageSpec.artifacts.forbidden.paths}; do
      if test -e "$out/$relative_path" || test -L "$out/$relative_path"; then
        echo "error: Qtbase installed forbidden target output: $relative_path" >&2
        exit 1
      fi
    done
    for directory in doc examples sbom tests; do
      if test -e "$out/$directory" || test -L "$out/$directory"; then
        echo "error: Qtbase installed forbidden directory: $directory" >&2
        exit 1
      fi
    done
    forbidden_file="$(find "$out" \( \
      \( \( -type d -o -type l \) \( -name '*.app' -o -name '*.framework' \) \) -o \
      \( \( -type f -o -type l \) \
         \( -name '*.dylib' -o -name '*.so' -o -name '*.so.*' \) \) \
      \) -print -quit)"
    if test -n "$forbidden_file"; then
      echo "error: Qtbase installed a non-static target: $forbidden_file" >&2
      exit 1
    fi

    for executable_root in "$out/bin" "$out/libexec"; do
      test -d "$executable_root" || continue
      while IFS= read -r -d "" executable; do
        if file "$executable" | grep -Fq 'Mach-O'; then
          echo "error: Qtbase installed a target or host Mach-O tool: $executable" >&2
          exit 1
        fi
      done < <(find "$executable_root" -type f -perm -0100 -print0)
    done

    build_internals="$out/lib/cmake/Qt6BuildInternals/QtBuildInternalsExtra.cmake"
    test "$(grep -Fxc 'set(QT_SOURCE_TREE "''${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/.qtbase-source-unavailable" CACHE PATH' "$build_internals")" -eq 1
    test "$(grep -Fxc 'QMAKE_INCDIR_LIBJPEG = $$[QT_INSTALL_HEADERS/get]/QtJpeg' "$out/mkspecs/modules/qt_ext_libjpeg.pri")" -eq 1
    test "$(grep -Fxc 'QMAKE_INCDIR_PCRE2 =' "$out/mkspecs/modules/qt_ext_pcre2.pri")" -eq 1
    test -f "$out/include/QtJpeg/jpeglib.h"
    test -f "$out/include/QtJpeg/jconfig.h"
    test ! -e "$out/.qtbase-source-unavailable"
    if grep -R -a -l -F ${lib.escapeShellArg (toString qtbase.src)} "$out"; then
      echo "error: Qtbase output refers to its source archive" >&2
      exit 1
    fi

    qconfig="$out/mkspecs/qconfig.pri"
    grep -Fq 'QT_VERSION = ${packageSpec.version}' "$qconfig"
    grep -Eq '^QT_CONFIG .* static([[:space:]]|$)' "$qconfig"
    grep -Eq '^QT.global.disabled_features .* framework([[:space:]]|$)' "$qconfig"
    grep -Fq 'QT_MAC_SDK_VERSION = ${toolchain.sdkVersion}' "$qconfig"
    grep -Fq 'QMAKE_IOS_DEPLOYMENT_TARGET = ${toolchain.deploymentTarget}' "$qconfig"
    grep -Fq 'QT_ARCHS = ${toolchain.architecture}' "$qconfig"

    qt_toolchain="$out/lib/cmake/Qt6/qt.toolchain.cmake"
    test "$(grep -Fxc '    set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "")' "$qt_toolchain")" -eq 1
    if grep -Fq '${toolchain.sdkRoot}' "$qt_toolchain"; then
      echo "error: Qtbase installed an absolute Xcode SDK path in qt.toolchain.cmake" >&2
      exit 1
    fi

    for config in \
      "$out/bin/target_qt.conf" \
      "$out/mkspecs/qconfig.pri" \
      "$out/mkspecs/modules/qt_lib_gui_private.pri" \
      "$out/lib/cmake/Qt6/Qt6Dependencies.cmake"; do
      if grep -E -q '(^|[=;: ])(/usr/local|/opt/homebrew)([/;:]|$)' "$config"; then
        echo "error: Qtbase target configuration refers to an undeclared host prefix: $config" >&2
        exit 1
      fi
    done
  '';

  passthru = {
    iosQtModule = packageSpec.name;
    iosQtStaticArchives = staticArchives;
    iosQtStandaloneObjects = standaloneObjects;
  };

  meta = {
    description = "Static Qt ${packageSpec.version} base modules for the pinned Krita iPadOS target";
    inherit (qtbase.meta) license;
  };
}
